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

#include "src/ui/widgets/gk_display_image.hpp"
#include <utility>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QMessageBox>
#include <QApplication>

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
GkDisplayImage::GkDisplayImage(const Gui::sstvWindow &sstv_win, QPointer<GkEventLogger> eventLogger, QWidget *parent)
{
    setParent(parent);
    setAlignment(Qt::AlignCenter);

    gkEventLogger = std::move(eventLogger);

    sstvWindow = sstv_win;
    if (sstvWindow != Gui::sstvWindow::None) {
        QPointer<QAction> pAction1 = new QAction(tr("Transmit (TX)"), this);
        QPointer<QAction> pAction2 = new QAction(tr("Load"), this);
        QPointer<QAction> pAction3 = new QAction(tr("Copy to Clipboard"), this);
        QPointer<QAction> pAction4 = new QAction(tr("Save As"), this);
        QPointer<QAction> pAction5 = new QAction(tr("Clear"), this);
        QPointer<QAction> pAction6 = new QAction(tr("Delete"), this);

        this->addAction(pAction1);
        this->addAction(pAction2);
        this->addAction(pAction3);
        this->addAction(pAction4);
        this->addAction(pAction5);
        this->addAction(pAction6);

        QObject::connect(pAction1, SIGNAL(triggered()), this, SLOT(txImage()));
        QObject::connect(pAction2, SIGNAL(triggered()), this, SLOT(loadImage()));
        QObject::connect(pAction3, SIGNAL(triggered()), this, SLOT(copyToClipboard()));
        QObject::connect(pAction4, SIGNAL(triggered()), this, SLOT(saveImage()));
        QObject::connect(pAction5, SIGNAL(triggered()), this, SLOT(clearImages()));
        QObject::connect(pAction6, SIGNAL(triggered()), this, SLOT(delImage()));
    }

    switch (sstvWindow) {
    case Gui::sstvWindow::rxLiveImage:
        sstvResource = tr("SSTV (RX) Live Image");
        break;
    case Gui::sstvWindow::rxSavedImage:
        sstvResource = tr("SSTV (RX) Saved Image");
        break;
    case Gui::sstvWindow::txSendImage:
        sstvResource = tr("SSTV (TX)");
        break;
    default:
        sstvResource = tr("SSTV (TX)");
        break;
    }

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
    return pixmapMem.isNull() ? this->height() : ((qreal)pixmapMem.height() * width) / pixmapMem.width();
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

QPixmap GkDisplayImage::pixmap() const
{
    return pixmapMem;
}

/**
 * @brief GkDisplayImage::scaledPixmap
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return
 */
QPixmap GkDisplayImage::scaledPixmap() const
{
    auto scaled = pixmapMem.scaled(this->size() * devicePixelRatioF(), Qt::KeepAspectRatio, Qt::SmoothTransformation); scaled.setDevicePixelRatio(devicePixelRatioF());
    return scaled;
}

void GkDisplayImage::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        QMenu menu(this);
        menu.addActions(this->actions());
        menu.exec(e->globalPos());
    }

    return;
}

/**
 * @brief GkDisplayImage::setPixmap
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param p
 */
void GkDisplayImage::setPixmap(const QPixmap &p)
{
    pixmapMem = p;
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

    if(!pixmapMem.isNull()) {
        QLabel::setPixmap(scaledPixmap());
    }

    return;
}

/**
 * @brief GkDisplayImage::txImage
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::txImage()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkDisplayImage::loadImage
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::loadImage()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkDisplayImage::copyToClipboard
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::copyToClipboard()
{
    QApplication::clipboard()->setPixmap(pixmapMem);
    gkEventLogger->publishEvent(tr("Image from %1 copied to the operating system's clipboard.").arg(sstvResource), GkSeverity::Info);

    return;
}

/**
 * @brief GkDisplayImage::saveImage
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::saveImage()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkDisplayImage::clearImages
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::clearImages()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief GkDisplayImage::delImage
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkDisplayImage::delImage()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}
