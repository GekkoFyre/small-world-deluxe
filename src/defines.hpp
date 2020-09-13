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

#include "src/gk_string_funcs.hpp"
#include <portaudiocpp/PortAudioCpp.hxx>
#include <boost/exception/all.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/filesystem.hpp>
#include <hamlib/rigclass.h>
#include <qwt/qwt_interval.h>
#include <sentry.h>
#include <list>
#include <vector>
#include <string>
#include <locale>
#include <cstdio>
#include <memory>
#include <cstdlib>
#include <utility>
#include <iostream>
#include <exception>
#include <streambuf>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QPointer>
#include <QDateTime>
#include <QStringList>
#include <QtUsb/QUsbDevice>
#include <QSerialPortInfo>

#ifdef _WIN32
#ifdef PA_USE_ASIO
#include <pa_asio.h>
#include "contrib/portaudio/cpp/include/portaudiocpp/AsioDeviceAdapter.hxx"
#endif

#include <winsdkver.h>
#include <Windows.h>
#include <tchar.h> // https://linuxgazette.net/147/pfeiffer.html
#if defined(_WIN32) || defined(__MINGW64__)
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
    typedef std::string gkwstring;
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(_WIN32) || defined(__MINGW64__)
#define snprintf _snprintf
#endif

namespace GekkoFyre {

#define GK_EXIT_TIMEOUT (6)                             // The amount of time, in seconds, to leave 'Small World Deluxe' hanging upon exit before terminating forcefully!
#define MIN_MAIN_WINDOW_WIDTH (1024)
#define MIN_MAIN_WINDOW_HEIGHT (768)
#define MAX_TOLERATE_WINDOW_WIDTH (16384)               // This value is mostly for error correction purposes.
#define DLG_BOX_WINDOW_WIDTH (480)                      // The width of non-Qt generated dialog boxes
#define DLG_BOX_WINDOW_HEIGHT (120)                     // The height of non-Qt generated dialog boxes
#define GK_ZLIB_BUFFER_SIZE (4096)                      // The size of the buffer, in kilobytes, as-used by Zlib throughout Small World Deluxe's code-base...

//
// Amateur radio specific functions
//
#define GK_RADIO_VFO_FLOAT_PNT_PREC (5)                 // The floating point precision, in terms of number of digits, to be used in making comparisons (i.e. the 'epsilon') of frequencies, etc.

#define AUDIO_OUTPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_OUTPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_INPUT_CHANNEL_MAX_LIMIT (1024)
#define AUDIO_INPUT_CHANNEL_MIN_LIMIT (-1024)
#define AUDIO_FRAMES_PER_BUFFER (512)                   // Frames per buffer, i.e. the number of sample frames that PortAudio will request from the callback. Many apps may want to use paFramesPerBufferUnspecified, which tells PortAudio to pick the best, possibly changing, buffer size
#define AUDIO_TEST_SAMPLE_TABLE_SIZE (200)

#define AUDIO_SINE_WAVE_PLAYBACK_SECS (3)               // Play the sine wave test sample for three seconds!
#define AUDIO_VU_METER_UPDATE_MILLISECS (125)           // How often the volume meter should update, in milliseconds.
#define AUDIO_VU_METER_PEAK_DECAY_RATE (0.001)          // Unknown
#define AUDIO_VU_METER_PEAK_HOLD_LEVEL_DURATION (2000)  // Measured in milliseconds

//
// Mostly regarding FFTW functions
//
#define GK_FFT_SIZE (4096)

//
// Concerns spectrograph / waterfall calculations and settings
//
#define SPECTRO_REFRESH_CYCLE_MILLISECS (1000)          // How often the spectrograph / waterfall should update, in milliseconds.
#define SPECTRO_TIME_UPDATE_MILLISECS (15000)           // How often, in milliseconds, the spectrograph updates the timing information on the y-axis.
#define SPECTRO_MAX_BUFFER_SIZE (10)                    // The maximum number of items to store within the buffers associated with the spectrograph.
#define SPECTRO_X_MIN_AXIS_SIZE (0.0f)                  // The default, lower-limit of the x-axis on the spectrograph / waterfall, in hertz.
#define SPECTRO_X_MAX_AXIS_SIZE (2500.0f)               // The default, upper-limit of the x-axis on the spectrograph / waterfall, in hertz.
#define SPECTRO_Y_AXIS_SIZE (60000)                     // The maximum size of the y-axis, in milliseconds, given that it is based on a timescale.
#define SPECTRO_Y_AXIS_MINOR (15)
#define SPECTRO_Y_AXIS_MAJOR (8)

#define GRAPH_DISPLAY_WATERFALL_STD_IDX (0)             // Display the standard waterfall!
#define GRAPH_DISPLAY_2D_SINEWAVE_IDX (1)               // Display the 2D Sinewave graph!

#define GRAPH_DISPLAY_500_MILLISECS_IDX (0)             // Display '500 milliseconds' within the QComboBox!
#define GRAPH_DISPLAY_1_SECONDS_IDX (1)                 // Display '1 seconds' within the QComboBox!
#define GRAPH_DISPLAY_2_SECONDS_IDX (2)                 // Display '2 seconds' within the QComboBox!
#define GRAPH_DISPLAY_5_SECONDS_IDX (3)                 // Display '5 seconds' within the QComboBox!
#define GRAPH_DISPLAY_10_SECONDS_IDX (4)                // Display '10 seconds' within the QComboBox!

//
// Audio encoding/decoding
//
#define AUDIO_CODECS_OGG_VORBIS_ENCODE_QUALITY (1.0)    // The quality at which to encode an Ogg Vorbis file!
#define AUDIO_CODECS_OGG_VORBIS_BUFFER_SIZE (64 * 1024) // The buffering size for each frame, in kilobytes, to be used with Ogg Vorbis encoding.
#define AUDIO_CODECS_OPUS_MAX_PACKETS (1500)            // Not sure what this is for but it seems to be necessary!

//
// RS232 & USB Connections
//
#define RS232_DEFAULT_BAUD_SPEED (9600)                 // The default baud speed made for any RS232 connections, mostly made for testing purposes.
#define RS232_DEFAULT_TIMEOUT (5000)                    // The default timeout value for any RS232 connections, mostly made for testing purposes.

//
// QTableView Models
//
#define GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX (0)       // The desired ordering for the 'Frequency' heading within the QTableView model for class, `GkFreqTableViewModel`.
#define GK_FREQ_TABLEVIEW_MODEL_MODE_IDX (1)            // The desired ordering for the 'Mode' heading within the QTableView model for class, `GkFreqTableViewModel`.
#define GK_FREQ_TABLEVIEW_MODEL_IARU_REGION_IDX (2)     // The desired ordering for the 'IARU Region' heading within the QTableView model for class, `GkFreqTableViewModel`.
#define GK_FREQ_TABLEVIEW_MODEL_TOTAL_IDX (3)           // The total amount of indexes (i.e. columns) for the QTableView model, `GkFreqTableViewModel`. Be sure to keep this up-to-date!
#define GK_FREQ_TABLEVIEW_MODEL_NUM_PRECISION (15)      // The number of decimal places for which to display the frequencies as!

#define GK_EVENTLOG_TABLEVIEW_MODEL_EVENT_NO_IDX (0)    // The desired ordering for the 'Event No.' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_DATETIME_IDX (1)    // The desired ordering for the 'Date & Time' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX (2)    // The desired ordering for the 'Severity' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX (3)     // The desired ordering for the 'Message' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_TOTAL_IDX (4)       // The total amount of indexes (i.e. columns) for the QTableView model, `GkEventLoggerTableViewModel`. Be sure to keep this up-to-date!

#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_OFFSET_IDX (0)
#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_DATETIME_IDX (1)
#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_AGE_IDX (2)
#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_SNR_IDX (3)
#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_MSG_IDX (4)
#define GK_ACTIVE_MSGS_TABLEVIEW_MODEL_TOTAL_IDX (5)    // The total amount of indexes (i.e. columns) for the QTableView model, `GkActiveMsgsTableViewModel`. Be sure to keep this up-to-date!

#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_CALLSIGN_IDX (0)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_DATETIME_IDX (1)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_AGE_IDX (2)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_SNR_IDX (3)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_OFFSET_IDX (4)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_CHECKMARK_IDX (5)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_NAME_IDX (6)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_COMMENT_IDX (7)
#define GK_CSIGN_MSGS_TABLEVIEW_MODEL_TOTAL_IDX (8)     // The total amount of indexes (i.e. columns) for the QTableView model, `GkActiveMsgsTableViewModel`. Be sure to keep this up-to-date!

//
// Hamlib related
//
#define GK_HAMLIB_DEFAULT_TIMEOUT (15000)               // The default timeout value for Hamlib, measured in milliseconds.

//
// SSTV related
//
#define GK_SSTV_FILE_DLG_LOAD_IMGS_MAX_FILES_WARN (32)  // The maximum amount of individual images/files to allow to be loaded through a QFileDialog before warning the user about any implications of loading too many into memory at once!

//
// CODEC2 Modem related
//
#define GK_CODEC2_FRAME_SIZE (320)                      // The size of a single frame, in bytes, for the Codec2 modem.

//
// JT65 Modem related
//
#define GK_WSJTX_JT65_CALLSIGN_SYMBOL_MAX_SIZE (6)      // The maximum amount of characters, or rather, symbols that the callsigns used in communications with JT65 can be.

#ifndef M_PI
#define M_PI (3.14159265358979323846) /* pi */
#endif

namespace General {
    constexpr char companyName[] = "GekkoFyre Networks";
    constexpr char productName[] = "Small World Deluxe";
    constexpr char executableName[] = "smallworld";
    constexpr char appVersion[] = "0.0.1";
    constexpr char appRelease[] = "Pre-alpha";
    constexpr char codeRepository[] = "https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe";

