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

//-V::1042

#include "src/gk_string_funcs.hpp"
#include "src/contrib/hamlib++/include/hamlib/rigclass.h"
#include <boost/exception/all.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/filesystem.hpp>
#include <qwt/qwt_interval.h>
#include <qxmpp/QXmppGlobal.h>
#include <qxmpp/QXmppVCardIq.h>
#include <qxmpp/QXmppMessage.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppRosterIq.h>
#include <qxmpp/QXmppArchiveIq.h>
#include <map>
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
#include <QList>
#include <QIcon>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QtGlobal>
#include <QPointer>
#include <QDateTime>
#include <QHostInfo>
#include <QByteArray>
#include <QStringList>
#include <QAudioFormat>
#include <QHostAddress>
#include <QSerialPortInfo>
#include <QAudioDeviceInfo>

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <winsdkver.h>
#include <Windows.h>
#include <icftypes.h>
#include <comdef.h>
#include <tchar.h> // https://linuxgazette.net/147/pfeiffer.html
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <sentry.h>
#include <hamlib/rig.h>
#include <hamlib/riglist.h>

#ifdef __cplusplus
} // extern "C"
#endif

#define USE_KISS_FFT 1

namespace GekkoFyre {

#define GK_EXIT_TIMEOUT (6)                                     // The amount of time, in seconds, to leave 'Small World Deluxe' hanging upon exit before terminating forcefully!
#define MIN_MAIN_WINDOW_WIDTH (1024)
#define MIN_MAIN_WINDOW_HEIGHT (768)
#define MAX_TOLERATE_WINDOW_WIDTH (16384)                       // This value is mostly for error correction purposes.
#define DLG_BOX_WINDOW_WIDTH (480)                              // The width of non-Qt generated dialog boxes
#define DLG_BOX_WINDOW_HEIGHT (120)                             // The height of non-Qt generated dialog boxes
#define GK_ZLIB_BUFFER_SIZE (4096)                              // The size of the buffer, in kilobytes, as-used by Zlib throughout Small World Deluxe's code-base...
#define GK_MSG_BOX_SETTINGS_DLG_TIMER (10000)                   // The amount of time, in milliseconds, before displaying any potential QMessageBox's to the user at startup about configuring settings. This is needed to allow SWD to initialize everything firstly.
#define GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING (8)              // The maximum amount of zero padding to add towards error messages produced by HRESULT's within Microsoft Window's specific C/C++ code.

//
// Firewall and network security specific settings
//
#define GK_SECURITY_FIREWALL_TCP_PORT_HTTP_80 (80)
#define GK_SECURITY_FIREWALL_TCP_PORT_HTTPS_443 (443)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_CLIENT_5222 (5222)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_CLIENT_SSL_5223 (5223)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_SERVER_5269 (5269)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_HTTP_BINDING_7070 (7070)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_HTTPS_BINDING_7443 (7443)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_CONNECT_MGR_5262 (5262)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_CONNECT_MGR_SSL_5263 (5263)
#define GK_SECURITY_FIREWALL_TCP_PORT_XMPP_FILE_XFER_PROXY_7777 (7777)

//
// XMPP specific constants
//
#define GK_XMPP_CREATE_CONN_PROG_BAR_MIN_PERCT (0)
#define GK_XMPP_CREATE_CONN_PROG_BAR_MAX_PERCT (100)
#define GK_XMPP_CREATE_CONN_PROG_BAR_TOT_PERCT (4)

#define GK_DEFAULT_XMPP_SERVER_PORT (5222)
#define GK_XMPP_AVAIL_COMBO_AVAILABLE_IDX (0)
#define GK_XMPP_AVAIL_COMBO_AWAY_FROM_KB_IDX (1)
#define GK_XMPP_AVAIL_COMBO_EXTENDED_AWAY_IDX (2)
#define GK_XMPP_AVAIL_COMBO_INVISIBLE_IDX (3)
#define GK_XMPP_AVAIL_COMBO_BUSY_DND_IDX (4)
#define GK_XMPP_AVAIL_COMBO_UNAVAILABLE_IDX (5)
#define GK_XMPP_SERVER_TYPE_COMBO_GEKKOFYRE_IDX (0)
#define GK_XMPP_SERVER_TYPE_COMBO_CUSTOM_IDX (1)
#define GK_XMPP_IGNORE_SSL_ERRORS_COMBO_FALSE (0)
#define GK_XMPP_IGNORE_SSL_ERRORS_COMBO_TRUE (1)
#define GK_XMPP_URI_LOOKUP_DNS_SRV_METHOD (0)
#define GK_XMPP_URI_LOOKUP_MANUAL_METHOD (1)

#define GK_XMPP_VCARD_ROSTER_UPDATE_SECS (20)
#define GK_XMPP_NETWORK_STATE_UPDATE_SECS (1)
#define GK_XMPP_HANDLE_DISCONNECTION_SINGLE_SHOT_TIMER_SECS (5)

//
// Networking settings (also sometimes related to XMPP!)
//
#define GK_NETWORK_PING_TIMEOUT_MILLISECS (3000)                // The amount of time, in milliseconds, until a network ping attempt should timeout within.
#define GK_NETWORK_PING_COUNT (3)                               // The amount of times to attempt a network ping until either giving up or ending altogether.
#define GK_NETWORK_CONN_TIMEOUT_MILLSECS (30000)                // The amount of time, in milliseconds, until timeout for an attempted network connection.

//
// Amateur radio specific functions
//
#define GK_RADIO_VFO_FLOAT_PNT_PREC (5)                         // The floating point precision, in terms of number of digits, to be used in making comparisons (i.e. the 'epsilon') of frequencies, etc.

#define AUDIO_FRAMES_PER_BUFFER (1024)                          // Frames per buffer, i.e. the number of sample frames that RtAudio will request from the callback.
#define GK_AUDIO_MAX_CHANNELS (2)                               // The current maximum number of audio channels that Small World Deluxe is able to process for any given multimedia audio file!
#define GK_AUDIO_PEAK_MAX (32768)                               // The maximum integer/signal value possible within the QAudio system.
#define GK_AUDIO_PEAK_MIN (-32768)                              // The minimum integer/signal value possible within the QAudio system.

#define AUDIO_OPUS_FRAMES_PER_BUFFER (960)                      // This is specific to the Opus multimedia encoding/decoding library.
#define AUDIO_OPUS_MAX_FRAMES_PER_BUFFER (1276)
#define AUDIO_OPUS_INT_SIZE (2)
#define AUDIO_OPUS_MAX_FRAME_SIZE (1276)
#define AUDIO_OPUS_FILE_PTR_READ_SIZE (256)                     // To be used with `fread` <http://www.cplusplus.com/reference/cstdio/fread/>.

#define AUDIO_SINE_WAVE_PLAYBACK_SECS (3)                       // Play the sine wave test sample for three seconds!
#define AUDIO_VU_METER_UPDATE_MILLISECS (125)                   // How often the volume meter should update, in milliseconds.
#define AUDIO_VU_METER_PEAK_DECAY_RATE (0.001)                  // Unknown
#define AUDIO_VU_METER_PEAK_HOLD_LEVEL_DURATION (2000)          // Measured in milliseconds
#define GK_AUDIO_VOL_INIT_PERCENTAGE (25.0)
#define GK_AUDIO_VOL_MAX_PERCENTAGE (75.0)
#define GK_AUDIO_VOL_FACTOR (1.3334)

#define AUDIO_PLAYBACK_CODEC_PCM_IDX (3)
#define AUDIO_PLAYBACK_CODEC_LOOPBACK_IDX (4)
#define AUDIO_PLAYBACK_CODEC_VORBIS_IDX (0)
#define AUDIO_PLAYBACK_CODEC_OPUS_IDX (1)
#define AUDIO_PLAYBACK_CODEC_FLAC_IDX (2)

//
// Mostly regarding FFTW functions
//
#define GK_FFT_SIZE (4096)

//
// Concerns spectrograph / waterfall calculations and settings
//
#define SPECTRO_REFRESH_CYCLE_MILLISECS (1000)          // How often the spectrograph / waterfall should update, in milliseconds.
#define SPECTRO_X_MIN_AXIS_SIZE (0)                     // The default, lower-limit of the x-axis on the spectrograph / waterfall, in hertz.
#define SPECTRO_X_MAX_AXIS_SIZE (2500)                  // The default, upper-limit of the x-axis on the spectrograph / waterfall, in hertz.
#define SPECTRO_Y_AXIS_SIZE (60000)                     // The maximum size of the y-axis, in milliseconds, given that it is based on a timescale.

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
// About screen
//
#define GK_ABOUT_SCREEN_BTN_PIXMAP_RADIO_IDX (1)
#define GK_ABOUT_SCREEN_BTN_PIXMAP_ARCTIC_COMM_ONE_IDX (2)
#define GK_ABOUT_SCREEN_BTN_PIXMAP_ARCTIC_COMM_TWO_IDX (3)

//
// QTableView Models
//
#define GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX (0)       // The desired ordering for the 'Frequency' heading within the QTableView model for class, `GkFreqTableModel`.
#define GK_FREQ_TABLEVIEW_MODEL_MODE_IDX (1)            // The desired ordering for the 'Mode' heading within the QTableView model for class, `GkFreqTableModel`.
#define GK_FREQ_TABLEVIEW_MODEL_IARU_REGION_IDX (2)     // The desired ordering for the 'IARU Region' heading within the QTableView model for class, `GkFreqTableModel`.
#define GK_FREQ_TABLEVIEW_MODEL_TOTAL_IDX (3)           // The total amount of indexes (i.e. columns) for the QTableView model, `GkFreqTableModel`. Be sure to keep this up-to-date!
#define GK_FREQ_TABLEVIEW_MODEL_NUM_PRECISION (15)      // The number of decimal places for which to display the frequencies as!

#define GK_EVENTLOG_TABLEVIEW_MODEL_EVENT_NO_IDX (0)    // The desired ordering for the 'Event No.' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_DATETIME_IDX (1)    // The desired ordering for the 'Date & Time' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_SEVERITY_IDX (2)    // The desired ordering for the 'Severity' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_MESSAGE_IDX (3)     // The desired ordering for the 'Message' heading within the QTableView model for class, `GkEventLoggerTableViewModel`.
#define GK_EVENTLOG_TABLEVIEW_MODEL_TOTAL_IDX (4)       // The total amount of indexes (i.e. columns) for the QTableView model, `GkEventLoggerTableViewModel`. Be sure to keep this up-to-date!

#define GK_EVENTLOG_SEVERITY_NONE_IDX (0)
#define GK_EVENTLOG_SEVERITY_VERBOSE_IDX (1)
#define GK_EVENTLOG_SEVERITY_DEBUG_IDX (2)
#define GK_EVENTLOG_SEVERITY_INFO_IDX (3)
#define GK_EVENTLOG_SEVERITY_WARNING_IDX (4)
#define GK_EVENTLOG_SEVERITY_ERROR_IDX (5)
#define GK_EVENTLOG_SEVERITY_FATAL_IDX (6)

#define GK_EVENTLOG_TASKBAR_FLASHER_PERIOD_COUNT (15)   // Total amount of periods to flash for!
#define GK_EVENTLOG_TASKBAR_FLASHER_DURAT_COUNT (750)   // Duration, in milliseconds, between each flashing period.

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
// QTableView Models for XMPP
//
#define GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_PRESENCE_IDX (0)
#define GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_BAREJID_IDX (1)
#define GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_NICKNAME_IDX (2)
#define GK_XMPP_ROSTER_PRESENCE_TABLEVIEW_MODEL_TOTAL_IDX (3)

#define GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_PRESENCE_IDX (0)
#define GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_BAREJID_IDX (1)
#define GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_NICKNAME_IDX (2)
#define GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_REASON_IDX (3)
#define GK_XMPP_ROSTER_PENDING_TABLEVIEW_MODEL_TOTAL_IDX (4)

#define GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_BAREJID_IDX (0)
#define GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_REASON_IDX (1)
#define GK_XMPP_ROSTER_BLOCKED_TABLEVIEW_MODEL_TOTAL_IDX (2)

#define GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_DATETIME_IDX (0)
#define GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_NICKNAME_IDX (1)
#define GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_MSG_IDX (2)
#define GK_XMPP_RECV_MSGS_TABLEVIEW_MODEL_TOTAL_IDX (3)

// Hamlib related
//
#define GK_HAMLIB_DEFAULT_TIMEOUT (3000)                // The default timeout value for Hamlib, measured in milliseconds.

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
    constexpr char companyNameMin[] = "GekkoFyre";
    constexpr char productName[] = "Small World Deluxe";
    constexpr char executableName[] = "smallworld";
    constexpr char appVersion[] = "0.0.1"; // TODO: Make sure to update this upon every release/revision change!
    constexpr char appRelease[] = "Pre-alpha"; // TODO: The same as the immediate above also applies to this line!
    constexpr char xmppVersion[] = "Alpha"; // TODO: And to this as well!
    constexpr char xmppResourceGFyre[] = "GekkoFyre";
    constexpr char codeRepository[] = "https://code.gekkofyre.io/amateur-radio/small-world-deluxe";
    constexpr char officialWebsite[] = "https://swdeluxe.io/";

