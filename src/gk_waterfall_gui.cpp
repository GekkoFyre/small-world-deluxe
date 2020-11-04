/**
 **     __                 _ _   __    __           _     _
 **    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
 **    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
 **    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
 **    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
 **
 **                  ___     _
 **                 /   \___| |_   ___  _____
 **                / /\ / _ \ | | | \ \/ / _ \
 **               / /_//  __/ | |_| |>  <  __/
 **              /___,' \___|_|\__,_/_/\_\___|
 **
 **
 **   If you have downloaded the source code for "Small World Deluxe" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Small World Deluxe is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Small world is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/gk_waterfall_gui.hpp"
#include <qwt/qwt_plot_renderer.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_panner.h>
#include <algorithm>
#include <stdexcept>
#include <exception>
#include <utility>
#include <QColormap>
#include <QTimer>
#include <QApplication>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

std::mutex spectro_main_mtx;
std::mutex mtx_spectro_raster_draw;
std::mutex mtx_spectro_align_scales;
std::mutex mtx_spectro_refresh_date_time;

/**
 * @brief GkQwtColorMap::controlPointsToQwtColorMap
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param ctrlPts
 * @return
 */
QwtColorMap *GkQwtColorMap::controlPointsToQwtColorMap(const ColorMaps::ControlPoints &ctrlPts)
{
    using namespace ColorMaps;

    if (ctrlPts.size() < 2 ||
        std::get<0>(ctrlPts.front()) != 0. ||
        std::get<0>(ctrlPts.back())  != 1. ||
        !std::is_sorted(ctrlPts.cbegin(), ctrlPts.cend(),
                        [](const ControlPoint& x, const ControlPoint& y)
                        {
                            // Strict weak ordering
                            return std::get<0>(x) < std::get<0>(y);
                        })) {
        return nullptr;
    }

    QColor from, to;
    from.setRgbF(std::get<1>(ctrlPts.front()), std::get<2>(ctrlPts.front()), std::get<3>(ctrlPts.front()));
    to.setRgbF(std::get<1>(ctrlPts.back()), std::get<2>(ctrlPts.back()), std::get<3>(ctrlPts.back()));

    QwtLinearColorMap *lcm = new QwtLinearColorMap(from, to, QwtColorMap::RGB);

    for (size_t i = 1; i < ctrlPts.size() - 1; ++i) {
        QColor cs;
        cs.setRgbF(std::get<1>(ctrlPts[i]), std::get<2>(ctrlPts[i]), std::get<3>(ctrlPts[i]));
        lcm->addColorStop(std::get<0>(ctrlPts[i]), cs);
    }

    return lcm;
}

/**
 * @brief GkSpectroWaterfall::GkSpectroWaterfall
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 * <https://github.com/medvedvvs/QwtWaterfall>
 */
GkSpectroWaterfall::GkSpectroWaterfall(QPointer<StringFuncs> stringFuncs, QPointer<GkEventLogger> eventLogger, const bool &enablePanner,
                                       const bool &enableZoomer, QWidget *parent) : m_spectrogram(new QwtPlotSpectrogram),
                                       gkAlpha(255), QWidget(parent)
{
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        setParent(parent);
        gkStringFuncs = std::move(stringFuncs);
        gkEventLogger = std::move(eventLogger);
    } catch (const std::exception &e) {
        #if defined(_WIN32) || defined(__MINGW64__)
        HWND hwnd_spectro_gui_main = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_gui_main, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_gui_main);
        #else
        gkEventLogger->publishEvent(tr("An error occurred during the handling of waterfall / spectrograph data!"), GkSeverity::Error, e.what(), true);
        #endif
    }

    return;
}

GkSpectroWaterfall::~GkSpectroWaterfall()
{}

/**
 * @brief GkSpectroWaterfall::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values
 * @param numCols
 */
void GkSpectroWaterfall::insertData(const QVector<double> &values, const int &numCols)
{
    Q_UNUSED(numCols);

    return;
}

