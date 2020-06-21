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

#pragma once

#include "src/defines.hpp"
#include <QObject>
#include <QWidget>

namespace GekkoFyre {
class GkVuMeter : public QWidget {
    Q_OBJECT

public:
    explicit GkVuMeter(QWidget *parent = 0);
    ~GkVuMeter();

    void paintEvent(QPaintEvent *event) override;

public slots:
    void reset();
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

private slots:
    void redrawTimerExpired();

private:
    // Height of RMS level bar.
    // Range 0.0 - 1.0.
    qreal m_rmsLevel;

    // Most recent peak level.
    // Range 0.0 - 1.0.
    qreal m_peakLevel;

    // Height of peak level bar.
    // This is calculated by decaying m_peakLevel depending on the
    // elapsed time since m_peakLevelChanged, and the value of m_decayRate.
    qreal m_decayedPeakLevel;

    // Time at which m_peakLevel was last changed.
    QTime m_peakLevelChanged;

    // Rate at which peak level bar decays.
    // Expressed in level units / millisecond.
    qreal m_peakDecayRate;

    // High watermark of peak level.
    // Range 0.0 - 1.0.
    qreal m_peakHoldLevel;

    // Time at which m_peakHoldLevel was last changed.
    QTime m_peakHoldLevelChanged;

    QTimer *m_redrawTimer;

    QColor m_rmsColor;
    QColor m_peakColor;

};
};