    constexpr char gk_sentry_uri[] = "https://5532275153ce4eb4865b89eb2441f356@sentry.gekkofyre.io/2";
    constexpr char gk_sentry_user_side_uri[] = "https://sentry.gekkofyre.io/";
    constexpr char gk_sentry_env[] = "development";
}

namespace Filesystem {
    constexpr char resourceFile[] = "resources.rcc";

    constexpr char defaultDirAppend[] = "SmallWorld";                   // The dir to append at the end of a default path, such as within the user's profile directory.
    constexpr char fileName[] = "settings";                             // The filename for the database itself which is TAR archived and compressed
    constexpr char tarExtension[] = ".tar";                             // The file extension given to (mostly uncompressed) TAR archive
    constexpr char tmpExtension[] = ".tmp";                             // The file extension give to temporary files

    constexpr char gk_sentry_dump_dir[] = "crash-db";                   // The directory for where any possible memory dumps and crash analyses will have to live!S
    constexpr char gk_sentry_dump_file_ext[] = ".log";                  // The file extension for memory-dumps (i.e. 'minidumps') as generated by Crashpad for Sentry.
    constexpr char gk_crashpad_handler_win[] = "crashpad_handler.exe";  // The name of the Crashpad handler executable under Microsoft Windows operating systems.
    constexpr char gk_crashpad_handler_linux[] = "crashpad_handler";    // The name of the Crashpad handler executable under Linux and possibly Unix-like operating systems.

