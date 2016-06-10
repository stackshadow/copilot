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

// we need the entryeditor
#include "doDBEntryEditor/doDBEntryEditor.h"

doDBRelationPlugin::            doDBRelationPlugin() : doDBPlugin() {


// create relation
    this->dbRelation = new doDBRelation();
    this->dbRelationEditor = NULL;
    this->connectionID = "";

    this->btnEditRelations = NULL;

// db tree
    this->dbTree = NULL;
    this->dbTreeItemTypeRelatedTable = -1;

    this->connectionMode = false;

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


// basic infos
    this->srcInfos = new QLineEdit();
    this->srcInfos->setReadOnly(true);
    this->srcInfos->setVisible( false );
    itemViewLayout->addWidget( this->srcInfos );

// edit button
    this->btnEditRelations = new QPushButton( "Relation Bearbeiten" );
    this->btnEditRelations->setVisible( false );
    connect( this->btnEditRelations, SIGNAL(clicked()), this, SLOT(editorShow()) );
    itemViewLayout->addWidget( this->btnEditRelations );

// connect button
    this->btnConnectRelation = new QPushButton( "Verbinden" );
    this->btnConnectRelation->setVisible( false );
    connect( this->btnConnectRelation, SIGNAL(clicked()), this, SLOT(connectionStart()) );
    itemViewLayout->addWidget( this->btnConnectRelation );

    this->btnConnectRelationSave = new QPushButton( "Verbinden abschießen" );
    this->btnConnectRelationSave->setVisible( false );
    connect( this->btnConnectRelationSave, SIGNAL(clicked()), this, SLOT(connectionSave()) );
    itemViewLayout->addWidget( this->btnConnectRelationSave );

    this->btnConnectRelationCancel = new QPushButton( "Verbinden abbrechen" );
    this->btnConnectRelationCancel->setVisible( false );
    connect( this->btnConnectRelationCancel, SIGNAL(clicked()), this, SLOT(connectionCancel()) );
    itemViewLayout->addWidget( this->btnConnectRelationCancel );



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

    // connect mode
        if( this->connectionMode ){
            this->btnEditRelations->setVisible(false);
            this->btnConnectRelation->setVisible(false);
            this->btnConnectRelationSave->setVisible(false);
            this->btnConnectRelationCancel->setVisible(false);
        } else {
            // remember connection ID
            this->connectionID = connectionID;
        // show relation editor
            this->btnEditRelations->setVisible(true);
        }





    } else {
        this->btnEditRelations->setVisible(false);
    }


// clicked on table
    if( itemType == doDBtree::typeEntry ){

    // connect mode
        if( this->connectionMode ){

            this->relTable = doDBtree::itemTableName( item );
            this->relItemID = doDBtree::itemID( item );

            this->connectionCheck();

        } else {

            this->btnConnectRelation->setVisible( true );

        // remember
            this->connectionID = doDBtree::itemConnectionID( item );
            this->srcDislpayName = doDBtree::itemDisplayValue( item );
            this->srcTable = doDBtree::itemTableName( item );
            this->srcItemID = doDBtree::itemID( item );


        }



    } else {
        this->btnConnectRelation->setVisible( false );
    }

    return true;
}


