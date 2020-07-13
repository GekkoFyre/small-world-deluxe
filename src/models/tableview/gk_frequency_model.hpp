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
#include "src/dek_db.hpp"
#include <memory>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QPointer>
#include <QTableView>
#include <QModelIndex>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

namespace GekkoFyre {

class GkFreqTableViewModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit GkFreqTableViewModel(std::shared_ptr<GekkoFyre::GkLevelDb> database, QWidget *parent = nullptr);
    ~GkFreqTableViewModel();

    void populateData(const QList<GekkoFyre::AmateurRadio::GkFreqs> &frequencies);
    void populateData(const QList<GekkoFyre::AmateurRadio::GkFreqs> &frequencies, const bool &populate_freq_db);
    void insertData(const GekkoFyre::AmateurRadio::GkFreqs &freq_val);
    void insertData(const GekkoFyre::AmateurRadio::GkFreqs &freq_val, const bool &populate_freq_db);
    void removeData(const GekkoFyre::AmateurRadio::GkFreqs &freq_val);
    void removeData(const GekkoFyre::AmateurRadio::GkFreqs &freq_val, const bool &remove_freq_db);
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

public slots:
    void customHeaderMenuRequested(QPoint pos);

signals:
    void removeFreq(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_remove);
    void addFreq(const GekkoFyre::AmateurRadio::GkFreqs &freq_to_add);

private:
    std::shared_ptr<GekkoFyre::GkLevelDb> GkDb;
    QList<GekkoFyre::AmateurRadio::GkFreqs> m_data;

    QPointer<QTableView> table;
    QPointer<QMenu> menu;
    QPointer<QSortFilterProxyModel> proxyModel;

};
};