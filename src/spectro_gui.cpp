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
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <cmath>
#include <memory>
#include <QList>
#include <QColormap>
#include <QPointer>
#include <QVector>

using namespace GekkoFyre;
using namespace Spectrograph;

/**
 * @brief SpectroGui::SpectroGui
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
SpectroGui::SpectroGui(QWidget *parent) : QwtPlot(parent), gkAlpha(255)
{
    std::mutex spectro_main_mtx;
    std::lock_guard<std::mutex> lck_guard(spectro_main_mtx);

    gkSpectrogram = std::make_unique<QwtPlotSpectrogram>();
    gkMatrixRaster = new QwtMatrixRasterData();
    gkSpectrogram->setRenderThreadCount(0); // use system specific thread count
    gkSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);

    axis_y_right = axisWidget(QwtPlot::yRight);

    QList<double> contour_levels;
    for (double level = 0.5; level < 10.0; level += 1.0) {
        contour_levels += level;
    }

    gkSpectrogram->setContourLevels(contour_levels);
    gkSpectrogram->setData(this);
    gkSpectrogram->attach(this);

    setInterval(Qt::XAxis, QwtInterval(-1.5, 1.5));
    setInterval(Qt::YAxis, QwtInterval(-1.5, 1.5));
    setInterval(Qt::ZAxis, QwtInterval(0.0, 10.0));

    const QwtInterval z_interval = gkSpectrogram->data()->interval(Qt::ZAxis);

    // A color bar on the right axis
    QwtScaleWidget *right_axis = axisWidget(QwtPlot::yRight);
    right_axis->setTitle("Intensity");
    right_axis->setColorBarEnabled(true);

    setAxisScale(QwtPlot::yRight, z_interval.minValue(), z_interval.maxValue());
    enableAxis(QwtPlot::yRight);

    plotLayout()->setAlignCanvasToScales(true);

    setColorMap(ColorMap::RGBMap);

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
    std::lock_guard<std::mutex> lck_guard(spectro_gui_mtx);
    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, toggled);
    replot();

    return;
}

void SpectroGui::showSpectrogram(const bool &toggled)
{
    std::lock_guard<std::mutex> lck_guard(spectro_gui_mtx);
    gkSpectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, toggled);
    gkSpectrogram->setDefaultContourPen(toggled ? QPen(Qt::black, 0) : QPen(Qt::NoPen));

    replot();

    return;
}

void SpectroGui::setColorMap(const int &idx)
{
    std::lock_guard<std::mutex> lck_guard(spectro_gui_mtx);
    const QwtInterval z_interval = gkSpectrogram->data()->interval(Qt::ZAxis);

    gkMapType = idx;

    int alpha = gkAlpha;
    switch(idx) {
    case ColorMap::HueMap:
        {
            gkSpectrogram->setColorMap(new HueColorMap());
            axis_y_right->setColorMap(z_interval, new HueColorMap());
            break;
        }
    case ColorMap::AlphaMap:
        {
            alpha = 255;
            gkSpectrogram->setColorMap(new AlphaColorMap());
            axis_y_right->setColorMap(z_interval, new AlphaColorMap());
            break;
        }
    case ColorMap::RGBMap:
        break;
    default:
        {
            gkSpectrogram->setColorMap(new LinearColorMapRGB());
            axis_y_right->setColorMap(z_interval, new LinearColorMapRGB());
        }
    }

    gkSpectrogram->setAlpha(alpha);
    replot();

    return;
}

void SpectroGui::setAlpha(const int &alpha)
{
    std::lock_guard<std::mutex> lck_guard(spectro_gui_mtx);

    //
    // It does not make sense to set an alpha value in combination with a colour map
    // interpolating the alpha value.
    //

    gkAlpha = alpha;

    if (gkMapType != ColorMap::AlphaMap) {
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

bool SpectroGui::insertData2D(const double &x_axis, const double &y_axis) const
{
    // QwtMatrixRasterData *raster;
    // gkSpectrogram->setData();

    return false;
}

void SpectroGui::setMatrixData(const std::vector<double> &values, int numColumns)
{
    size_t rows = (values.size() / numColumns);
    gkMatrixRaster->setInterval(Qt::XAxis, QwtInterval(0, numColumns));
    gkMatrixRaster->setInterval(Qt::YAxis, QwtInterval(0, rows));

    double minValue = *std::min_element(std::begin(values), std::end(values));
    double maxValue = *std::max_element(std::begin(values), std::end(values));
    gkMatrixRaster->setInterval(Qt::ZAxis, QwtInterval(minValue, maxValue));

    QVector<double> x_axis_conv(values.begin(), values.end());
    gkMatrixRaster->setValueMatrix(x_axis_conv, numColumns);
    gkSpectrogram->setData(gkMatrixRaster);

    const QwtInterval zInterval = gkSpectrogram->data()->interval(Qt::ZAxis);
    setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());

    x_axis_conv.clear();
    x_axis_conv.shrink_to_fit();
    setColorMap(gkMapType);
}
