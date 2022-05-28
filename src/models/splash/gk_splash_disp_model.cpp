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
#include <QScreen>
#include <QWindow>
#include <QTextCursor>
#include <QApplication>
#include <QStyleOption>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QDesktopWidget>

using namespace GekkoFyre;
using namespace System;

/**
 * @brief GkSplashScreen::
 * @author Qt5 source code <https://www.qt.io/>.
 * @param pixmap
 * @param f
 */
GkSplashScreen::GkSplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) : QWidget(0, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    setPixmap(pixmap);

    return;
}

/**
 * @brief GkSplashScreen::GkSplashScreen copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param parent
 * @param pixmap
 * @param f
 */
GkSplashScreen::GkSplashScreen(QWidget *parent, const QPixmap &pixmap, Qt::WindowFlags f) : QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    setPixmap(pixmap, GkSplashScreen::screenFor(parent));
    return;
}

/**
 * @brief GkSplashScreen::~GkSplashScreen copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 */
GkSplashScreen::~GkSplashScreen()
{
    return;
}

/**
 * @brief GkSplashScreen::setPixmap copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param pixmap
 */
void GkSplashScreen::setPixmap(const QPixmap &p, const QScreen *screen)
{
    pixmap = p;
    setAttribute(Qt::WA_TranslucentBackground, pixmap.hasAlpha());
    QRect r(QPoint(), pixmap.size() / pixmap.devicePixelRatio());
    resize(r.size());
    if (screen) {
        move(screen->geometry().center() - r.center());
    }

    if (isVisible()) {
        repaint();
    }

    return;
}

/**
 * @brief @brief GkSplashScreen::screenNumberOf copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param dsw
 * @return
 */
static inline int screenNumberOf(const QDesktopWidget *dsw)
{
    auto desktopWidget = static_cast<QDesktopWidget *>(QApplication::desktop());
    return desktopWidget->screenNumber();
}

/**
 * @brief GkSplashScreen::screenFor copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param w
 * @return
 */
const QScreen *GkSplashScreen::screenFor(const QWidget *w)
{
    for (const QWidget *p = w; p != nullptr; p = p->parentWidget()) {
        if (auto dsw = qobject_cast<const QDesktopWidget *>(p)) {
            return QGuiApplication::screens().value(screenNumberOf(dsw));
        }

        if (QWindow *window = p->windowHandle()) {
            return window->screen();
        }
    }

    #if QT_CONFIG(cursor)
    // Note: We could rely on QPlatformWindow::initialGeometry() to center it
    // over the cursor, but not all platforms (namely Android) use that.
    if (QGuiApplication::screens().size() > 1) {
        if (auto screenAtCursor = QGuiApplication::screenAt(QCursor::pos())) {
            return screenAtCursor;
        }
    }
    #endif // cursor

    return QGuiApplication::primaryScreen();
}

/**
 * @brief GkSplashScreen::waitForWindowExposed copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param window
 * @param timeout
 * @return
 */
inline static bool waitForWindowExposed(QWindow *window, int timeout = 1000)
{
    enum { TimeOutMs = 10 };
    QElapsedTimer timer;
    timer.start();
    while (!window->isExposed()) {
        const int remaining = timeout - int(timer.elapsed());
        if (remaining <= 0) {
            break;
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        #if defined(Q_OS_WINRT)
        WaitForSingleObjectEx(GetCurrentThread(), TimeOutMs, false);
        #elif defined(Q_OS_WIN)
        Sleep(uint(TimeOutMs));
        #else
        struct timespec ts = { TimeOutMs / 1000, (TimeOutMs % 1000) * 1000 * 1000 };
        nanosleep(&ts, NULL);
        #endif
    }

    return window->isExposed();
}

/**
 * @brief GkSplashScreen::finish copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param w
 */
void GkSplashScreen::finish(QWidget *mainWin)
{
    if (mainWin) {
        if (!mainWin->windowHandle())
            mainWin->createWinId();
        waitForWindowExposed(mainWin->windowHandle());
    }

    close();
    return;
}

/**
 * @brief GkSplashScreen::repaint copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 */
void GkSplashScreen::repaint()
{
    QWidget::repaint();
    QCoreApplication::processEvents();

    return;
}

/**
 * @brief GkSplashScreen::message copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @return
 */
QString GkSplashScreen::message() const
{
    return currStatus;
}

/**
 * @brief GkSplashScreen::showMessage copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param message
 * @param alignment
 * @param color
 */
void GkSplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    currStatus = message;
    currAlign = alignment;
    currColor = color;
    emit messageChanged(currStatus);
    repaint();

    return;
}

