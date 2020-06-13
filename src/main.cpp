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

#include "src/defines.hpp"
#include "src/ui/mainwindow.hpp"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/locale/generator.hpp>
#include <singleapplication.h>
#include <exception>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <locale>
#include <clocale>
#include <QApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QString>
#include <QLocale>
#include <QWidget>
#include <QResource>

#ifdef __MINGW32__
#include <SDL2/SDL.h>
#endif

namespace fs = boost::filesystem;

int main(int argc, char *argv[])
{
    try {
        #ifdef _WIN32
        // We wish to enforce the encoding on Microsoft Windows (typically UTF-8)
        std::locale::global(boost::locale::generator().generate("en_US.UTF-8"));
        #else
        // This suffices for Linux, Apple OS/X, etc.
        std::locale::global(std::locale::classic());
        #endif

        //
        // https://doc.qt.io/qt-5/qlocale.html
        // Sets the default locale for Qt5 and its libraries
        //
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

        std::cout << "Setting the locale has succeeded." << std::endl;
    } catch (const std::exception &e) {
        std::cout << QString("Setting the locale has failed!\n\n%1").arg(QString::fromStdString(e.what())).toStdString() << std::endl;
        return -1;
    }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    SingleApplication app(argc, argv, false, SingleApplication::Mode::System);
    if (app.isSecondary()) {
        app.exit();
    }

    QCoreApplication::setOrganizationName(GekkoFyre::General::companyName);
    QCoreApplication::setOrganizationDomain(GekkoFyre::General::codeRepository);
    QCoreApplication::setApplicationName(GekkoFyre::General::productName);
    QCoreApplication::setApplicationVersion(GekkoFyre::General::appVersion);

    fs::path slash = "/";
    fs::path native_slash = slash.make_preferred().native();
    fs::path resource_path = fs::path(QCoreApplication::applicationDirPath().toStdString() + native_slash.string()  + GekkoFyre::Filesystem::resourceFile);
    QResource::registerResource(QString::fromStdString(resource_path.string())); // https://doc.qt.io/qt-5/resources.html

    #ifdef __MINGW32__
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Failed to initialize the SDL2 library!" << std::endl;
        return -1;
    }
    #endif

    //
    // Display a splash screen!
    //
    QPixmap pixmap(":/resources/contrib/images/vector/gekkofyre-networks/rionquosue/logo_blank_border_text_square_rionquosue.svg");
    int width = pixmap.width();
    int height = pixmap.height();
    QSplashScreen splash(pixmap.scaled((width / 2), (height / 2), Qt::KeepAspectRatio), Qt::WindowStaysOnTopHint);
    splash.show();
    QTimer::singleShot(3000, &splash, &QWidget::close);

    MainWindow w;
    w.show();

    return app.exec();
}