    constexpr char gk_sentry_dsn_uri[] = "https://2c7b92c7437b4728a2856a6ca71ddbe6@sentry.gekkofyre.io/2";
    constexpr char gk_sentry_user_side_uri[] = "https://sentry.gekkofyre.io/";
    constexpr char gk_sentry_env[] = "development"; // TODO: Make sure to change this upon releasing a proper version of Small World Deluxe!
    constexpr char gk_sentry_project_name[] = "small-world-deluxe"; // The actual name of the project as it is both registered and appears within Sentry itself; it is critical that this is set correctly!

    namespace Xmpp {
        constexpr char captchaNamespace[] = "urn:xmpp:captcha";
        namespace GoogleLevelDb {
            constexpr char jidLookupKey[] = "GkXmppStoredJid";
            constexpr char keyToConvXmlStream[] = "GkXmlStream";
            constexpr char keyToConvAvatarImg[] = "GkAvatarImg";
            constexpr char keyToConvMsgHistory[] = "msg";
            constexpr char keyToConvTimestampHistory[] = "timestamp";
        }
    }
}

namespace Filesystem {
    constexpr char resourceFile[] = "resources.rcc";

    constexpr char defaultDirAppend[] = "SmallWorld";                   // The dir to append at the end of a default path, such as within the user's profile directory.
    constexpr char fileName[] = "settings";                             // The filename for the database itself which is TAR archived and compressed
    constexpr char xmppVCardDir[] = "xmpp-vcards";                      // The directory for where VCards obtained via XMPP are stored within.
    constexpr char fileLogData[] = "log.dat";                           // Where a record of the most up-to-date logging records are kept, from the last application run.
    constexpr char tarExtension[] = ".tar";                             // The file extension given to (mostly uncompressed) TAR archive
    constexpr char tmpExtension[] = ".tmp";                             // The file extension give to temporary files

