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

#include "main.h"
#include "doDBRelationPlugin.h"

#include "doDBDebug/doDBDebug.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"

#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"


doDBRelationPlugin::            doDBRelationPlugin() : doDBPlugin() {


// create relation
    this->dbRelation = new doDBRelation();
    this->dbRelationEditor = NULL;
    this->connectionID = "";

    this->btnEditRelations = NULL;

// db tree
    this->dbTree = NULL;
    this->dbTreeItemTypeRelatedTable = -1;

}

doDBRelationPlugin::            ~doDBRelationPlugin(){

}




void doDBRelationPlugin::       prepareTree( doDBtree *dbTree ){
// save
    this->dbTree = dbTree;

// setup types
    this->dbTreeItemTypeRelatedTable = dbTree->newItemType();
}


void doDBRelationPlugin::       prepareItemView( QLayout *itemViewLayout ){


// edit button
    this->btnEditRelations = new QPushButton( "Relation Bearbeiten" );
    this->btnEditRelations->setVisible( false );
    connect( this->btnEditRelations, SIGNAL(clicked()), this, SLOT(editorShow()) );
    itemViewLayout->addWidget( this->btnEditRelations );

// connect button
    this->btnConnectRelation = new QPushButton( "Verbinden" );
    this->btnConnectRelation->setVisible( false );
    //connect( this->btnConnectRelation, SIGNAL(clicked()), this, SLOT(editorShow()) );
    itemViewLayout->addWidget( this->btnConnectRelation );



}





bool doDBRelationPlugin::       dbTreeItemClicked( QTreeWidgetItem * item, int column ){


// vars
    doDBtree::treeItemType      itemType;
    QString                     connectionID;

// get Stuff from the selected item
    itemType = doDBtree::itemType( item );
    connectionID = doDBtree::itemConnectionID( item );

// clicked on table
    if( itemType == doDBtree::typeTable ){
        this->connectionID = connectionID;
        this->btnEditRelations->setVisible( true );
    } else {
        this->btnEditRelations->setVisible( false );
        //this->tableFolderEditor->setVisible( false );
    }


// clicked on table
    if( itemType == doDBtree::typeEntry ){
        this->btnConnectRelation->setVisible( true );
    } else {
        this->btnConnectRelation->setVisible( false );
    }

    return true;
}


bool doDBRelationPlugin::       dbTreeItemExpanded( QTreeWidgetItem * item ){

// vars
    doDBConnection              *connection;
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;


// get Stuff from the selected item
    tableName = doDBtree::itemTableName( item );
    connectionID = doDBtree::itemConnectionID( item );
    itemID = doDBtree::itemID( item );
    itemType = doDBtree::itemType( item );


// load relation from DB ( if needed )
    if( this->connectionID != connectionID ){
        this->connectionID = connectionID;
        this->dbRelation->dbRelationLoad( this->connectionID.toUtf8() );
    }

// the connection we need
    connection = doDBConnections::ptr->connectionGet( connectionID.toUtf8() );
    if( connection == NULL ){
        return true; // next plugin
    }


// if selected item is an entry
    if( itemType == doDBtree::typeEntry ){

        etDBObject *dbObject = connection->dbObject;
        if( dbObject == NULL ) return true; // next plugin

    // append related tables
        const char *relatedTable;
        const char *relatedTableDisplayName = NULL;
        bool relatedTableExist = this->dbRelation->relatedTableGetFirst( tableName.toUtf8(), NULL, &relatedTable, NULL );
        while( relatedTableExist ){


            // get the display name
            if( etDBObjectTablePick(dbObject,relatedTable) == etID_NO ){
                relatedTableExist = this->dbRelation->relatedTableGetNext( NULL, &relatedTable, NULL );
                continue;
            }

            const char *tableDisplayName = NULL;
            if( etDBObjectTableDisplayNameGet(dbObject, "", tableDisplayName) != etID_YES ){
                goto next;
            }

            //etDBObjectTableDisplayNameGet(  )
            this->dbTree->append( item, tableDisplayName, relatedTable, connectionID.toUtf8(), "", this->dbTreeItemTypeRelatedTable );

        next:
            relatedTableExist = this->dbRelation->relatedTableGetNext( NULL, &relatedTable, NULL );
        }

    }

// if we expand an related Table -> show related data
    if( itemType == this->dbTreeItemTypeRelatedTable ){

        QTreeWidgetItem *parentItem = item->parent();
        if( parentItem == NULL ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Related table without parent ! This is impossible !" );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return true; // next plugin
        }

    // set the selected item in the dbTree
        this->dbTree->selectedItem = item;

    // get infos about the parent of the relation
        QString srcTable = this->dbTree->itemTableName( parentItem );
        QString srcTableItemID = this->dbTree->itemID( parentItem );


        this->dbRelation->dbDataGet( connectionID.toUtf8(), srcTable.toUtf8(), srcTableItemID.toUtf8(), tableName.toUtf8(), this->dbTree, doDBtree::callbackEntryAdd  );



    }


    return true; // next plugin
}


bool doDBRelationPlugin::       dbTreeItemCollapsed( QTreeWidgetItem * item ){
    return true; // next plugin
}




void doDBRelationPlugin::       editorShow(){


// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;

// get connection
    connection = doDBConnections::ptr->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return;

// load the relation from db
    this->dbRelation->dbRelationLoad( this->connectionID.toUtf8() );

// create if needed
    if( this->dbRelationEditor == NULL ){
        this->dbRelationEditor = new doDBRelationEditor(NULL);


    // show it
        QLayout *layout = this->btnEditRelations->parentWidget()->layout();
        if( layout != NULL ){
            layout->addWidget(this->dbRelationEditor);
        }

        connect( this->dbRelationEditor, SIGNAL(closed()), this, SLOT(editorClosed()) );
    }

// init it
    this->dbRelationEditor->showRelation( dbObject, this->dbRelation );
    this->dbRelationEditor->setVisible(true);
}


void doDBRelationPlugin::       editorClosed(){
    this->dbRelation->dbRelationSave( this->connectionID.toUtf8() );
}


