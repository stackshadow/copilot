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

//
    this->itemSelected = NULL;
    this->connectionSelected = NULL;
    this->srcEntry = NULL;

// register messages
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitToolBar, true );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitTree, true );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitStackedRight, true );

    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemExpanded );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemCollapsed );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemSelected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgItemDeleted );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionConnected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionDisconnected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionSelected );
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



bool doDBRelationPlugin::           recieveMessage( messageID type, void* payload ){

// entry handling
    doDBEntry*          entry = (doDBEntry*)payload;
    doDBConnection*     dbConnection = (doDBConnection*)payload;

    if( type == doDBPlugin::msgItemExpanded ){
        this->itemExpanded( entry );
        return true;
    }

    if( type == doDBPlugin::msgItemCollapsed ){
        this->itemCollapsed( entry );
        return true;
    }

    if( type == doDBPlugin::msgItemSelected ){
        this->itemClicked( entry );
        return true;
    }

    if( type == doDBPlugin::msgItemDeleted ){
        doDBEntry* entry = (doDBEntry*)payload;

        if( this->itemSelected == entry ){
            doDBEntry::decRef( &this->itemSelected );
            this->itemSelected = NULL;
        }

        return true;
    }

    if( type == doDBPlugin::msgConnectionConnected ){
        this->btnEditRelations->setVisible(true);           // enable button
        this->connectionSelected = dbConnection;            // remember connection
        return true;
    }

    if( type == doDBPlugin::msgConnectionDisconnected ){
        this->btnEditRelations->setVisible(false);          // disable button
        this->connectionSelected = NULL;                    // forget connection
        return true;
    }

    if( type == doDBPlugin::msgConnectionSelected ){

        if( dbConnection == NULL ){
            this->btnEditRelations->setVisible(false);
            return true;
        }

        if( dbConnection->isConnected() == true ){
            this->btnEditRelations->setVisible(true);
            this->connectionSelected = dbConnection;
        } else {
            this->btnEditRelations->setVisible(false);
            this->connectionSelected = NULL;
        }

        return true;
    }

// stuff that normally run once
    QLayout*        layout = (QLayout*)payload;
    QGroupBox*      toolBarGroup = NULL;

    if( type == doDBPlugin::msgInitToolBar ){
        toolBarGroup = new QGroupBox( "Relation" );
        toolBarGroup->setLayout( new QHBoxLayout() );
        toolBarGroup->setLayoutDirection( Qt::LeftToRight );
        toolBarGroup->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

        this->btnEditRelations = new QPushButton( "Relation\nBearbeiten" );
        this->btnEditRelations->setVisible(false);
        connect( this->btnEditRelations, SIGNAL(clicked()), this, SLOT(editorShow()) );
        toolBarGroup->layout()->setMargin(1);
        toolBarGroup->layout()->addWidget( this->btnEditRelations );

    // basic infos
        this->srcInfos = new QLineEdit();
        this->srcInfos->setReadOnly(true);
        this->srcInfos->setVisible( false );
        toolBarGroup->layout()->addWidget( this->srcInfos );

    // connect button
        this->btnConnectRelation = new QPushButton( "Verbinden" );
        this->btnConnectRelation->setVisible( false );
        connect( this->btnConnectRelation, SIGNAL(clicked()), this, SLOT(connectionStart()) );
        toolBarGroup->layout()->addWidget( this->btnConnectRelation );

        this->btnConnectRelationSave = new QPushButton( "Verbinden abschießen" );
        this->btnConnectRelationSave->setVisible( false );
        connect( this->btnConnectRelationSave, SIGNAL(clicked()), this, SLOT(connectionSave()) );
        toolBarGroup->layout()->addWidget( this->btnConnectRelationSave );

        this->btnConnectRelationCancel = new QPushButton( "Verbinden abbrechen" );
        this->btnConnectRelationCancel->setVisible( false );
        connect( this->btnConnectRelationCancel, SIGNAL(clicked()), this, SLOT(connectionCancel()) );
        toolBarGroup->layout()->addWidget( this->btnConnectRelationCancel );

        layout->addWidget( toolBarGroup );
        return true;
    }

    if( type == doDBPlugin::msgInitTree ){
    // save
        this->dbTree = (doDBtree*)payload;
    // setup types
        this->dbTreeItemTypeRelatedTable = this->dbTree->newItemType();

        return true;
    }

    if( type == doDBPlugin::msgInitStackedRight ){
        this->stackedWidget = (QStackedWidget*)payload;
    }


    return true;
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
    connectionID = entry->connectionID();
    connection = entry->connection();
    entry->item( &tableName, &itemID, (int*)&itemType, NULL );

// the connection we need
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
            this->dbTree->append( entry->treeWidgetItem(), tableDisplayName, relatedTable, connectionID.toUtf8(), "", this->dbTreeItemTypeRelatedTable );

        }

    }

