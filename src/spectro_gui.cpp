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
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_magnifier.h>
#include <cmath>
#include <algorithm>
#include <QList>
#include <QColormap>
#include <QTimer>

using namespace GekkoFyre;
using namespace Spectrograph;

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
SpectroGui::SpectroGui(QWidget *parent) : gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    gkMapType = Spectrograph::GkColorMap::HueMap;

    QObject::connect(this, SIGNAL(refresh()), this, SLOT(updateSpectro()));

    const size_t actual_num_threads = boost::thread::hardware_concurrency();
    size_t threads_to_use = 0;
    if ((actual_num_threads / 4) >= 2) {
        threads_to_use = (actual_num_threads / 4);
    } else {
        threads_to_use = 2;
    }

    gkCanvas = new QwtPlotCanvas(this);
    gkSpectrogram = new QwtPlotSpectrogram();
    gkSpectrogram->setRenderThreadCount(threads_to_use); // use system specific thread count
    gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    gkSpectrogram->setRenderHint(QwtPlotItem::RenderAntialiased);
    // gkSpectrogram->attach(this);

    gkMatrixRaster = new QwtMatrixRasterData();
    setCanvas(gkCanvas);

    axis_y_right = new QwtScaleWidget(axisWidget(QwtPlot::yRight));
    axis_y_right->setHidden(true);
    z_interval = new QwtInterval(gkMatrixRaster->interval(Qt::ZAxis));

    // A color bar on the right axis
    QwtScaleWidget *right_axis = axisWidget(QwtPlot::yRight);
    right_axis->setTitle("Intensity");
    right_axis->setColorBarWidth(40);
    right_axis->setColorBarEnabled(true);
    right_axis->setColorMap(*z_interval, new LinearColorMapRGB());

    setColorMap(gkMapType);

    //
    // Instructions!
    // ---------------------------------------
    // Zooming is the Left Button on the mouse
    // Panning is Middle Button, again by the mouse
    // Right-click zooms out by '1'
    // Ctrl + Right-click will zoom out to full-size
    //

    zoomer = new GkZoomer(gkCanvas);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);

    QwtPlotPanner *panner = new QwtPlotPanner(gkCanvas);
    panner->setAxisEnabled(QwtPlot::yRight, false);
    panner->setMouseButton(Qt::MidButton);

    QwtPlotMagnifier *magnifier = new QwtPlotMagnifier(gkCanvas);
    magnifier->setMouseButton(Qt::NoButton);

    //
    // Avoid jumping when labels with either more or less digits appear or
    // disappear when scrolling vertically...
    //

    const QFontMetrics fm(axisWidget(QwtPlot::yLeft)->font());
    QwtScaleDraw *sd = axisScaleDraw(QwtPlot::yLeft); // Note: This pointer is cleaned up automatically
    sd->setMinimumExtent(fm.width("100.00"));

    const QColor c(Qt::darkBlue);
    zoomer->setRubberBandPen(c);
    zoomer->setTrackerPen(c);

    gkSpectrogram->setData(gkMatrixRaster);
    plotLayout()->setAlignCanvasToScales(true);

    QList<double> contour_levels;
    for (double level = 1.0; level < 16384; level += 16.0) {
        contour_levels += level;
    }

    gkSpectrogram->setContourLevels(contour_levels);

    //
    // Configures the x-axis and y-axis for the spectrograph!
    //
    setAxisScale(QwtPlot::xBottom, 0.0, 3.0);
    setAxisMaxMinor(QwtPlot::xBottom, 0);
    enableAxis(QwtPlot::yRight, true);

    time_already_set = false;
    QTimer *time_manager = new QTimer(this);
    QObject::connect(time_manager, SIGNAL(timeout()), this, SLOT(manageTimeFlow()));
    QObject::connect(time_manager, SIGNAL(timeout()), this, SLOT(modifyAxisInterval()));
    time_manager->start(AUDIO_SPECTRO_UPDATE_MILLISECS);

    manage_time_flow = boost::thread(&SpectroGui::manageTimeFlow, this);
    modify_axis_interval = boost::thread(&SpectroGui::modifyAxisInterval, this);

    manage_time_flow.detach();
    modify_axis_interval.detach();

    modifyAxisInterval();
    emit refresh();
}

SpectroGui::~SpectroGui()
{
    if (manage_time_flow.joinable()) {
        manage_time_flow.join();
    }

    if (modify_axis_interval.joinable()) {
        modify_axis_interval.join();
    }
}

void SpectroGui::showContour(const int &toggled)
{
    std::mutex spectro_contour_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_contour_mtx);
    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, toggled);

    emit refresh();

    return;
}

