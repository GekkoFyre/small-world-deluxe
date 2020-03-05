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
#include "src/string_funcs_windows.hpp"
#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>
#include <qwt_color_map.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_plot_canvas.h>
#include <qwt_raster_data.h>
#include <qwt_interval.h>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <thread>
#include <future>
#include <QObject>
#include <QWidget>
#include <QVector>
#include <QMouseEvent>

namespace GekkoFyre {

class SpectroRasterData: public QwtMatrixRasterData {

public:
    SpectroRasterData() {
        const double matrix[] = {
            1, 2, 4, 1,
            6, 3, 5, 2,
            4, 2, 1, 5,
            5, 4, 2, 3
        };

        QVector<double> values;
        for (uint i = 0; i < sizeof(matrix) / sizeof(double); ++i) {
            values += matrix[i];
        }

        const int numColumns = 4;
        setValueMatrix(values, numColumns);

        setInterval(Qt::XAxis, QwtInterval(-0.5, 3.5, QwtInterval::ExcludeMaximum));
        setInterval(Qt::YAxis, QwtInterval(-0.5, 3.5, QwtInterval::ExcludeMaximum));
        setInterval(Qt::ZAxis, QwtInterval(1.0, 6.0));
    }
};

class GkSpectrograph: public QwtPlot {
    Q_OBJECT

public:
    GkSpectrograph(QWidget *parent = nullptr);
    ~GkSpectrograph();

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);
};

class SpectroGui: public GkSpectrograph, private QwtPlotSpectrogram, public QwtMatrixRasterData {
    Q_OBJECT

public:
    SpectroGui(std::shared_ptr<GekkoFyre::StringFuncs> stringFuncs, QWidget *parent = nullptr);
    ~SpectroGui() override;

    QwtPlotSpectrogram *gkSpectrogram;

    void showContour(const int &toggled);
    void setAlpha(const int &alpha);
    void setTheme(const QColor &colour);

    virtual void setResampleMode(int mode);
    virtual double value(double x, double y) const override {
        const double c = 0.842;

        const double v1 = (x * x + (y - c) * (y + c));
        const double v2 = (x * (y + c) + x * (y + c));

        return (1.0 / (v1 * v1 + v2 * v2));
    }

public slots:
    void showSpectrogram(const bool &toggled);
    void applyData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &values,
                   const std::vector<short> &raw_audio_data,
                   const int &hanning_window_size, const size_t &buffer_size);

private slots:
    void calcInterval();

signals:
    void sendSpectroData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &values,
                         const std::vector<short> &raw_audio_data,
                         const int &hanning_window_size, const size_t &buffer_size);

private:
    QwtMatrixRasterData *gkMatrixRaster;
    QwtPlotZoomer *zoomer;
    QwtScaleWidget *rightAxis;
    QwtInterval z_interval;
    QwtInterval x_interval;
    QwtInterval y_interval;

    size_t num_rows;
    double autoscaleValueUpdated;

    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    GekkoFyre::Spectrograph::GkColorMap gkMapType;
    int gkAlpha;
    bool calc_first_data;       // Whether we have made our first calculation or not
    bool time_already_set;
    bool already_read_data;

    GekkoFyre::Spectrograph::MatrixData calc_z_history;
    std::unique_ptr<QVector<double>> z_data_history;
    std::unique_ptr<QVector<double>> time_data_history;
    std::vector<short> raw_plot_data;

    //
    // Threads
    //
    boost::thread calc_interval_thread;

    template<class in_it, class out_it>
    out_it copy_every_nth(in_it b, in_it e, out_it r, size_t n) {
        for (size_t i = distance(b, e) / n; --i; advance (b, n)) {
            *r++ = *b;
        }

        return r;
    }

    void calcMatrixData(const std::vector<GekkoFyre::Spectrograph::RawFFT> &values,
                        const int &hanning_window_size, const size_t &buffer_size,
                        std::promise<Spectrograph::MatrixData> matrix_data_promise);
    void appendData();
    void removeStaleData();
    void clearPlots();
    void plotNewData();
    double resetAutoscaleVal();

    void preparePlot();
    void resetAxisRanges();
    int calcWindowWidth();
};

class GkZoomer: public QwtPlotZoomer {

public:
    GkZoomer(QWidget *canvas): QwtPlotZoomer(canvas) {
        setTrackerMode(AlwaysOn);
    }

    virtual QwtText trackerTextF(const QPointF &pos) const {
        QColor bg(Qt::white);
        bg.setAlpha(200);

        QwtText text = QwtPlotZoomer::trackerTextF(pos);
        text.setBackgroundBrush(QBrush(bg));
        return text;
    }
};

class LinearColorMapRGB: public QwtLinearColorMap {

public:
    LinearColorMapRGB(const QwtInterval &rgb_values): QwtLinearColorMap(Qt::darkCyan, Qt::red, QwtColorMap::RGB) {
        setColorInterval(QColor(0, 0, 30), QColor(0.5 * 255, 0, 0));
        addColorStop(0.00, Qt::white);
        addColorStop(0.20, Qt::blue);
        addColorStop(0.40, Qt::cyan);
        addColorStop(0.60, Qt::yellow);
        addColorStop(0.80, Qt::red);
        addColorStop(1.00, Qt::darkRed);
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