    constexpr char linux_sys_tty[] = "/sys/class/tty/";                 // The location of the TTY-devices under most major Linux distributions
}

namespace System {
    /**
     * @brief The membuf struct
     * @author Dietmar Kühl <https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory/13059195#13059195>
     * @note <https://stackoverflow.com/a/52492027>
     */
    struct membuf: std::streambuf {
        membuf(char const* base, size_t size) {
            char* p(const_cast<char*>(base));
            this->setg(p, p, p + size);
        }
    };

    /**
     * @brief The imemstream struct
     * @author Dietmar Kühl <https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory/13059195#13059195>
     * @note <https://stackoverflow.com/a/52492027>
     */
    struct imemstream: virtual membuf, std::istream {
        imemstream(char const *base, size_t size):
            membuf(base, size), std::istream(static_cast<std::streambuf*>(this)) {
        }
    };

    namespace Cli {
        enum CommandLineParseResult
        {
            CommandLineOk,
            CommandLineError,
            CommandLineVersionRequested,
            CommandLineHelpRequested
        };
    }

    namespace Events {
        namespace Logging {
            enum GkSeverity {
                Fatal,
                Error,
                Warning,
                Info,
                Debug,
                Verbose,
                None
            };

            enum GkSentry {
                AskedDialog,
                GivenConsent
            };

