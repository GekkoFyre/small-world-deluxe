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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/splash/gk_splash_disp_model.hpp"
#include <utility>
#include <QPixmap>
#include <QStyleOption>

using namespace GekkoFyre;
using namespace System;

GkSplashDispModel::GkSplashDispModel(QApplication *aApp, QWidget *parent) : QSplashScreen(parent), app(aApp), m_progress(0)
{
    QPixmap pixmap(":/resources/contrib/images/vector/gekkofyre-networks/rionquosue/logo_blank_border_text_square_rionquosue.svg");
    qint32 width = pixmap.width();
    qint32 height = pixmap.height();
    setPixmap(pixmap.scaled((width / 2), (height / 2), Qt::KeepAspectRatio));
    setCursor(Qt::BusyCursor);

    // showMessage(General::companyName, Qt::AlignBottom); // Display a small message along the botom of the splash-screen!
    return;
}

GkSplashDispModel::~GkSplashDispModel()
{
    return;
}

/**
 * @brief GkSplashDispModel::setProgress
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkSplashDispModel::setProgress(const qint32 &value)
{
    m_progress = value;
    if (m_progress > 100) {
        m_progress = 100;
    }

    if (m_progress < 0) {
        m_progress = 0;
    }

    update();
    return;
}

/**
 * @brief GkSplashDispModel::drawContents
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param painter
 */
void GkSplashDispModel::drawContents(QPainter *painter)
{
    QSplashScreen::drawContents(painter);

    // Set a style for the QProgressBar...
    QStyleOptionProgressBar pbStyle;
    pbStyle.initFrom(this);
    pbStyle.state = QStyle::State_Enabled;
    pbStyle.textVisible = false;
    pbStyle.minimum = 0;
    pbStyle.maximum = 100;
    pbStyle.progress = m_progress;
    pbStyle.invertedAppearance = false;
    pbStyle.rect = QRect(0, 265, 380, 19); // Where is it.

    // Now we draw the object in question!
    style()->drawControl(QStyle::CE_ProgressBar, &pbStyle, painter, this);
}