// if we expand an related Table -> show related data
    if( itemType == this->dbTreeItemTypeRelatedTable ){

    // get the parent table name
        QTreeWidgetItem* parentItem = entry->treeWidgetItem()->parent();
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
    entry->item( NULL, NULL, &itemType, NULL );


    if( itemType == this->dbTreeItemTypeRelatedTable ){
        qDeleteAll( entry->treeWidgetItem()->takeChildren() );
    }

    return true; // next plugin
}


bool doDBRelationPlugin::           itemClicked( doDBEntry* entry ){


// vars
    QString                     tableName;
    QString                     itemID;
    doDBtree::treeItemType      itemType;


// remember
    doDBEntry::decRef( &this->itemSelected );
    this->itemSelected = entry;
    this->itemSelected->incRef();
    this->itemSelected->item( &tableName, &itemID, (int*)&itemType, NULL );

// clicked on table
    if( itemType == doDBtree::typeTable ){

    // connect mode
        if( this->connectionMode ){
            this->btnConnectRelation->setVisible(false);
            this->btnConnectRelationSave->setVisible(false);
            this->btnConnectRelationCancel->setVisible(false);
        }
    }

// clicked on entry
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
    connection = this->connectionSelected;
    if( connection == NULL ) return;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return;

// load the relation from db
    this->dbRelation->dbRelationLoad( connection );

// create if needed
    if( this->dbRelationEditor == NULL ){
        this->dbRelationEditor = new doDBRelationEditor(NULL);
        connect( this->dbRelationEditor, SIGNAL(closed()), this, SLOT(editorClosed()) );

    // show it
        if( this->stackedWidget != NULL ){
            this->stackedWidget->addWidget( this->dbRelationEditor );
            this->stackedWidget->setCurrentWidget( this->dbRelationEditor );
            this->stackedWidget->setVisible(true);
        }
    }



// init it
    this->dbRelationEditor->showRelation( dbObject, this->dbRelation );
    this->dbRelationEditor->setVisible(true);

}


void doDBRelationPlugin::           editorClosed(){
    delete this->dbRelationEditor;
    this->dbRelationEditor = NULL;
    this->dbRelation->dbRelationSave( this->connectionSelected );
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
    this->srcEntry = this->itemSelected;
    this->srcEntry->incRef();
    this->srcEntry->item( &this->srcTable, &this->srcItemID, NULL, NULL );

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
    this->itemSelected->item( &this->relTable, &this->relItemID, NULL, NULL );


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

// enable all tables
    this->dbTree->disableAllTables(false);

// disable connection mode
    this->relSelected = false;
    this->connectionMode = false;

// release our srcEntry ( if needed )
    doDBEntry::decRef( &this->srcEntry );
    this->srcEntry = NULL;
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
    this->itemSelected->item( &itemTableName, &itemID, NULL, NULL );

// get related data
    if( ! connection->dbDataRead( itemTableName.toUtf8(), itemID.toUtf8() ) ) return;

// get relColumn-Value
    const char*     relColumnValueChar = NULL;
    QString         relColumnValue;
    if( etDBObjectValueGet( connection->dbObject, this->relColumn.toUtf8(), relColumnValueChar ) != etID_YES ) return;
    relColumnValue = relColumnValueChar;

// get source data
    if( ! connection->dbDataRead( this->srcTable.toUtf8(), this->srcItemID.toUtf8() ) ) return;

// set the src-column with the related-colum
    etDBObjectValueSet( connection->dbObject, this->srcColumn.toUtf8(), relColumnValue.toUtf8() );
    connection->dbDataChange( this->srcTable.toUtf8() );

// fire the plugins
    doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgItemSelected, this->srcEntry );

// finish
    this->connectionCancel();

// delete child items from source-item
//    qDeleteAll( this->srcItem->takeChildren() );
//    this->srcItem->setChildIndicatorPolicy( QTreeWidgetItem::DontShowIndicator );
//    this->srcItem->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

}