            enum GkOptIn {
                UserUniqueId
            };
        }

        struct GkMsg {
            qint64 date;                                // The date and time at which the message was spawned!
            Logging::GkSeverity severity;               // The severity of the event, whether it was 'informational' or a 'fatal error'.
            QString message;                            // The actual details of the message itself.
            QVariant arguments;                         // If any arguments were provided alongside the message itself as well.
        };

        namespace Logging {
            struct GkEventLogging {
                GkMsg mesg;                             // Details concerning the event log itself.
                int event_no;                           // The unique 'index number' for the given event.
                bool show;                              // Whether to show this event within the UI interface(s) or not.
            };
        }
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
            CatConnType,
            PttConnType,
            ComDeviceCat,
            ComDevicePtt,
            ComDeviceCatPortType,
            ComDevicePttPortType,
            ComBaudRate,
            StopBits,
            DataBits,
            Handshake,
            ForceCtrlLinesDtr,
            ForceCtrlLinesRts,
            PTTMethod,
            TXAudioSrc,
            PTTMode,
            SplitOperation,
            PTTAdvCmd,
            RXAudioInitStart
        };

        enum audio_cfg {
            soundcardInput,
            soundcardOutput,
            settingsDbLoc,
            LogsDirLoc,
            AudioRecLoc,
            AudioInputChannels,
            AudioOutputChannels,
            AudioInputSampleRate,
            AudioOutputSampleRate
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
            defStationInfo,
            MsgAudioNotif,
            FailAudioNotif
        };

        enum general_mainwindow_cfg {
            WindowMaximized,
            WindowHSize,
            WindowVSize
        };

        enum Codec2Mode {
            freeDvMode2020,
            freeDvMode700D,
            freeDvMode700C,
            freeDvMode800XA,
            freeDvMode2400B,
            freeDvMode2400A,
            freeDvMode1600
        };

        enum Codec2ModeCustom {
            GekkoFyreV1,
            Disabled
        };

        struct UsbVers3 {
            int interface_number;                                                   // Number of this interface!
            int alternate_setting;                                                  // Value used to select this alternate setting for this interface
            int max_packet_size;                                                    // Maximum packet size this endpoint is capable of sending/receiving
            int interval;                                                           // Interval for polling endpoint for data transfers
            int refresh;                                                            // For audio devices only: the rate at which synchronization feedback is provided
            int sync_address;                                                       // For audio devices only: the address if the synch endpoint
        };

        struct GkLibUsb {
            QString mfg;                                                            // The manufacturer of this USB device, as determined by 'libusb'.
            QString product;                                                        // The product name/identity of this USB device, as determined by 'libusb'.
            QString serial;                                                         // The unique serial number associated with this USB device, as determined by 'libusb'.
            QString path;                                                           // The system path for the given USB device, as provided by `lsusb` <https://github.com/gregkh/usbutils>.
            quint16 dev_num;                                                        // The number associated with the USB device itself, as per the format, `/dev/tty<number>`.
        };

        struct GkUsb2Exts {
            // USB 2.0 capabilities                                                 // Structure which exists strictly for USB >2.0+ devices.
            quint8 dev_cap_type;                                                    // Compatibility type for USB 2.0 devices.
            quint32 bm_attribs;                                                     // Bitmap encoding of supported device level features.
        };

