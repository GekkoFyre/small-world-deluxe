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

#include <libusb.h>
#include <fftw3.h>
#include <vector>
#include <exception>
#include <string>
#include <locale>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QDateTime>

#ifdef _WIN32
#ifdef PA_USE_ASIO
#include <pa_asio.h>
#include "contrib/portaudio/cpp/include/portaudiocpp/AsioDeviceAdapter.hxx"
#endif

#include <winsdkver.h>
#include <Windows.h>
#include <tchar.h> // https://linuxgazette.net/147/pfeiffer.html
#if defined(_MSC_VER) && (_MSC_VER > 1900)
#include <atlbase.h>
#include <atlstr.h>
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <hamlib/rig.h>
#include <hamlib/riglist.h>

#ifdef _WIN32
    typedef std::wstring gkwstring;
#if defined(_MSC_VER) && (_MSC_VER > 1915)
#endif
#elif __linux__
#include <libusb-1.0/libusb.h>
    typedef std::string gkwstring;
#endif

#ifdef __cplusplus
} // extern "C"
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
#define MAX_TOLERATE_WINDOW_WIDTH (16384)               // This value is mostly for error correction purposes.
#define DLG_BOX_WINDOW_WIDTH (480)                      // The width of non-Qt generated dialog boxes
#define DLG_BOX_WINDOW_HEIGHT (120)                     // The height of non-Qt generated dialog boxes

#define AUDIO_OUTPUT_MAX_VOL_SIMPLE (100)               // The maximum volume in simple units (i.e. non decible units)
#define AUDIO_OUTPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_OUTPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_INPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_INPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_FRAMES_PER_BUFFER (256)                   // Frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback. Many apps may want to use paFramesPerBufferUnspecified, which tells PortAudio to pick the best, possibly changing, buffer size
#define AUDIO_TEST_SAMPLE_LENGTH_SEC (3)
#define AUDIO_TEST_SAMPLE_TABLE_SIZE (200)
#define AUDIO_SPEC_FLOOR_DECIBELS (-180.0)

#define AUDIO_BUFFER_STREAMING_SECS (1)
#define AUDIO_SINE_WAVE_PLAYBACK_SECS (3)               // Play the sine wave test sample for three seconds!
#define AUDIO_VU_METER_UPDATE_MILLISECS (500)           // How often the volume meter should update, in milliseconds.

//
// Mostly regarding FFTW functions
//
#define AUDIO_SIGNAL_LENGTH (2048)                      // For audio applications, '2048' seems to be a good length.
#define FFTW_HOP_SIZE (32768)                           // Choose a smaller hop-size if you want a higher resolution! Needs to be a power of two.
#define SPECTRO_BANDWIDTH_MAX_SIZE (2048)               // The size and bandwidth of the spectrograph / waterfall window, in hertz.
#define SPECTRO_BANDWIDTH_MIN_SIZE (125)                // The size and bandwidth of the spectrograph / waterfall window, in hertz.

//
// Concerns spectrograph / waterfall calculations and settings
//
#define SPECTRO_REFRESH_CYCLE_MILLISECS (1000)          // How often the spectrograph / waterfall should update, in milliseconds.
#define SPECTRO_TIME_UPDATE_MILLISECS (15000)           // How often, in milliseconds, the spectrograph updates the timing information on the y-axis.
#define SPECTRO_TIME_HORIZON (60)                       // Not sure what this is, as it has been reverse engineered from something else.
#define SPECTRO_MAX_BUFFER_SIZE (10)                    // The maximum number of items to store within the buffers associated with the spectrograph.
#define SPECTRO_Y_AXIS_SIZE (10000)                     // The maximum size of the y-axis, in milliseconds, given that it is based on a timescale.

//
// Audio encoding/decoding
//
#define AUDIO_CODECS_OGG_VORBIS_ENCODE_QUALITY (1.0)    // The quality at which to encode an Ogg Vorbis file!
#define AUDIO_CODECS_OGG_VORBIS_BUFFER_SIZE (64 * 1024) // The buffering size for each frame, in kilobytes, to be used with Ogg Vorbis encoding.
#define AUDIO_CODECS_OPUS_MAX_PACKETS (1500)            // Not sure what this is for but it seems to be necessary!

#ifndef M_PI
#define M_PI (3.14159265358979323846) /* pi */
#endif

namespace General {
    constexpr char companyName[] = "GekkoFyre Networks";
    constexpr char productName[] = "Small World Deluxe";
    constexpr char appVersion[] = "0.0.1";
    constexpr char appRelease[] = "Pre-alpha";
    constexpr char codeRepository[] = "https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe";
}

