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

#ifndef doDBEntryEditor_H
#define doDBEntryEditor_H

#include <QObject>
#include <QWidget>

#include "doDBTree/doDBTree.h"
#include "doDBEntryEditor/doDBEntryEditor.ui.h"
#include "doDBConnection/doDBConnection.h"

#include "db/etDBObject.h"
#include "db/etDBObjectValue.h"


class doDBEntryEditor :
public QWidget
{
Q_OBJECT

public:
    typedef enum mode {
        modeNothing = 0,
        modeCreate,
        modeChanged,
        modeRemove
    } mode;

public:
    doDBEntryEditor( QWidget *parentWidget );
    ~doDBEntryEditor();


public:
    void            dbObjectShow( etDBObject *dbObject, QString tableName );
    void            valueCleanAll();
    static void     valueSet( void *userdata, const char *columnName, const char *columnValue );

private:
    void            columnClean();
    int             columnAppend( QString columnName, QString columnDisplayName );
    int             columnFind( QString columnName );

    QString         tableName();
    QString         columnName( int row );
    QString         columnDisplayName( int row );
    QString         oldValue( int row );
    QString         newValue( int row );

private slots:
    void            entryCreate();
    void            entrySave();
    void            entryDeleteStep1();
    void            entryDeleteStep2();

signals:
    void            saveNew( etDBObject *dbObject, const char *tableName );
    void            saveChanged( etDBObject *dbObject, const char *tableName );

private:
    Ui::doDBEntryEditor     ui;
    doDBtree                *dbTree;
    etDBObject              *dbObject;


    mode                    editMode;

    int                     primaryKeyRow;
    QString                 primaryKeyValue;

};

#endif