/**
 * @brief GkSpectroWaterfall::setDataDimensions
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param dXMin
 * @param dXMax
 * @param historyExtent
 * @param layerPoints
 */
void GkSpectroWaterfall::setDataDimensions(double dXMin, double dXMax, const size_t &historyExtent, const size_t &layerPoints)
{
    gkWaterfallData = std::make_unique<WaterfallData<double>>(dXMin, dXMax, historyExtent, layerPoints);
    m_spectrogram->setData(gkWaterfallData.get());

    setupCurves();
    freeCurvesData();
    allocateCurvesData();

    // After changing data dimensions, we need to reset curves markers
    // to show the last received data on  the horizontal axis and the history
    // of the middle point
    m_markerX = (dXMax - dXMin) / 2;
    m_markerY = historyExtent - 1;

    m_horCurveMarker->setValue(m_markerX, 0.0);
    m_vertCurveMarker->setValue(0.0, m_markerY);

    // scale x
    m_plotHorCurve->setAxisScale(QwtPlot::xBottom, dXMin, dXMax);
    m_plotSpectrogram->setAxisScale(QwtPlot::xBottom, dXMin, dXMax);

    return;
}

/**
 * @brief GkSpectroWaterfall::getDataDimensions
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param dXMin
 * @param dXMax
 * @param historyExtent
 * @param layerPoints
 */
void GkSpectroWaterfall::getDataDimensions(double &dXMin, double &dXMax, size_t &historyExtent, size_t &layerPoints) const
{
    if (gkWaterfallData) {
        dXMin = gkWaterfallData->getXMin();
        dXMax = gkWaterfallData->getXMax();
        historyExtent = gkWaterfallData->getMaxHistoryLength();
        layerPoints = gkWaterfallData->getLayerPoints();
    } else {
        dXMin = 0;
        dXMax = 0;
        historyExtent = 0;
        layerPoints = 0;
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::setMarker
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param x
 * @param y
 * @return
 */
bool GkSpectroWaterfall::setMarker(const double x, const double y)
{
    if (!gkWaterfallData) {
        return false;
    }

    const QwtInterval xInterval = gkWaterfallData->interval(Qt::XAxis);
    const QwtInterval yInterval = gkWaterfallData->interval(Qt::YAxis);
    if (!(xInterval.contains(x) && yInterval.contains(y))) {
        return false;
    }

    m_markerX = x;

    const double offset = gkWaterfallData->getOffset();
    m_markerY = y - offset;

    // Update curves' markers positions
    m_horCurveMarker->setValue(m_markerX, 0.0);
    m_vertCurveMarker->setValue(0.0, y);

    updateCurvesData();

    m_plotHorCurve->replot();
    m_plotVertCurve->replot();

    return true;
}

/**
 * @brief GkSpectroWaterfall::replot
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param forceRepaint
 */
void GkSpectroWaterfall::replot(bool forceRepaint)
{
    if (!m_plotSpectrogram->isVisible()) {
        // Temporary solution for older Qwt versions
        QApplication::postEvent(m_plotHorCurve, new QEvent(QEvent::LayoutRequest));
        QApplication::postEvent(m_plotVertCurve, new QEvent(QEvent::LayoutRequest));
        QApplication::postEvent(m_plotSpectrogram, new QEvent(QEvent::LayoutRequest));
    }

    updateLayout();
    if (forceRepaint) {
        m_plotHorCurve->repaint();
        m_plotVertCurve->repaint();
        m_plotSpectrogram->repaint();
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::setWaterfallVisibility
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param bVisible
 */
void GkSpectroWaterfall::setWaterfallVisibility(const bool bVisible)
{
    m_spectrogram->setVisible(bVisible);
    return;
}

/**
 * @brief GkSpectroWaterfall::setTitle
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param qstrNewTitle
 */
void GkSpectroWaterfall::setTitle(const QString &qstrNewTitle)
{
    m_plotSpectrogram->setTitle(qstrNewTitle);
    return;
}

/**
 * @brief GkSpectroWaterfall::setXLabel
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param qstrTitle
 * @param fontPointSize
 */
void GkSpectroWaterfall::setXLabel(const QString &qstrTitle, const int fontPointSize)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::xBottom, title);
    return;
}

/**
 * @brief GkSpectroWaterfall::setYLabel
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param qstrTitle
 * @param fontPointSize
 */
void GkSpectroWaterfall::setYLabel(const QString &qstrTitle, const int fontPointSize)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::yLeft, title);
    return;
}

