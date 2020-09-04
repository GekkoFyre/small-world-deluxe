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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/spectro_gui.hpp"
#include <qwt/qwt_plot_renderer.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_panner.h>
#include <algorithm>
#include <stdexcept>
#include <exception>
#include <utility>
#include <QColormap>
#include <QTimer>

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
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 * <https://github.com/medvedvvs/QwtWaterfall>
 */
SpectroGui::SpectroGui(QPointer<StringFuncs> stringFuncs, QPointer<GkEventLogger> eventLogger, const bool &enablePanner,
                       const bool &enableZoomer, QWidget *parent)
    : gkAlpha(255)
{
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        setParent(parent);
        gkStringFuncs = std::move(stringFuncs);
        gkEventLogger = std::move(eventLogger);

        //
        // This is the default graph-type that will be initialized when Small World Deluxe is launched by a user!
        //
        graph_in_use = GkGraphType::GkWaterfall;

        gkSpectro = std::make_unique<QwtPlotSpectrogram>();
        canvas = new QwtPlotCanvas();
        m_plotHorCurve = new QwtPlot();
        m_horCurve = std::make_unique<QwtPlotCurve>();

        setAxisScaleDraw(QwtPlot::yLeft, new GkSpectroTimeScaleDraw(*this));

        //
        // Initialize any variables here!
        //
        zoomActive = false;
        buf_total_size = 0;
        buf_overall_size = ((SPECTRO_Y_AXIS_SIZE / SPECTRO_REFRESH_CYCLE_MILLISECS) * GK_FFT_SIZE); // Obtain the total size of the matrix values!
        gkRasterBuf.reserve(buf_overall_size + GK_FFT_SIZE);

        canvas->setBorderRadius(8);
        canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
        canvas->setStyleSheet("border-radius: 8px; background-color: #000080");
        setCanvas(canvas);

        gkSpectro->setRenderThreadCount(0); // https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation?redirectedfrom=MSDN
        gkSpectro->setCachePolicy(QwtPlotRasterItem::PaintCache);
        gkSpectro->setDisplayMode(QwtPlotSpectrogram::DisplayMode::ImageMode, true);
        gkSpectro->setColorMap(new LinearColorMapRGB());

        // These are said to use quite a few system resources!
        gkSpectro->setRenderHint(QwtPlotItem::RenderAntialiased);

        QList<double> contourLevels;
        for (double level = 0.5; level < 10.0; level += 1.0) {
            contourLevels += level;
        }

        gkSpectro->setContourLevels(contourLevels);
        gkSpectro->setData(gkWaterfallData);
        // gkSpectrogram->attach(this);

        const static qint64 start_time = QDateTime::currentMSecsSinceEpoch();
        spectro_begin_time = start_time;
        spectro_latest_update = start_time; // Set the initial value for this too!

        //
        // Setup y-axis scaling
        // https://www.qtcentre.org/threads/55345-QwtPlot-problem-with-date-time-label-at-major-ticks
        // https://stackoverflow.com/questions/57342087/qwtplotspectrogram-with-log-scales
        //
        setAxisTitle(QwtPlot::yLeft, tr("Time (secs ago)"));
        setAxisLabelRotation(QwtPlot::yLeft, -50.0); // Puts the label markings (i.e. frequency response labels) at an angle
        setAxisLabelAlignment(QwtPlot::yLeft, Qt::AlignVCenter);

        setAxisScaleDraw(QwtPlot::yLeft, new GkSpectroTimeScaleDraw(*this));

        // const QwtInterval zInterval = gkSpectrogram->data()->interval(Qt::ZAxis);
        setAxisScale(QwtPlot::yLeft, spectro_begin_time, spectro_latest_update, 1000);
        enableAxis(QwtPlot::yLeft, true);

        //
        // https://qwt.sourceforge.io/class_qwt_matrix_raster_data.html#a69db38d8f920edb9dc3f0953ca16db8f
        // Set the type of colour-map used!
        //
        plotLayout()->setAlignCanvasToScales(true);

        curve = std::make_unique<QwtPlotCurve>();
        curve->setTitle("Frequency Response");
        curve->setPen(Qt::black, 4);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->attach(this);

        //
        // Colour Bar on the right axis
        // https://www.qtcentre.org/threads/69050-Simple-example-with-QwtPlot-multiaxes
        //
        top_x_axis = axisWidget(QwtPlot::xTop);
        top_x_axis->setTitle(tr("Bandwidth (Hz)"));
        top_x_axis->setEnabled(true);
        setAxisScale(QwtPlot::xTop, 100, 2500, 250);
        enableAxis(QwtPlot::xTop, true);

        enableAxis(QwtPlot::xBottom, false);

        right_y_axis = axisWidget(QwtPlot::yRight);
        right_y_axis->setColorBarWidth(16);
        right_y_axis->setColorBarEnabled(true);
        right_y_axis->setColorMap(gkSpectro->interval(Qt::ZAxis), new LinearColorMapRGB());
        right_y_axis->setEnabled(true);
        enableAxis(QwtPlot::yRight, true);

        //
        // Instructions!
        // ---------------------------------------
        // Zooming is the Left Button on the mouse
        // Panning is Middle Button, again by the mouse
        // Right-click zooms out by '1'
        // Ctrl + Right-click will zoom out to full-size
        //

        zoomer = new GkZoomer(canvas);
        zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        zoomer->setEnabled(enableZoomer);

        panner = new QwtPlotPanner(canvas);
        panner->setAxisEnabled(QwtPlot::xTop, true);
        panner->setMouseButton(Qt::MidButton);
        panner->setEnabled(enablePanner);

        const QColor c(Qt::darkBlue);
        zoomer->setRubberBandPen(c);
        zoomer->setTrackerPen(c);

        alignScales();
        gkSpectro->attach(this);

        setAutoReplot(false);
        plotLayout()->setAlignCanvasToScales(true);
        gkSpectro->invalidateCache();
        replot();
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_spectro_gui_main = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_gui_main, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_gui_main);
        #else
        gkEventLogger->publishEvent(tr("An error occurred during the handling of waterfall / spectrograph data!"), GkSeverity::Error, e.what(), true);
        #endif
    }

    return;
}

