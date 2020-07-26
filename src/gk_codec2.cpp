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

#include "src/gk_codec2.hpp"
#include <codec2/freedv_api.h>
#include <snappy.h>
#include <QDataStream>
#include <utility>
#include <sstream>
#include <cstring>

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

GkCodec2::GkCodec2(const Codec2Mode &freedv_mode, const Codec2ModeCustom &custom_mode, const int &freedv_clip, const int &freedv_txbpf,
                   QPointer<GkEventLogger> eventLogger, std::shared_ptr<GekkoFyre::PaAudioBuf<qint16>> output_audio_buf, QObject *parent)
{
    try {
        setParent(parent);
        outputAudioBuf = std::move(output_audio_buf);
        gkEventLogger = std::move(eventLogger);

        gkFreeDvMode = freedv_mode;
        gkCustomMode = custom_mode;
        gkFreeDvClip = freedv_clip;
        gkFreeDvTXBpf = freedv_txbpf;

        return;
    }  catch (const std::exception &e) {
        std::throw_with_nested(tr("Error with initializing Codec2 library! Error:\n\n%1")
                               .arg(QString::fromStdString(e.what())).toStdString());
    }

    return;
}

GkCodec2::~GkCodec2()
{
    return;
}

/**
 * @brief GkCodec2::transmitData prepares a waveform that is readily transmissible over-the-air with another function that can do
 * just that.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param byte_array The data that is to be prepped for transmission.
 * @param play_output_sound Whether to play the modulating signal over the user's output audio device or not.
 * @return The prepped data for transmission.
 * @note <https://github.com/drowe67/codec2/blob/master/README_data.md>
 * <https://github.com/drowe67/codec2/blob/master/src/freedv_data_raw_tx.c>
 * <https://cpp.hotexamples.com/examples/-/-/codec2_encode/cpp-codec2_encode-function-examples.html>
 */
int GkCodec2::transmitData(const QByteArray &byte_array, const bool &play_output_sound)
{
    try {
        auto txData = createPayloadForTx(byte_array);

        struct CODEC2 *codec2;
        codec2 = codec2_create(convertFreeDvModeToInt(gkFreeDvMode));
        if (codec2 == nullptr) {
            throw std::runtime_error(tr("Issue encountered with opening Codec2 modem for transmission! Are you out of memory?").toStdString());
        }

        // For streaming bytes it is much easier to use modes that have a multiple of 8 payload bits/frame...
        int nbit = codec2_bits_per_frame(codec2);
        auto out_bits = (unsigned char *)malloc(nbit * sizeof(char));

        gkEventLogger->publishEvent(tr("Bits per modem frame: %1. Samples per modem frame: %2.")
                                    .arg(QString::number(codec2_bits_per_frame(codec2)))
                                    .arg(QString::number(codec2_samples_per_frame(codec2))), GkSeverity::Debug);

        for (const auto &to_tx: txData) {
            imemstream in(to_tx.data(), (size_t)to_tx.size());
            for (;;) {
                std::string buffer;
                if (!std::getline(in, buffer)) { continue; }
                unsigned char *uchrs = reinterpret_cast<unsigned char *>(const_cast<char *>(buffer.c_str()));

                // Stream the data until finish!
                codec2_encode(codec2, out_bits, (short *)uchrs);

                if (play_output_sound) {
                    size_t out_len = std::strlen((char *)out_bits);
                    outputAudioBuf->append(convCharToAudioData(out_bits, out_len), out_len);
                }
            }
        }

        codec2_destroy(codec2);
    }  catch (const std::exception &e) {
        std::throw_with_nested(tr("An issue has occurred with transmitting data via the Codec2 modem! Error:\n\n%1")
                               .arg(QString::fromStdString(e.what())).toStdString());
    }

    return paAbort;
}

/**
 * @brief GkCodec2::createPayloadForTx creates a payload out of a QByteArray of data that's more suitable for transmission of
 * said data over radio waves. It will convert the data to be transmitted towards Base64 encoding while padding out said data
 * to a length that's more easily divisible by a value of eight.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param byte_array The QByteArray to be processed.
 * @return The data to be transmitted over the radio waves.
 * @note <https://stackoverflow.com/questions/9811235/best-way-to-split-a-vector-into-two-smaller-arrays>
 */
QList<QByteArray> GkCodec2::createPayloadForTx(const QByteArray &byte_array)
{
    try {
        QByteArray base64_conv_data;
        if (gkCustomMode == Codec2ModeCustom::GekkoFyreV1) {
            std::string compressed_string;
            snappy::Compress(byte_array.data(), byte_array.size(), &compressed_string);
            QByteArray comprData(compressed_string.c_str(), compressed_string.length());
            base64_conv_data = comprData.toBase64(QByteArray::KeepTrailingEquals); // Convert to Base64 for easier transmission!
        } else {
            // Keeps the trailing padding equal signs at the end of the encoded data, so the data
            // is always a size multiple of four.
            base64_conv_data = byte_array.toBase64(QByteArray::KeepTrailingEquals); // Convert to Base64 for easier transmission!
        }

        int payload_size = base64_conv_data.size();
        QList<QByteArray> payload_data;

        if (payload_size > GK_CODEC2_FRAME_SIZE) {
            // We need to break up the payload!
            QByteArray tmp_data;
            while (payload_size > GK_CODEC2_FRAME_SIZE) {
                for (int i = 0; i < GK_CODEC2_FRAME_SIZE; ++i) {
                    tmp_data.insert(i, base64_conv_data[i]);
                }

                payload_size -= GK_CODEC2_FRAME_SIZE;
                payload_data.append(tmp_data);
            }
        }

        // Add any remaining data...
        QByteArray tmp_data;
        for (int i = 0; i < payload_size; ++i) {
            tmp_data.insert(i, base64_conv_data[i]);
        }

        payload_data.append(tmp_data);
        return payload_data;
    }  catch (const std::exception &e) {
        std::throw_with_nested(tr("An issue has occurred with transmitting audio via the Codec2 modem! Error:\n\n%1")
                               .arg(QString::fromStdString(e.what())).toStdString());
    }

    return QList<QByteArray>();
}

/**
 * @brief GkCodec2::convCharToAudioData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param str
 * @param len
 * @return
 */
short *GkCodec2::convCharToAudioData(unsigned char *str, const size_t &len)
{
    short *buffer = new short();
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = (short)str[i];
    }

    return buffer;
}

/**
 * @brief GkCodec2::convAudioDataToChar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param audio_data
 * @param len
 * @return
 */
unsigned char *GkCodec2::convAudioDataToChar(short *audio_data, const size_t &len)
{
    unsigned char *buffer = new unsigned char();
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = (unsigned char)audio_data[i];
    }

    return buffer;
}

/**
 * @brief GkCodec2::convertFreeDvModeToInt
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freedv_mode
 */
int GkCodec2::convertFreeDvModeToInt(const Codec2Mode &freedv_mode)
{
    switch (freedv_mode) {
    case Codec2Mode::freeDvMode2020:
        return FREEDV_MODE_2020;
    case Codec2Mode::freeDvMode700D:
        return FREEDV_MODE_700D;
    case Codec2Mode::freeDvMode700C:
        return FREEDV_MODE_700C;
    case Codec2Mode::freeDvMode800XA:
        return FREEDV_MODE_800XA;
    case Codec2Mode::freeDvMode2400B:
        return FREEDV_MODE_2400B;
    case Codec2Mode::freeDvMode2400A:
        return FREEDV_MODE_2400A;
    case Codec2Mode::freeDvMode1600:
        return FREEDV_MODE_1600;
    default:
        return -1;
    }

    return -1;
}
