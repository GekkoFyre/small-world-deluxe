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

#include "src/gk_multimedia.hpp"
#include <utility>
#include <exception>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

#ifdef __cplusplus
} // extern "C"
#endif

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
 * @brief GkMultimedia::GkMultimedia
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkMultimedia::GkMultimedia(QPointer<GekkoFyre::GkAudioDevices> audio_devs, QPointer<GekkoFyre::StringFuncs> stringFuncs,
                           QPointer<GekkoFyre::GkEventLogger> eventLogger, QObject *parent) : QObject(parent)
{
    gkAudioDevices = std::move(audio_devs);
    gkStringFuncs = std::move(stringFuncs);
    gkEventLogger = std::move(eventLogger);

    return;
}

GkMultimedia::~GkMultimedia()
{}

/**
 * @brief GkMultimedia::analyzeAudioFileMetadata will analyze a given multimedia audio file and output the metadata
 * contained within, provided there is any.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_path The canonical location of the multimedia audio file to be analyzed in question.
 * @return The meta data of the analyzed multimedia audio file.
 */
GkAudioFramework::AudioFileInfo GkMultimedia::analyzeAudioFileMetadata(const QFileInfo &file_path) const
{
    try {
        if (file_path.exists() && file_path.isReadable()) { // Check that the QFileInfo parameter given is valid and the file in question exists!
            if (file_path.isFile()) { // Are we dealing with a file or directory?
                qint32 ret = 0;
                GkAudioFramework::AudioFileInfo audioFileInfo;
                AVFormatContext *pFormatCtx = avformat_alloc_context();
                if (pFormatCtx) {
                    AVDictionaryEntry *tag = nullptr;
                    ret = avformat_open_input(&pFormatCtx, file_path.canonicalFilePath().toStdString().c_str(), nullptr, nullptr);
                    if (ret < 0) {
                        throw std::runtime_error(tr("Error with opening file via FFmpeg for multimedia analysis. File: %1")
                        .arg(file_path.canonicalFilePath()).toStdString());
                    }

                    ret = avformat_find_stream_info(pFormatCtx, nullptr);
                    if (ret < 0) {
                        throw std::runtime_error(tr("Error with finding stream info within file via FFmpeg. File: %1")
                        .arg(file_path.canonicalFilePath()).toStdString());
                    }

                    audioFileInfo.file_size = file_path.size();
                    audioFileInfo.file_size_hr = gkStringFuncs->fileSizeHumanReadable(audioFileInfo.file_size);
                    audioFileInfo.length_in_secs = pFormatCtx->duration;
                    audioFileInfo.bit_depth = pFormatCtx->bit_rate; // TODO: Adjust this so it's mathematically correct!
                    audioFileInfo.sample_rate = pFormatCtx->bit_rate;

                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;

                    //
                    // Now print out the key/value pairs!
                    bool existingValues = false;
                    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
                        QString key = tag->key;
                        QString value = tag->value;
                        if (!key.isEmpty() && !value.isEmpty()) {
                            std::cout << QString("[ %1 ]: %2").arg(key, value).toStdString() << std::endl;
                            existingValues = true;
                        }
                    }

                    if (!existingValues) { // Were any key/value pairs detected? If not, let the end-user know so that they don't suspect an error otherwise!
                        std::cout << tr("Unable to detect any metadata within multimedia file, \"%1\"!").arg(file_path.canonicalFilePath()).toStdString() << std::endl;
                    }

                    //
                    // Print a separator...
                    std::cout << "--------------------------------------------------" << std::endl;

                    //
                    // Terminate any hanging pointers so that there are no memory leaks!
                    avformat_close_input(&pFormatCtx);
                    avformat_free_context(pFormatCtx);
                } else {
                    throw std::runtime_error(tr("Unable to initialize FFmpeg object. Out of memory?").toStdString());
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return GkAudioFramework::AudioFileInfo();
}
