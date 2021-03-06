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

#pragma once

#include "src/defines.hpp"
#include <QObject>
#include <QWidget>
#include <QString>
#include <QPixmap>
#include <QPointer>
#include <QPainter>
#include <QApplication>
#include <QSplashScreen>

namespace GekkoFyre {

class GkSplashScreen : public QWidget {
    Q_OBJECT

public:
    explicit GkSplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
    GkSplashScreen(QWidget *parent, const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~GkSplashScreen();

    void finish(QWidget *mainWin);
    void repaint();
    QString message() const;

    QPixmap pixmap;
    QString currStatus;
    QColor currColor;
    int currAlign;
    void setPixmap(const QPixmap &p, const QScreen *screen = nullptr);
    static const QScreen *screenFor(const QWidget *w);

public slots:
    void showMessage(const QString &message, int alignment = Qt::AlignLeft, const QColor &color = Qt::black);
    void clearMessage();

signals:
    void messageChanged(const QString &message);

protected:
    bool event(QEvent *e) override;
    virtual void drawContents(QPainter *painter);
    void mousePressEvent(QMouseEvent *) override;

private:
    Q_DISABLE_COPY(GkSplashScreen);

};

class GkSplashDispModel : public GkSplashScreen {
    Q_OBJECT

public:
    explicit GkSplashDispModel(QApplication *aApp, QWidget *parent = nullptr, const bool &drawProgressBar = false);
    ~GkSplashDispModel() override;

    QApplication *app;

public slots:
    void setProgress(const qint32 &value);

protected:
    void drawContents(QPainter *painter) Q_DECL_OVERRIDE;

private:
    qint32 m_progress;
    bool m_drawProgressBar;

};
};
