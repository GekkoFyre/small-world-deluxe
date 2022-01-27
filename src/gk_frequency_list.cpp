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

#include "src/gk_frequency_list.hpp"
#include <cmath>
#include <cstdlib>
#include <utility>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;

/**
 * @brief GkFrequencies::GkFrequencies has been setup in such a way that when a signal is emitted here (for the addition or removal
 * of a frequency and its component values), it too will be added to the relevant QTableViews where required, as well as the Google
 * LevelDB database if appropriate. Signals emitting from said QTableViews will also ensure that the relevant frequencies and their
 * component values will be added/deleted where required too.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkFrequencies::GkFrequencies(QPointer<GekkoFyre::GkLevelDb> database, QObject *parent)
{
    gkDb = std::move(database);

    QObject::connect(this, SIGNAL(updateFrequencies(const quint64 &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)),
                     this, SLOT(updateFreqsInMem(const quint64 &, const GekkoFyre::AmateurRadio::DigitalModes &, const GekkoFyre::AmateurRadio::IARURegions &, const bool &)));

    return;
}

GkFrequencies::~GkFrequencies()
{
    return;
}

/**
 * @brief GkFreqList::publishFreqList will publish a list of frequencies to the global std::vector which
 * stores such data in memory for use throughout Small World Deluxe.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note This data has been drawn from the JTDX project
 * <https://github.com/jtdx-project/jtdx/blob/0d45ce10b3f0ebb0b37a88dae71a5c2b25383dd2/FrequencyList.cpp>.
 */