        struct GkUsb3Exts {
            // USB 3.0 capabilities                                                 // Structure which exists strictly for USB >3.0+ devices.
            quint8 dev_cap_type;                                                    // Capability type.
            quint8 bm_attribs;                                                      // Bitmap encoding of supported device level features.
            quint16 write_speed_supported;                                          // Bitmap encoding of the speed supported by this device when operating in SuperSpeed mode.
            quint8 functionality_support;                                           // The lowest speed at which all the functionality supported by the device is available to the user. For example if the device supports all its functionality when connected at full speed and above then it sets this value to 1.
            quint8 b_u1_dev_exit_lat;                                               // U1 Device Exit Latency.
            quint16 b_u2_dev_exit_lat;                                              // U2 Device Exit Latency.
        };

        struct GkBosUsb {                                                           // USB Binary Object Store structure.
            GkUsb2Exts usb_2;                                                       // Extensions for USB >2.0+ devices.
            GkUsb3Exts usb_3;                                                       // Extensions for USB >3.0+ devices.
            GkLibUsb lib_usb;                                                       // Additional information pertaining to USB devices, both old and new.
            quint16 pid;
            quint16 vid;
            quint8 addr;
            quint8 bus;
        };

        struct GkUsbPort {
            GkBosUsb bos_usb;                                                       // USB Binary Object Store structure.
            QString name;                                                           // The actual name of the USB port as displayed by the operating system itself
            quint16 port;                                                           // The USB port number as determined by `QtUsb`
            quint16 bus;                                                            // The USB BUS number as determined by `QtUsb`
            quint16 pid;                                                            // The USB port's own Product ID as determined by 'QtUsb'
            quint16 vid;                                                            // The USB port's own Vendor ID as determined by 'QtUsb'
            quint16 d_class;                                                        // Unknown
            quint16 d_sub_class;                                                    // Unknown
        };

        struct GkComPort {
            QSerialPortInfo port_info;                                              // Details on the COM/RS232/Serial ports themselves
            uint32_t def_baudrate;                                                  // The defined baudrate for this serial port in question
        };

        namespace Audio {
            struct GkPaAudioData {
                int frameIndex;                                                     // Frame index into sample array
                int maxFrameIndex;                                                  // Maximum frame index given into sample array
                qint16 *recordedSamples;                                            // Audio samples that have been recorded and saved to a buffer
                portaudio::SampleDataFormat sample_format;                          // Currently used sample format by given audio source, whether output or input
            };

            struct GkDevice {
                std::string dev_name_formatted;                                     // The name of the device itself, formatted
                bool default_dev;                                                   // Is this the default device for the system?
                bool default_disp;                                                  // Used for filtering purposes
                bool is_dev_active;                                                 // Is the audio device in question currently active and streaming data?
                double def_sample_rate;                                             // Default sample rate
                boost::tribool is_output_dev;                                       // Is the audio device in question an input? Output if FALSE, UNSURE if either
                int dev_number;                                                     // The number of this device; this is saved to the Google LevelDB database as the user's preference
                PaError dev_err;                                                    // Any errors that belong to this audio device specifically
                std::list<double> supp_sample_rates;                                // Supported sample rates by this audio device
                long asio_min_latency;                                              // ASIO specific
                long asio_max_latency;                                              // ASIO specific
                long asio_granularity;                                              // ASIO specific
                long asio_pref_latency;                                             // ASIO specific
                long asio_min_buffer_size;                                          // ASIO specific
                long asio_max_buffer_size;                                          // ASIO specific
                long asio_pref_buffer_size;                                         // ASIO specific
                int dev_input_channel_count;                                        // The number of channels this INPUT audio device supports
                int dev_output_channel_count;                                       // The number of channels this OUTPUT audio device supports
                audio_channels sel_channels;                                        // The selected audio channel configuration
                PaError asio_err;                                                   // ASIO specific error related information
                PaDeviceInfo device_info;                                           // All information pertaining to this audio device
                PaStreamParameters stream_parameters;                               // Device-specific information such as the sample format, etc.
                portaudio::DirectionSpecificStreamParameters cpp_stream_param;      // Device-specific information such as the sample format, etc. but for the C++ bindings instead
                PaHostApiTypeId host_type_id;
            };
        }
    }
}

