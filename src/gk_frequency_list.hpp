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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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

#pragma once

#include "src/defines.hpp"
#include "src/dek_db.hpp"
#include <memory>
#include <QObject>
#include <QList>

namespace GekkoFyre {

class GkFrequencies : public QObject {
    Q_OBJECT

public:
    explicit GkFrequencies(QPointer<GekkoFyre::GkLevelDb> database, QObject *parent = nullptr);
    ~GkFrequencies() override;

    void publishFreqList();

    bool approximatelyEqual(const float &a, const float &b, const float &epsilon);
    bool essentiallyEqual(const float &a, const float &b, const float &epsilon);
    bool definitelyGreaterThan(const float &a, const float &b, const float &epsilon);
    bool definitelyLessThan(const float &a, const float &b, const float &epsilon);

    QList<GekkoFyre::AmateurRadio::GkFreqs> listOfFreqs();
    int size();
    GekkoFyre::AmateurRadio::GkFreqs at(const int &idx);

signals:
    void updateFrequencies(const quint64 &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                           const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq);
    void removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_remove);
    void addFreq(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_add);

private slots:
    void updateFreqsInMem(const quint64 &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                          const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq);

private:
    QList<GekkoFyre::AmateurRadio::GkFreqs> frequencyList;
    QPointer<GekkoFyre::GkLevelDb> gkDb;

};
};