void SpectroGui::showSpectrogram(const bool &toggled)
{
    std::mutex spectro_show_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_show_mtx);
    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, toggled);
    gkSpectrogram->setDefaultContourPen(toggled ? QPen(Qt::black, 0) : QPen(Qt::NoPen));

    emit refresh();

    return;
}

void SpectroGui::setColorMap(const Spectrograph::GkColorMap &map)
{
    std::mutex spectro_color_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_color_mtx);

    gkMapType = Spectrograph::GkColorMap::HueMap;

    int alpha = gkAlpha;
    switch (map) {
    case GkColorMap::HueMap:
        {
            gkSpectrogram->setColorMap(new HueColorMap());
            axis_y_right->setColorMap(*z_interval, new HueColorMap());
            break;
        }
    case GkColorMap::AlphaMap:
        {
            alpha = 255;
            gkSpectrogram->setColorMap(new AlphaColorMap());
            axis_y_right->setColorMap(*z_interval, new AlphaColorMap());
            break;
        }
    case GkColorMap::RGBMap:
        break;
    default:
        {
            gkSpectrogram->setColorMap(new LinearColorMapRGB());
            axis_y_right->setColorMap(*z_interval, new LinearColorMapRGB());
            break;
        }
    }

    gkSpectrogram->setAlpha(alpha);

    emit refresh();

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

        emit refresh();
    }

    return;
}

void SpectroGui::setTheme(const QColor &colour)
{
    std::mutex spectro_theme_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_theme_mtx);

    QwtPlotZoomer *zoomer = new GkZoomer(gkCanvas);
    zoomer->setRubberBandPen(colour);
    zoomer->setTrackerPen(colour);
}

/**
 * @brief SpectroGui::setXAxisRange controls the FFT data as it is calculated.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param x_min The most minimum response received.
 * @param x_max The most largest response received.
 */
void SpectroGui::setXAxisRange(const double &x_min, const double &x_max)
{
    x_min_ = x_min;
    x_max_ = x_max;

    if (x_min > x_max_) {
        x_max_ = x_max;
    }

    if (std::isgreaterequal(x_min, 0.0)) {
        x_min_ = x_min;
    } else {
        x_min_ = 0.0;
    }

    // plotLayout()->setAlignCanvasToScales(true);

    return;
}

/**
 * @brief SpectroGui::setYAxisRange Controls the spectrograph's timing scale.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param y_min The start of when receiving began.
 * @param y_max The current time as of now.
 */
void SpectroGui::setYAxisRange(const double &y_min, const double &y_max)
{
    y_min_ = y_min;
    y_max_ = y_max;

    if (y_min > y_max_) {
        y_max_ = y_max;
    }

    if (std::isgreaterequal(y_min, 0.0)) {
        y_min_ = y_min;
    } else {
        y_min_ = 0.0;
    }

    // plotLayout()->setAlignCanvasToScales(true);

    return;
}

/**
 * @brief SpectroGui::setZAxisRange
 * @param z_min
 * @param z_max
 */
void SpectroGui::setZAxisRange(const double &z_min, const double &z_max)
{
    Q_UNUSED(z_min);
    z_min_ = 0.00;

    if (z_max > z_max_) {
        z_max_ = z_max;
    }

    setAxisScale(QwtPlot::yRight, z_min_, z_max_);
    emit refresh();

    return;
}

/**
 * @brief SpectroGui::manageTimeFlow Manages the flow of time and therefore the y-axis as
 * a result, inserting timestamps where needed and updating the y-axis when required.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @see SpectroGui::setTimeFlow().
 */
void SpectroGui::manageTimeFlow()
{
    std::mutex spectro_manage_time_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_manage_time_mtx);

    static std::chrono::system_clock::time_point time_now;
    if (!time_already_set) {
        time_now = std::chrono::system_clock::now();
        time_already_set = true;
    }

    const std::chrono::system_clock::time_point time_curr = std::chrono::system_clock::now();
    setTimeFlow(time_now, time_curr);

    return;
}

/**
 * @brief SpectroGui::setTimeFlow Sets the time flow quotients to the x-axis and y-axis so
 * that the scales appear as they should on the spectrograph / waterfall.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param time_start The beginning time for when the application was launched.
 * @param time_curr The timestamp for as of right now.
 * @see SpectroGui::manageTimeFlow().
 */
