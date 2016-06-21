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

#include <QDebug>
#include <QUuid>

#include "doDBEntryPlugin.h"
#include "doDBEntryEditor.h"

#include "doDBDebug/doDBDebug.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"

#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"



doDBEntryPlugin::               doDBEntryPlugin() : doDBPlugin() {

    this->selectedEntry = NULL;
    this->dbEntryEditor = new doDBEntryEditor(NULL);
    connect( this->dbEntryEditor, SIGNAL(entryNew(etDBObject*,const char*)), this, SLOT(entryEditorItemSaveNew(etDBObject*,const char*)) );
    connect( this->dbEntryEditor, SIGNAL(entryChanged(etDBObject*,const char*)), this, SLOT(entryEditorItemChanged(etDBObject*,const char*)) );
    connect( this->dbEntryEditor, SIGNAL(entryDelete(etDBObject*,const char*,const char*)), this, SLOT(entryEditorItemDelete(etDBObject*,const char*,const char*)) );

}

doDBEntryPlugin::               ~doDBEntryPlugin(){

}




QString doDBEntryPlugin::       valueGet( QString valueName ){

    if( valueName == "name" ){
        return "entryEditor";
    }
    if( valueName == "creator" ){
        return "stackshadow";
    }
    if( valueName == "description" ){
        return "The default editor for values";
    }


    return "";
}


void doDBEntryPlugin::          prepareLayout( QString name, QLayout* layout ){

    if( name == "detailView" ){

    // hide the itemEditor
        this->dbEntryEditor->setVisible(false);

    // append the editor to the viewLayout
        layout->addWidget( this->dbEntryEditor );

    }


}


bool doDBEntryPlugin::          handleAction( QString action, doDBEntry* entry ){

    if( action == "itemExpanded" ){
        return true;
    }

    if( action == "itemCollapsed" ){
        return true;
    }

    if( action == "itemClicked" ){
        this->load( entry );
        return true;
    }

}




bool doDBEntryPlugin::          load( doDBEntry *entry ){

// vars
    QString                     itemTableName;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    etDBObject*                 dbObject;
    doDBConnection*             connection;


    if( this->selectedEntry != entry ){

        if( this->selectedEntry != NULL ){

        // release the old one
            this->selectedEntry->decRef();
            this->selectedEntry = NULL;
        }

    // lock the new one
        this->selectedEntry = entry;
        this->selectedEntry->incRef();
    }

// get Stuff from the selected item
    connection = this->selectedEntry->connection();
    this->selectedEntry->item( &itemTableName, &itemID, (int*)&itemType );

// if selected item is an entry
    if(  itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){

    // dbObject
        dbObject = connection->dbObject;

    // pick the table
        etDBObjectTablePick( dbObject, itemTableName.toUtf8() );

    // clear values
        etDBObjectValueClean( dbObject );


        if( itemType == doDBtree::typeEntry ){
            connection->dbDataGet( itemTableName.toUtf8(), itemID.toUtf8() );
        }


    // show the editor
        this->dbEntryEditor->dbObjectShow( dbObject, itemTableName );
        this->dbEntryEditor->setVisible( true );

    } else {
        this->dbEntryEditor->setVisible( false );
    }



// next event
    return true;
}


void doDBEntryPlugin::          entryEditStart( etDBObject *dbObject, const char *tableName ){

}


void doDBEntryPlugin::          entryEditAbort( etDBObject *dbObject, const char *tableName ){

}


void doDBEntryPlugin::          entryEditorItemSaveNew( etDBObject *dbObject, const char *tableName ){

    doDBConnection *connection = this->selectedEntry->connection();
    if( connection == NULL ) return;

    if( connection->dbDataNew( tableName ) == true ){

        const char* primaryKeyColumn;
        const char* primaryKey;

        if( etDBObjectTableColumnPrimaryGet( dbObject, primaryKeyColumn ) != etID_YES ) return;
        if( etDBObjectValueGet( dbObject, primaryKeyColumn, primaryKey ) != etID_YES ) return;

    // set the selected item
    // TODO
    /*
        this->selectedEntry->tableName = tableName;
        this->selectedEntry->itemID = primaryKey;
        this->selectedEntry->type = (int)doDBtree::typeEntry;

        this->dbTreeItemClicked( this->selectedEntry );
        */

    }


}


void doDBEntryPlugin::          entryEditorItemChanged( etDBObject *dbObject, const char *tableName ){

    doDBConnection *connection = this->selectedEntry->connection();
    if( connection == NULL ) return;

    connection->dbDataChange( tableName );
}


void doDBEntryPlugin::          entryEditorItemDelete( etDBObject *dbObject, const char *tableName, const char *itemID ){

    doDBConnection *connection = this->selectedEntry->connection();
    if( connection == NULL ) return;

    if( connection->dbDataDelete( tableName, itemID ) == true ){
        this->dbEntryEditor->valueCleanAll();

        // remove the item from the dbTree
        QTreeWidgetItem* treeItem = this->selectedEntry->treeWidgetItem();
        if( treeItem != NULL ){
            QFont font = treeItem->font(0);
            font.setStrikeOut(true);
            treeItem->setFont(0,font);
            treeItem->setDisabled(true);
        }


    }


}


