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
#include <boost/exception/all.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_date_scale_engine.h>
#include <qwt_date_scale_draw.h>
#include <cmath>
#include <algorithm>
#include <utility>
#include <QList>
#include <QColormap>
#include <QTimer>
#include <QDateTime>

using namespace GekkoFyre;
using namespace Spectrograph;

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @note <http://dronin.org/doxygen/ground/html/plotdata_8h_source.html>
 */
SpectroGui::SpectroGui(std::shared_ptr<StringFuncs> stringFuncs, QWidget *parent)
    : gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        gkStringFuncs = std::move(stringFuncs);

        //
        // Calculate the width of the x-axis on the spectrograph
        //
        spectro_window_width = calcWindowWidth();

        gkMatrixRaster = new QwtMatrixRasterData();
        gkSpectrogram = new QwtPlotSpectrogram();
        gkSpectrogram->setRenderThreadCount(0); // Use system specific thread count
        gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
        gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::DisplayMode::ImageMode, true);

        already_read_data = false;
        calc_first_data = false;
        autoscaleValueUpdated = 0;
        z_data_history = std::make_unique<QVector<double>>();
        time_data_history = std::make_unique<QVector<double>>();

        // These are said to use quite a few system resources!
        gkSpectrogram->setRenderHint(QwtPlotItem::RenderAntialiased);
        gkMatrixRaster->setResampleMode(ResampleMode::BilinearInterpolation);

        QList<double> contourLevels;
        for (double level = 0.5; level < 10.0; level += 1.0) {
            contourLevels += level;
        }

        gkSpectrogram->setContourLevels(contourLevels);

        gkSpectrogram->setData(gkMatrixRaster);
        gkSpectrogram->attach(this);

        const QwtInterval zInterval = gkSpectrogram->data()->interval(Qt::ZAxis);
        QObject::connect(this, SIGNAL(sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<short> &, const int &, const size_t &)),
                         this, SLOT(applyData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &, const std::vector<short> &, const int &, const size_t &)));

        // A color bar on the right axis
        rightAxis = axisWidget(QwtPlot::yRight);
        rightAxis->setTitle("Intensity");
        rightAxis->setColorBarEnabled(true);

        setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());
        enableAxis(QwtPlot::yRight);

        plotLayout()->setAlignCanvasToScales(true);

        //
        // https://qwt.sourceforge.io/class_qwt_matrix_raster_data.html#a69db38d8f920edb9dc3f0953ca16db8f
        // Set the type of colour-map used!
        //
        QwtScaleWidget *axis = axisWidget(QwtPlot::yRight);
        z_interval = gkSpectrogram->data()->interval(Qt::ZAxis);
        gkSpectrogram->setColorMap(new LinearColorMapRGB(z_interval));
        axis->setColorMap(z_interval, new LinearColorMapRGB(z_interval));

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

        QwtPlotPanner *panner = new QwtPlotPanner(canvas());
        panner->setAxisEnabled(QwtPlot::yRight, false);
        panner->setMouseButton(Qt::MidButton);

        const QColor c(Qt::darkBlue);
        zoomer->setRubberBandPen(c);
        zoomer->setTrackerPen(c);

        QTimer *calc_interval_timer = new QTimer(this);
        QObject::connect(calc_interval_timer, SIGNAL(timeout()), this, SLOT(calcInterval()));
        calc_interval_timer->start(SPECTRO_TIME_UPDATE_MILLISECS);

        calc_interval_thread = boost::thread(&SpectroGui::calcInterval, this);
        calc_interval_thread.detach();

        //
        // Prepares the spectrograph / waterfall for the receiving of new data!
        //
        preparePlot();

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
    if (calc_interval_thread.joinable()) {
        calc_interval_thread.join();
    }

    //
    // Do not delete the raster data, as this is done by the spectrogram's destructor anyway
    //

    delete zoomer;
    delete rightAxis;

    gkSpectrogram->detach();
    delete gkSpectrogram;
}