/**
 * @brief GkSpectroWaterfall::setZLabel
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param qstrTitle
 * @param fontPointSize
 */
void GkSpectroWaterfall::setZLabel(const QString &qstrTitle, const int fontPointSize)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::yRight, title);
    m_plotHorCurve->setAxisTitle(QwtPlot::yLeft, title);
    m_plotHorCurve->setAxisTitle(QwtPlot::yRight, title);
    m_plotVertCurve->setAxisTitle(QwtPlot::xBottom, title);
    return;
}

/**
 * @brief GkSpectroWaterfall::setXTooltipUnit
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param xUnit
 */
void GkSpectroWaterfall::setXTooltipUnit(const QString &xUnit)
{
    m_xUnit = xUnit;
    return;
}

/**
 * @brief GkSpectroWaterfall::setZTooltipUnit
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param zUnit
 */
void GkSpectroWaterfall::setZTooltipUnit(const QString &zUnit)
{
    m_zUnit = zUnit;
    return;
}

/**
 * @brief GkSpectroWaterfall::addData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param dataPtr
 * @param dataLen
 * @param timestamp
 * @return
 */
bool GkSpectroWaterfall::addData(const double *const dataPtr, const size_t dataLen, const time_t timestamp)
{
    if (!gkWaterfallData) {
        return false;
    }

    const bool bRet = gkWaterfallData->addData(dataPtr, dataLen, timestamp);
    if (bRet) {
        updateCurvesData();

        // refresh spectrogram content and Y-axis labels
        //m_spectrogram->invalidateCache();

        auto const ySpectroLeftAxis = static_cast<GkWaterfallTimeScaleDraw *>(m_plotSpectrogram->axisScaleDraw(QwtPlot::yLeft));
        // ySpectroLeftAxis->invalidateCache();

        auto const yHistoLeftAxis = static_cast<GkWaterfallTimeScaleDraw *>(m_plotVertCurve->axisScaleDraw(QwtPlot::yLeft));
        // yHistoLeftAxis->invalidateCache();

        const double currentOffset = getOffset();
        const size_t maxHistory = gkWaterfallData->getMaxHistoryLength();

        const QwtScaleDiv& yDiv = m_plotSpectrogram->axisScaleDiv(QwtPlot::yLeft);
        const double yMin = (m_zoomActive) ? yDiv.lowerBound() + 1 : currentOffset;
        const double yMax = (m_zoomActive) ? yDiv.upperBound() + 1 : maxHistory + currentOffset;

        m_plotSpectrogram->setAxisScale(QwtPlot::yLeft, yMin, yMax);
        m_plotVertCurve->setAxisScale(QwtPlot::yLeft, yMin, yMax);

        m_vertCurveMarker->setValue(0.0, m_markerY + currentOffset);
    }

    return bRet;
}

/**
 * @brief GkSpectroWaterfall::setRange
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param dLower
 * @param dUpper
 */
