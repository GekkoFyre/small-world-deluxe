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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <QObject>
#include <QBuffer>
#include <QPointer>
#include <QSaveFile>
#include <QIODevice>
#include <QByteArray>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

#ifdef __cplusplus
extern "C"
{
#endif

#include <codec2/codec2.h>

#ifdef __cplusplus
}
#endif

namespace GekkoFyre {

class GkCodec2Sink : public QIODevice {
    Q_OBJECT

public:
    explicit GkCodec2Sink(const QString &fileLoc, const qint32 &codec2_mode, const qint32 &natural, const bool &save_pcm_copy,
                          QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent = nullptr);
    ~GkCodec2Sink() override;

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    bool isFailed();
    bool isDone();

public slots:
    void start();
    void stop();

signals:
    void volume(qint32 vol);

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Status variables
    GekkoFyre::GkAudioFramework::GkAudioRecordStatus m_recActive;

    //
    // Encoder variables
    CODEC2 *codec2;                                             // The Codec2 pointer itself.
    bool m_initialized = false;                                 // Whether an encoding operation has begun or not; therefore block other attempts until this singular one has stopped.
    QString m_fileLoc;                                          // The filename to write-out the encoded data towards!
    qint32 m_mode;                                              // The Codec2 mode to employ!

    //
    // Buffers
    short *buf;
    unsigned char *bits;
    qint32 nsam;
    qint32 nbit;
    qint32 nbyte;
    qint32 buf_empt;

    //
    // Filesystem and related
    QFileInfo m_fileInfo;
    QPointer<QFile> m_file;                                 // The file that the encoded data is to be saved towards.
    QPointer<QFile> m_filePcm;                              // If the user desires so, then a PCM WAV file can be created as an adjunct too!

    //
    // Miscellaneous
    bool m_failed;
    bool m_done;
    bool m_savePcmCopy;

};
};
