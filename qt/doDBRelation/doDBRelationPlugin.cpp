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

doDBRelationPlugin::                doDBRelationPlugin() : doDBPlugin() {


// create relation
    this->dbRelation = new doDBRelation();
    this->dbRelationEditor = NULL;

    this->btnEditRelations = NULL;

// db tree
    this->dbTree = NULL;
    this->dbTreeItemTypeRelatedTable = -1;

    this->connectionMode = false;

}

doDBRelationPlugin::                ~doDBRelationPlugin(){

}





QString doDBRelationPlugin::        valueGet( QString valueName ){

    if( valueName == "name" ){
        return "relation";
    }
    if( valueName == "creator" ){
        return "stackshadow";
    }
    if( valueName == "description" ){
        return "Handle relations of tables. \n - provide an relation-editor\n - add relations to tree-widget";
    }


    return "";
}


void doDBRelationPlugin::           prepareLayout( QString name, QLayout* layout ){

    if( name == "detailView" ){

    // basic infos
        this->srcInfos = new QLineEdit();
        this->srcInfos->setReadOnly(true);
        this->srcInfos->setVisible( false );
        layout->addWidget( this->srcInfos );

    // edit button
        this->btnEditRelations = new QPushButton( "Relation Bearbeiten" );
        this->btnEditRelations->setVisible( false );
        connect( this->btnEditRelations, SIGNAL(clicked()), this, SLOT(editorShow()) );
        layout->addWidget( this->btnEditRelations );

    // connect button
        this->btnConnectRelation = new QPushButton( "Verbinden" );
        this->btnConnectRelation->setVisible( false );
        connect( this->btnConnectRelation, SIGNAL(clicked()), this, SLOT(connectionStart()) );
        layout->addWidget( this->btnConnectRelation );

        this->btnConnectRelationSave = new QPushButton( "Verbinden abschießen" );
        this->btnConnectRelationSave->setVisible( false );
        connect( this->btnConnectRelationSave, SIGNAL(clicked()), this, SLOT(connectionSave()) );
        layout->addWidget( this->btnConnectRelationSave );

        this->btnConnectRelationCancel = new QPushButton( "Verbinden abbrechen" );
        this->btnConnectRelationCancel->setVisible( false );
        connect( this->btnConnectRelationCancel, SIGNAL(clicked()), this, SLOT(connectionCancel()) );
        layout->addWidget( this->btnConnectRelationCancel );

    }


}


bool doDBRelationPlugin::           handleAction( QString action, doDBEntry* entry ){

    if( action == "itemExpanded" ){
        this->itemExpanded( entry );
        return true;
    }

    if( action == "itemCollapsed" ){
        this->itemCollapsed( entry );
        return true;
    }

    if( action == "itemClicked" ){
        this->itemClicked( entry );
        return true;
    }

}




void doDBRelationPlugin::           prepareTree( doDBtree *dbTree ){
// save
    this->dbTree = dbTree;

// setup types
    this->dbTreeItemTypeRelatedTable = dbTree->newItemType();
}




bool doDBRelationPlugin::           itemExpanded( doDBEntry* entry ){

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

    // get infos about the expanded item
    this->itemSelected = entry;
    connectionID = this->itemSelected->connectionID();
    this->itemSelected->item( &tableName, &itemID, (int*)&itemType );

// the connection we need
    connection = this->itemSelected->connection();
    if( connection == NULL ) return true; // next plugin

// load relation from db ( if needed )
    this->dbRelation->dbRelationLoad( connection );





// if selected item is an entry
    if( itemType == doDBtree::typeEntry ){

    // remember the parent table + itemid
        this->parentTable = tableName;
        this->parentID = itemID;

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
            this->dbTree->append( this->itemSelected->treeWidgetItem(), tableDisplayName, relatedTable, connectionID.toUtf8(), "", this->dbTreeItemTypeRelatedTable );

        }

    }

// if we expand an related Table -> show related data
    if( itemType == this->dbTreeItemTypeRelatedTable ){

    // get the parent table name
        QTreeWidgetItem* parentItem = this->itemSelected->treeWidgetItem()->parent();
        if( parentItem == NULL ) return true;
        this->parentTable = doDBtree::itemTableName( parentItem );
        this->parentID = doDBtree::itemID( parentItem );


        this->dbRelation->dbDataGet( connectionID.toUtf8(), this->parentTable.toUtf8(), this->parentID.toUtf8(), tableName.toUtf8(), this->dbTree, doDBtree::callbackEntryAdd  );


    }


    return true; // next plugin
}


bool doDBRelationPlugin::           itemCollapsed( doDBEntry* entry ){

// vars
    int         itemType;

// save entry
    this->itemSelected = entry;
    this->itemSelected->item( NULL, NULL, &itemType );


    if( itemType == this->dbTreeItemTypeRelatedTable ){
        qDeleteAll(this->itemSelected->treeWidgetItem()->takeChildren());
    }

    return true; // next plugin
}


