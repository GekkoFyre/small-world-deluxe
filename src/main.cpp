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

#include "src/defines.hpp"
#include "src/ui/mainwindow.hpp"
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/locale/generator.hpp>
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
#include <QWidget>
#include <QResource>

namespace fs = boost::filesystem;

int main(int argc, char *argv[])
{
    try {
        #ifdef _WIN32
        // We wish to enforce the encoding on Microsoft Windows (typically UTF-8)
        std::locale::global(boost::locale::generator().generate("C"));
        #else
        // This suffices for Linux, Apple OS/X, etc.
        std::locale::global(std::locale::classic());
        #endif
        std::cout << "Setting the locale has succeeded." << std::endl;
    } catch (const std::exception &e) {
        std::cout << QString("Setting the locale has failed!\n\n%1").arg(QString::fromStdString(e.what())).toStdString() << std::endl;
    }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName(GekkoFyre::General::companyName);
    QCoreApplication::setOrganizationDomain(GekkoFyre::General::codeRepository);
    QCoreApplication::setApplicationName(GekkoFyre::General::productName);
    QCoreApplication::setApplicationVersion(GekkoFyre::General::appVersion);

    fs::path slash = "/";
    fs::path native_slash = slash.make_preferred().native();
    fs::path resource_path = fs::path(QCoreApplication::applicationDirPath().toStdString() + native_slash.string()  + GekkoFyre::Filesystem::resourceFile);
    QResource::registerResource(QString::fromStdString(resource_path.string())); // https://doc.qt.io/qt-5/resources.html

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

    return a.exec();
}