SpectroGui::~SpectroGui()
{}

/**
 * @brief SpectroGui::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values
 * @param numCols
 */
bool SpectroGui::insertData(const QVector<double> &values, const time_t &timestamp)
{
    try {
        mtx_raster_data.lock();
        if (!gkWaterfallData)
        {
            return false;
        }

        const bool bRet = gkWaterfallData->addData(values.data(), values.size(), timestamp);
        if (bRet)
        {
            updateCurvesData();

            // refresh spectrogram content and Y axis labels
            // gkSpectro->invalidateCache();

            auto const ySpectroLeftAxis = static_cast<GkSpectroTimeScaleDraw *>(axisScaleDraw(QwtPlot::yLeft));
            ySpectroLeftAxis->invalidateCache();

            const double currentOffset = getOffset();
            const size_t maxHistory = gkWaterfallData->getMaxHistoryLength();

            const QwtScaleDiv &yDiv = axisScaleDiv(QwtPlot::yLeft);
            const double yMin = (zoomActive) ? yDiv.lowerBound() + 1 : currentOffset;
            const double yMax = (zoomActive) ? yDiv.upperBound() + 1 : maxHistory + currentOffset;

            setAxisScale(QwtPlot::yLeft, yMin, yMax);
        }

        mtx_raster_data.unlock();
        return bRet;
    } catch (const std::exception &e) {
        throw std::runtime_error(tr("An error has occurred whilst doing calculations for the spectrograph / waterfall! Error:\n\n%1")
        .arg(QString::fromStdString(e.what())).toStdString());
    }

    return false;
}

/**
 * @brief SpectroGui::setDataDimensions
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dXMin
 * @param dXMax
 * @param historyExtent
 * @param layerPoints
 * @see
 */
void SpectroGui::setDataDimensions(const double &dXMin, const double &dXMax, const size_t &historyExtent,
                                   const size_t &layerPoints)
{
    gkWaterfallData = new GkWaterfallData<double>(dXMin, dXMax, historyExtent, layerPoints);


    return;
}

/**
 * @brief SpectroGui::getDataDimensions
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dXMin
 * @param dXMax
 * @param historyExtent
 * @param layerPoints
 * @see SpectroGui::setDataDimensions().
 */
void SpectroGui::getDataDimensions(double &dXMin, double &dXMax, size_t &historyExtent, size_t &layerPoints) const
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
 * @brief SpectroGui::alignScales will align the scales to the canvas frame.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::alignScales()
{
    std::lock_guard<std::mutex> lck_guard(mtx_spectro_align_scales);

    for (int i = 0; i < QwtPlot::axisCnt; ++i) {
        QwtScaleWidget *scale_widget = axisWidget(i);
        if (scale_widget) {
            scale_widget->setMargin(0);
        }

        QwtScaleDraw *scale_draw = axisScaleDraw(i);
        if (scale_draw) {
            scale_draw->enableComponent(QwtAbstractScaleDraw::Backbone, false);
        }
    }

    plotLayout()->setAlignCanvasToScales(true);

    return;
}

/**
 * @brief SpectroGui::changeSpectroType
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param graph_type
 * @param enable
 */
