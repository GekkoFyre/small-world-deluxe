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
#include <boost/thread.hpp>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <cmath>
#include <memory>
#include <algorithm>
#include <QList>
#include <QColormap>
#include <QPointer>
#include <QVector>
#include <QTimer>

using namespace GekkoFyre;
using namespace Spectrograph;

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
SpectroGui::SpectroGui(const int &num_data_points, QWidget *parent) : QwtPlot(parent), gkAlpha(255),
    x_axis_data_points(num_data_points)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    gkMapType = 0;
    z_axis_set = false;

    const size_t actual_num_threads = boost::thread::hardware_concurrency();
    size_t threads_to_use = 0;
    if ((actual_num_threads / 4) >= 2) {
        threads_to_use = (actual_num_threads / 4);
    } else {
        threads_to_use = 2;
    }

    gkSpectrogram = new QwtPlotSpectrogram();
    gkSpectrogram->setRenderThreadCount(threads_to_use); // use system specific thread count
    gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    gkSpectrogram->attach(this);

    axis_y_right = new QwtScaleWidget(axisWidget(QwtPlot::yRight));
    axis_y_right->setHidden(true);
    z_interval = new QwtInterval(interval(Qt::ZAxis));

    QList<double> contour_levels;
    for (double level = 0.5; level < 10.0; level += 1.0) {
        contour_levels += level;
    }

    gkSpectrogram->setContourLevels(contour_levels);

    // A color bar on the right axis
    // QwtScaleWidget *right_axis = axisWidget(QwtPlot::yRight);
    // right_axis->setTitle("Intensity");
    // right_axis->setColorBarWidth(40);
    // right_axis->setColorBarEnabled(true);
    // right_axis->setColorMap(*z_interval, new HueColorMap());

    plotLayout()->setAlignCanvasToScales(true);

    setColorMap(GkColorMap::HueMap);

    //
    // Instructions!
    // ---------------------------------------
    // Zooming is the Left Button on the mouse
    // Panning is Middle Button, again by the mouse
    // Right-click zooms out by '1'
    // Ctrl + Right-click will zoom out to full-size
    //

    QPointer<QwtPlotZoomer> zoomer = new MyZoomer(canvas());
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);

    std::unique_ptr<QwtPlotPanner> panner = std::make_unique<QwtPlotPanner>(canvas());
    panner->setAxisEnabled(QwtPlot::yRight, false);
    panner->setMouseButton(Qt::MidButton);

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
}

SpectroGui::~SpectroGui()
{}

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

    replot();

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

    std::unique_ptr<QwtPlotZoomer> zoomer = std::make_unique<MyZoomer>(canvas());
    zoomer->setRubberBandPen(colour);
    zoomer->setTrackerPen(colour);
}

/**
 * @brief SpectroGui::setYAxisRange
 * @param y_min
 * @param y_max
 */
void SpectroGui::setYAxisRange(const double &y_min, const double &y_max)
{
    y_min_ = y_min;
    y_max_ = y_max;

    setInterval(Qt::YAxis, QwtInterval(y_min_, y_max_));
    plotLayout()->setAlignCanvasToScales(true);
    replot();

    return;
}

/**
 * @brief SpectroGui::setXAxisRange
 * @param x_min
 * @param x_max
 */
void SpectroGui::setXAxisRange(const double &x_min, const double &x_max)
{
    x_min_ = x_min;
    x_max_ = x_max;

    setInterval(Qt::XAxis, QwtInterval(x_min_, x_max_));
    plotLayout()->setAlignCanvasToScales(true);
    replot();

    return;
}

/**
 * @brief SpectroGui::setZAxisRange
 * @param z_min
 * @param z_max
 */
void SpectroGui::setZAxisRange(const double &z_min, const double &z_max)
{
    if (std::isgreaterequal(z_min, 0.0)) {
        if (z_min < z_min_) {
            z_min_ = z_min;
        }
    }

    if (z_max > z_max_) {
        z_max_ = z_max;
    }

    setInterval(Qt::ZAxis, QwtInterval(z_min_, z_max_));
    plotLayout()->setAlignCanvasToScales(true);
    replot();

    z_axis_set = true;

    return;
}

void SpectroGui::setMatrixData(const std::vector<double> &values, int numColumns)
{
    std::mutex spectro_matrix_data_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_matrix_data_mtx);

    size_t rows = (values.size() / numColumns);
    setXAxisRange(0, numColumns);
    setYAxisRange(0, rows);

    double minValue = *std::min_element(std::begin(values), std::end(values));
    double maxValue = *std::max_element(std::begin(values), std::end(values));

    setZAxisRange(minValue, maxValue);
    setAxisScale(QwtPlot::yRight, z_min_, z_max_);

    enableAxis(QwtPlot::yRight, true);
    updateAxes();

    QVector<double> x_axis_conv(values.begin(), values.end());
    setValueMatrix(x_axis_conv, numColumns);
    gkSpectrogram->setData(this);

    x_axis_conv.clear();
    x_axis_conv.shrink_to_fit();
    setColorMap(Spectrograph::GkColorMap::HueMap);

    replot();
}
