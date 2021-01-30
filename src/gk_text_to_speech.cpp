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

#include "src/gk_text_to_speech.hpp"
#include <utility>
#include <QTextToSpeechEngine>
#include <QLocale>

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

GkTextToSpeech::GkTextToSpeech(QPointer<GekkoFyre::GkLevelDb> dbPtr, QPointer<GekkoFyre::GkEventLogger> eventLogger,
                               QObject *parent) : QTextToSpeech(parent)
{
    setParent(parent);
    gkDb = std::move(dbPtr);
    gkEventLogger = std::move(eventLogger);

    m_speech = new QTextToSpeech(this);
    engineName.clear();
    engineSelected(0);
}

GkTextToSpeech::~GkTextToSpeech()
{}

void GkTextToSpeech::speak()
{
    m_speech->say(tr("Thank you for using version %1 of %2, we hope that you are liking this software application so far. Please expect further updates in the near future.")
    .arg(General::appVersion).arg(General::productName));

    return;
}

void GkTextToSpeech::stop()
{
    m_speech->stop();

    return;
}

void GkTextToSpeech::setRate(const int &rate)
{
    m_speech->setRate(rate / 10.0);

    return;
}

void GkTextToSpeech::setPitch(const int &pitch)
{
    m_speech->setPitch(pitch / 10.0);

    return;
}

void GkTextToSpeech::setVolume(const int &volume)
{
    m_speech->setVolume(volume / 100.0);

    return;
}

void GkTextToSpeech::stateChanged(const QTextToSpeech::State &state)
{
    if (state == QTextToSpeech::Speaking) {
        gkEventLogger->publishEvent(tr("Text-to-speech engine has been started..."), GkSeverity::Info, "", true, true, true);
    } else if (state == QTextToSpeech::Ready) {
        gkEventLogger->publishEvent(tr("Text-to-speech engine has been terminated..."), GkSeverity::Info, "", true, true, true);
    } else if (state == QTextToSpeech::Paused) {
        gkEventLogger->publishEvent(tr("Text-to-speech engine has been paused..."), GkSeverity::Info, "", false, true, true);
    } else {
        gkEventLogger->publishEvent(tr("An error has been encountered with the text-to-speech engine!"), GkSeverity::Error, "", true, true, true);
    }

    return;
}

void GkTextToSpeech::engineSelected(int index)
{
    if (engineName == "Default") {
        m_speech.clear();
        m_speech = new QTextToSpeech(this);
    } else {
        m_speech = new QTextToSpeech(engineName, this);
    }

    emit clearLangItems();

    // Populate the languages combobox before connecting its signal.
    const QVector<QLocale> locales = m_speech->availableLocales();
    QLocale current = m_speech->locale();
    for (const QLocale &locale : locales) {
        QString name(QString("%1 (%2)")
        .arg(QLocale::languageToString(locale.language()))
        .arg(QLocale::countryToString(locale.country())));

        QVariant localeVariant(locale);
        emit addLangItem(name, localeVariant);
        if (locale.name() == current.name()) {
            current = locale;
        }
    }

    // QObject::connect(m_speech, SIGNAL(stateChanged(const State &)), this, SLOT(stateChanged(const State &)));
    QObject::connect(m_speech, SIGNAL(localeChanged(const QLocale &)), this, SLOT(localeChanged(const QLocale &)));

    localeChanged(current);
    return;
}

void GkTextToSpeech::engineSelected(const QString &name)
{
    if (!name.isNull() && !name.isEmpty()) {
        engineName = name;
    }

    return;
}

/**
 * @brief GkTextToSpeech::languageSelected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param language
 */
void GkTextToSpeech::languageSelected(const QLocale &language)
{
    m_speech->setLocale(language);

    return;
}

/**
 * @brief GkTextToSpeech::languageSelected
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param lang_idx
 * @see `ui->comboBox_access_stt_language` under class, `DialogSettings`.
 */
void GkTextToSpeech::languageSelected(const int &lang_idx)
{
    //
    // TODO: Finish this section ASAP!
    //

    return;
}

void GkTextToSpeech::voiceSelected(const int &idx)
{
    if (idx >= 0) {
        m_speech->setVoice(m_voices.at(idx));
    }

    return;
}

void GkTextToSpeech::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    emit clearVoiceItems();

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    qint32 counter = 0;
    for (const QVoice &voice : qAsConst(m_voices)) {
        emit addVoiceItem(QString("%1 - %2 - %3").arg(voice.name())
                                  .arg(QVoice::genderName(voice.gender()))
                                  .arg(QVoice::ageName(voice.age())), "");
        if (voice.name() == currentVoice.name()) {
            emit setVoiceCurrentIndex(counter - 1);
        }

        ++counter;
    }

    return;
}