    //
    // Nuspell & Spelling dictionaries
    constexpr char nuspellLibraryDir[] = "Library";                     // The 'Library' dir which is used by the Nuspell libs
    constexpr char nuspellSpellDir[] = "Spelling";                      // The 'Spelling' dir, present underneath 'Library', which is used by the Nuspell libs
    constexpr char nuspellSpellDic[] = "index";                         // The *.dic and *.aff file for Nuspell dictionaries
    constexpr char enchantSpellDefLang[] = "en_US";                     // The default dictionary language to use for Nuspell

    //
    // User interface language
    constexpr char userInterfaceDefLang[] = "English (Generic)";        // The default language choice for the User Interface itself

    constexpr char gk_sentry_dump_endpoint[] = "https://sentry.gekkofyre.io/api/2/minidump/?sentry_key=2c7b92c7437b4728a2856a6ca71ddbe6"; // [ Minidump Endpoint ] Use this endpoint to upload minidump crash reports, for example with Electron, Crashpad or Breakpad
    constexpr char gk_sentry_dump_dir[] = "crash-db";                   // The directory for where any possible memory dumps and crash analyses will have to live!S
    constexpr char gk_sentry_dump_file_ext[] = ".log";                  // The file extension for memory-dumps (i.e. 'minidumps') as generated by Crashpad for Sentry.
    constexpr char gk_crashpad_handler_win[] = "crashpad_handler.exe";  // The name of the Crashpad handler executable under Microsoft Windows operating systems.
    constexpr char gk_crashpad_handler_linux[] = "crashpad_handler";    // The name of the Crashpad handler executable under Linux and possibly Unix-like operating systems.

