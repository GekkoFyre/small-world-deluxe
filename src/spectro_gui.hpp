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
#include <qwt_plot_canvas.h>
#include <qwt_raster_data.h>
#include <qwt_interval.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_engine.h>
#include <qwt_date_scale_engine.h>
#include <qwt_date_scale_draw.h>
#include <mutex>
#include <vector>
#include <cmath>
#include <thread>
#include <future>
#include <QTimer>
#include <QObject>
#include <QWidget>
#include <QVector>
#include <QPointer>
#include <QDateTime>
#include <QMouseEvent>

#ifdef _WIN32
#include "src/string_funcs_windows.hpp"
#elif __linux__
#include "src/string_funcs_linux.hpp"
#endif

namespace GekkoFyre {

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

/**
 * @brief The GkSpectroData class creates the coloured spectrogram plots.
 */
class GkSpectroData: public QwtRasterData {

public:
    GkSpectroData() {
        // Example: https://stackoverflow.com/questions/35563246/qwtplotspectrogram-doesnt-work-no-imagemode-plot
        // setInterval(Qt::XAxis, QwtInterval(-1.5, 1.5));
        // setInterval(Qt::YAxis, QwtInterval(-1.5, 1.5));
        // setInterval(Qt::ZAxis, QwtInterval(0.0, 10.0));
    }

    virtual double value(double x, double y) const {
        const double c = 0.842;
        const double v1 = x * x + (y - c) * (y + c);
        const double v2 = x * (y + c) + x * (y + c);

        return (1.0 / (v1 * v1 + v2 * v2));
    }
};

/**
 * @brief The LinearColorMapRGB class
 * @note <https://www.qtcentre.org/threads/60248-Any-examples-of-different-color-tables-with-Qwt>
 */
class LinearColorMapRGB: public QwtLinearColorMap {

public:
    LinearColorMapRGB(): QwtLinearColorMap(Qt::darkCyan, Qt::red, QwtColorMap::RGB) {
        setColorInterval(QColor(0, 0, 30), QColor(0.5 * 255, 0, 0));
        addColorStop(0.00, Qt::cyan);
        addColorStop(0.20, Qt::blue);
        addColorStop(0.40, Qt::yellow);
        addColorStop(0.60, Qt::darkYellow);
        addColorStop(0.80, Qt::red);
        addColorStop(1.00, Qt::darkRed);
    }
};

class GkSpectrograph: public QwtPlot, public GkSpectroData, public QwtMatrixRasterData {
    Q_OBJECT

public:
    explicit GkSpectrograph(QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);
};

class SpectroGui: public GkSpectrograph {
    Q_OBJECT

public:
    SpectroGui(std::shared_ptr<GekkoFyre::StringFuncs> stringFuncs, const bool &enablePanner = false,
               const bool &enableZoomer = false, QWidget *parent = nullptr);
    ~SpectroGui();

    QwtPlotSpectrogram *gkSpectrogram;

    void setAlpha(const int &alpha);
    void setTheme(const QColor &colour);

protected:
    void alignScales();

public slots:
    void showSpectrogram(const bool &toggled);
    void refreshDateTime(const qint64 &latest_time_update, const qint64 &time_since);

private:
    QwtPlotZoomer *zoomer;
    LinearColorMapRGB *colour_map;
    QwtDateScaleDraw *date_scale_draw;
    QwtDateScaleEngine *date_scale_engine;
    QwtScaleWidget *top_x_axis;

    std::shared_ptr<GekkoFyre::StringFuncs> gkStringFuncs;
    GekkoFyre::Spectrograph::GkColorMap gkMapType;
    int gkAlpha;                                                // Controls the alpha value of the waterfall chart.
    qint64 spectro_begin_time;                                  // The time at which the spectrograph was initialized.
    qint64 spectro_latest_update;                               // The latest time for when the spectrograph was updated with new data/information.

    Spectrograph::MatrixData calc_z_history;
    std::vector<int> raw_plot_data;

    size_t y_axis_num_minor_steps;
    size_t y_axis_num_major_steps;
    double y_axis_step_size;
    bool enablePlotRefresh;

    //
    // Date & Timing
    //

    //
    // Threads
    //

    template<class in_it, class out_it>
    out_it copy_every_nth(in_it b, in_it e, out_it r, size_t n) {
        for (size_t i = distance(b, e) / n; --i; advance (b, n)) {
            *r++ = *b;
        }

        return r;
    }
};
};