void SpectroGui::showContour(const int &toggled)
{
    std::mutex spectro_contour_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_contour_mtx);

    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, toggled);
    replot();

    return;
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

    //
    // Set the default values, so there should hopefully be no errors for anything being empty/nullptr!
    //
    QwtInterval default_data;
    default_data.setMaxValue(0);
    default_data.setMinValue(0);

    MatrixData matrix_ret_data;
    matrix_ret_data.z_data_calcs = QVector<double>();
    matrix_ret_data.y_axis_incr = 0;
    matrix_ret_data.num_cols = 0;
    matrix_ret_data.num_cols_double_pwr = 0;
    matrix_ret_data.min_z_axis_val = 0;
    matrix_ret_data.max_z_axis_val = 0;
    matrix_ret_data.y_axis_size = 0;
    matrix_ret_data.x_axis_size = 0;
    matrix_ret_data.z_interval = default_data;
    matrix_ret_data.y_interval = default_data;
    matrix_ret_data.x_interval = default_data;

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

        matrix_ret_data.y_axis_size = SPECTRO_BANDWIDTH_SIZE; // The amount of bandwidth we wish to display on the spectrograph / waterfall
        matrix_ret_data.min_z_axis_val = *std::min_element(std::begin(x_values_modified), std::end(x_values_modified));
        matrix_ret_data.max_z_axis_val = *std::max_element(std::begin(x_values_modified), std::end(x_values_modified));

        size_t bandwidth_counter = 0;
        static size_t y_axis_incr = 0; // Calculates how many columns we are filling up within the spectrograph / waterfall
        QVector<double> conv_x_values;
        for (int i = 0; i < x_values_modified.size(); ++i) {
            ++bandwidth_counter; // Calculates when we've filled up a column with the given amount of bandwidth
            const short x_axis_val = x_values_modified.at(i);
            conv_x_values.push_back((double)x_axis_val); // Convert from `short` to `double`

            if (x_values_modified[i] > gkMatrixRaster->interval(Qt::ZAxis).maxValue()) {
                gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(0, x_values_modified[i]));
                autoscaleValueUpdated = x_values_modified[i];
            }

            //
            // Every time we fill up a line to the amount of bandwidth we wish to
            // show, go to the next column...
            //
            if (bandwidth_counter % matrix_ret_data.y_axis_size == 0) {
                bandwidth_counter = 0;
                ++y_axis_incr;
            }
        }

        matrix_ret_data.z_data_calcs = conv_x_values;
        matrix_ret_data.y_axis_incr = y_axis_incr;

        //
        // Black magic is abound in these parts...
        //
        const size_t num_cols = y_axis_incr;
        const size_t num_cols_pwr = (y_axis_incr * y_axis_incr);
        const size_t num_cols_double_pwr = (num_cols_pwr * num_cols_pwr);

        matrix_ret_data.num_cols = num_cols;
        matrix_ret_data.num_cols_double_pwr = num_cols_double_pwr;

        matrix_ret_data.y_interval.setMaxValue(num_cols);
        matrix_ret_data.z_interval.setMaxValue(num_cols_double_pwr);

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
 * @brief SpectroGui::appendData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <http://dronin.org/doxygen/ground/html/spectrogramplotdata_8cpp_source.html>,
 * <http://dronin.org/doxygen/ground/html/spectrogramscopeconfig_8cpp_source.html>
 */
void SpectroGui::appendData()
{
    QDateTime time_now = QDateTime::currentDateTime();

    time_data_history->append(time_now.toTime_t() + time_now.time().msec() / 1000.0);
    while (time_data_history->back() - time_data_history->front() > SPECTRO_TIME_HORIZON) {
        time_data_history->pop_front();
        z_data_history->remove(0, std::fminl(spectro_window_width, z_data_history->size()));
    }

    return;
}

/**
 * @brief SpectroGui::removeStaleData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::removeStaleData()
{
    return;
}

void SpectroGui::clearPlots()
{
    time_data_history->clear();
    time_data_history->shrink_to_fit();
    z_data_history->clear();
    z_data_history->shrink_to_fit();
    resetAxisRanges();

    return;
}

/**
 * @brief SpectroGui::plotNewData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::plotNewData()
{
    removeStaleData();

    // Check for any new data
    if (!already_read_data) {
        // Plot new data

        // Check autoscale as for some reason, `QwtSpectrogram` doesn't support autoscale
        if (std::fabs(z_interval.maxValue()) == 0.0f) {
            double new_value = resetAutoscaleVal();
            if (new_value != 0) {
                rightAxis->setColorMap(QwtInterval(0, new_value), new LinearColorMapRGB(z_interval));
            }
        }
    }

    return;
}

/**
 * @brief SpectroGui::resetAutoscaleVal
 * @return
 */
