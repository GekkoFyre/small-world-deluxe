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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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

#include "src/gk_app_vers.hpp"
#include <singleapplication.h>
#include <QLocale>
#include <QString>
#include <QTranslator>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //
    // Setup the translation settings!
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale: uiLanguages) {
        const QString baseName = "smallworld_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            #ifndef GK_ENBL_VALGRIND_SUPPORT
            SingleApplication::instance()->installTranslator(&translator);
            #else
            QApplication::instance()->installTranslator(&qtTranslator);
            #endif
            break;
        }
    }

    #ifndef GK_ENBL_VALGRIND_SUPPORT
    //
    // This, unintentionally, prevents the launching of Valgrind, Dr. Memory, etc. and other such
    // diagnostic tools...
    SingleApplication app(argc, argv, false, SingleApplication::Mode::System);
    if (app.isSecondary()) {
        app.exit();
    }

    #else
    QApplication app(argc, argv);
    #endif

    QCoreApplication::setOrganizationName("GekkoFyre Networks");
    QCoreApplication::setOrganizationDomain("https://code.gekkofyre.io/amateur-radio/small-world-deluxe");
    QCoreApplication::setApplicationName("Small World Deluxe");
    QCoreApplication::setApplicationVersion(GekkoFyre::General::appVersion);

    return a.exec();
}
