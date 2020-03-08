/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
 **
 **
 **   If you have downloaded the source code for "Dekoder for Morse" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Dekoder for Morse is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Dekoder is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Dekoder for Morse.  If not, see <http://www.gnu.org/licenses/>.
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

        // Initialize the structure!
        calc_z_history = setDefMatrixStrucVals();

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

        QObject::connect(this, SIGNAL(sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<short> &, const int &, const size_t &)),
                         this, SLOT(applyData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<short> &, const int &, const size_t &)));

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

        date_plotter = new QTimer(this);
        QObject::connect(date_plotter, SIGNAL(timeout()), this, SLOT(appendDateTime()));
        date_plotter->start(SPECTRO_REFRESH_CYCLE_MILLISECS);

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

        QObject::connect(this, SIGNAL(stopSpectroRecv()), this, SLOT(stopSpectro()));

        //
        // Prepares the spectrograph / waterfall for the receiving of new data!
        //
        preparePlot();

        setAutoReplot(false);
        plotLayout()->setAlignCanvasToScales(true);
        gkSpectrogram->invalidateCache();
        replot();
    } catch (const std::exception &e) {
        HWND hwnd_spectro_gui_main;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_gui_main, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_gui_main);
    }

    return;
}

SpectroGui::~SpectroGui()
{
    emit stopSpectroRecv();

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

void SpectroGui::calcMatrixData(const std::vector<RawFFT> &values, const int &hanning_window_size,
                                const size_t &buffer_size,
                                std::promise<Spectrograph::MatrixData> matrix_data_promise)
{
    Q_UNUSED(buffer_size);

    std::mutex spectro_calc_matrix_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_calc_matrix_data_mtx);

    MatrixData matrix_ret_data = setDefMatrixStrucVals();
    GkTimingData timing_data;
    GkAxisData axis_data;

    try {
        std::vector<double> x_values;
        for (size_t i = 0; i < values.size(); ++i) {
            for (size_t j = 0; j < hanning_window_size; ++j) {
                const short x_axis_val = values.at(i).chunk_forward_0[j][0];
                x_values.push_back(x_axis_val);
            }
        }

        //
        // Modifies the received FFT data so that only every second value is kept, since the
        // values which are discarded are only garbage. Not sure why this is...
        //
        std::vector<short> x_values_modified(x_values.size() / 2);
        copy_every_nth(x_values.begin(), x_values.end(), x_values_modified.begin(), 2);

        matrix_ret_data.min_z_axis_val = *std::min_element(std::begin(x_values_modified), std::end(x_values_modified));
        matrix_ret_data.max_z_axis_val = *std::max_element(std::begin(x_values_modified), std::end(x_values_modified));

        if (std::isnan(matrix_ret_data.min_z_axis_val) || std::isnan(matrix_ret_data.max_z_axis_val)) {
            throw std::runtime_error(tr("The minimum or maximum value given for the z-axis is NaN!").toStdString());
        }

        QVector<double> conv_x_values;
        for (int i = 0; i < x_values_modified.size(); ++i) {
            const short x_axis_val = x_values_modified.at(i);
            conv_x_values.push_back((double)x_axis_val); // Convert from `short` to `double`

            if (x_values_modified[i] > gkMatrixRaster->interval(Qt::ZAxis).maxValue()) {
                //
                // Warning: Do not delete the following without a very good reason!
                //
                gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(0, x_values_modified[i]));
                setAxisScale(QwtPlot::xTop, 0, x_values_modified[i]);
            }
        }

        qint64 time_now = QDateTime::currentMSecsSinceEpoch();
        spectro_latest_update = time_now;
        timing_data.curr_time = time_now;
        timing_data.relative_start_time = spectro_begin_time;

        matrix_ret_data.timing.push_back(timing_data);
        axis_data.z_interval.setMaxValue(matrix_ret_data.window_size);
        matrix_ret_data.z_data_calcs.insert(time_now, std::make_pair(conv_x_values, axis_data));
        matrix_ret_data.window_size = canvas()->width();

        conv_x_values.clear();
        conv_x_values.shrink_to_fit();
        x_values.clear();
        x_values.shrink_to_fit();
        x_values_modified.clear();
        x_values_modified.shrink_to_fit();
    } catch (const std::exception &e) {
        HWND hwnd_spectro_calc_matrix;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_calc_matrix, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_calc_matrix);
    }

    matrix_data_promise.set_value(matrix_ret_data);
    return;
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
        enablePlotRefresh = false;
    }

    return;
}

