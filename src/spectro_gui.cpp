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

#include "spectro_gui.hpp"
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_panner.h>
#include <algorithm>
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

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 * <https://github.com/medvedvvs/QwtWaterfall>
 */
SpectroGui::SpectroGui(std::shared_ptr<StringFuncs> stringFuncs, const bool &enablePanner,
                       const bool &enableZoomer, QWidget *parent)
    : gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        gkStringFuncs = std::move(stringFuncs);

        gkRasterData = std::make_unique<GkSpectroRasterData>();
        gkMatrixData = std::make_unique<QwtMatrixRasterData>();
        color_map = new LinearColorMapRGB();
        canvas = new QwtPlotCanvas();

        // y_axis_num_minor_steps = 5.0f;
        // y_axis_num_major_steps = 3.0f;
        // y_axis_step_size = 1.0f;

        canvas->setBorderRadius(8);
        canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
        canvas->setStyleSheet("border-radius: 8px; background-color: #389638");
        setCanvas(canvas);

        gkRasterData->setRenderThreadCount(0); // Use system specific thread count
        gkRasterData->setCachePolicy(QwtPlotRasterItem::PaintCache);
        gkRasterData->setDisplayMode(QwtPlotSpectrogram::DisplayMode::ImageMode, true);
        gkRasterData->setColorMap(color_map);

        // These are said to use quite a few system resources!
        gkRasterData->setRenderHint(QwtPlotItem::RenderAntialiased);

        QList<double> contourLevels;
        for (double level = 0.5; level < 10.0; level += 1.0) {
            contourLevels += level;
        }

        gkRasterData->setContourLevels(contourLevels);
        gkRasterData->setData(gkMatrixData.get());
        // gkSpectrogram->attach(this);

        gkMatrixData->setInterval(Qt::XAxis, QwtInterval(0, 2500.0f));
        gkMatrixData->setInterval(Qt::ZAxis, QwtInterval(0, 5000.0f));

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
        setAxisLabelAlignment(QwtPlot::yLeft, Qt::AlignVCenter | Qt::AlignVCenter);

        date_scale_draw = new QwtDateScaleDraw(Qt::UTC);
        date_scale_engine = new QwtDateScaleEngine(Qt::UTC);
        date_scale_draw->setTimeSpec(Qt::TimeSpec::UTC);
        date_scale_engine->setTimeSpec(Qt::TimeSpec::UTC);
        date_scale_draw->setDateFormat(QwtDate::Second, tr("hh:mm:ss"));

        setAxisScaleDraw(QwtPlot::yLeft, date_scale_draw);
        setAxisScaleEngine(QwtPlot::yLeft, date_scale_engine);
        date_scale_engine->divideScale(spectro_begin_time, spectro_latest_update, y_axis_num_major_steps,
                                       y_axis_num_minor_steps, y_axis_step_size);

        // const QwtInterval zInterval = gkSpectrogram->data()->interval(Qt::ZAxis);
        setAxisScale(QwtPlot::yLeft, spectro_begin_time, spectro_latest_update, 1000);
        enableAxis(QwtPlot::yLeft, true);

        //
        // https://qwt.sourceforge.io/class_qwt_matrix_raster_data.html#a69db38d8f920edb9dc3f0953ca16db8f
        // Set the type of colour-map used!
        //
        plotLayout()->setAlignCanvasToScales(true);

        curve = new QwtPlotCurve();
        curve->setTitle("Frequency Response");
        curve->setPen(Qt::black, 4);
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->attach(this);

        //
        // Colour Bar on the right axis
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
        right_y_axis->setColorMap(gkRasterData->interval(Qt::ZAxis), color_map);
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
        gkRasterData->attach(this);

        setAutoReplot(false);
        plotLayout()->setAlignCanvasToScales(true);
        gkRasterData->invalidateCache();
        replot();
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_spectro_gui_main = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_gui_main, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_gui_main);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()));
        #endif
    }

    return;
}

SpectroGui::~SpectroGui()
{
    if (color_map != nullptr) {
        delete color_map;
    }

    if (canvas != nullptr) {
        delete canvas;
    }

    if (date_scale_draw != nullptr) {
        delete date_scale_draw;
    }

    if (date_scale_engine != nullptr) {
        delete date_scale_engine;
    }

    if (curve != nullptr) {
        delete curve;
    }

    if (panner != nullptr) {
        delete panner;
    }

    return;
}

void SpectroGui::insertData(const QVector<double> values, const int &numCols)
{
    gkMatrixData->setValueMatrix(values, numCols);

    return;
}

/**
 * @brief SpectroGui::alignScales will align the scales to the canvas frame.
 */
void SpectroGui::alignScales()
{
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

void SpectroGui::showSpectrogram(const bool &toggled)
{
    return;
}

/**
 * @brief SpectroGui::refreshDateTime refreshes any date/time objects within the spectrograph class.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::refreshDateTime(const qint64 &latest_time_update, const qint64 &time_since)
{
    setAxisScale(QwtPlot::yLeft, latest_time_update - SPECTRO_Y_AXIS_SIZE, latest_time_update);
    setAxisMaxMinor(QwtPlot::yLeft, 15);
    setAxisMaxMajor(QwtPlot::yLeft, 8);
    // setAxisScale(QwtPlot::xTop, x_axis_bandwidth_min_size, x_axis_bandwidth_max_size, 250);

    //
    // Breakup the FFT caclulations into specific time units!
    //
    gkMatrixData->setInterval(Qt::YAxis, QwtInterval(time_since, latest_time_update));

    // QwtScaleDiv y_axis_left_scale_div = QwtScaleDiv(y_axis_num_minor_steps, y_axis_num_major_steps);
    // setAxisScaleDiv(QwtPlot::yLeft, y_axis_left_scale_div);

    gkRasterData->invalidateCache();
    replot();

    return;
}

/**
 * @brief GkSpectroRasterData::draw
 * @author Thomas <https://stackoverflow.com/questions/57342087/qwtplotspectrogram-with-log-scales>
 * @param painter
 * @param xMap
 * @param yMap
 * @param canvasRect
 */
void GkSpectroRasterData::draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const
{
    QwtScaleMap xMapLin(xMap);
    QwtScaleMap yMapLin(yMap);

    auto const xi = data()->interval(Qt::XAxis);
    auto const yi = data()->interval(Qt::YAxis);

    auto const dx = xMapLin.transform(xMap.s1());
    xMapLin.setScaleInterval(xi.minValue(), xi.maxValue());
    auto const dy = yMapLin.transform(yMap.s2());
    yMapLin.setScaleInterval(yi.minValue(), yi.maxValue());

    xMapLin.setTransformation(new QwtNullTransform());
    yMapLin.setTransformation(new QwtNullTransform());

    QwtPlotSpectrogram::draw(painter, xMapLin, yMapLin, canvasRect.translated(dx, -dy));
}
