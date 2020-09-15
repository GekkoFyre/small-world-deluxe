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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "gk_vu_meter_widget.hpp"
#include <cmath>
#include <QPainter>
#include <QTimer>
#include <QDebug>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

namespace fs = boost::filesystem;
namespace sys = boost::system;

/**
 * @brief GkVuMeter::GkVuMeter digitally simulates a volume meter as one would sort of appear in-real-life.
 * @author Qt5 by Qt Project et al. <https://www.qt.io/>
 * @param parent
 * @note <https://doc.qt.io/qt-5.9/qtmultimedia-multimedia-spectrum-example.html>,
 * HPP <https://doc.qt.io/qt-5.9/qtmultimedia-multimedia-spectrum-app-levelmeter-h.html>,
 * CPP <https://doc.qt.io/qt-5.9/qtmultimedia-multimedia-spectrum-app-levelmeter-cpp.html>
 */
GkVuMeter::GkVuMeter(QWidget *parent) : QWidget(parent), m_rmsLevel(0.0), m_peakLevel(0.0), m_decayedPeakLevel(0.0),
    m_peakDecayRate(AUDIO_VU_METER_PEAK_DECAY_RATE), m_peakHoldLevel(0.0), m_redrawTimer(new QTimer(this)), m_rmsColor(Qt::red),
    m_peakColor(255, 200, 200, 255)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMinimumWidth(30);

    QObject::connect(m_redrawTimer, SIGNAL(timeout()), this, SLOT(redrawTimerExpired()));
    m_redrawTimer->start(AUDIO_VU_METER_UPDATE_MILLISECS);

    return;
}

GkVuMeter::~GkVuMeter()
{
    return;
}

void GkVuMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect bar = rect();

    bar.setTop(rect().top() + (1.0 - m_peakHoldLevel) * rect().height());
    bar.setBottom(bar.top() + 5);
    painter.fillRect(bar, m_rmsColor);
    bar.setBottom(rect().bottom());

    bar.setTop(rect().top() + (1.0 - m_decayedPeakLevel) * rect().height());
    painter.fillRect(bar, m_peakColor);

    bar.setTop(rect().top() + (1.0 - m_rmsLevel) * rect().height());
    painter.fillRect(bar, m_rmsColor);
}

void GkVuMeter::reset()
{
    m_rmsLevel = 0.0;
    m_peakLevel = 0.0;
    update();

    return;
}

void GkVuMeter::levelChanged(const qreal &rmsLevel, const qreal &peakLevel, const int &numSamples)
{
    // Smooth the RMS signal
    const qreal smooth = std::pow(qreal(0.9), static_cast<qreal>(numSamples) / 256); // TODO: remove this magic number
    m_rmsLevel = (m_rmsLevel * smooth) + (rmsLevel * (1.0 - smooth));

    if (peakLevel > m_decayedPeakLevel) {
        m_peakLevel = peakLevel;
        m_decayedPeakLevel = peakLevel;
        m_peakLevelChanged.start();
    }

    if (peakLevel > m_peakHoldLevel) {
        m_peakHoldLevel = peakLevel;
        m_peakHoldLevelChanged.start();
    }

    update();
    return;
}

void GkVuMeter::redrawTimerExpired()
{
    // Decay the peak signal
    const int elapsedMs = m_peakLevelChanged.elapsed();
    const qreal decayAmount = m_peakDecayRate * elapsedMs;
    if (decayAmount < m_peakLevel) {
        m_decayedPeakLevel = m_peakLevel - decayAmount;
    } else {
        m_decayedPeakLevel = 0.0;
    }

    // Check whether to clear the peak hold level
    if (m_peakHoldLevelChanged.elapsed() > AUDIO_VU_METER_PEAK_HOLD_LEVEL_DURATION) {
        m_peakHoldLevel = 0.0;
    }

    update();
    return;
}
