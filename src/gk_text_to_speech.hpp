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
#include "src/dek_db.hpp"
#include "src/gk_logger.hpp"
#include <QVoice>
#include <QObject>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QTextToSpeech>

namespace GekkoFyre {

/**
 * @class GkTextToSpeech
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://doc.qt.io/qt-5/qtspeech-hello-speak-example.html>.
 */
class GkTextToSpeech : public QTextToSpeech {
    Q_OBJECT

public:
    explicit GkTextToSpeech(QPointer<GekkoFyre::GkLevelDb> dbPtr, QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkTextToSpeech() override;

public slots:
    void speak();
    void stop();

    void setRate(const int &rate);
    void setPitch(const int &pitch);
    void setVolume(const int &volume);

    void stateChanged(const QTextToSpeech::State &state);
    void engineSelected(int index);
    void engineSelected(const QString &name);
    void languageSelected(const QLocale &language);
    void voiceSelected(const int &idx);

    void localeChanged(const QLocale &locale);

private:
    QPointer<GekkoFyre::GkLevelDb> GkDb;
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    QPointer<QTextToSpeech> m_speech;
    QVector<QVoice> m_voices;
    QString engineName;

};
};
