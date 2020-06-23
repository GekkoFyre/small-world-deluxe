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
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <algorithm>
#include <utility>
#include <QColormap>
#include <QTimer>

using namespace GekkoFyre;
using namespace Spectrograph;

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 */
SpectroGui::SpectroGui(std::shared_ptr<StringFuncs> stringFuncs, const bool &enablePanner,
                       const bool &enableZoomer, QWidget *parent)
    : gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        gkStringFuncs = std::move(stringFuncs);

        gkMatrixRaster = new QwtMatrixRasterData();
        gkSpectrogram = new QwtPlotSpectrogram();
        gkSpectrogram->setRenderThreadCount(0); // Use system specific thread count
        gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
        gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::DisplayMode::ImageMode, true);

        already_read_data = false;
        calc_first_data = false;

        // These are said to use quite a few system resources!
        gkSpectrogram->setRenderHint(QwtPlotItem::RenderAntialiased);
        gkMatrixRaster->setResampleMode(ResampleMode::BilinearInterpolation);

        gkSpectrogram->setData(gkMatrixRaster);
        gkSpectrogram->attach(this);

        const static qint64 start_time = QDateTime::currentMSecsSinceEpoch();
        spectro_begin_time = start_time;
        spectro_latest_update = start_time; // Set the initial value for this too!
        enablePlotRefresh = false;

        QObject::connect(this, SIGNAL(sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)),
                         this, SLOT(applyData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<int> &, const int &, const size_t &)));

        //
        // https://qwt.sourceforge.io/class_qwt_matrix_raster_data.html#a69db38d8f920edb9dc3f0953ca16db8f
        // Set the type of colour-map used!
        //
        calc_z_history.curr_axis_info.z_interval = gkSpectrogram->data()->interval(Qt::ZAxis);
        colour_map = new LinearColorMapRGB(calc_z_history.curr_axis_info.z_interval);
        gkSpectrogram->setColorMap(colour_map);

        //
        // Setup y-axis scaling
        // https://www.qtcentre.org/threads/55345-QwtPlot-problem-with-date-time-label-at-major-ticks
        // https://stackoverflow.com/questions/57342087/qwtplotspectrogram-with-log-scales
        //
        date_scale_draw = new QwtDateScaleDraw(Qt::UTC);
        date_scale_engine = new QwtDateScaleEngine(Qt::UTC);
        date_scale_draw->setTimeSpec(Qt::TimeSpec::UTC);
        date_scale_engine->setTimeSpec(Qt::TimeSpec::UTC);
        date_scale_draw->setDateFormat(QwtDate::Second, tr("hh:mm:ss"));

        y_axis_num_minor_steps = 10;
        y_axis_num_major_steps = 2;
        y_axis_step_size = 1.0;

        setAxisScaleDraw(QwtPlot::yLeft, date_scale_draw);
        setAxisScaleEngine(QwtPlot::yLeft, date_scale_engine);
        // setAxisTitle(QwtPlot::yLeft, tr("Date & Time"));
        setAxisMaxMinor(QwtPlot::yLeft, spectro_begin_time);
        setAxisMaxMajor(QwtPlot::yLeft, spectro_latest_update);
        date_scale_engine->divideScale(spectro_begin_time, spectro_latest_update, y_axis_num_major_steps,
                                       y_axis_num_minor_steps, y_axis_step_size);

        right_axis = axisWidget(QwtPlot::xTop);
        right_axis->setTitle(tr("Intensity"));
        right_axis->setColorBarWidth(16);
        right_axis->setColorBarEnabled(true);
        right_axis->setColorMap(gkSpectrogram->data()->interval(Qt::ZAxis), colour_map);
        right_axis->setEnabled(true);
        enableAxis(QwtPlot::xTop);

        //
        // Setup x-axis scaling
        //
        setAxisTitle(QwtPlot::xBottom, tr("Frequency (Hz)"));
        setAxisLabelRotation(QwtPlot::xBottom, -50.0); // Puts the label markings (i.e. frequency response labels) at an angle
        setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignCenter | Qt::AlignBottom);

        //
        // Instructions!
        // ---------------------------------------
        // Zooming is the Left Button on the mouse
        // Panning is Middle Button, again by the mouse
        // Right-click zooms out by '1'
        // Ctrl + Right-click will zoom out to full-size
        //

        zoomer = new GkZoomer(canvas());
        zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
        zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
        zoomer->setEnabled(enableZoomer);

        QwtPlotPanner *panner = new QwtPlotPanner(canvas());
        panner->setAxisEnabled(QwtPlot::xBottom, false);
        panner->setMouseButton(Qt::MidButton);
        panner->setEnabled(enablePanner);

        const QColor c(Qt::darkBlue);
        zoomer->setRubberBandPen(c);
        zoomer->setTrackerPen(c);

        refresh_data_timer = new QTimer(this);
        QObject::connect(refresh_data_timer, SIGNAL(timeout()), this, SLOT(refreshData()));
        refresh_data_timer->start(SPECTRO_REFRESH_CYCLE_MILLISECS);

        refresh_data_thread = std::thread(&SpectroGui::refreshData, this);
        refresh_data_thread.detach();

        QObject::connect(this, SIGNAL(stopSpectroRecv(const bool &, const int &)),
                         this, SLOT(stopSpectro(const bool &, const int &)));

        setAutoReplot(false);
        plotLayout()->setAlignCanvasToScales(true);
        gkSpectrogram->invalidateCache();
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
    emit stopSpectroRecv(true);

    if (refresh_data_thread.joinable()) {
        refresh_data_thread.join();
    }

    //
    // Do not delete the raster data nor colour map, as this is done by the spectrogram's destructor anyway
    //

    delete zoomer;
    delete date_scale_draw;
    delete date_scale_engine;
    delete gkSpectrogram;
}

