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


    this->dbEntryEditor = new doDBEntryEditor(NULL);


}

doDBEntryPlugin::               ~doDBEntryPlugin(){

}



void doDBEntryPlugin::          prepareItemView( QLayout *itemViewLayout ){

// hide the itemEditor
    this->dbEntryEditor->setVisible(false);

// append the editor to the viewLayout
    itemViewLayout->addWidget( this->dbEntryEditor );

}


bool doDBEntryPlugin::          dbTreeItemClicked( QTreeWidgetItem * item, int column ){

    // vars
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    etDBObject                  *dbObject;

// get Stuff from the selected item
    tableName = doDBtree::itemTableName( item );
    connectionID = doDBtree::itemConnectionID( item );
    itemID = doDBtree::itemID( item );
    itemType = doDBtree::itemType( item );

// if selected item is an entry
    if(  itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){

    // if connection changed, we get the new one
        if( connectionID != this->connectionID || this->connection == NULL ){

        // get the connection
            this->connection = doDBConnections::ptr->connectionGet( connectionID.toUtf8() );
            if( this->connection == NULL ) return true;

        // get the dbobject
            this->connectionID = connectionID;
        }


    // dbObject
        dbObject = this->connection->dbObject;

    // pick the table
        etDBObjectTablePick( dbObject, tableName.toUtf8() );

    // clear values
        etDBObjectValueClean( dbObject );


        if(  itemType == doDBtree::typeEntry ){
            this->connection->dbDataGet( tableName.toUtf8(), itemID.toUtf8() );
        }


    // show the editor
        this->dbEntryEditor->dbObjectShow( dbObject, tableName );
        this->dbEntryEditor->setVisible( true );

    } else {
        this->dbEntryEditor->setVisible( false );
    }



// next event
    return true;
}


/*
// if selected item is an entry
    if(  itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){

        connection = doDBConnections::ptr->connectionGet( connectionID.toUtf8() );
        if( connection == NULL ) return;

    // get the dbobject
        this->entryEditorConnID = connectionID;
        dbObject = connection->dbObject;

    // pick the table
        etDBObjectTablePick( dbObject, tableName.toUtf8() );

    // clear values
        etDBObjectValueClean( dbObject );


        if(  itemType == doDBtree::typeEntry ){
            connection->dbDataGet( tableName.toUtf8(), itemID.toUtf8() );
        }

    }
*/