bool doDBRelationPlugin::       dbTreeItemExpanded( QTreeWidgetItem * item ){

// on connection mode, we do nothing
    if( this->connectionMode ){
        return true;
    }

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

        this->dbRelation->relationGetReset();
        while( this->dbRelation->relatedTableGetNext( tableName.toUtf8(), NULL, &relatedTable, NULL ) ){


            // get the display name
            if( etDBObjectTablePick(dbObject,relatedTable) == etID_NO ){
                continue;
            }

            const char *tableDisplayName = NULL;
            if( etDBObjectTableDisplayNameGet(dbObject, "", tableDisplayName) != etID_YES ){
                continue;
            }

            //etDBObjectTableDisplayNameGet(  )
            this->dbTree->append( item, tableDisplayName, relatedTable, connectionID.toUtf8(), "", this->dbTreeItemTypeRelatedTable );

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


void doDBRelationPlugin::       connectionStart(){

// show / hide
    this->srcInfos->setVisible( true );
    this->btnConnectRelation->setVisible( false );
    this->btnConnectRelationSave->setVisible( true );
    this->btnConnectRelationCancel->setVisible( true );

// show the selected info
    this->srcInfos->setText( this->srcItemID + ": " + this->srcDislpayName );

// disable all tables
    this->dbTree->disableAllTables();



// enable only tables with an relation
    const char *srcColumn = NULL;
    const char *relatedTable = NULL;
    const char *relatedColumn = NULL;
    this->dbRelation->relationGetReset();

    while( this->dbRelation->relatedTableGetNext( this->srcTable.toUtf8(), NULL, &relatedTable, &relatedColumn ) ){
        this->dbTree->enableTable( relatedTable );
    }


    QTreeWidgetItem *item = this->dbTree->topLevelItem(0);
    item->setDisabled(true);

// now we activate the connection-mode
    this->connectionMode = true;
}


void doDBRelationPlugin::       connectionCheck(){

// enable only tables with an relation
    const char *srcPrimaryKeyColumn;
    const char *srcColumn = NULL;
    const char *srcColumnValue = NULL;
    const char *relatedTable = NULL;
    const char *relatedColumn = NULL;
    const char *relatedColumnValueChar = NULL;
    QString relatedColumnValue = "";
    this->dbRelation->relationGetReset();

    if( this->dbRelation->relatedTableFindNext( this->srcTable.toUtf8(), &srcColumn, this->relTable.toUtf8(), &relatedColumn ) ){

    // get connection
        doDBConnection *connection = NULL;
        connection = doDBConnections::ptr->connectionGet( this->connectionID.toUtf8() );
        if( connection == NULL ) return;

    // check if the column where we would like to write into is not the primary key !
        if( etDBObjectTablePick( connection->dbObject, this->srcTable.toUtf8() ) != etID_YES ) return;
        if( etDBObjectTableColumnPrimaryGet( connection->dbObject, srcPrimaryKeyColumn ) != etID_YES ) return;
        if( QString(srcColumn) == srcPrimaryKeyColumn ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "You could not write to the primary column ! Fix your relation" );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            this->btnConnectRelationSave->setVisible(false);
            return;
        }


    // cool save the columns
        this->srcColumn = srcColumn;
        this->relColumn = relatedColumn;

    // and enable the save-button
        this->btnConnectRelationSave->setVisible(true);
        this->btnConnectRelationCancel->setVisible(true);

        relatedTable = this->relTable.toUtf8();

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Prepare to save column content of column '%s' from table '%s' to '%s'", relatedColumn, relatedTable, srcColumn );
        etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

/*
    // request original column-data
        if ( connection->dbDataGet( doDBtree::itemTableName( item ).toUtf8(), doDBtree::itemID( item ).toUtf8() ) != true ) return true;

        etDBObjectValueGet( connection->dbObject, relatedColumn, relatedColumnValueChar );
        relatedColumnValue = relatedColumnValueChar;


    // request item to connect
        if ( connection->dbDataGet( this->srcTable.toUtf8(), this->srcItemID.toUtf8() ) != true ) return true;

    // overwrite value
        etDBObjectValueSet( connection->dbObject, srcColumn, relatedColumnValue.toUtf8() );
*/
        return;
    }

    this->btnConnectRelationSave->setVisible(false);
}


void doDBRelationPlugin::       connectionCancel(){

// show / hide
    this->srcInfos->setVisible( false );
    this->btnConnectRelation->setVisible( true );
    this->btnConnectRelationSave->setVisible( false );
    this->btnConnectRelationCancel->setVisible( false );

// disable all tables
    this->dbTree->disableAllTables(false);

// disable connection mode
    this->connectionMode = false;
}


void doDBRelationPlugin::       connectionSave(){

}