void SpectroGui::showSpectrogram(const bool &toggled)
{
    std::mutex spectro_show_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_show_mtx);

    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, toggled);
    gkSpectrogram->setDefaultContourPen(toggled ? QPen(Qt::black, 0) : QPen(Qt::NoPen));

    gkSpectrogram->invalidateCache();
    replot();

    return;
}

void SpectroGui::setAlpha(const int &alpha)
{
    std::mutex spectro_alpha_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_alpha_mtx);

    //
    // It does not make sense to set an alpha value in combination with a colour map
    // interpolating the alpha value.
    //

    gkAlpha = alpha;

    if (gkMapType != GkColorMap::AlphaMap) {
        gkSpectrogram->setAlpha(alpha);

        replot();
    }

    return;
}

void SpectroGui::setTheme(const QColor &colour)
{
    std::mutex spectro_theme_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_theme_mtx);

    QwtPlotZoomer *zoomer = new GkZoomer(canvas());
    zoomer->setRubberBandPen(colour);
    zoomer->setTrackerPen(colour);
}

/**
 * @brief SpectroGui::refreshData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::refreshData()
{
    std::mutex spectro_refresh_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_refresh_data_mtx);

    if ((calc_z_history.timing.size() > SPECTRO_MAX_BUFFER_SIZE) ||
            (calc_z_history.z_data_calcs.size() > SPECTRO_MAX_BUFFER_SIZE)) {
        if (!enablePlotRefresh) {
            enablePlotRefresh = true;
        }
    }

    // Clear the data buffers if they are too large, and over a defined size
    if (!calc_z_history.timing.empty()) {
        while (calc_z_history.timing.size() > SPECTRO_MAX_BUFFER_SIZE) {
            // Remove the excess data from the very beginning!
            calc_z_history.timing.erase(calc_z_history.timing.begin());
        }
    }

    // Clear the data buffers if they are too large, and over a defined size
    if (!calc_z_history.z_data_calcs.empty()) {
        while (calc_z_history.z_data_calcs.size() > SPECTRO_MAX_BUFFER_SIZE) {
            // Remove the excess data from the very beginning!
            calc_z_history.z_data_calcs.erase(calc_z_history.z_data_calcs.begin());
        }
    }

    if (enablePlotRefresh) {
        calcInterval();
        replot();
    }

    return;
}

/**
 * @brief SpectroGui::stopSpectroRecv disables the receiving of audio data and stops any
 * functionality within the spectrograph itself, effectively disabling it.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::stopSpectro(const bool &recording_is_stopped, const int &wait_time)
{
    Q_UNUSED(recording_is_stopped);
    Q_UNUSED(wait_time);

    refresh_data_timer->stop();

    calc_z_history.z_data_calcs.clear();
    calc_z_history.timing.clear();
    calc_z_history.timing.shrink_to_fit();
    raw_plot_data.clear();
    raw_plot_data.shrink_to_fit();

    gkMatrixRaster->discardRaster();
    gkSpectrogram->invalidateCache();

    return;
}

/**
 * @brief SpectroGui::applyData Updates the spectrograph / waterfall with the relevant data samples
 * as required, inserting the calculated FFT results into a value matrix.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values The actual, STFT calculations themselves.
 * @param hanning_window_size The size of the calculated hanning window, required for calculating the
 * requisite FFT samples.
 * @param buffer_size The size of the audio buffer itself.
 * @note <https://www.qtcentre.org/threads/67219-How-to-implement-QwtPlotSpectrogram-with-QwtMatrixRasterData>,
 * <https://qwt.sourceforge.io/class_qwt_date_scale_draw.html>,
 * <https://qwt.sourceforge.io/class_qwt_date_scale_engine.html>
 */