/**
 * @brief SpectroGui::stopSpectroRecv disables the receiving of audio data and stops any
 * functionality within the spectrograph itself, effectively disabling it.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::stopSpectro()
{
    date_plotter->stop();
    refresh_data_timer->stop();

    calc_z_history.z_data_calcs.clear();
    calc_z_history.timing.clear();
    calc_z_history.timing.shrink_to_fit();
    raw_plot_data.clear();
    raw_plot_data.shrink_to_fit();

    gkMatrixRaster->discardRaster();
    gkSpectrogram->invalidateCache();
    replot();

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
                           const std::vector<short> &raw_audio_data, const int &hanning_window_size,
                           const size_t &buffer_size)
{
    std::mutex spectro_apply_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_apply_data_mtx);

    try {
        // Start a new thread since it will block the current (GUI-based) thread otherwise...
        std::promise<MatrixData> calc_matrix_promise;
        std::future<MatrixData> calc_matrix_future = calc_matrix_promise.get_future();
        std::thread calc_matrix_thread(&SpectroGui::calcMatrixData, this, values, hanning_window_size, buffer_size, std::move(calc_matrix_promise));
        auto matrix_data = calc_matrix_future.get(); // TODO: Current source of blocking the GUI-thread; need to fix!

        for (const auto &mapped_data: matrix_data.z_data_calcs.toStdMap()) {
            calc_z_history.z_data_calcs.insert(mapped_data.first, mapped_data.second);
        }

        // Copy the timing data into the more permanent buffer...
        if (!matrix_data.timing.empty()) {
            calc_z_history.timing.assign(matrix_data.timing.begin(), matrix_data.timing.end());
        }

        //
        // Calculate the actual window size, as divided by the time (left y-axis) interval...
        //
        const size_t divide_window_by = (SPECTRO_Y_AXIS_SIZE / 1000);
        const size_t actual_window_size = (matrix_data.window_size / divide_window_by);

        calc_z_history.min_z_axis_val = matrix_data.min_z_axis_val;
        calc_z_history.max_z_axis_val = matrix_data.max_z_axis_val;
        calc_z_history.window_size = actual_window_size;
        calc_z_history.hanning_win = matrix_data.hanning_win;

        raw_plot_data = raw_audio_data;
        num_rows = (calc_z_history.z_data_calcs.size() / calc_z_history.window_size);

        calc_z_history.curr_axis_info.z_interval.setMinValue(std::abs(calc_z_history.min_z_axis_val));
        calc_z_history.curr_axis_info.z_interval.setMaxValue(std::abs(calc_z_history.max_z_axis_val));

        if (!calc_first_data) {
            calc_first_data = true;
        }

        static size_t y_axis_counter = 0;
        static size_t x_axis_counter = 0;
        if (!calc_z_history.z_data_calcs.empty() && enablePlotRefresh) {
            for (const auto &raster: calc_z_history.z_data_calcs) {
                if (raster.first.empty()) {
                    for (const auto &data: raster.first) {
                        ++x_axis_counter;
                        gkMatrixRaster->setValue(y_axis_counter, x_axis_counter, data);

                        if (x_axis_counter == calc_z_history.window_size) {
                            x_axis_counter = 0;
                            ++y_axis_counter;
                        }
                    }
                }
            }
        }

        calc_z_history.curr_axis_info.y_interval.setMinValue(spectro_begin_time);
        calc_z_history.curr_axis_info.y_interval.setMaxValue(spectro_latest_update);
        calc_z_history.curr_axis_info.x_interval.setMinValue(SPECTRO_BANDWIDTH_MIN_SIZE);
        calc_z_history.curr_axis_info.x_interval.setMaxValue(SPECTRO_BANDWIDTH_MAX_SIZE);

        calc_matrix_thread.join();
        replot();

        return;
    } catch (const std::exception &e) {
        HWND hwnd_spectro_apply_data;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_apply_data, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_apply_data);
    }

    return;
}

/**
 * @brief SpectroGui::preparePlot As the name hints at, this prepares the plot for the receiving
 * of new data and resets the spectrograph back to its original state.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::preparePlot()
{
    calc_z_history.curr_axis_info.y_interval.setMinValue(spectro_begin_time);
    calc_z_history.curr_axis_info.y_interval.setMaxValue(spectro_latest_update); // TODO: Just a temporary figure for the y-axis
    calc_z_history.curr_axis_info.x_interval.setMinValue(SPECTRO_BANDWIDTH_MIN_SIZE);
    calc_z_history.curr_axis_info.x_interval.setMaxValue(SPECTRO_BANDWIDTH_MAX_SIZE);

    return;
}

/**
 * @brief SpectroGui::setDefMatrixStrucVals Initializes the `GekkoFyre::Spectrograph::MatrixData()` structure, in
 * order to hopefully avoid errors and/or exceptions down the line.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The initialized `GekkoFyre::Spectrograph::MatrixData()` structure.
 */
