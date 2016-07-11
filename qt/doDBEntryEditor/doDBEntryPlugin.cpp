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
#include <QStackedWidget>

#include "doDBEntryPlugin.h"
#include "doDBEntryEditor.h"

#include "doDBDebug/doDBDebug.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBPlugin/doDBPlugins.h"

#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"



doDBEntryPlugin::               doDBEntryPlugin() : doDBPlugin() {

    this->selectedEntry = NULL;
    this->dbEntryEditor = new doDBEntryEditor(NULL);
    connect( this->dbEntryEditor, SIGNAL(entryNew(etDBObject*,const char*)), this, SLOT(entryEditorItemSaveNew(etDBObject*,const char*)) );
    connect( this->dbEntryEditor, SIGNAL(entryChanged(etDBObject*,const char*)), this, SLOT(entryEditorItemChanged(etDBObject*,const char*)) );
    connect( this->dbEntryEditor, SIGNAL(entryDelete(etDBObject*,const char*,const char*)), this, SLOT(entryEditorItemDelete(etDBObject*,const char*,const char*)) );

// register for messages
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitStackedRight, true );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemSelected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemDeleted );

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



bool doDBEntryPlugin::          recieveMessage( messageID type, void* payload ){

// Item stuff

    if( type == doDBPlugin::msgItemSelected ){
        doDBEntry* entry = (doDBEntry*)payload;

        this->load( entry );
        return true;
    }

    if( type == doDBPlugin::msgItemDeleted ){
        doDBEntry* entry = (doDBEntry*)payload;

        if( this->selectedEntry == entry ){
            doDBEntry::decRef( &this->selectedEntry );
            this->selectedEntry = NULL;
        }

        return true;
    }

// inits
    if( type == doDBPlugin::msgInitStackedRight ){
        QStackedWidget* stackedWidget = (QStackedWidget*)payload;

        this->dbEntryEditor->setVisible(false);
        stackedWidget->addWidget( this->dbEntryEditor );
        stackedWidget->setCurrentWidget( this->dbEntryEditor );

        return true;
    }

    return true;
}




bool doDBEntryPlugin::          load( doDBEntry *entry ){

// vars
    QString                     itemTableName;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    etDBObject*                 dbObject;
    doDBConnection*             connection;


    if( this->selectedEntry != entry ){

    // release the old one
        doDBEntry::decRef( &this->selectedEntry );

    // lock the new one
        this->selectedEntry = entry;
        this->selectedEntry->incRef();
    }

// get Stuff from the selected item
    connection = this->selectedEntry->connection();
    this->selectedEntry->item( &itemTableName, &itemID, (int*)&itemType, NULL );

// if selected item is an entry
    if(  itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){

    // dbObject
        dbObject = connection->dbObject;

    // pick the table
        etDBObjectTablePick( dbObject, itemTableName.toUtf8() );

    // clear values
        etDBObjectValueClean( dbObject );

    // load all values
        if( itemType == doDBtree::typeEntry ){
            connection->dbDataRead( itemTableName.toUtf8(), itemID.toUtf8() );
            connection->dbDataNext();
        }


    // show the editor
        this->dbEntryEditor->dbObjectShow( dbObject, itemTableName );
        this->dbEntryEditor->setVisible( true );
        this->dbEntryEditor->modeSet( doDBEntryEditor::modeView );

    } else {
        this->dbEntryEditor->setVisible( false );
        this->dbEntryEditor->modeSet( doDBEntryEditor::modeView );
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

    // create a new doDBEntry
        doDBEntry* dbEntryNew = new doDBEntry();
        dbEntryNew->copy( this->selectedEntry );

    // set
        int type = (int)doDBtree::typeEntry;
        doDBEntry::itemSet( &dbEntryNew, tableName, primaryKey, &type, NULL );
        doDBEntry::treeWidgetItemSet( &dbEntryNew, NULL );

    // fire the broadcast
        doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgItemSelected, dbEntryNew );

    }


}


void doDBEntryPlugin::          entryEditorItemChanged( etDBObject *dbObject, const char *tableName ){

    doDBConnection *connection = this->selectedEntry->connection();
    if( connection == NULL ) return;

    if( connection->dbDataChange( tableName ) == true ){
        doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgItemChanged, this->selectedEntry );
    }

}


void doDBEntryPlugin::          entryEditorItemDelete( etDBObject *dbObject, const char *tableName, const char *itemID ){

    doDBConnection *connection = this->selectedEntry->connection();
    if( connection == NULL ) return;

    if( connection->dbDataDelete( tableName, itemID ) == true ){

        // remove the item from the dbTree
        QTreeWidgetItem* treeItem = this->selectedEntry->treeWidgetItem();
        if( treeItem != NULL ){
            QFont font = treeItem->font(0);
            font.setStrikeOut(true);
            treeItem->setFont(0,font);
            treeItem->setDisabled(true);
        }


    // notice all that we delete the item
        doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgItemDeleted, this->selectedEntry  );

    // we dont need the entry anymore
        doDBEntry::decRef( &this->selectedEntry );


    }


}


