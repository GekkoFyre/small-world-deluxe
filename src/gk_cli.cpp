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

#include "gk_cli.hpp"
#include <boost/exception/all.hpp>
#include <vector>
#include <iostream>
#include <ostream>
#include <QMessageBox>
#include <QStringList>
#include <QCoreApplication>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace System;
using namespace Cli;

namespace fs = boost::filesystem;
namespace sys = boost::system;

GkCli::GkCli(std::shared_ptr<QCommandLineParser> parser, std::shared_ptr<FileIo> fileIo,
             std::shared_ptr<GkLevelDb> database, std::shared_ptr<RadioLibs> radioLibs,
             QObject *parent)
{
    gkCliParser = parser;
    gkFileIo = fileIo;
    gkDb = database;
    gkRadioLibs = radioLibs;
}

GkCli::~GkCli()
{}

/**
 * @brief GkCli::parseCommandLine
 * @param error_msg
 * @return
 * @note <https://doc.qt.io/qt-5/qcommandlineparser.html#how-to-use-qcommandlineparser-in-complex-applications>
 */
CommandLineParseResult GkCli::parseCommandLine(QString *error_msg)
{
    try {
        gkCliParser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);

        const QCommandLineOption helpOption = gkCliParser->addHelpOption();
        const QCommandLineOption versionOption = gkCliParser->addVersionOption();

        gkCliParser->setApplicationDescription(tr("%1 is a 'new age' weak-signal digital communicator "
                                                  "powered by low bit rate, digital voice codecs originally meant for "
                                                  "telephony. Typical usage requires a SSB radio transceiver and a "
                                                  "personal computer with a capable sound-card.").arg(General::productName));

        if (!gkCliParser->parse(QCoreApplication::arguments())) {
            *error_msg = gkCliParser->errorText();
            return CommandLineError;
        }

        if (gkCliParser->isSet(versionOption)) {
            return CommandLineVersionRequested;
        }

        if (gkCliParser->isSet(helpOption)) {
            return CommandLineHelpRequested;
        }

        const QStringList pos_args = gkCliParser->positionalArguments();
        if (pos_args.isEmpty()) {
            *error_msg = tr("Argument 'name' missing.");
            return CommandLineError;
        }

        if (pos_args.size() > 1) {
            *error_msg = tr("Several 'name' arguments specified.");
            return CommandLineError;
        }

        return CommandLineOk;
    } catch (const std::exception &e) {
        std::cerr << error_msg->toStdString() << std::endl;
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occurred!\n\n%1").arg(*error_msg), QMessageBox::Ok);
    }

    return CommandLineError;
}