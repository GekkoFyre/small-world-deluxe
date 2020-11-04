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
    return nullptr;
}

/**
 * @brief GkSpectroWaterfall::GkSpectroWaterfall
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 * <https://github.com/medvedvvs/QwtWaterfall>
 */
GkSpectroWaterfall::GkSpectroWaterfall(QPointer<StringFuncs> stringFuncs, QPointer<GkEventLogger> eventLogger, const bool &enablePanner,
                                       const bool &enableZoomer, QWidget *parent) : m_spectrogram(new QwtPlotSpectrogram), gkAlpha(255),
                                       QWidget(parent)
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
 * @brief GkSpectroWaterfall::refreshDateTime refreshes any date/time objects within the spectrograph class.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkSpectroWaterfall::refreshDateTime(const qint64 &latest_time_update, const qint64 &time_since)
{
    return;
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
    return;
}

/**
 * @brief GkSpectroWaterfall::freeCurvesData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::freeCurvesData()
{
    return;
}

/**
 * @brief GkSpectroWaterfall::setupCurves
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::setupCurves()
{
    return;
}

/**
 * @brief GkSpectroWaterfall::updateCurvesData
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 */
void GkSpectroWaterfall::updateCurvesData()
{
    return;
}