namespace Filesystem {
    constexpr char resourceFile[] = "resources.rcc";

    constexpr char defaultDirAppend[] = "SmallWorld";   // The dir to append at the end of a default path, such as within the user's profile directory.
    constexpr char fileName[] = "settings";             // The filename for the database itself which is TAR archived and compressed
    constexpr char tarExtension[] = ".tar";             // The file extension given to (mostly uncompressed) TAR archive
    constexpr char tmpExtension[] = ".tmp";             // The file extension give to temporary files
    constexpr char file_storage[] = "files.ini";        // Where the names of the files will be stored, regarding ZLIB compression

    constexpr char linux_sys_tty[] = "/sys/class/tty/"; // The location of the TTY-devices under most major Linux distributions
}

namespace System {
    namespace Cli {
        enum CommandLineParseResult
        {
            CommandLineOk,
            CommandLineError,
            CommandLineVersionRequested,
            CommandLineHelpRequested
        };
    }
}

namespace Database {
    namespace Settings {
        constexpr char dbName[] = "Database/DbName";
        constexpr char dbExt[] = "Database/DbExt";
        constexpr char dbLoc[] = "Database/DbLoc";

        enum init_cfg {                                 // The initial configuration that will be used by Small World Deluxe, through <QSettings> so that Google LevelDB maybe initialized.
            DbName,                                     // The name of the initial, Google LevelDB database.
            DbExt,                                      // The file extension (if any) of the Google LevelDB database.
            DbLoc                                       // The location of the Google LevelDB database.
        };

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
    enum GkColorMap
    {
        RGBMap,
        IndexMap,
        HueMap,
        AlphaMap
    };

    struct RawFFT {
        fftw_complex *chunk_forward_0;
        fftw_complex *chunk_forward_1;
        std::vector<double> power_density_spectrum;
        double hanning_win;
        size_t window_size;
    };

    struct Window {
        int y;
        int x;
    };

    struct Graphing {
        RawFFT fft;
        Window axis;
    };

    struct GkAxisData {
        QwtInterval z_interval;                                                 // Interval values for the z-axis.
        QwtInterval x_interval;                                                 // Interval values for the x-axis.
        QwtInterval y_interval;                                                 // Interval values for the y-axis.
    };

    struct GkTimingData {
        qint64 relative_start_time;                                             // The 'relative starting time' for when the spectrograph was initialized.
        qint64 relative_stop_time;                                              // The 'relative stopping time' for when the spectrograph was deinitialized.
        qint64 curr_time;                                                       // The more up-to-date time, as a UNIX epoch.
    };

    //
    // Used for the raster/matrix data calculations within the spectrograph/waterfall of QMainWindow!
    //
    struct MatrixData {
        QMap<qint64, std::pair<QVector<double>, GkAxisData>> z_data_calcs;      // STFT (i.e. Fast Fourier Transformation) calculations as processed by GekkoFyre::SpectroFFTW::stft().
        std::vector<GkTimingData> timing;                                       // Information that pertains to timing as it relates to the spectrograph / waterfall.
        GkAxisData curr_axis_info;                                              // Information that pertains to the axis' and their intervals as of the immediate moment.
        qint64 actual_start_time;                                               // The actual starting time at which the spectrograph was initialized.
        double min_z_axis_val;                                                  // Most minimum value as presented by the z-axis (the coloration portion).
        double max_z_axis_val;                                                  // Most maximum value as presented by the z-axis (the coloration portion).
        int window_size;                                                        // The value as passed towards GekkoFyre::SpectroFFTW::stft().
        size_t hanning_win;                                                     // The 'window hanning' value.
    };
}

namespace GkAudioFramework {
    enum CodecSupport {
        PCM,
        OggVorbis,
        Opus,
        FLAC,
        Unknown
    };

    enum Bitrate {
        LosslessCompressed,
        LosslessUncompressed,
        VBR,
        Kbps64,
        Kbps128,
        Kbps192,
        Kbps256,
        Kbps320,
        Default
    };

    struct AudioFileInfo {
        boost::filesystem::path audio_file_path;                                // The path to the audio file itself, if known.
        bool is_output;                                                         // Are we dealing with this as an input or output file?
        double sample_rate;                                                     // The sample rate of the file.
        CodecSupport type_codec;                                                // The codec of the audio file, if known.
        Database::Settings::audio_channels num_audio_channels;                  // The number of audio channels (i.e. if stereo or mono).
        long bitrate_lower;                                                     // The lower end of the bitrate scale for the specified file.
        long bitrate_upper;                                                     // The upper end of the bitrate scale for the specified file.
        long bitrate_nominal;                                                   // The nominal bitrate for the specified file.
    };
}
};
