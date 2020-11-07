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
#include <string>
#include <QFile>
#include <QString>
#include <QBuffer>
#include <QPointer>
#include <QIODevice>
#include <QByteArray>
#include <QEventLoop>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDecoder>

enum State { Recording, Stopped };

namespace GekkoFyre {

class GkFFTAudioPcmStream : public QIODevice, public QPointer {
    Q_OBJECT

public:
    explicit GkFFTAudioPcmStream(QPointer<QAudioInput> audioInput, const GekkoFyre::Database::Settings::Audio::GkDevice &audio_device_details,
                                 QObject *parent = nullptr);
    virtual ~GkFFTAudioPcmStream();

    void play(const QString &filePath);
    void stop();

    bool atEnd() const Q_DECL_OVERRIDE;

protected:
    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char * data, qint64 maxSize) Q_DECL_OVERRIDE;

private slots:
    void bufferReady();
    void finished();

private:
    //
    // QAudioSystem initialization and buffers
    //
    QBuffer m_input;
    QBuffer m_output;
    QByteArray m_data;
    QPointer<QAudioInput> gkAudioInput;
    QPointer<QEventLoop> gkAudioInputEventLoop;
    GekkoFyre::Database::Settings::Audio::GkDevice pref_audio_device;

    State m_state;

    bool isInited;
    bool isDecodingFinished;

    void clear();

};
};
