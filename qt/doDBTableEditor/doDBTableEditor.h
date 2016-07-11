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

#ifndef doDBTableEditor_H
#define doDBTableEditor_H

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QComboBox>

#include "doDBTableEditor/doDBTableEditor.ui.h"

#include "doDBConnection/doDBConnection.h"

#include "core/etIDState.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTable.h"


class doDBTableEditor :
public QWidget
{

Q_OBJECT

public:
    enum state {
        MODE_NONE,
        MODE_TABLE_NEW,
        MODE_TABLE_CHANGE
    };

public:
    doDBTableEditor( QWidget *parent );
    ~doDBTableEditor();

    void                showObject( etDBObject *dbObject );
    static void         cBoxTablesAppend( void *userdata, const char *connID, const char *tableName, const char *tableDisplayName );

private:
// handle connection list
    doDBConnection*     connectionGet( QString UUID );

    void                cBoxTablesRefresh();
    void                cBoxTablesClean();

    void                listColumnsRefresh();
    void                listColumnsAppend( const char *columnName, const char *displayName );


private slots:
    void                cBoxTableSelected( int index );
    void                tableNew();
    void                tableSave();
    void                tableCreateInDB();
    void                listColumnsSelected( int row, int column );
    void                columnNew();
    void                columnSave();
    void                close();


signals:
    void                newTable( etDBObject *dbObject, QString newTableName );
    void                changeTable( etDBObject *dbObject, QString newTableName );
    void                newColumn( etDBObject *dbObject, QString newTableName, QString newColumnName );
    void                changeColumn( etDBObject *dbObject, QString newColumnName );

    void                closed();

private:
    int                         mode;
    Ui::doDBTableEditorUi       ui;
    etDBObject                  *dbObject;

};

#endif // DODBTABLEEDITORWINDOW_H