void SpectroGui::setTimeFlow(const std::chrono::system_clock::time_point &time_start,
                             const std::chrono::system_clock::time_point &time_curr)
{
    std::mutex spectro_set_time_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_set_time_mtx);

    auto start_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time_start);
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time_curr);
    auto start_epoch = start_ms.time_since_epoch();
    auto now_epoch = now_ms.time_since_epoch();

    auto start_count = start_epoch.count();
    auto now_count = now_epoch.count();
    setYAxisRange(start_count, now_count);

    // setAxisScale(QwtPlot::yLeft, 0.0, 3.0);
    // setAxisMaxMinor(QwtPlot::yLeft, 0);

    return;
}

/**
 * @brief SpectroGui::setMatrixData Updates the spectrograph / waterfall with the relevant data samples
 * as required, inserting the calculated FFT results into a value matrix.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values The actual, STFT calculations themselves.
 * @param hanning_window_size The size of the calculated hanning window, required for calculating the
 * requisite FFT samples.
 * @param buffer_size The size of the audio buffer itself.
 */
void SpectroGui::setMatrixData(const std::vector<RawFFT> &values, const int &hanning_window_size,
                               const size_t &buffer_size)
{
    std::mutex spectro_matrix_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_matrix_data_mtx);

    std::vector<double> x_values;
    for (size_t i = 0; i < values.size(); ++i) {
        for (size_t j = 0; j < hanning_window_size; ++j) {
            const short x_axis_val = *values.at(i).chunk_forward_0[j];
            x_values.push_back(x_axis_val);
        }
    }

    //
    // Modifies the received FFT data so that only every second value is kept, since the
    // values which are discarded are only garbage. Not sure why this is...
    //
    std::vector<short> x_values_modified(x_values.size() / 2);
    copy_every_nth(x_values.begin(), x_values.end(), x_values_modified.begin(), 2);

    // const size_t numColumns = x_values_modified.size();
    // const size_t rows = (buffer_size / numColumns);
    const size_t y_axis_size = SPECTRO_BANDWIDTH_SIZE; // The amount of bandwidth we wish to display on the spectrograph / waterfall

    // double minZValue = *std::min_element(std::begin(x_values_modified), std::end(x_values_modified));
    double maxZValue = *std::max_element(std::begin(x_values_modified), std::end(x_values_modified));

    //
    // Update the intervals for both the x-axis and z-axis. The y-axis is instead handled
    // by the functions, SpectroGui::manageTimeFlow() and SpectroGui::setTimeFlow().
    //
    setXAxisRange(0, SPECTRO_BANDWIDTH_SIZE);
    setZAxisRange(0, maxZValue);

    size_t bandwidth_counter = 0;
    static size_t y_axis_incr = 0; // Calculates how many columns we are filling up within the spectrograph / waterfall
    for (int i = 0; i < x_values_modified.size(); ++i) {
        ++bandwidth_counter; // Calculates when we've filled up a column with the given amount of bandwidth
        QVector<double> temp_vec;
        const short x_axis_val = x_values_modified.at(i);
        temp_vec.push_back((double)x_axis_val); // Convert from `short` to `double`

        //
        // Every time we fill up a line to the amount of bandwidth we wish to
        // show, go to the next column...
        //
        if (bandwidth_counter % y_axis_size == 0) {
            gkMatrixRaster->setValueMatrix(temp_vec, y_axis_incr);
            temp_vec.clear();
            temp_vec.shrink_to_fit();
            bandwidth_counter = 0;
            ++y_axis_incr;
        }
    }

    x_values.clear();
    x_values.shrink_to_fit();
    x_values_modified.clear();
    x_values_modified.shrink_to_fit();
    setColorMap(gkMapType);

    emit refresh();
}

void SpectroGui::setResampleMode(int mode)
{
    RasterDataTestFunc *rasterData = static_cast<RasterDataTestFunc *>(gkSpectrogram->data());
    rasterData->setResampleMode(static_cast<QwtMatrixRasterData::ResampleMode>(mode));

    emit refresh();
}

void SpectroGui::updateSpectro()
{
    gkSpectrogram->invalidateCache();
    // updateAxes();
    replot();

    return;
}

/**
 * @brief SpectroGui::modifyAxisInterval Updates the interval for each axis, on a
 * constantly timed basis.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroGui::modifyAxisInterval()
{
    std::mutex spectro_interval_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_interval_mtx);

    gkMatrixRaster->setInterval(Qt::XAxis, QwtInterval(x_min_, x_max_, QwtInterval::ExcludeMaximum));
    gkMatrixRaster->setInterval(Qt::YAxis, QwtInterval(y_min_, y_max_, QwtInterval::ExcludeMaximum));
    gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(0, z_max_));

    emit refresh();

    return;
}