double SpectroGui::resetAutoscaleVal()
{
    double tmp_val = autoscaleValueUpdated;
    autoscaleValueUpdated = 0;

    return tmp_val;
}

/**
 * @brief SpectroGui::applyData Updates the spectrograph / waterfall with the relevant data samples
 * as required, inserting the calculated FFT results into a value matrix.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values The actual, STFT calculations themselves.
 * @param hanning_window_size The size of the calculated hanning window, required for calculating the
 * requisite FFT samples.
 * @param buffer_size The size of the audio buffer itself.
 */
void SpectroGui::applyData(const std::vector<RawFFT> &values,
                           const std::vector<short> &raw_audio_data, const int &hanning_window_size,
                           const size_t &buffer_size)
{
    try {
        // Start a new thread since it will block the current (GUI-based) thread otherwise...
        std::promise<MatrixData> calc_matrix_promise;
        std::future<MatrixData> calc_matrix_future = calc_matrix_promise.get_future();
        std::thread calc_matrix_thread(&SpectroGui::calcMatrixData, this, values, hanning_window_size, buffer_size, std::move(calc_matrix_promise));
        const MatrixData calc_z_history = calc_matrix_future.get(); // TODO: Current source of blocking the GUI-thread; need to fix!

        raw_plot_data = raw_audio_data;

        const int new_window_width = calc_z_history.z_data_calcs.size(); // FFT output is already half, so no need to divide this value by two!
        if (new_window_width != spectro_window_width) {
            spectro_window_width = new_window_width;
        }

        z_interval.setMinValue(std::abs(calc_z_history.min_z_axis_val / 2));
        z_interval.setMaxValue(std::abs(calc_z_history.max_z_axis_val / 2));

        gkMatrixRaster->setValueMatrix(calc_z_history.z_data_calcs, spectro_window_width);
        if (!calc_first_data) {
            calc_first_data = true;
        }

        y_interval.setMaxValue(calc_z_history.num_cols);
        x_interval.setMaxValue(calc_z_history.num_cols_double_pwr);

        calc_matrix_thread.join();

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
    x_interval.setMaxValue(SPECTRO_BANDWIDTH_SIZE);
    x_interval.setMinValue(0.00);
    y_interval.setMaxValue(1024.00); // TODO: Just a temporary figure for the y-axis
    y_interval.setMinValue(0.00);

    //
    // Color map values
    //
    z_interval.setMinValue(0.00);
    // z_interval.setMaxValue(1.00);

    resetAxisRanges();

    return;
}

/**
 * @brief SpectroGui::resetAxisRanges returns the y, x, and z axis' back to their default
 * starting values.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::resetAxisRanges()
{
    // plot()->setAutoReplot(false);
    // setAxisScale(QwtPlot::yLeft, 0, 1024, 128);
    // setAxisScale(QwtPlot::yRight, 0, 12, 3);
    // setAxisScale(QwtPlot::xBottom, 0, 2048, 256);

    return;
}

int SpectroGui::calcWindowWidth()
{
    const int window_width = this->window()->size().rwidth();
    if ((window_width > 0) && (window_width <= MAX_TOLERATE_WINDOW_WIDTH)) {
        return window_width;
    } else {
        return -1;
    }
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

    gkMatrixRaster->setInterval(Qt::XAxis, QwtInterval(x_interval.minValue(), x_interval.maxValue(), QwtInterval::ExcludeMaximum));
    gkMatrixRaster->setInterval(Qt::YAxis, QwtInterval(y_interval.minValue(), y_interval.maxValue(), QwtInterval::ExcludeMaximum));
    gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(z_interval.minValue(), z_interval.maxValue()));
    resetAxisRanges();

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
    setAxisAutoScale(QwtPlot::yLeft, true);

    update();
    QwtPlot::mouseDoubleClickEvent(e);

    return;
}
