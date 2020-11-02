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

#pragma once

#include "src/defines.hpp"
#include <memory>
#include <string>
#include <QString>
#include <QIODevice>
#include <QAudioFormat>

#ifdef __cplusplus
extern "C"
{
#endif

#include <sndfile.h>

#ifdef __cplusplus
} // extern "C"
#endif

namespace GekkoFyre {

class GkSndfileCpp : public QIODevice {

public:
    explicit GkSndfileCpp();
    virtual ~GkSndfileCpp();

    bool isSequential() const { return false; }
    bool open(QString fileName, OpenMode mode, QAudioFormat format = QAudioFormat());
    void close();
    bool canReadLine() const { return false; }
    qint64 pos() const;
    qint64 size() const;
    bool seek(qint64 pos);
    bool atEnd() const;
    bool reset();
    QAudioFormat format();

protected:
    qint64 readData(char * data, qint64 maxSize) ;
    qint64 writeData(const char * data, qint64 maxSize);

private:
    SNDFILE *m_sndfile;
    SF_INFO m_info;

};
};