    constexpr char linux_sys_tty[] = "/sys/class/tty/";                 // The location of the TTY-devices under most major Linux distributions
}

namespace Network {
    enum GkNetworkProtocol {
        TCP,
        UDP,
        Any
    };

    namespace GkXmpp {
        enum GkUriLookupMethod {
            QtDnsSrv,
            Manual
        };

        enum GkSaslAuthMethod {
            Plain,
            ScramSha1,
            External,
            DigestMd5,
            CramMd5,
            Anonymous
        };

        enum GkNetworkState {
            None,
            Connecting,
            Connected,
            Disconnected,
            WaitForRegistrationForm,
            WaitForRegistrationConfirmation
        };

        enum GkRegUiRole {                                  // Whether the user needs to create an account or is already a pre-existing user.
            AccountCreate,
            AccountLogin,
            AccountChangePassword,
            AccountChangeEmail
        };

        enum GkOnlineStatus {                               // The online availability of the user in question.
            Online,
            Away,
            DoNotDisturb,
            NotAvailable,
            Invisible,
            NetworkError
        };

        enum GkServerType {                                  // Information pertaining to DNS Lookups for the QXmpp library.
            GekkoFyre,
            Custom,
            Unknown
        };

        enum GkVcardKeyConv {
            XmlStream,
            AvatarImg
        };