MatrixData SpectroGui::setDefMatrixStrucVals()
{
    //
    // Set the default values, so there should hopefully be no errors for anything being empty/nullptr!
    //
    QwtInterval default_data;
    default_data.setMaxValue(0);
    default_data.setMinValue(0);

    MatrixData matrix_ret_data;
    GkAxisData axis_data;
    matrix_ret_data.z_data_calcs = QMap<qint64, std::pair<QVector<double>, GkAxisData>>();
    matrix_ret_data.window_size = 0;
    matrix_ret_data.min_z_axis_val = 0;
    matrix_ret_data.max_z_axis_val = 0;
    axis_data.z_interval = default_data;
    axis_data.y_interval = default_data;
    axis_data.x_interval = default_data;

    matrix_ret_data.curr_axis_info = axis_data;
    matrix_ret_data.timing = std::vector<GkTimingData>();

    return matrix_ret_data;
}

/**
 * @brief SpectroGui::convMapToVec returns the FFT data from the mapped z-axis information.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param z_calc_information The stored and mapped results of the calculated FFT z-axis info.
 * @return The to be extracted 2D vector.
 */
QVector<double> SpectroGui::convMapToVec(const QMap<qint64, std::pair<QVector<double>, GkAxisData>> &z_calc_information)
{
    std::mutex spectro_conv_map_vec_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_conv_map_vec_mtx);

    try {
        QVector<double> ret_val;
        for (const auto &curr_data: z_calc_information.toStdMap()) {
            //
            // This data is relevant to the current time on the user's local system, in
            // the UTC timezone.
            //
            if (curr_data.first == spectro_latest_update) {
                ret_val = curr_data.second.first;
                break;
            }
        }

        return ret_val;
    } catch (const std::exception &e) {
        HWND hwnd_spectro_conv_vec;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_conv_vec, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_conv_vec);
    }

    return QVector<double>();
}

/**
 * @brief SpectroGui::mergeVecsForMatrix
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param z_calc_information
 * @return
 * @see GekkoFyre::SpectroGui::applyData().
 */
QVector<double> SpectroGui::mergeVecsForMatrix(const QMap<qint64, std::pair<QVector<double>, GkAxisData>> &z_calc_data)
{
    std::mutex spectro_merge_vecs_matrix_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_merge_vecs_matrix_mtx);

    try {
        if (!z_calc_data.empty()) {
            QVector<double> merged_data;
            for (const auto &to_merge: z_calc_data.toStdMap()) {
                if (!to_merge.second.first.empty()) {
                    //
                    // Merge all the disparate vectors into one, big vector!
                    //
                    std::copy(to_merge.second.first.begin(), to_merge.second.first.end(), std::back_inserter(merged_data));
                }
            }

            return merged_data;
        }
    } catch (const std::exception &e) {
        HWND hwnd_spectro_merge_vec;
        gkStringFuncs->modalDlgBoxOk(hwnd_spectro_merge_vec, tr("Error!"), tr("An error occurred during the handling of waterfall / spectrograph data!\n\n%1").arg(e.what()), MB_ICONERROR);
        DestroyWindow(hwnd_spectro_merge_vec);
    }

    return QVector<double>();
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

void SpectroGui::setResampleMode(int mode)
{
    SpectroRasterData *rasterData = dynamic_cast<SpectroRasterData *>(gkSpectrogram->data());
    rasterData->setResampleMode(static_cast<QwtMatrixRasterData::ResampleMode>(mode));

    gkSpectrogram->invalidateCache();
    replot();
}

void SpectroGui::calcInterval()
{
    std::mutex spectro_calc_interval_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_calc_interval_mtx);

    gkMatrixRaster->setInterval(Qt::XAxis, QwtInterval(calc_z_history.curr_axis_info.x_interval.minValue(),
                                                       calc_z_history.curr_axis_info.x_interval.maxValue()));
    gkMatrixRaster->setInterval(Qt::YAxis, QwtInterval(calc_z_history.curr_axis_info.y_interval.minValue(),
                                                       calc_z_history.curr_axis_info.y_interval.maxValue()));
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