/**
 * @brief GkSplashScreen::clearMessage copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 */
void GkSplashScreen::clearMessage()
{
    currStatus.clear();
    emit messageChanged(currStatus);

    return;
}

/**
 * @brief GkSplashScreen::event copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param e
 * @return
 */
bool GkSplashScreen::event(QEvent *e)
{
    if (e->type() == QEvent::Paint) {
        QPainter painter(this);
        painter.setLayoutDirection(layoutDirection());
        if (!pixmap.isNull()) {
            painter.drawPixmap(QPoint(), pixmap);
        }

        drawContents(&painter);
    }

    return QWidget::event(e);
}

/**
 * @brief GkSplashScreen::drawContents copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 * @param painter
 */
void GkSplashScreen::drawContents(QPainter *painter)
{
    painter->setPen(currColor);
    QRect r = rect().adjusted(5, 5, -5, -5);
    if (Qt::mightBeRichText(currStatus)) {
        QTextDocument doc;
        #ifdef QT_NO_TEXTHTMLPARSER
        doc.setPlainText(d->currStatus);
        #else
        doc.setHtml(currStatus);
        #endif
        doc.setTextWidth(r.width());
        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::Alignment(currAlign));
        fmt.setLayoutDirection(layoutDirection());
        cursor.mergeBlockFormat(fmt);
        const QSizeF txtSize = doc.size();
        if (currAlign & Qt::AlignBottom) {
            r.setTop(r.height() - txtSize.height());
        } else if (currAlign & Qt::AlignVCenter) {
            r.setTop(r.height() / 2 - txtSize.height() / 2);
        }

        painter->save();
        painter->translate(r.topLeft());
        doc.drawContents(painter);
        painter->restore();
    } else {
        painter->drawText(r, currAlign, currStatus);
    }

    return;
}

/**
 * @brief GkSplashScreen::mousePressEvent copied from the Qt5 source code since it is now deprecated in Qt6.
 * @author Qt5 source code <https://www.qt.io/>.
 */
void GkSplashScreen::mousePressEvent(QMouseEvent *)
{
    hide();
    return;
}

GkSplashDispModel::GkSplashDispModel(QApplication *aApp, QWidget *parent, const bool &drawProgressBar) : GkSplashScreen(parent), app(aApp), m_progress(0),
                                                                                                         m_drawProgressBar(drawProgressBar)
{
    QPixmap pixmap(":/resources/contrib/images/vector/gekkofyre-networks/rionquosue/logo_blank_border_text_square_rionquosue.svg");
    qint32 width = pixmap.width();
    qint32 height = pixmap.height();
    setPixmap(pixmap.scaled((width / 2), (height / 2), Qt::KeepAspectRatio));
    setCursor(Qt::BusyCursor);

    //
    // TODO: Fix both the colour of how this should appear, along with the location of where it appears too!
    // showMessage(tr(General::splashDispWelcomeMsg), Qt::AlignBottom | Qt::AlignCenter); // Display a small message along the bottom of the QSplashScreen!

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
    if (m_drawProgressBar) {
        m_progress = value;
        if (m_progress > 100) {
            m_progress = 100;
        }

        if (m_progress < 0) {
            m_progress = 0;
        }

        update();
    }

    return;
}

/**
 * @brief GkSplashDispModel::drawContents
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param painter
 */
void GkSplashDispModel::drawContents(QPainter *painter)
{
    if (m_drawProgressBar) {
        GkSplashScreen::drawContents(painter);

        // Set a style for the QProgressBar...
        QStyleOptionProgressBar pbStyle;
        pbStyle.initFrom(this);
        pbStyle.state = QStyle::State_Enabled;
        pbStyle.textVisible = false;
        pbStyle.minimum = 0;
        pbStyle.maximum = 100;
        pbStyle.progress = m_progress;
        pbStyle.invertedAppearance = false;

        //
        // The location and size of where/how the QProgressBar should appear!
        pbStyle.rect = QRect((static_cast<qint32>(width() * GK_SPLASH_SCREEN_START_SHOW_LEFT)), static_cast<qint32>((height() * GK_SPLASH_SCREEN_START_SHOW_TOP)),
                             static_cast<qint32>((width() * GK_SPLASH_SCREEN_START_SIZE_WIDTH)), static_cast<qint32>((height() * GK_SPLASH_SCREEN_START_SIZE_HEIGHT)));

        // Now we draw the object in question!
        style()->drawControl(QStyle::CE_ProgressBar, &pbStyle, painter, this);
    }

    return;
}
