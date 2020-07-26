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

#pragma once

#include "src/defines.hpp"
#include "src/gk_logger.hpp"
#include <QMouseEvent>
#include <QPointer>
#include <QObject>
#include <QWidget>
#include <QString>
#include <QPixmap>
#include <QLabel>
#include <QSize>

namespace GekkoFyre {

class GkDisplayImage : public QLabel {
    Q_OBJECT

public:
    explicit GkDisplayImage(const GekkoFyre::AmateurRadio::Gui::sstvWindow &sstv_win, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                            QWidget *parent = nullptr);
    ~GkDisplayImage();

    virtual int heightForWidth(int width) const;
    virtual QSize sizeHint() const;
    virtual QPixmap pixmap() const;
    QPixmap scaledPixmap() const;

protected:
    virtual void mouseReleaseEvent(QMouseEvent *e);

public slots:
    void setPixmap(const QPixmap &p);
    void resizeEvent(QResizeEvent *e);

private slots:
    void txImage();
    void loadImage();
    void copyToClipboard();
    void saveImage();
    void clearImages();
    void delImage();

private:
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;
    QPixmap pixmapMem;
    GekkoFyre::AmateurRadio::Gui::sstvWindow sstvWindow;

    QString sstvResource;

};
};
