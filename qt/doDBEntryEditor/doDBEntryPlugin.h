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

#ifndef doDBEntryPlugin_H
#define doDBEntryPlugin_H

#include <QString>
#include <QPushButton>
#include <QLayout>

#include "core/etIDState.h"
#include "string/etString.h"
#include "string/etStringChar.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTableColumn.h"
#include "dbdriver/etDBDriver.h"

#include "doDBPlugin/doDBPlugin.h"
#include "doDBTree/doDBTree.h"
#include "doDBEntryEditor/doDBEntryEditor.h"



class doDBEntryPlugin :
public doDBPlugin {
    Q_OBJECT

public:
                            doDBEntryPlugin();
                            ~doDBEntryPlugin();

// overload
    QString                 valueGet( QString valueName );
    void                    prepareLayout( QString name, QLayout* layout );
    bool                    handleAction( QString action, doDBEntry* entry );

// data handle
    bool                    load( doDBEntry *entry );



public slots:
    void                    entryEditStart( etDBObject *dbObject, const char *tableName );
    void                    entryEditAbort( etDBObject *dbObject, const char *tableName );
    void                    entryEditorItemSaveNew( etDBObject *dbObject, const char *tableName );
    void                    entryEditorItemChanged( etDBObject *dbObject, const char *tableName );
    void                    entryEditorItemDelete( etDBObject *dbObject, const char *tableName, const char *itemID );

private:
    doDBEntry*              selectedEntry;
    doDBEntryEditor*        dbEntryEditor;



};






#endif


