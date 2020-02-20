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
 **   Small World is distributed in the hope that it will be useful,
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

#pragma once

#include "src/defines.hpp"
#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>
#include <qwt_color_map.h>
#include <qwt_matrix_raster_data.h>
#include <mutex>
#include <QObject>
#include <QWidget>
#include <QVector>

namespace GekkoFyre {

//
// http://www.setnode.com/blog/qt-staticmetaobject-is-not-a-member-of/
//
class SpectroGui: public QwtPlot, private QwtMatrixRasterData {

public:
    SpectroGui(QWidget *parent = nullptr);
    ~SpectroGui() override;

    std::unique_ptr<QwtPlotSpectrogram> gkSpectrogram;
    QwtMatrixRasterData *gkMatrixRaster;
    QwtScaleWidget *axis_y_right;

    void showContour(const int &toggled);
    void showSpectrogram(const bool &toggled);
    void setColorMap(const int &idx);
    void setAlpha(const int &alpha);
    void setTheme(const QColor &colour);
    bool insertData2D(const double &x_axis, const double &y_axis) const;

    virtual double value(double x, double y) const override {
        const double c = 0.842;

        const double v1 = (x * x + (y - c) * (y + c));
        const double v2 = (x * (y + c) + x * (y + c));

        return (1.0 / (v1 * v1 + v2 * v2));
    }

    virtual void setMatrixData(const QVector<double> &values, int numColumns);

private:
    int gkMapType;
    int gkAlpha;

    //
    // Mutexes
    //
    std::mutex spectro_gui_mtx;
};

class MyZoomer: public QwtPlotZoomer {

public:
    MyZoomer(QWidget *canvas): QwtPlotZoomer(canvas) {
        setTrackerMode(AlwaysOn);
    }

    virtual QwtText trackerTextF(const QPointF &pos) const {
        QColor bg(Qt::white);
        bg.setAlpha(200);

        QwtText text = QwtPlotZoomer::trackerTextF(pos);
        text.setBackgroundBrush( QBrush(bg));
        return text;
    }
};

class LinearColorMapRGB: public QwtLinearColorMap {

public:
    LinearColorMapRGB(): QwtLinearColorMap(Qt::darkCyan, Qt::red, QwtColorMap::RGB) {
        addColorStop(0.1, Qt::cyan);
        addColorStop(0.6, Qt::green);
        addColorStop(0.95, Qt::yellow);
    }
};

class HueColorMap: public QwtColorMap {

public:
    HueColorMap(): gkHue1(0), gkHue2(359), gkSaturation(150), gkHueValue(200) {
        updateTable();
    }

    virtual QRgb rgb(const QwtInterval &interval, double value) const {
        if (qIsNaN(value)) {
            return 0u;
        }

        const double width = interval.width();
        if (width <= 0) {
            return 0u;
        }

        if (value <= interval.minValue()) {
            return gkHueRgbMin;
        }

        if (value >= interval.maxValue()) {
            return gkHueRgbMax;
        }

        const double ratio = (value - interval.minValue()) / width;
        int hue = gkHue1 + qRound(ratio * (gkHue2 - gkHue1));

        if (hue >= 360) {
            hue -= 360;

            if (hue >= 360) {
                hue = hue % 360;
            }
        }

        return gkHueRgbTable[hue];
    }

    virtual unsigned char colorIndex(const QwtInterval &qwt_interval, double dbl_value) const {
        //
        // Indexed colours are currently unsupported!
        //

        Q_UNUSED(qwt_interval);
        Q_UNUSED(dbl_value);

        return 0;
    }

private:
    void updateTable() {
        for (int i = 0; i < 360; ++i) {
            gkHueRgbTable[i] = QColor::fromHsv( i, gkSaturation, gkHueValue ).rgb();
        }

        gkHueRgbMin = gkHueRgbTable[gkHue1 % 360];
        gkHueRgbMax = gkHueRgbTable[gkHue2 % 360];
    }

    int gkHue1, gkHue2, gkSaturation, gkHueValue;
    QRgb gkHueRgbMin, gkHueRgbMax, gkHueRgbTable[360];
};

class AlphaColorMap: public QwtAlphaColorMap {

public:
    AlphaColorMap() {
        setColor(QColor("SteelBlue"));
    }
};

};