void GkFrequencies::publishFreqList()
{
    emit updateFrequencies(136000, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(136130, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(136130, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(136130, DigitalModes::T10, IARURegions::ALL, false);

    emit updateFrequencies(474200, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(474200, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(474200, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(474200, DigitalModes::WSPR, IARURegions::ALL, false);

    emit updateFrequencies(1836600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(1838000, DigitalModes::JT65, IARURegions::ALL, false); // squeezed allocations
    emit updateFrequencies(1839000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(1839500, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(1840000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(1908000, DigitalModes::FT8, IARURegions::R1, false);

    emit updateFrequencies(3567000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(3568600, DigitalModes::WSPR, IARURegions::ALL, false); // needs guard marker and lock out
    emit updateFrequencies(3570000, DigitalModes::JT65, IARURegions::ALL, false); // JA compatible
    emit updateFrequencies(3572000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(3572500, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(3573000, DigitalModes::FT8, IARURegions::ALL, false); // above as below JT65 is out of DM allocation
    emit updateFrequencies(3575000, DigitalModes::FT4, IARURegions::ALL, false);  // provisional
    emit updateFrequencies(3568000, DigitalModes::FT4, IARURegions::R3, false);   // provisional
    emit updateFrequencies(3585000, DigitalModes::FT8, IARURegions::ALL, false);

    emit updateFrequencies(5126000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(5357000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(5357500, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(5357000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(5287200, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(5362000, DigitalModes::FT8, IARURegions::ALL, false);

    emit updateFrequencies(7038600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(7047500, DigitalModes::FT4, IARURegions::ALL, false); // provisional - moved
    emit updateFrequencies(7056000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(7074000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(7076000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(7078000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(7079000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(7080000, DigitalModes::FT8, IARURegions::ALL, false);
                                             // up 500Hz to clear
                                             // W1AW code practice QRG

    emit updateFrequencies(10131000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(10136000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(10138000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(10138700, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(10140000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(10141000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(10140000, DigitalModes::FT4, IARURegions::ALL, false); // provisional
    emit updateFrequencies(10143000, DigitalModes::FT8, IARURegions::ALL, false);

    emit updateFrequencies(14095600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(14074000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(14076000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(14078000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(14079000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(14080000, DigitalModes::FT4, IARURegions::ALL, false); // provisional
    emit updateFrequencies(14090000, DigitalModes::FT8, IARURegions::ALL, false);

    emit updateFrequencies(18095000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(18100000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(18102000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(18104000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(18104000, DigitalModes::FT4, IARURegions::ALL, false); // provisional
    emit updateFrequencies(18104600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(18105000, DigitalModes::T10, IARURegions::ALL, false);

    emit updateFrequencies(21074000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(21076000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(21078000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(21079000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(21091000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(21094600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(21140000, DigitalModes::FT4, IARURegions::ALL, false);

    emit updateFrequencies(24911000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(24915000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(24917000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(24919000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(24919000, DigitalModes::FT4, IARURegions::ALL, false); // provisional
    emit updateFrequencies(24929000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(24924600, DigitalModes::WSPR, IARURegions::ALL, false);

    emit updateFrequencies(28074000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(28076000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(28078000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(28079000, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(28095000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(28124600, DigitalModes::WSPR, IARURegions::ALL, false);
    emit updateFrequencies(28180000, DigitalModes::FT4, IARURegions::ALL, false);

    emit updateFrequencies(50276000, DigitalModes::JT65, IARURegions::R2, false);
    emit updateFrequencies(50276000, DigitalModes::JT65, IARURegions::R3, false);
    emit updateFrequencies(50293000, DigitalModes::WSPR, IARURegions::R2, false);
    emit updateFrequencies(50293000, DigitalModes::WSPR, IARURegions::R3, false);
    emit updateFrequencies(50310000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(50312000, DigitalModes::JT9, IARURegions::ALL, false);
    emit updateFrequencies(50312500, DigitalModes::T10, IARURegions::ALL, false);
    emit updateFrequencies(50313000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(50318000, DigitalModes::FT4, IARURegions::ALL, false); // provisional
    emit updateFrequencies(50323000, DigitalModes::FT8, IARURegions::ALL, false);

    emit updateFrequencies(70100000, DigitalModes::FT8, IARURegions::R1, false);
    emit updateFrequencies(70154000, DigitalModes::FT8, IARURegions::R1, false);
    emit updateFrequencies(70102000, DigitalModes::JT65, IARURegions::R1, false);
    emit updateFrequencies(70104000, DigitalModes::JT9, IARURegions::R1, false);
    emit updateFrequencies(70091000, DigitalModes::WSPR, IARURegions::R1, false);

    emit updateFrequencies(144120000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(144170000, DigitalModes::FT4, IARURegions::ALL, false);
    emit updateFrequencies(144174000, DigitalModes::FT8, IARURegions::ALL, false);
    emit updateFrequencies(144489000, DigitalModes::WSPR, IARURegions::ALL, false);

    emit updateFrequencies(222065000, DigitalModes::JT65, IARURegions::R2, false);

    emit updateFrequencies(432065000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(432300000, DigitalModes::WSPR, IARURegions::ALL, false);

    emit updateFrequencies(902065000, DigitalModes::JT65, IARURegions::R2, false);

    emit updateFrequencies(1296065000, DigitalModes::JT65, IARURegions::ALL, false);
    emit updateFrequencies(1296500000, DigitalModes::WSPR, IARURegions::ALL, false);

    emit updateFrequencies(2301065000, DigitalModes::JT65, IARURegions::ALL, false);

    emit updateFrequencies(2304065000, DigitalModes::JT65, IARURegions::ALL, false);

    emit updateFrequencies(2320065000, DigitalModes::JT65, IARURegions::ALL, false);

    emit updateFrequencies(3400065000, DigitalModes::JT65, IARURegions::ALL, false);

    emit updateFrequencies(5760065000, DigitalModes::JT65, IARURegions::ALL, false);

    gkDb->writeFreqInit(); // Write to the database that we've initialized Google LevelDB with these aforementioned base values!

    return;
}

/**
 * @brief GkFreqList::approximatelyEqual checks to see if the two floating pointers are approximately
 * equal or not.
 * @author The Art of Computer Programming by Donald Knuth <https://en.wikipedia.org/wiki/The_Art_of_Computer_Programming>.
 * @param a The comparison to be made against.
 * @param b The comparison to be made towards.
 * @param epsilon The precision, or rather, to how many digits should the comparison be equal towards.
 * @return Whether the desired outcome was true or false.
 */
bool GkFrequencies::approximatelyEqual(const float &a, const float &b, const float &epsilon)
{
    return std::fabs(a - b) <= ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

/**
 * @brief GkFreqList::essentiallyEqual checks to see if the two floating pointers are essentially equal
 * or not.
 * @author The Art of Computer Programming by Donald Knuth <https://en.wikipedia.org/wiki/The_Art_of_Computer_Programming>.
 * @param a The comparison to be made against.
 * @param b The comparison to be made towards.
 * @param epsilon The precision, or rather, to how many digits should the comparison be equal towards.
 * @return Whether the desired outcome was true or false.
 */
bool GkFrequencies::essentiallyEqual(const float &a, const float &b, const float &epsilon)
{
    return std::fabs(a - b) <= ((std::fabs(a) > std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

/**
 * @brief GkFreqList::definitelyGreaterThan checks to see if one of the two floating pointers are *definitely*
 * greater than the other.
 * @author The Art of Computer Programming by Donald Knuth <https://en.wikipedia.org/wiki/The_Art_of_Computer_Programming>.
 * @param a The comparison to be made against.
 * @param b The comparison to be made towards.
 * @param epsilon The precision, or rather, to how many digits should the comparison be equal towards.
 * @return Whether the desired outcome was true or false.
 */
bool GkFrequencies::definitelyGreaterThan(const float &a, const float &b, const float &epsilon)
{
    return (a - b) > ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

/**
 * @brief GkFreqList::definitelyLessThan checks to see if one of the two floating pointers are *definitely*
 * lesser than the other.
 * @author The Art of Computer Programming by Donald Knuth <https://en.wikipedia.org/wiki/The_Art_of_Computer_Programming>.
 * @param a The comparison to be made against.
 * @param b The comparison to be made towards.
 * @param epsilon The precision, or rather, to how many digits should the comparison be equal towards.
 * @return Whether the desired outcome was true or false.
 */
bool GkFrequencies::definitelyLessThan(const float &a, const float &b, const float &epsilon)
{
    return (b - a) > ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

QList<GekkoFyre::AmateurRadio::GkFreqs> GkFrequencies::listOfFreqs()
{
    return frequencyList;
}

int GkFrequencies::size()
{
    return frequencyList.size();
}

GkFreqs GkFrequencies::at(const int &idx)
{
    return frequencyList.at(idx);
}

/**
 * @brief GkFreqList::updateFreqsInMem Update the radio rig's used frequencies within memory, either by
 * adding or removing them from the global QVector.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequency The frequency to update.
 * @param digital_mode What digital mode is being used, whether it be WSPR, JT65, FT8, etc.
 * @param iaru_region The IARU Region that this particular frequency applies towards, such as ALL, R1, etc.
 * @param remove_freq Whether to remove the frequency in question from the global list or not.
 */
void GkFrequencies::updateFreqsInMem(const quint64 &frequency, const GekkoFyre::AmateurRadio::DigitalModes &digital_mode,
                                     const GekkoFyre::AmateurRadio::IARURegions &iaru_region, const bool &remove_freq)
{
    GkFreqs freq;
    freq.frequency = frequency;
    freq.digital_mode = digital_mode;
    freq.iaru_region = iaru_region;
    if (remove_freq) {
        //
        // We are removing the frequency from the global std::vector!
        //
        if (!frequencyList.empty()) {
            for (int i = 0; i < frequencyList.size(); ++i) {
                if (frequencyList[i].frequency == freq.frequency) {
                    frequencyList.erase(frequencyList.begin() + i);
                    emit removeFreq(frequencyList[i]);

                    break;
                }
            }
        }
    } else {
        //
        // We are adding the frequency to the global std::vector!
        //
        frequencyList.reserve(1);
        frequencyList.push_back(freq);
        emit addFreq(freq);
    }

    return;
}
