/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
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

#include <fftw3.h>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <portaudiocpp/SampleDataFormat.hxx>
#include <portaudiocpp/Device.hxx>
#include <portaudiocpp/AsioDeviceAdapter.hxx>
#include <boost/logic/tribool.hpp>
#include <vector>
#include <string>
#include <locale>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <QString>

#ifdef _WIN32
#include <winsdkver.h>
#include <atlbase.h>
#include <atlstr.h>
#include <Windows.h>
#include <tchar.h> // https://linuxgazette.net/147/pfeiffer.html
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <hamlib/rig.h>
#include <hamlib/riglist.h>
#include <portaudio.h>

#ifdef _WIN32
#include <libusb.h>
    typedef std::wstring gkwstring;
#ifdef PA_USE_ASIO
#include "pa_asio.h"
#endif
#elif __linux__
#include <libusb-1.0/libusb.h>
    typedef std::string gkwstring;
#endif

#ifdef __cplusplus
}
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define snprintf _snprintf
#endif

#ifdef _UNICODE
  typedef std::wstring tstring;
#else
  typedef std::string tstring;
#endif

namespace GekkoFyre {

#define GK_EXIT_TIMEOUT (6)                             // The amount of time, in seconds, to leave 'Small World Deluxe' hanging upon exit before terminating forcefully!
#define MIN_MAIN_WINDOW_WIDTH (1024)
#define MIN_MAIN_WINDOW_HEIGHT (768)

#define AUDIO_OUTPUT_MAX_VOL_SIMPLE (100)               // The maximum volume in simple units (i.e. non decible units)
#define AUDIO_OUTPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_OUTPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_INPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_INPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_FRAMES_PER_BUFFER (256)                   // Frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback. Many apps may want to use paFramesPerBufferUnspecified, which tells PortAudio to pick the best, possibly changing, buffer size
#define AUDIO_TEST_SAMPLE_LENGTH_SEC (3)
#define AUDIO_TEST_SAMPLE_TABLE_SIZE (200)
#define AUDIO_SPEC_FLOOR_DECIBELS (-180.0)

#define AUDIO_BUFFER_STREAMING_SECS (3)
#define AUDIO_SINE_WAVE_PLAYBACK_SECS (3)               // Play the sine wave test sample for three seconds!
#define AUDIO_VU_METER_UPDATE_MILLISECS (500)                // How often the volume meter should update, in milliseconds.

// Mostly regarding FFTW functions
#define AUDIO_SIGNAL_LENGTH (2048)
#define FFTW_HOP_SIZE (128)

#ifndef M_PI
#define M_PI (3.14159265358979323846) /* pi */
#endif

typedef std::vector<char> char_array;

namespace Filesystem {
    constexpr char resourceFile[] = "resources.rcc";
    constexpr char fileName[] = "settings.db";          // The filename for the database itself which is TAR archived and compressed
    constexpr char dbFolder[] = "settings_db_temp";     // The filename for the intermittently alive, database folder
    constexpr char tarExtension[] = ".tar";             // The file extension given to (mostly uncompressed) TAR archive
    constexpr char tmpExtension[] = ".tmp";             // The file extension give to temporary files
    constexpr char file_storage[] = "files.ini";        // Where the names of the files will be stored, regarding ZLIB compression

    constexpr char linux_sys_tty[] = "/sys/class/tty/"; // The location of the TTY-devices under most major Linux distributions
}

namespace Database {
    namespace Settings {
        enum radio_cfg {
            RigBrand,
            RigModel,
            RigModelIndex,
            RigVersion,
            ComDevice,
            ComBaudRate,
            StopBits,
            PollingInterval,
            ModeDelay,
            Sideband,
            CWisLSB,
            FlowControl,
            PTTCommand,
            Retries,
            RetryInterv,
            WriteDelay,
            PostWriteDelay
        };

        enum audio_cfg {
            soundcardInput,
            soundcardOutput,
            settingsDbLoc,
            LogsDirLoc,
            AudioRecLoc,
            AudioInputChannels,
            AudioOutputChannels
        };

        enum audio_channels {
            Mono,
            Left,
            Right,
            Both,
            Unknown
        };

        enum general_stat_cfg {
            myCallsign,
            myMaidenhead,
            defCqMsg,
            defReplyMsg,
            defStationInfo
        };

        enum general_mainwindow_cfg {
            WindowMaximized,
            WindowHSize,
            WindowVSize
        };