        struct GkXmppMamMsg {
            bool presented = false;
            QXmppMessage message;
        };

        struct GkXmppArchiveMsg {
            bool presented = false;
            QXmppArchiveMessage message;
        };

        struct GkClientSettings {
            bool allow_msg_history;                         // Shall we keep a message history with this server, provided it's a supported extension?
            bool allow_file_xfers;                          // Shall we allow file transfers with this server, provided it's a supported extension?
            bool allow_mucs;                                // Shall we allow multi-user chats, provided it's a supported extension?
            bool auto_connect;                              // Do we allow automatic connections to the given XMPP server upon startup of Small World Deluxe?
            bool auto_reconnect;                            // Do we attempt a reconnection upon each disconnection from a XMPP server, up to a specified maximum limit (i.e. for safety and to prevent banning)?
            GkUriLookupMethod uri_lookup_method;            // The method by which we lookup the connection settings for the given XMPP server, whether by automatic DNS SRV Lookup or manual user input.
            bool enable_ssl;                                // Enable the absolute usage of SSL/TLS, otherwise throw an exception if not available!
            bool ignore_ssl_errors;                         // Whether to ignore any SSL errors presented by the server and/or client.
            qint32 network_timeout;                         // A timeout value for anything network related, such as serving DNS queries for SRV records, to finally making a connection towards XMPP servers.
            QByteArray upload_avatar_pixmap;                // The byte-array data for the avatar that's to be uploaded upon next making a successful connection to the given XMPP server.
        };

        struct GkHost {                                     // Host information as related to the QXmpp libraries.
            GkClientSettings settings_client;               // Client settings that apply when making a connection to the XMPP server.
            GkServerType type;
            QHostAddress domain;
            QString url;
            QHostInfo info;
            quint16 port;
        };

        struct GkXmppCallsign {
            GkHost server;
            QString bareJid;
            QXmppVCardIq vCard;
            QList<GkXmppArchiveMsg> archive_messages;
            QList<GkXmppMamMsg> messages;
            std::shared_ptr<QXmppPresence> presence;
            QXmppRosterIq::Item::SubscriptionType subStatus;
        };

        struct GkXmppBlocklist {
            GkHost server;
            QString jid;
            QString username;
            QString nickname;
        };

        struct GkUserConn {                                 // User and server information as related to the QXmpp libraries.
            GkHost server;
            GkOnlineStatus status;                          // The online availability of the user in question.
            QList<GkXmppBlocklist> blocked;                 // A blocklist of XMPP users who have been blocked by the connecting client.
            QString jid;                                    // The Account ID as known to the XMPP server itself; used particularly for logging-in.
            QString username;                               // The username, which is the JID without the server URL or resource attached.
            QString password;                               // The password which is needed for logging-in successfully to the XMPP server.
            QString nickname;                               // The desired nickname of the user, as it appears to others on the XMPP server network.
            QString firstName;                              // The first name of the user, if provided.
            QString lastName;                               // The last name of the user, if provided.
            QString email;                                  // The email address, if any, that's associated with this end-user.
        };

        struct GkPresenceTableViewModel {
            QIcon presence;
            QString bareJid;
            QString nickName;
            bool added;
        };

        struct GkPendingTableViewModel {
            QIcon presence;
            QString bareJid;
            QString nickName;
            QString reason;
            bool added;
        };

        struct GkBlockedTableViewModel {
            QString bareJid;
            QString reason;
            bool added;
        };

