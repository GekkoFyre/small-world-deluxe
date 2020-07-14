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

#include "src/gk_display_image.hpp"

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

/**
 * @brief GkDisplayImage::GkDisplayImage
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkDisplayImage::GkDisplayImage(QWidget *parent)
{
    setParent(parent);

    return;
}

GkDisplayImage::~GkDisplayImage()
{
    return;
}

/**
 * @brief GkDisplayImage::heightForWidth
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param width
 * @return
 */
int GkDisplayImage::heightForWidth(int width) const
{
    return pixmap.isNull() ? this->height() : ((qreal)pixmap.height() * width) / pixmap.width();
}

/**
 * @brief GkDisplayImage::sizeHint
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QSize GkDisplayImage::sizeHint() const
{
    int w = this->width();
    return QSize(w, heightForWidth(w));
}

/**
 * @brief GkDisplayImage::scaledPixmap
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QPixmap GkDisplayImage::scaledPixmap() const
{
    auto scaled = pixmap.scaled(this->size() * devicePixelRatioF(), Qt::KeepAspectRatio, Qt::SmoothTransformation); scaled.setDevicePixelRatio(devicePixelRatioF());
    return scaled;
}

/**
 * @brief GkDisplayImage::setPixmap
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param p
 */
void GkDisplayImage::setPixmap(const QPixmap &p)
{
    pixmap = p;
    QLabel::setPixmap(scaledPixmap());

    return;
}

/**
 * @brief GkDisplayImage::resizeEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param e
 */
void GkDisplayImage::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);

    if(!pixmap.isNull()) {
        QLabel::setPixmap(scaledPixmap());
    }

    return;
}
