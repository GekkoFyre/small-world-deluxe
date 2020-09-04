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
#include "src/gk_logger.hpp"
#include "src/gk_waterfall_data.hpp"
#include <qwt/qwt.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_spectrogram.h>
#include <qwt/qwt_plot_zoomer.h>
#include <qwt/qwt_color_map.h>
#include <qwt/qwt_matrix_raster_data.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_raster_data.h>
#include <qwt/qwt_plot_panner.h>
#include <qwt/qwt_interval.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_date_scale_engine.h>
#include <qwt/qwt_date_scale_draw.h>
#include <mutex>
#include <cmath>
#include <vector>
#include <thread>
#include <future>
#include <memory>
#include <chrono>
#include <QList>
#include <QTimer>
#include <QObject>
#include <QWidget>
#include <QVector>
#include <QPointer>
#include <QDateTime>
#include <QMouseEvent>

namespace GekkoFyre {

class GkZoomer: public QwtPlotZoomer {

public:
    explicit GkZoomer(QWidget *canvas): QwtPlotZoomer(QwtPlot::xTop, QwtPlot::yLeft, canvas) {
        setTrackerMode(AlwaysOn);
    }

    [[nodiscard]] QwtText trackerTextF(const QPointF &pos) const override {
        QColor bg(Qt::white);
        bg.setAlpha(200);

        QwtText text = QwtPlotZoomer::trackerTextF(pos);
        text.setBackgroundBrush(QBrush(bg));
        return text;
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
        addColorStop(1.00, Qt::cyan);
        addColorStop(0.75, Qt::blue);
        addColorStop(0.50, Qt::darkCyan);
        addColorStop(0.25, Qt::darkBlue);
    }
};

class SpectroGui: public QwtPlot {
    Q_OBJECT

public:
    explicit SpectroGui(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger, const bool &enablePanner = false,
                        const bool &enableZoomer = false, QWidget *parent = nullptr);
    ~SpectroGui() override;

    void setDataDimensions(const double &dXMin, const double &dXMax,    // x-axis bounds
                           const size_t &historyExtent,                 // defines the y-axis width (i.e. number of layers)
                           const size_t &layerPoints);                  // FFT/Data points in a single layer
    void getDataDimensions(double &dXMin, double &dXMax, size_t &historyExtent, size_t &layerPoints) const;

    //
    // Data
    //
    bool insertData(const QVector<double> &values, const std::time_t &timestamp);
    [[nodiscard]] std::time_t getLayerDate(const double &y) const;
    void setRange(double dLower, double dUpper);

    [[nodiscard]] double getOffset() const { return (gkWaterfallData) ? gkWaterfallData->getOffset() : 0; }

protected:
    void alignScales();

public slots:
    void changeSpectroType(const GekkoFyre::Spectrograph::GkGraphType &graph_type);
    void updateFFTSize(const int &value);

protected:
    void updateCurvesData();

    void allocateCurvesData();
    void setupCurves();

private:
    QPointer<GkWaterfallData<double>> gkWaterfallData;
    QPointer<QwtPlotZoomer> zoomer;
    QPointer<QwtPlotCanvas> canvas;
    std::unique_ptr<QwtPlotSpectrogram> gkSpectro;
    std::unique_ptr<QwtPlotCurve> curve;
    QPointer<QwtPlotPanner> panner;
    QwtScaleWidget *top_x_axis;                                 // This makes use of RAII!
    QwtScaleWidget *right_y_axis;                               // This makes use of RAII!

    int buf_overall_size;
    int buf_total_size;
    bool zoomActive;
    double m_markerX = 0;
    double m_markerY = 0;
    bool m_bColorBarInitialized = false;

    QPointer<QwtPlot> m_plotHorCurve;
    std::unique_ptr<QwtPlotCurve> m_horCurve;
    QVector<double> m_horCurveXAxisData;
    QVector<double> m_horCurveYAxisData;

    QList<double> gkRasterBuf;

    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    int gkAlpha;                                                // Controls the alpha value of the waterfall chart.
    qint64 spectro_begin_time;                                  // The time at which the spectrograph was initialized.
    qint64 spectro_latest_update;                               // The latest time for when the spectrograph was updated with new data/information.

    //
    // Threads
    //
    std::mutex mtx_raster_data;

    //
    // Signals-related
    //
    GekkoFyre::Spectrograph::GkGraphType graph_in_use;

    template<class in_it, class out_it>
    out_it copy_every_nth(in_it b, in_it e, out_it r, size_t n) {
        for (size_t i = distance(b, e) / n; --i; advance (b, n)) {
            *r++ = *b;
        }

        return r;
    }
};

/**
 * @class GekkoFyre::GkSpectroTimeScaleDraw
 * @author Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>
 */
class GkSpectroTimeScaleDraw : public QwtScaleDraw {
    const SpectroGui &m_waterfallPlot;
    mutable QDateTime m_dateTime;

public:
    GkSpectroTimeScaleDraw(const SpectroGui &spectroGui) : m_waterfallPlot(spectroGui) {}
    ~GkSpectroTimeScaleDraw() override {}

    using QwtScaleDraw::invalidateCache;
    virtual QwtText label(double v) const {
        std::time_t ret = m_waterfallPlot.getLayerDate(v - m_waterfallPlot.getOffset());
        if (ret > 0) {
            m_dateTime.setTime_t(ret);
            // Need something else other than time_t to have 'zzz'
            // return m_dateTime.toString("hh:mm:ss:zzz");
            return m_dateTime.toString("dd.MM.yy\nhh:mm:ss");
        }

        return QwtText();
    }
};
};