void SpectroGui::applyData(const std::vector<RawFFT> &values,
                           const std::vector<int> &raw_audio_data, const int &hanning_window_size,
                           const size_t &buffer_size)
{
    std::mutex spectro_apply_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_apply_data_mtx);

    try {
        return;
    } catch (const std::exception &e) {
        #if defined(_MSC_VER) && (_MSC_VER > 1900)
        HWND hwnd_spectro_apply_data = nullptr;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_apply_data, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_apply_data);
        #else
        gkStringFuncs->modalDlgBoxLinux(SDL_MESSAGEBOX_ERROR, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()));
        #endif
    }

    return;
}

/**
 * @brief SpectroGui::getEarliestPlottedTime garners the earliest plotted time from the saved
 * history buffer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param z_history_data The saved history buffer to be read.
 * @return The analyzed time.
 */
qint64 SpectroGui::getEarliestPlottedTime(const std::vector<GkTimingData> &timing_info)
{
    std::mutex spectro_earliest_plot_time_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_earliest_plot_time_mtx);

    qint64 earliest_plot_time = 0;
    if (!timing_info.empty()) {
        std::vector<qint64> tmp_timing_data;
        for (const auto &timing: timing_info) {
            tmp_timing_data.push_back(timing.curr_time);
        }

        earliest_plot_time = *std::min_element(std::begin(tmp_timing_data), std::end(tmp_timing_data));
    }

    return earliest_plot_time;
}

/**
 * @brief SpectroGui::getLatestPlottedTime garners the latest plotted time from the saved
 * history buffer.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param z_history_data The saved history buffer to be read.
 * @return The analyzed time.
 */
qint64 SpectroGui::getLatestPlottedTime(const std::vector<GkTimingData> &timing_info)
{
    std::mutex spectro_latest_plot_time_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_latest_plot_time_mtx);

    qint64 latest_plot_time = 0;
    if (!timing_info.empty()) {
        std::vector<qint64> tmp_timing_data;
        for (const auto &timing: timing_info) {
            tmp_timing_data.push_back(timing.curr_time);
        }

        latest_plot_time = *std::max_element(std::begin(tmp_timing_data), std::end(tmp_timing_data));
    }

    return latest_plot_time;
}

void SpectroGui::calcInterval()
{
    std::mutex spectro_calc_interval_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_calc_interval_mtx);

    gkMatrixRaster->setInterval(Qt::XAxis, QwtInterval(calc_z_history.curr_axis_info.x_interval.minValue(),
                                                       calc_z_history.curr_axis_info.x_interval.maxValue()));

    // TODO: This code below causes the application to crash quite severely! Not sure why...
    // gkMatrixRaster->setInterval(Qt::YAxis, QwtInterval(calc_z_history.curr_axis_info.y_interval.minValue(),
    //                                                    calc_z_history.curr_axis_info.y_interval.maxValue()));

    gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(calc_z_history.curr_axis_info.z_interval.minValue(),
                                                       calc_z_history.curr_axis_info.z_interval.maxValue()));
    right_axis->setColorMap(QwtInterval(calc_z_history.curr_axis_info.z_interval.minValue(),
                                        calc_z_history.curr_axis_info.z_interval.maxValue()), colour_map);

    setAxisScale(QwtPlot::yLeft, (getLatestPlottedTime(calc_z_history.timing) - SPECTRO_Y_AXIS_SIZE),
                 getLatestPlottedTime(calc_z_history.timing), SPECTRO_REFRESH_CYCLE_MILLISECS);

    plotLayout()->setAlignCanvasToScales(true);
    return;
}

/**
 * @brief GkSpectrograph::mouseDoubleClickEvent
 * @param e
 * @note <http://dronin.org/doxygen/ground/html/scopegadgetwidget_8cpp_source.html>
 */
GkSpectrograph::GkSpectrograph(QWidget *parent)
{}

GkSpectrograph::~GkSpectrograph()
{}

void GkSpectrograph::mouseDoubleClickEvent(QMouseEvent *e)
{
    // Reset the zoom-level of the spectrograph upon double-click of the mouse
    setAxisAutoScale(QwtPlot::yLeft, false);

    update();
    QwtPlot::mouseDoubleClickEvent(e);

    return;
}