bool doDBRelationPlugin::           itemClicked( doDBEntry* entry ){


// vars
    QString                     tableName;
    QString                     itemID;
    doDBtree::treeItemType      itemType;

// get Stuff from the selected item
    this->itemSelected = entry;

    entry->item( &tableName, &itemID, (int*)&itemType );

// clicked on table
    if( itemType == doDBtree::typeTable ){

    // connect mode
        if( this->connectionMode ){
            this->btnEditRelations->setVisible(false);
            this->btnConnectRelation->setVisible(false);
            this->btnConnectRelationSave->setVisible(false);
            this->btnConnectRelationCancel->setVisible(false);
        } else {
        // show relation editor
            this->btnEditRelations->setVisible(true);
        }

    } else {
        this->btnEditRelations->setVisible(false);
    }


// clicked on table
    if( itemType == doDBtree::typeEntry ){

    // normal mode
        if( ! this->connectionMode ){
            this->btnConnectRelation->setVisible( true );
        }
    // connect mode
        else {
            this->connectionCheck();
        }



    } else {
        this->btnConnectRelation->setVisible( false );
    }

    return true;
}





void doDBRelationPlugin::           editorShow(){


// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;

// get connection
    connection = this->itemSelected->connection();
    if( connection == NULL ) return;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return;

// load the relation from db
    this->dbRelation->dbRelationLoad( connection );

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


void doDBRelationPlugin::           editorClosed(){
    this->dbRelation->dbRelationSave( this->itemSelected->connection() );
}


void doDBRelationPlugin::           connectionStart(){

// show / hide
    this->srcInfos->setVisible( true );
    this->btnConnectRelation->setVisible( false );
    this->btnConnectRelationSave->setVisible( true );
    this->btnConnectRelationCancel->setVisible( true );

// show the selected info
//    this->srcInfos->setText( this->srcItemID + ": " + this->srcDislpayName );

// disable all tables
    this->dbTree->disableAllTables();

// remeber the table/itemid for later use of the connect mode
    this->itemSelected->item( &this->srcTable, &this->srcItemID, NULL );

// enable only tables with an relation
    const char *srcColumn = NULL;
    const char *relatedTable = NULL;
    const char *relatedColumn = NULL;
    this->dbRelation->relationGetReset();

    while( this->dbRelation->relatedTableGetNext( this->srcTable.toUtf8(), NULL, &relatedTable, &relatedColumn ) ){
        //this->dbTree->enableTable( relatedTable );
        this->dbTree->enableTableItem( relatedTable );
    }

// now we activate the connection-mode
    this->relSelected = false;
    this->connectionMode = true;
}


void doDBRelationPlugin::           connectionCheck(){

// enable only tables with an relation
    const char *srcPrimaryKeyColumn;
    const char *srcColumn = NULL;
    const char *srcColumnValue = NULL;
    const char *relatedTable = NULL;
    const char *relatedColumn = NULL;
    const char *relatedColumnValueChar = NULL;
    QString relatedColumnValue = "";


// remeber the table/itemid for later use of the connect mode
    this->itemSelected->item( &this->relTable, &this->relItemID, NULL );


    this->dbRelation->relationGetReset();
    if( this->dbRelation->relatedTableFindNext( this->srcTable.toUtf8(), &srcColumn, this->relTable.toUtf8(), &relatedColumn ) ){

    // get connection
        doDBConnection *connection = this->itemSelected->connection();
        if( connection == NULL ) return;

    // check if the column where we would like to write into is not the primary key !
        if( etDBObjectTablePick( connection->dbObject, this->srcTable.toUtf8() ) != etID_YES ) return;
        if( etDBObjectTableColumnPrimaryGet( connection->dbObject, srcPrimaryKeyColumn ) != etID_YES ) return;
        if( QString(srcColumn) == srcPrimaryKeyColumn ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Your action will overwrite the uuid of your source, this is forbidden !" );
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

        this->relSelected = true;

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


void doDBRelationPlugin::           connectionCancel(){

// show / hide
    this->srcInfos->setVisible( false );
    this->btnConnectRelation->setVisible( true );
    this->btnConnectRelationSave->setVisible( false );
    this->btnConnectRelationCancel->setVisible( false );

// disable all tables
    this->dbTree->disableAllTables(false);

// disable connection mode
    this->relSelected = false;
    this->connectionMode = false;
}


void doDBRelationPlugin::           connectionSave(){
// no correct relation selected
    if( this->relSelected != true ){
        return;
    }

// get value of related column
    doDBConnection *connection = this->itemSelected->connection();
    if( connection == NULL ) return;

    QString itemTableName;
    QString itemID;
    this->itemSelected->item( &itemTableName, &itemID, NULL );

// get related data
    if( ! connection->dbDataGet( itemTableName.toUtf8(), itemID.toUtf8() ) ) return;

// get relColumn-Value
    const char*     relColumnValueChar = NULL;
    QString         relColumnValue;
    if( etDBObjectValueGet( connection->dbObject, this->relColumn.toUtf8(), relColumnValueChar ) != etID_YES ) return;
    relColumnValue = relColumnValueChar;

// get source data
    if( ! connection->dbDataGet( this->srcTable.toUtf8(), this->srcItemID.toUtf8() ) ) return;

// set the src-column with the related-colum
    etDBObjectValueSet( connection->dbObject, this->srcColumn.toUtf8(), relColumnValue.toUtf8() );
    connection->dbDataChange( this->srcTable.toUtf8() );

// finish
    this->connectionCancel();

// delete child items from source-item
//    qDeleteAll( this->srcItem->takeChildren() );
//    this->srcItem->setChildIndicatorPolicy( QTreeWidgetItem::DontShowIndicator );
//    this->srcItem->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

// fire the plugins
    doDBPlugins::ptr->handleAction( "itemClicked", this->itemSelected );
}















