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
 **   Small world is distributed in the hope that it will be useful,
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

#include "src/spectro_curve.hpp"
#include <qwt/qwt_series_data.h>
#include <cmath>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

/**
 * @brief GkSpectroCurve::GkSpectroCurve
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param stringFuncs
 * @param eventLogger
 * @param enablePanner
 * @param enableZoomer
 * @param parent
 */
GkSpectroCurve::GkSpectroCurve(QPointer<GekkoFyre::StringFuncs> stringFuncs, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               const double &sampleRate, const quint32 &fftSize, const bool &enablePanner,
                               const bool &enableZoomer, QWidget *parent)
{
    setParent(parent);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    gkSampleRate = sampleRate;
    gkFftSize = fftSize;
    gkEnablePanner = enablePanner;
    gkEnableZoomer = enableZoomer;

    gkCurve = std::make_unique<QwtPlotCurve>();
    gkCurveZoomer = new QwtPlotZoomer(this->canvas(), true);
    initiatePlot(this, tr("Frequency"), tr("Magnitude"), 0, (gkSampleRate / 2), -120, 100);
    gkCurveZoomer = new QwtPlotZoomer(canvas(), true);
    gkCurveZoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    gkCurveZoomer->setMousePattern(QwtEventPattern::MouseSelect3,Qt::RightButton);

    gkCurve->attach(this);

    curveXData.reserve(fftSize / 2 + 1);
    curveYData.reserve(fftSize / 2 + 1);
}

/**
 * @brief GkSpectroCurve::~GkSpectroCurve
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkSpectroCurve::~GkSpectroCurve()
{}

/**
 * @brief GkSpectroCurve::processFrame processes the incoming FFT data for the spectrograph curve and allows for the displaying
 * of it on the graph itself.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fftMagnitude The magnitude values derived from the FFT calculations.
 */
void GkSpectroCurve::processFrame(const std::vector<double> &fftMagnitude)
{
    const quint32 n = gkFftSize / 2 + 1;
    const double one_over_n = 1 / ((double) n); // To make it a little bit faster!

    for (quint32 i = 0; i < n; ++i) {
        curveYData[i] = 20.0 * std::log10(one_over_n * fftMagnitude[i]);
    }

    gkCurve->setSamples(curveXData.data(), curveYData.data(), gkFftSize / 2 + 1);
    replot();

    return;
}

/**
 * @brief GkSpectroCurve::initiatePlot
 * @author Martin Kumm <http://www.martin-kumm.de/wiki/doku.php?id=05Misc:A_Template_for_Audio_DSP_Applications>
 * @param plot
 * @param xTitle
 * @param yTitle
 * @param xmin
 * @param xmax
 * @param ymin
 * @param ymax
 */
void GkSpectroCurve::initiatePlot(QwtPlot *plot, const QString &xTitle, const QString &yTitle, const int &xmin,
                                  const int &xmax, const int &ymin, const int &ymax)
{
    QwtText xQwTitle = QwtText(xTitle);
    QwtText yQwTitle = QwtText(yTitle);

    QwtText title = QwtText("QwtPlot");
    title.setFont(QFont("SansSerif", 8));

    QwtText xAxisTitle = xQwTitle;
    xAxisTitle.setFont(QFont("SansSerif", 8));
    QwtText yAxisTitle = yQwTitle;
    yAxisTitle.setFont(QFont("SansSerif", 8));

    plot->setAxisAutoScale(QwtPlot::yLeft);
    plot->setAxisAutoScale(QwtPlot::xBottom);

    QFont axisFont = QFont("SansSerif", 6);
    plot->setAxisFont(QwtPlot::xBottom, axisFont);
    plot->setAxisFont(QwtPlot::yLeft, axisFont);

    //
    // Axis
    //
    plot->setAxisTitle(QwtPlot::xBottom, xAxisTitle);
    plot->setAxisTitle(QwtPlot::yLeft, yAxisTitle);

    std::unique_ptr<QwtPlotGrid> grid = std::make_unique<QwtPlotGrid>();
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->setMinorPen(QPen(Qt::gray, 0 , Qt::DotLine));
    grid->attach(plot);

    plot->setCanvasBackground(QColor(255,255,255));
    plot->setGeometry(QRect(20, 20, 600, 220));
    plot->resize(200, 200);

    plot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
    plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);

    QPointer<QwtPlotPanner> panner = new QwtPlotPanner(plot->canvas());
    panner->setMouseButton(Qt::MidButton);

    return;
}
