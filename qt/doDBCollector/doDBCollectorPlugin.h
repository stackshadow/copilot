/*  Copyright (C) 2016 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef doDBCollectorPlugin_H
#define doDBCollectorPlugin_H

#include <QPushButton>

#include "doDBPlugin/doDBPlugin.h"
#include "doDBCollectorWidget.h"
#include "doDBEntry/doDBEntry.h"

class doDBCollectorPlugin :
public doDBPlugin {
    Q_OBJECT

public:
                            doDBCollectorPlugin();
                            ~doDBCollectorPlugin();

// overload
    QString                 valueGet( QString valueName );
    bool                    recieveMessage( messageID type, void* payload );

private:
    QPushButton*            btnItemRemember;
    QPushButton*            btnItemRemove;
    QPushButton*            btnClean;
    doDBCollector*          collectorWidget;

    doDBEntry*              dbEntry;

private slots:
    void                    rememberItem();
    void                    itemClicked(int row, int column);


};
















#endif


