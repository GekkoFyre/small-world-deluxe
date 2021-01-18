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

#include "src/ui/widgets/gk_submit_msg.hpp"
#include <QVariant>

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
 * @brief GkPlainTextSubmit::GkPlainTextSubmit
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkPlainTextSubmit::GkPlainTextSubmit(QWidget *parent)
{
    setParent(parent);

    return;
}

GkPlainTextSubmit::~GkPlainTextSubmit()
{
    return;
}

/**
 * @brief GkPlainTextSubmit::keyPressEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkPlainTextSubmit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // The ENTER key has been pressed!
        QVariant curr_value = toPlainText();
        clear();
        emit execFuncAfterEvent(curr_value.toString()); // Execute desired functions now!
    } else {
        // Some other key has been pressed, process as normal...
        QPlainTextEdit::keyPressEvent(event);
    }

    return;
}

/**
 * @brief GkComboBoxSubmit::GkComboBoxSubmit
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkComboBoxSubmit::GkComboBoxSubmit(QWidget *parent)
{
    setParent(parent);

    return;
}

GkComboBoxSubmit::~GkComboBoxSubmit()
{
    return;
}

/**
 * @brief GkComboBoxSubmit::keyPressEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkComboBoxSubmit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // The ENTER key has been pressed!
        QVariant curr_val = currentText();
        emit execFuncAfterEvent(curr_val.toULongLong()); // Execute desired functions now!
    } else {
        // Some other key has been pressed, process as normal...
        QComboBox::keyPressEvent(event);
    }

    return;
}
