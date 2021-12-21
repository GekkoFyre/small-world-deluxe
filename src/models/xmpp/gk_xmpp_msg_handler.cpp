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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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

#include "src/models/xmpp/gk_xmpp_msg_handler.hpp"
#include <utility>
#include <iostream>
#include <exception>
#include <QPainter>
#include <QTextDocument>

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
 * @brief GkXmppMsgEngine::GkXmppMsgEngine
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param err_msg
 * @param err
 */
GkXmppMsgEngine::GkXmppMsgEngine(QObject *parent) : QAbstractItemDelegate(parent)
{
    return;
}

GkXmppMsgEngine::~GkXmppMsgEngine() {}

/**
 * @brief GkXmppMsgEngine::paint
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param painter
 * @param option
 * @param index
 */
void GkXmppMsgEngine::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTextDocument doc;
    doc.setTextWidth(option.rect.width());
    doc.setMarkdown(index.data().toString());
    painter->save();
    painter->translate(option.rect.topLeft());
    doc.drawContents(painter,QRect(QPoint(0,0),option.rect.size()));
    painter->restore();

    return;
}

/**
 * @brief GkXmppMsgEngine::sizeHint
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param option
 * @param index
 * @return
 */
QSize GkXmppMsgEngine::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize();
}