        struct UsbDev {
            libusb_device *dev;                         // Primary underlying pointer to the `libusb` device
            libusb_interface *interface;                // Underlying pointer to the `libusb` interface
            libusb_context *context;                    // The underlying context to the `libusb` library
            libusb_device_descriptor config;            // Underlying pointer to the `libusb` configuration
            libusb_device_handle *handle;               // Underlying `libusb` device handle
            std::string mfg;                            // Information relating to the manufacturer
            std::string serial_number;                  // The Product Serial Number
            std::string product;                        // The Product ID
        };

        struct UsbPort {
            UsbDev usb_enum;                            // The USB Device structure, as above
            uint8_t port;                               // The USB port number as determined by `libusb`
            uint8_t bus;                                // The USB BUS number as determined by `libusb`
        };

        namespace Audio {
            struct GkDevice {
                std::string dev_name_formatted;         // The name of the device itself, formatted
                bool default_dev;                       // Is this the default device for the system?
                bool default_disp;                      // Used for filtering purposes
                double def_sample_rate;                 // Default sample rate
                boost::tribool is_output_dev;           // Is the audio device in question an input? Output if FALSE, UNSURE if either
                int dev_number;                         // The number of this device; this is saved to the Google LevelDB database as the user's preference
                PaError dev_err;                        // Any errors that belong to this audio device specifically
                std::vector<double> supp_sample_rates;  // Supported sample rates by this audio device
                long asio_min_latency;                  // ASIO specific
                long asio_max_latency;                  // ASIO specific
                long asio_granularity;                  // ASIO specific
                long asio_pref_latency;                 // ASIO specific
                long asio_min_buffer_size;              // ASIO specific
                long asio_max_buffer_size;              // ASIO specific
                long asio_pref_buffer_size;             // ASIO specific
                int dev_input_channel_count;            // The number of channels this INPUT audio device supports
                int dev_output_channel_count;           // The number of channels this OUTPUT audio device supports
                audio_channels sel_channels;            // The selected audio channel configuration
                PaError asio_err;                       // ASIO specific error related information
                PaDeviceInfo device_info;               // All information pertaining to this audio device
                PaStreamParameters stream_parameters;   // Device-specific information such as the sample format, etc.
                PaHostApiTypeId host_type_id;
            };
        }
    }
}

namespace AmateurRadio {
#define STATUS_CHECK_TIMEOUT 500       // Milliseconds

    enum rig_type {
        Transceiver,
        Handheld,
        Mobile,
        Receiver,
        PC_Receiver,
        Scanner,
        TrunkingScanner,
        Computer,
        Other,
        Unknown
    };

    enum bands {
        NONE,
        BAND160,
        BAND80,
        BAND60,
        BAND40,
        BAND30,
        BAND20,
        BAND15,
        BAND17,
        BAND12,
        BAND10,
        BAND6,
        BAND2,
        BAND222,
        BAND420,
        BAND630,
        BAND902,
        BAND1240,
        BAND2200,
        BAND2300,
        BAND3300,
        BAND5650,
        BAND10000,
        BAND24000,
        BAND47000,
        BAND76000,
        BAND122000,
        BAND134000,
        BAND241000
    };

    enum com_baud_rates {
        BAUD1200,
        BAUD2400,
        BAUD4800,
        BAUD9600,
        BAUD19200,
        BAUD38400,
        BAUD57600,
        BAUD115200
    };

    namespace Control {
        struct Radio {                      // https://github.com/Hamlib/Hamlib/blob/master/tests/example.c
            RIG *rig;                       // Hamlib rig pointer
            bool is_open;                   // Has HamLib been successfully initiated (including the RIG* pointer?)
            std::string rig_file;           // Hamlib rig temporary file
            std::string info_buf;           // Hamlib information buffer
            std::string mm;                 // Hamlib modulation mode
            rig_model_t rig_model;          // Hamlib rig model
            rig_debug_level_e verbosity;    // The debug level and verbosity of Hamlib
            com_baud_rates dev_baud_rate;   // Communication device baud rate
            freq_t freq;                    // Rig's primary frequency
            value_t raw_strength;           // Raw strength of the S-meter
            value_t strength;               // Calculated strength of the S-meter
            value_t power;                  // Rig's power output
            float s_meter;                  // S-meter values
            int status;                     // Hamlib status code
            int retcode;                    // Hamlib return code
            int isz;                        // No idea what this is for?
            unsigned int mwpower;           // Converted power reading to watts
            rmode_t mode;                   // Unknown?
            pbwidth_t width;                // Bandwidth
        };
    }
}

namespace Spectrograph {
    // http://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html
    struct RawFFT {
        fftw_complex *chunk_forward_0;
        fftw_complex *chunk_forward_1;
    };

    struct Window {
        struct axis {
            int rows;
            int cols;
        };
    };
}
};
