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

#include "src/models/spelling/gk_text_edit_spelling_highlight.hpp"
#include <utility>
#include <iostream>
#include <exception>
#include <QMenu>

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
 * @brief GkTextEditSpellHighlight::GkTextEditSpellHighlight
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param QObject
 */
GkTextEditSpellHighlight::GkTextEditSpellHighlight(QPointer<GekkoFyre::GkEventLogger> eventLogger,
                                                   QObject *parent) : QTextEdit()
{
    gkEventLogger = std::move(eventLogger);

    //
    // Initialize spelling and grammar checker, dictionaries, etc.
    m_spellCheckerHighlighter = new Sonnet::Highlighter(this);
    m_spellCheckerHighlighter->setCurrentLanguage(Filesystem::enchantSpellDefLang);

    return;
}

GkTextEditSpellHighlight::~GkTextEditSpellHighlight() {}

/**
 * @brief GkTextEditSpellHighlight::slotActivate
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkTextEditSpellHighlight::slotActivate()
{
    m_spellCheckerHighlighter->setActive(!m_spellCheckerHighlighter->isActive());
    if (m_spellCheckerHighlighter->isActive()) {
        gkEventLogger->publishEvent(tr("Spell-check highlighting has been enabled with dictionary, \"%1\"!")
        .arg(m_spellCheckerHighlighter->currentLanguage()), GkSeverity::Info, "",
        false, true, true, false, true);

        return;
    }

    gkEventLogger->publishEvent(tr("Spell-check highlighting has been disabled!"), GkSeverity::Info, "",
                                false, true, true, false, true);

    return;
}

/**
 * @brief GkTextEditSpellHighlight::contextMenuEvent
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param event
 */
void GkTextEditSpellHighlight::contextMenuEvent(QContextMenuEvent *event)
{
    m_spellCheckerHighlighter->setActive(true);
    QPointer<QMenu> spellingPopup = createStandardContextMenu();
    QPointer<QMenu> spellingSubMenu = new QMenu(spellingPopup);
    spellingSubMenu->setTitle(tr("Spell-check highlighting"));
    QObject::connect(spellingSubMenu, SIGNAL(triggered(QAction *)), this, SLOT(slotActivate()));
    QPointer<QAction> spellingAction = new QAction(tr("Enabled"), spellingPopup);
    spellingPopup->addSeparator();
    spellingPopup->addMenu(spellingSubMenu);
    spellingSubMenu->addAction(spellingAction);
    spellingPopup->exec(event->globalPos());
    delete spellingPopup;

    return;
}