        struct GkRecvMsgsTableViewModel {
            QDateTime timestamp;    // The current Date & Time, in UTC+0 format.
            QString bareJid;        // The bareJid this data belongs towards.
            QString nickName;       // The nickname of the given bareJid.
            QString message;        // The message the user wishes to send.
        };
    }
}

namespace GkXmppGekkoFyreCfg {
    constexpr char defaultUrl[] = "vk3vkk.chat";
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

    namespace UserInterface {
        enum GkSettingsDlgTab {
            GkGeneralStation,
            GkGeneralFreq,
            GkGeneralUi,
            GkGeneralUiBasic,
            GkGeneralUiThemes,
            GkGeneralXmpp,
            GkGeneralEventLogger,
            GkAudio,
            GkAudioConfig,
            GkAudioRecorder,
            GkAudioApiInfo,
            GkRadio,
            GkRadioRig,
            GkRadioApiInfo,
            GkCheckForUpdates
        };
    }

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

    namespace Security {
        enum GkFirewallStatus {
            Enabled,
            Disabled,
            Unsupported,
            NotFound,
            Unknown
        };

        enum GkFirewallCfg {
            GkAddPort,
            GkDelPort,
            GkAddApp,
            GkDelApp,
            GkIsPortAdded,
            GkIsAppAdded,
            GkReadPorts,
            GkReadApps,
            GkActivateFirewall,
            GkDisableFirewall,
            GkIsFirewallActive
        };

        struct GkFirewallSettings {
            bool sys_firewall_enabled;                                                          // Is the operating system's primary firewall enabled (e.g. the one that is built into Microsoft Windows)?
            bool swd_app_added;                                                                 // Has the Small World Deluxe application itself (i.e. the primary executable) been added to the firewall already?
            std::map<qint32, std::pair<Network::GkNetworkProtocol, bool>> network_ports;        // The value determines required TCP and/or UDP (or even Any) ports for Small World Deluxe to operate properly, that need to be enabled. The key signifies the port number in-question, while the value signifies the network protocol and whether the port is already enabled.
        };
    }
}

namespace Database {
    namespace Settings {
        namespace Language {
            enum GkLangSettings {
                UiLang,
                DictLang
            };

            enum GkUiLang {
                ChosenUiLang // NOTE! Google LevelDB database key is (without quotes): 'GkChosenUiLang'
            };

            enum GkDictionary {
                ChosenDictLang // NOTE! Google LevelDB database key is (without quotes): 'GkChosenDictLang'
            };
        }

        constexpr char dbName[] = "Database/DbName";
        constexpr char dbExt[] = "Database/DbExt";
        constexpr char dbLoc[] = "Database/DbLoc";

        enum init_cfg {                                 // The initial configuration that will be used by Small World Deluxe, through <QSettings> so that Google LevelDB maybe initialized.
            DbName,                                     // The name of the initial, Google LevelDB database.
            DbExt,                                      // The file extension (if any) of the Google LevelDB database.
            DbLoc                                       // The location of the Google LevelDB database.
        };

        enum GkEventLogCfg {
            GkLogVerbosity
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

        enum GkAudioCfg {
            settingsDbLoc,
            LogsDirLoc,
            AudioRecLoc,
            AudioInputChannels,
            AudioOutputChannels,
            AudioInputSampleRate,
            AudioOutputSampleRate,
            AudioInputBitrate,
            AudioOutputBitrate
        };

        enum GkAudioChannels {
            Mono,
            Left,
            Right,
            Both,
            Surround,
            Unknown
        };

        enum GkAudioSource {
            Input,
            Output,
            InputOutput
        };

        enum AudioPlaybackDlg {
            GkAudioDlgLastFolderBrowsed,
            GkRecordDlgLastFolderBrowsed
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

        enum GkXmppCfg {
            XmppAllowMsgHistory,
            XmppAllowFileXfers,
            XmppAllowMucs,
            XmppAutoConnect,
            XmppAutoReconnect,
            XmppAutoReconnectIgnore,
            XmppAvatarByteArray,
            XmppDomainUrl,
            XmppServerType,
            XmppDomainPort,
            XmppEnableSsl,
            XmppIgnoreSslErrors,
            XmppUsername,
            XmppJid,
            XmppPassword,
            XmppNickname,
            XmppEmailAddr,
            XmppUriLookupMethod,
            XmppNetworkTimeout,
            XmppCheckboxRememberCreds,
            XmppLastOnlinePresence
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