void GkSpectroWaterfall::setRange(double dLower, double dUpper)
{
    if (dLower > dUpper) {
        std::swap(dLower, dUpper);
    }

    if (m_plotSpectrogram->axisEnabled(QwtPlot::yRight)) {
        m_plotSpectrogram->setAxisScale(QwtPlot::yRight, dLower, dUpper);

        QwtScaleWidget* axis = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
        if (axis->isColorBarEnabled()) {
            // Waiting a proper method to get a reference to the QwtInterval
            // instead of resetting a new color map to the axis !
            QwtColorMap *colorMap;
            if (m_bColorBarInitialized) {
                colorMap = const_cast<QwtColorMap*>(axis->colorMap());
            } else {
                colorMap = GkQwtColorMap::controlPointsToQwtColorMap(m_ctrlPts);
                m_bColorBarInitialized = true;
            }
            axis->setColorMap(QwtInterval(dLower, dUpper), colorMap);
        }
    }

    // set vertical plot's X-axis and horizontal plot's Y-axis scales to the color bar min/max
    m_plotHorCurve->setAxisScale(QwtPlot::yLeft, dLower, dUpper);
    m_plotVertCurve->setAxisScale(QwtPlot::xBottom, dLower, dUpper);

    if (gkWaterfallData) {
        gkWaterfallData->setRange(dLower, dUpper);
    }

    m_spectrogram->invalidateCache();
    return;
}

/**
 * @brief GkSpectroWaterfall::getRange
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param rangeMin
 * @param rangeMax
 */