namespace AmateurRadio {
#define STATUS_CHECK_TIMEOUT 500       // Milliseconds

    namespace Gui {
        enum sstvWindow {
            rxLiveImage,
            rxSavedImage,
            txSendImage,
            None
        };
    }

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

    enum DigitalModes {
        WSPR,
        JT65,
        JT9,
        T10,
        FT8,
        FT4,
        Codec2
    };

    enum IARURegions {
        ALL,
        R1,
        R2,
        R3
    };

    enum GkFreqBands {
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

    enum GkConnType {
        GkRS232,
        GkUSB,
        GkParallel,
        GkCM108,
        GkGPIO,
        GkNone
    };

    enum GkConnMethod {
        CAT,
        PTT
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

    enum GkFreqsDb {
        GkStoredFreq,
        GkClosestBand,
        GkDigitalMode,
        GkIARURegion
    };

    struct GkFreqs {
        quint64 frequency;                                   // The exact frequency itself
        GkFreqBands closest_freq_band;                      // The closest matching frequency band grouping
        DigitalModes digital_mode;                          // The type of digital mode this frequency applies towards, or should apply toward
        IARURegions iaru_region;                            // The IARU Region that this frequency falls under
    };

    namespace Control {
        struct GkRadio {                                    // <https://github.com/Hamlib/Hamlib/blob/master/c%2B%2B/rigclass.cc>.
            std::shared_ptr<Rig> gkRig;                     // Hamlib rig pointer
            int rig_brand;                                  // Hamlib rig brand/manufacturer
            rig_model_t rig_model;                          // The actual amateur radio rig itself!
            std::unique_ptr<rig_caps> capabilities;         // Read-only; the capabilities of the configured amateur radio rig in question, as defined by Hamlib.
            std::unique_ptr<rig_state> rig_status;          // Rig state containing live data and customized fields
            powerstat_t power_status;                       // Whether the radio rig is electrically powered on or off
            hamlib_port_t port_details;                     // Information concerning details about RS232 ports, etc.
            ptt_t ptt_status;                               // PTT status
            ptt_type_t ptt_type;                            // PTT Type
            split_t split_mode;                             // Whether 'Split Mode' is enabled or disabled
            bool is_open;                                   // Has HamLib been successfully initiated (including the RIG* pointer?)
            std::string info_buf;                           // Hamlib information buffer
            GkConnType cat_conn_type;                       // The type of connection, whether USB, RS232, etc.
            GkConnType ptt_conn_type;                       // The type of connection, whether USB, RS232, etc.
            QString cat_conn_port;                          // The actual port address itself
            QString ptt_conn_port;                          // The actual port address itself
            std::string mm;                                 // Hamlib modulation mode
            rig_debug_level_e verbosity;                    // The debug level and verbosity of Hamlib
            com_baud_rates dev_baud_rate;                   // Communication device baud rate
            std::string adv_cmd;                            // The 'Advanced Command' parameters, if specified
            freq_t freq;                                    // Rig's primary frequency
            float power;                                    // Rig's power output
            float s_meter;                                  // S-meter values
            int raw_strength;                               // Raw strength of the S-meter
            int strength;
            int status;                                     // Hamlib status code
            int retcode;                                    // Hamlib return code
            unsigned int mwpower;                           // Converted power reading to watts
            rmode_t mode;                                   // The type of modulation that the transceiver is in, whether it be AM, FM, SSB, etc.
            QString mode_hr;                                // The type of modulation that the transceiver is in, but human readable.
            pbwidth_t width;                                // Bandwidth
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

    enum GkGraphType {
        GkWaterfall,
        GkSinewave
    };

    enum GkGraphTiming {
        GkGraphTime500Millisec,
        GkGraphTime1Sec,
        GkGraphTime2Sec,
        GkGraphTime5Sec,
        GkGraphTime10Sec
    };

    struct Window {
        int y;
        int x;
    };

    struct GkFFTSpectrum {
        double frequency;
        double magnitude;
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