        struct GkUsbPort {
            QString name;                                                           // The actual name of the USB port as displayed by the operating system itself
            GkUsb2Exts usb_2;                                                       // Extensions for USB >2.0+ devices.
            GkUsb3Exts usb_3;                                                       // Extensions for USB >3.0+ devices.
            GkLibUsb lib_usb;                                                       // Additional information pertaining to USB devices, both old and new.
            quint16 pid;
            quint16 vid;
            quint8 addr;
            quint8 bus;
            quint8 port;                                                            // The list of all port numbers from root for the specified device.
        };

        struct GkComPort {
            QSerialPortInfo port_info;                                              // Details on the COM/RS232/Serial ports themselves
            uint32_t def_baudrate;                                                  // The defined baudrate for this serial port in question
            bool is_usb;
        };

        namespace Audio {
            struct GkDevice {
                QString audio_dev_str;                                              // The name of the device itself, as a formatted string.
                QString realm_str;                                                  // The API that this particular audio device belongs towards.
                QAudioDeviceInfo audio_device_info;                                 // Pointer to the actual audio device in question.
                bool user_config_succ;                                              // Whether this audio device information has been gathered as the result of user activity or default action by Small World Deluxe.
                bool default_output_dev;                                            // Is this the default device for the system?
                bool default_input_dev;                                             // Is this the default device for the system?
                bool is_enabled;                                                    // Whether this device (as the `QAudioInput` or `QAudioOutput`) is enabled as the primary choice by the end-user, for example, with regards to the spectrograph / waterfall.
                GkAudioSource audio_src;                                            // Is the audio device in question an input? Output if FALSE, UNSURE if either.
                QAudioFormat user_settings;                                         // The user defined settings for this particular audio device.
                quint32 chosen_sample_rate;                                         // The chosen sample rate, as configured by the end-user.
                GkAudioChannels sel_channels;                                       // The selected audio channel configuration.
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
            Rig *gkRig = new Rig(RIG_MODEL_DUMMY);  // Hamlib rig pointer
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
    enum GkFftState { Recording, Stopped };

    enum GkFftEventType {
        record,
        stop,
        loopback
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

    struct GkFFTSpectrum {
        double frequency;
        double magnitude;
    };

    struct GkTimingData {
        qint64 relative_start_time;                                             // The 'relative starting time' for when the spectrograph was initialized.
        qint64 relative_stop_time;                                              // The 'relative stopping time' for when the spectrograph was deinitialized.
        qint64 curr_time;                                                       // The more up-to-date time, as a UNIX epoch.
    };
}

namespace GkAudioFramework {
    struct GkRecord {
        float *buffer;
        size_t bufferBytes;
        size_t totalFrames;
        size_t frameCounter;
        quint32 channels;
    };

    struct GkRtCallback {
        quint32 nRate;                                                          // Sampling rate (i.e. sample/sec)
        quint32 nChannel;                                                       // Channel number
        quint32 nFrame;                                                         // Frame number of Wave Table
        quint32 cur;                                                            // Current index of Wave Table (in Frame)
        float *wfTable;                                                         // Wave Table (interleaved)
    };

    enum GkAudioState { Playing, Stopped };

    enum CodecSupport {
        PCM,
        Loopback,
        OggVorbis,
        Opus,
        FLAC,
        Unsupported,
        Unknown
    };

    enum AudioEventType {
        start,
        stop,
        record,
        loopback
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
        QString track_title;                                                    // The title of the audio track (i.e. metadata) within the audio file itself, if there is such information present.
        QString file_size_hr;                                                   // The human-readable form of the file-size parameter.
        double sample_rate;                                                     // The sample rate of the file.
        double length_in_secs;                                                  // Length of the audio file within seconds as a time measurement.
        CodecSupport type_codec;                                                // The codec of the audio file, if known.
        Database::Settings::GkAudioChannels num_audio_channels;                 // The number of audio channels (i.e. if stereo or mono).
        qint64 file_size;                                                       // The storage size of the audio/media file itself.
        qint64 bitrate_lower;                                                   // The lower end of the bitrate scale for the specified file.
        qint64 bitrate_upper;                                                   // The upper end of the bitrate scale for the specified file.
        qint64 bitrate_nominal;                                                 // The nominal bitrate for the specified file.
        qint32 bit_depth;                                                       // Self-explanatory.
        qint32 num_samples_per_channel;                                         // The number of samples per each channel.
    };
}
};