void SpectroGui::changeSpectroType(const GekkoFyre::Spectrograph::GkGraphType &graph_type)
{
    try {
        switch (graph_type) {
        case GkGraphType::GkWaterfall:
            graph_in_use = GkGraphType::GkWaterfall;
            break;
        case GkGraphType::GkSinewave:
            graph_in_use = GkGraphType::GkSinewave;
            break;
        case GkGraphType::GkMomentInTime:
            graph_in_use = GkGraphType::GkMomentInTime;
            break;
        default:
            break;
        }
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_spectro_gui_main = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_gui_main, tr("Error!"), e.what(), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_gui_main);
        #else
        gkEventLogger->publishEvent(e.what(), GkSeverity::Error, "", true);
        #endif
    }

    return;
}

/**
 * @brief SpectroGui::updateFFTSize
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void SpectroGui::updateFFTSize(const int &value)
{
    return;
}

/**
 * @brief SpectroGui::updateCurvesData
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::updateCurvesData()
{
    // refresh curve's data
    const size_t currentHistory = gkWaterfallData->getHistoryLength();
    const size_t layerPts = gkWaterfallData->getLayerPoints();
    const size_t maxHistory = gkWaterfallData->getMaxHistoryLength();
    const double* wfData = gkWaterfallData->getData();

    const size_t markerY = m_markerY;
    if (markerY >= maxHistory)
    {
        return;
    }

    if (!m_horCurveXAxisData.isEmpty() && !m_horCurveYAxisData.isEmpty()) {
        std::copy(wfData + layerPts * markerY,
                  wfData + layerPts * (markerY + 1),
                  std::back_inserter(m_horCurveYAxisData));
        m_horCurve->setRawSamples(m_horCurveXAxisData.data(), m_horCurveYAxisData.data(), layerPts);
    }

    const double offset = gkWaterfallData->getOffset();
    
    return;
}

/**
 * @brief SpectroGui::allocateCurvesData
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::allocateCurvesData()
{
    if (m_horCurveXAxisData.isEmpty() || m_horCurveYAxisData.isEmpty() || !gkWaterfallData) {
        return;
    }

    const size_t layerPoints = gkWaterfallData->getLayerPoints();
    const double dXMin = gkWaterfallData->getXMin();
    const double dXMax = gkWaterfallData->getXMax();
    const size_t historyExtent = gkWaterfallData->getMaxHistoryLength();

    m_horCurveXAxisData.reserve(layerPoints);
    m_horCurveYAxisData.reserve(layerPoints);

    // Generate curve x-axis data
    const double dx = (dXMax - dXMin) / layerPoints; // x-axis spacing
    m_horCurveXAxisData[0] = dXMin;
    for (size_t x = 1u; x < layerPoints; ++x) {
        m_horCurveXAxisData[x] = m_horCurveXAxisData[x - 1] + dx;
    }

    // Reset marker to the default position
    m_markerX = (dXMax - dXMin) / 2;
    m_markerY = static_cast<double>(historyExtent) - 1;
}

/**
 * @brief SpectroGui::setupCurves
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::setupCurves()
{
    m_plotHorCurve->detachItems(QwtPlotItem::Rtti_PlotCurve, true);

    //
    // Horizontal Curve
    //
    m_horCurve->attach(m_plotHorCurve);
    m_horCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_horCurve->setStyle(QwtPlotCurve::Lines);
}

/**
 * @brief SpectroGui::getLayerDate
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>
 * @param y
 * @return
 */
std::time_t SpectroGui::getLayerDate(const double &y) const
{
    return gkWaterfallData ? gkWaterfallData->getLayerDate(y) : 0;
}

/**
 * @brief SpectroGui::setRange
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>,
 * * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dLower
 * @param dUpper
 */
void SpectroGui::setRange(double dLower, double dUpper)
{
    if (dLower > dUpper) {
        std::swap(dLower, dUpper);
    }

    if (axisEnabled(QwtPlot::yRight)) {
        setAxisScale(QwtPlot::yRight, dLower, dUpper);

        QwtScaleWidget *axis = axisWidget(QwtPlot::yRight);
        if (axis->isColorBarEnabled()) {
            // Waiting a proper method to get a reference to the QwtInterval
            // instead of resetting a new color map to the axis !
            if (!m_bColorBarInitialized) {
                m_bColorBarInitialized = true;
            }

            axis->setColorMap(QwtInterval(dLower, dUpper), new LinearColorMapRGB());
        }
    }

    // set vertical plot's X axis and horizontal plot's Y axis scales to the color bar min/max
    m_plotHorCurve->setAxisScale(QwtPlot::yLeft, dLower, dUpper);

    if (gkWaterfallData) {
        gkWaterfallData->setRange(dLower, dUpper);
    }

    gkSpectro->invalidateCache();
}
