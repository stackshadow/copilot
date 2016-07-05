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

#ifndef doDBCollector_H
#define doDBCollector_H

#include <QObject>
#include <QWidget>
#include <QTableWidget>

#include "doDBEntry/doDBEntry.h"

class doDBCollector :
public QTableWidget
{
Q_OBJECT

public:
    typedef enum columns {
        colDisplayName = 0,
        colCount
    } columns;

public:
                    doDBCollector( QWidget* parentWidget );
                    ~doDBCollector();

    void            entryAppend( doDBEntry* entry );
    void            selectedEntryRemove();


    doDBEntry*      getEntry( int index );

public slots:
    void            cleanAll();

private:
    void            refreshList();




private:
    QList<doDBEntry*>       entrys;

};

#endif
