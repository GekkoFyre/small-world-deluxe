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
#include <qwt_scale_engine.h>
#include <qwt_date_scale_engine.h>
#include <qwt_date_scale_draw.h>
#include <cmath>
#include <algorithm>
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
 */
SpectroGui::SpectroGui(std::shared_ptr<StringFuncs> stringFuncs, QWidget *parent)
    : gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    try {
        gkStringFuncs = stringFuncs;

        gkMatrixRaster = new QwtMatrixRasterData();
        gkSpectrogram = new QwtPlotSpectrogram();
        gkSpectrogram->setRenderThreadCount(0); // Use system specific thread count
        gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
        gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::DisplayMode::ImageMode, true);

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

        // A color bar on the right axis
        QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
        rightAxis->setTitle("Intensity");
        rightAxis->setColorBarEnabled(true);

        setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());
        enableAxis(QwtPlot::yRight);

        plotLayout()->setAlignCanvasToScales(true);

        //
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
        calc_interval_timer->start(AUDIO_SPECTRO_UPDATE_MILLISECS);

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

/**
 * @brief SpectroGui::applyData Updates the spectrograph / waterfall with the relevant data samples
 * as required, inserting the calculated FFT results into a value matrix.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param values The actual, STFT calculations themselves.
 * @param hanning_window_size The size of the calculated hanning window, required for calculating the
 * requisite FFT samples.
 * @param buffer_size The size of the audio buffer itself.
 */
void SpectroGui::applyData(const std::vector<RawFFT> &values, const int &hanning_window_size, const size_t &buffer_size)
{
    std::mutex spectro_matrix_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_matrix_data_mtx);

    try {
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

        const size_t y_axis_size = SPECTRO_BANDWIDTH_SIZE; // The amount of bandwidth we wish to display on the spectrograph / waterfall
        double min_x_val = *std::min_element(std::begin(x_values_modified), std::end(x_values_modified));
        double max_x_val = *std::max_element(std::begin(x_values_modified), std::end(x_values_modified));

        x_interval.setMinValue(std::abs(min_x_val / 2));
        x_interval.setMaxValue(std::abs(max_x_val / 2));

        size_t bandwidth_counter = 0;
        static size_t y_axis_incr = 0; // Calculates how many columns we are filling up within the spectrograph / waterfall
        QVector<double> conv_x_values;
        for (int i = 0; i < x_values_modified.size(); ++i) {
            ++bandwidth_counter; // Calculates when we've filled up a column with the given amount of bandwidth
            const short x_axis_val = x_values_modified.at(i);
            conv_x_values.push_back((double)x_axis_val); // Convert from `short` to `double`

            //
            // Every time we fill up a line to the amount of bandwidth we wish to
            // show, go to the next column...
            //
            if (bandwidth_counter % y_axis_size == 0) {
                bandwidth_counter = 0;
                ++y_axis_incr;
            }
        }

        gkMatrixRaster->setValueMatrix(conv_x_values, y_axis_incr);

        //
        // Black magic is abound in these parts...
        //
        const size_t num_cols = y_axis_incr;
        const size_t num_cols_pwr = (y_axis_incr * y_axis_incr);
        const size_t num_cols_double_pwr = (num_cols_pwr * num_cols_pwr);
        y_interval.setMaxValue(num_cols);
        z_interval.setMaxValue(num_cols_double_pwr);

        conv_x_values.clear();
        conv_x_values.shrink_to_fit();
        x_values.clear();
        x_values.shrink_to_fit();
        x_values_modified.clear();
        x_values_modified.shrink_to_fit();

        replot();
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
    x_interval.setMinValue(0);
    y_interval.setMaxValue(1024); // TODO: Just a temporary figure for the y-axis
    y_interval.setMinValue(0);
    z_interval.setMinValue(0);

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
    SpectroRasterData *rasterData = static_cast<SpectroRasterData *>(gkSpectrogram->data());
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
    replot();

    return;
}