void GkSpectroWaterfall::getRange(double &rangeMin, double &rangeMax) const
{
    if (gkWaterfallData) {
        gkWaterfallData->getRange(rangeMin, rangeMax);
    } else {
        rangeMin = 0;
        rangeMax = 1;
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::getDataRange
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param rangeMin
 * @param rangeMax
 */
void GkSpectroWaterfall::getDataRange(double &rangeMin, double &rangeMax) const
{
    if (gkWaterfallData) {
        gkWaterfallData->getDataRange(rangeMin, rangeMax);
    } else {
        rangeMin = 0;
        rangeMax = 1;
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::clear
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::clear()
{
    if (gkWaterfallData) {
        gkWaterfallData->clear();
    }

    setupCurves();
    freeCurvesData();
    allocateCurvesData();

    return;
}

/**
 * @brief GkSpectroWaterfall::getLayerDate
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param y
 * @return
 */
time_t GkSpectroWaterfall::getLayerDate(const double y) const
{
    return gkWaterfallData ? gkWaterfallData->getLayerDate(y) : 0;
}

/**
 * @brief GkSpectroWaterfall::updateLayout
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::updateLayout()
{
    return;
}

/**
 * @brief GkSpectroWaterfall::allocateCurvesData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::allocateCurvesData()
{
    if (m_horCurveXAxisData || m_horCurveYAxisData || m_vertCurveXAxisData ||
        m_vertCurveYAxisData || !gkWaterfallData) {
        return;
    }

    const size_t layerPoints = gkWaterfallData->getLayerPoints();
    const double dXMin = gkWaterfallData->getXMin();
    const double dXMax = gkWaterfallData->getXMax();
    const size_t historyExtent = gkWaterfallData->getMaxHistoryLength();

    m_horCurveXAxisData = new double[layerPoints];
    m_horCurveYAxisData = new double[layerPoints];
    m_vertCurveXAxisData = new double[historyExtent];
    m_vertCurveYAxisData = new double[historyExtent];

    // Generate curve X-axis data
    const double dx = (dXMax - dXMin) / layerPoints; // x spacing
    m_horCurveXAxisData[0] = dXMin;
    for (size_t x = 1u; x < layerPoints; ++x) {
        m_horCurveXAxisData[x] = m_horCurveXAxisData[x - 1] + dx;
    }

    // Reset marker to the default position
    m_markerX = (dXMax - dXMin) / 2;
    m_markerY = historyExtent - 1;

    return;
}

/**
 * @brief GkSpectroWaterfall::freeCurvesData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::freeCurvesData()
{
    if (m_horCurveXAxisData) {
        delete [] m_horCurveXAxisData;
        m_horCurveXAxisData = nullptr;
    }

    if (m_horCurveYAxisData) {
        delete [] m_horCurveYAxisData;
        m_horCurveYAxisData = nullptr;
    }

    if (m_vertCurveXAxisData) {
        delete [] m_vertCurveXAxisData;
        m_vertCurveXAxisData = nullptr;
    }

    if (m_vertCurveYAxisData) {
        delete [] m_vertCurveYAxisData;
        m_vertCurveYAxisData = nullptr;
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::setupCurves
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::setupCurves()
{
    m_plotHorCurve->detachItems(QwtPlotItem::Rtti_PlotCurve, true);
    m_plotVertCurve->detachItems(QwtPlotItem::Rtti_PlotCurve, true);

    m_horCurve = new QwtPlotCurve;
    m_vertCurve = new QwtPlotCurve;

    // Horizontal Curve
    m_horCurve->attach(m_plotHorCurve);
    m_horCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_horCurve->setStyle(QwtPlotCurve::Lines);

    // Vertical Curve
    m_vertCurve->attach(m_plotVertCurve);
    m_vertCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_vertCurve->setStyle(QwtPlotCurve::Lines);

    m_plotVertCurve->setAxisScaleDraw(QwtPlot::yLeft, new GkWaterfallTimeScaleDraw(*this));
    return;
}

/**
 * @brief GkSpectroWaterfall::updateCurvesData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::updateCurvesData()
{
    // refresh curve's data
    const size_t currentHistory = gkWaterfallData->getHistoryLength();
    const size_t layerPts = gkWaterfallData->getLayerPoints();
    const size_t maxHistory = gkWaterfallData->getMaxHistoryLength();
    const double *wfData = gkWaterfallData->getData();

    const size_t markerY = m_markerY;
    if (markerY >= maxHistory) {
        return;
    }

    if (m_horCurveXAxisData && m_horCurveYAxisData) {
        std::copy(wfData + layerPts * markerY,
                  wfData + layerPts * (markerY + 1),
                  m_horCurveYAxisData);
        m_horCurve->setRawSamples(m_horCurveXAxisData, m_horCurveYAxisData, layerPts);
    }

    const double offset = gkWaterfallData->getOffset();

    if (currentHistory > 0 && m_vertCurveXAxisData && m_vertCurveYAxisData) {
        size_t dataIndex = 0;
        for (size_t layer = maxHistory - currentHistory; layer < maxHistory; ++layer, ++dataIndex) {
            const double z = gkWaterfallData->value(m_markerX, layer + size_t(offset));
            const double t = double(layer) + offset;
            m_vertCurveXAxisData[dataIndex] = z;
            m_vertCurveYAxisData[dataIndex] = t;
        }

        m_vertCurve->setRawSamples(m_vertCurveXAxisData, m_vertCurveYAxisData, currentHistory);
    }
    
    return;
}

/**
 * @brief GkSpectroWaterfall::setPickerEnabled
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param enabled
 */
void GkSpectroWaterfall::setPickerEnabled(const bool enabled)
{
    m_panner->setEnabled(!enabled);
    m_picker->setEnabled(enabled);
    m_zoomer->setEnabled(!enabled);
    // m_zoomer->zoom(0);

    // Clear plots?
    return;
}

/**
 * @brief GkSpectroWaterfall::autoRescale
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param rect
 */
void GkSpectroWaterfall::autoRescale(const QRectF &rect)
{
    Q_UNUSED(rect)
    if (m_zoomer->zoomRectIndex() == 0) {
        // Rescale axis to data range
        m_plotSpectrogram->setAxisAutoScale(QwtPlot::xBottom, true);
        m_plotSpectrogram->setAxisAutoScale(QwtPlot::yLeft, true);
        m_zoomer->setZoomBase();
        m_zoomActive = false;
    } else {
        m_zoomActive = true;
    }

    return;
}

/**
 * @brief GkSpectroWaterfall::selectedPoint
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param pt
 */
void GkSpectroWaterfall::selectedPoint(const QPointF &pt)
{
    setMarker(pt.x(), pt.y());
    return;
}

/**
 * @brief GkSpectroWaterfall::scaleDivChanged
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::scaleDivChanged()
{
    // apparently, m_inScaleSync is a hack that can be replaced by
    // blocking signals on a widget but that could be cumbersome
    // or not possible as the Qwt API doesn't provide any mean to do that
    if (m_inScaleSync) {
        return;
    }

    m_inScaleSync = true;

    QwtPlot* updatedPlot;
    int axisId;
    if (m_plotHorCurve->axisWidget(QwtPlot::xBottom) == sender()) {
        updatedPlot = m_plotHorCurve;
        axisId = QwtPlot::xBottom;
    } else if (m_plotSpectrogram->axisWidget(QwtPlot::xBottom) == sender()) {
        updatedPlot = m_plotSpectrogram;
        axisId = QwtPlot::xBottom;
    } else if (m_plotSpectrogram->axisWidget(QwtPlot::yLeft) == sender()) {
        updatedPlot = m_plotSpectrogram;
        axisId = QwtPlot::yLeft;
    } else {
        updatedPlot = nullptr;
    }

    if (updatedPlot) {
        QwtPlot* plotToUpdate;
        if (axisId == QwtPlot::xBottom) {
            plotToUpdate = (updatedPlot == m_plotHorCurve) ? m_plotSpectrogram : m_plotHorCurve;
        } else {
            plotToUpdate = m_plotVertCurve;
        }

        plotToUpdate->setAxisScaleDiv(axisId, updatedPlot->axisScaleDiv(axisId));
        updateLayout();
    }

    m_inScaleSync = false;
    return;
}

bool GkSpectroWaterfall::setColorMap(const ColorMaps::ControlPoints &colorMap) {
    return false;
}

ColorMaps::ControlPoints GkSpectroWaterfall::getColorMap() const
{
    return m_ctrlPts;
}

/**
 * @brief GkWaterfallTimeScaleDraw::GkWaterfallTimeScaleDraw
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param waterfall
 */
GkWaterfallTimeScaleDraw::GkWaterfallTimeScaleDraw(const GkSpectroWaterfall &waterfall) : m_waterfallPlot(waterfall)
{
    return;
}

/**
 * @brief GkWaterfallTimeScaleDraw::~GkWaterfallTimeScaleDraw
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
GkWaterfallTimeScaleDraw::~GkWaterfallTimeScaleDraw()
{
    return;
}

/**
 * @brief GkWaterfallTimeScaleDraw::label
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param v
 * @return
 */
QwtText GkWaterfallTimeScaleDraw::label(double v) const
{
    time_t ret = m_waterfallPlot.getLayerDate(v - m_waterfallPlot.getOffset());
    if (ret > 0) {
        m_dateTime.setTime_t(ret);
        // need something else other than time_t to have 'zzz'
        //return m_dateTime.toString("hh:mm:ss:zzz");
        return m_dateTime.toString("dd.MM.yy\nhh:mm:ss");
    }

    return QwtText();
}

/**
 * @brief GkZoomer::trackerTextF
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @param pos
 * @return
 */
QwtText GkZoomer::trackerTextF(const QPointF &pos) const
{
    QColor bg(Qt::white);
    bg.setAlpha(200);

    const double distVal = pos.x();
    QwtText text;
    if (m_spectro->data()) {
        QString date;
        const double histVal = pos.y();
        time_t timeVal = m_waterfallPlot.getLayerDate(histVal - m_waterfallPlot.getOffset());
        if (timeVal > 0) {
            m_dateTime.setTime_t(timeVal);
            date = m_dateTime.toString("dd.MM.yy - hh:mm:ss");
        }

        const double tempVal = m_spectro->data()->value(pos.x(), pos.y());
        text = QString("%1%2, %3: %4%5")
                .arg(distVal)
                .arg(m_waterfallPlot.m_xUnit)
                .arg(date)
                .arg(tempVal)
                .arg(m_waterfallPlot.m_zUnit);
    } else {
        text = QString("%1%2: -%3")
                .arg(distVal)
                .arg(m_waterfallPlot.m_xUnit)
                .arg(m_waterfallPlot.m_zUnit);
    }

    text.setBackgroundBrush( QBrush( bg ) );
    return text;
}
