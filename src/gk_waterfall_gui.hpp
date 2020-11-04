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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include "src/gk_spectro_color_maps.hpp"
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
#include <qwt/qwt_plot_marker.h>
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
class GkQwtColorMap {

public:
    explicit GkQwtColorMap();
    ~GkQwtColorMap();

protected:
    QwtColorMap *controlPointsToQwtColorMap(const ColorMaps::ControlPoints& ctrlPts);

};

class GkSpectroWaterfall: public QWidget {
    Q_OBJECT

public:
    explicit GkSpectroWaterfall(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger, const bool &enablePanner = false,
                                const bool &enableZoomer = false, QWidget *parent = nullptr);
    ~GkSpectroWaterfall() override;

    void insertData(const QVector<double> &values, const int &numCols);
    void setDataDimensions(double dXMin, double dXMax, const size_t &historyExtent, const size_t &layerPoints);
    void getDataDimensions(double &dXMin, double &dXMax, size_t &historyExtent, size_t &layerPoints) const;

protected:
    QwtPlot* const m_plotHorCurve = nullptr;
    QwtPlot* const m_plotVertCurve = nullptr;
    QwtPlot* const m_plotSpectrogram = nullptr;
    QwtPlotCurve* m_horCurve = nullptr;
    QwtPlotCurve* m_vertCurve = nullptr;
    QwtPlotPicker* const m_picker = nullptr;
    QwtPlotPanner* const m_panner = nullptr;
    QwtPlotSpectrogram* const m_spectrogram = nullptr;
    QwtPlotZoomer* const m_zoomer = nullptr;
    QwtPlotMarker* const m_horCurveMarker = nullptr;
    QwtPlotMarker* const m_vertCurveMarker = nullptr;

    double m_markerX = 0;
    double m_markerY = 0;

    void updateLayout();

    void allocateCurvesData();
    void freeCurvesData();
    void setupCurves();
    void updateCurvesData();

public slots:
    void refreshDateTime(const qint64 &latest_time_update, const qint64 &time_since);

private:
    QPointer<QwtPlotCanvas> canvas;
    std::unique_ptr<QwtPlotCurve> curve;
    QPointer<QwtPlotPanner> panner;
    QwtScaleWidget *top_x_axis;         // This makes use of RAII!
    QwtScaleWidget *right_y_axis;       // This makes use of RAII!

    std::unique_ptr<WaterfallData<double>> gkWaterfallData;

    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    int gkAlpha;                                                // Controls the alpha value of the waterfall chart.

    //
    // Threads
    //
    std::mutex mtx_raster_data;

    template<class in_it, class out_it>
    out_it copy_every_nth(in_it b, in_it e, out_it r, size_t n) {
        for (size_t i = distance(b, e) / n; --i; advance (b, n)) {
            *r++ = *b;
        }

        return r;
    }
};
};
