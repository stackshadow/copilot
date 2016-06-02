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
#include "doDBConnection/doDBConnection.h"

#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"


doDBRelationPlugin::            doDBRelationPlugin() : QObject() {


// create relation
    this->dbRelation = new doDBRelation();
    this->dbRelationEditor = NULL;
    this->connectionID = "";

    this->btnEditRelations = NULL;

// db tree
    this->dbTree = NULL;
    this->dbTreeItemTypeRelatedTable = -1;
    this->dbTreeSelectedItem = NULL;

}

doDBRelationPlugin::            ~doDBRelationPlugin(){

}




void doDBRelationPlugin::       doDBTreeInit( doDBtree *dbTree ){


// save
    this->dbTree = dbTree;

// setup types
    this->dbTreeItemTypeRelatedTable = dbTree->newItemType();


// connect
    connect( this->dbTree, SIGNAL (itemExpanded(QTreeWidgetItem*)), this, SLOT (doDBTreeExpand(QTreeWidgetItem*)));
    connect( this->dbTree, SIGNAL (itemCollapsed(QTreeWidgetItem*)), this, SLOT (doDBTreeCollapsed(QTreeWidgetItem*)));
    connect( this->dbTree, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (doDBTreeClicked(QTreeWidgetItem*,int)));


}


void doDBRelationPlugin::       doDBItemViewInit( QLayout *itemView ){


// edit button
    this->btnEditRelations = new QPushButton( "Relation Bearbeiten" );
    this->btnEditRelations->setVisible( false );
    connect( this->btnEditRelations, SIGNAL(clicked()), this, SLOT(editorShow()) );
    itemView->addWidget( this->btnEditRelations );




}




bool doDBRelationPlugin::       dbRelationLoad(){

// check
    if( this->connectionID.length() <= 0 ) return false;


// vars
    doDBConnection              *connection = NULL;
    const char                  *displayName = NULL;
    const char                  *dbValueString = NULL;


// get connection
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return false;

// get display name ( for debugging purpose )
    displayName = connection->displayNameGet();



    if( connection->dbDoDBValueGet( "doDBRelations", &dbValueString ) ){
        this->dbRelation->relationImport( dbValueString );
    }



}


bool doDBRelationPlugin::       dbRelationSave(){
// check
    if( this->connectionID.length() <= 0 ) return false;

// vars
    doDBConnection              *connection = NULL;
    const char                  *displayName = NULL;
    const char                  *jsonDump = NULL;

// get connection
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return false;

    this->dbRelation->relationExport( &jsonDump );
    if( jsonDump != NULL ){
        connection->dbDoDBValueSet( "doDBRelations", jsonDump );
        free( (void*)jsonDump );
    }




}




bool doDBRelationPlugin::       dbDataGet( const char *srcTable, const char *srcTableItemID, const char *relatedTable, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) ){

// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;
    QString         dbObjectLockID;
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *srcPrimaryColumn = NULL;
    const char      *srcPrimaryColumnValue = NULL;
    const char      *relDisplayColumn = NULL;
    const char      *relDisplayColumnValue = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// get connection
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return false;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return false;


// get the related columns
    bool relationPresent = this->dbRelation->relatedTableFindFirst( srcTable, &srcColumn, relatedTable, &relColumn );
    while( relationPresent ){


    // get all columns from selected row
        if( connection->dbDataGet( srcTable, srcTableItemID )  != true ) break;


    // okay, get the value of the source column
        etDBObjectValueGet( dbObject, srcColumn, srcColumnValue );


    // create filter
        etDBObjectFilterClear( dbObject );
        etDBObjectFilterAdd(  dbObject, 1, etDBFILTER_OP_AND, relColumn, etDBFILTER_TYPE_EQUAL, srcColumnValue );

    // pick related table
        if( etDBObjectTablePick( dbObject, relatedTable ) != etID_YES ){
            return false;
        }

    // get the column which respresent the visible value the value from the related table
        etDBObjectTableColumnMainGet( dbObject, relDisplayColumn );
/*
    // run the query
        if( etDBDriverDataGet( this->dbDriver, dbObject ) != etID_YES ) {
            goto next;
        }


    // iterate through data
        while( etDBDriverDataNext( this->dbDriver, dbObject ) == etID_YES ){


            if( etDBObjectValueGet( this->dbObject, primaryColumn, primaryColumnValue ) != etID_YES ){
                continue;
            }


            if( etDBObjectValueGet( this->dbObject, relDisplayColumn, relDisplayColumnValue ) != etID_YES ){
                doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no display column use primaryKeyColumn").arg(relatedTable) );
                relDisplayColumnValue = primaryColumnValue;
            }

            callback( userdata, relatedTable, connID, primaryColumnValue, relDisplayColumnValue );

        }
*/
        next:
        relationPresent = this->dbRelation->relatedTableFindNext( &srcColumn, relatedTable, &relColumn );

    }

    return true;
}





void doDBRelationPlugin::       doDBTreeExpand( QTreeWidgetItem * item ){

// vars
    doDBConnection              *connection;
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;


// get Stuff from the selected item
    tableName = this->dbTree->itemTableName( item );
    connectionID = this->dbTree->itemConnectionID( item );
    itemID = this->dbTree->itemID( item );
    itemType = this->dbTree->itemType( item );


// if selected item is an entry
    if( itemType == doDBtree::typeEntry ){

    // item is selected
        connection = doDBCore->connectionGet( connectionID.toUtf8() );
        if( connection == NULL ){
            return;
        }

    // load relation from DB ( if needed )
        if( this->connectionID != connectionID ){
            this->connectionID = connectionID;
            this->dbRelationLoad();
        }


        etDBObject *dbObject = connection->dbObject;
        if( dbObject == NULL ) return;

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
            this->dbTree->append( item, tableDisplayName, relatedTable, connectionID.toUtf8(), "", doDBtree::typeRelatedTable );

        next:
            relatedTableExist = this->dbRelation->relatedTableGetNext( NULL, &relatedTable, NULL );
        }


        //connection->treeFillDataRelated( this->dataTree, item );

        //connection->dataGet( tableName.toUtf8(), itemID.toUtf8() );
    }







}


void doDBRelationPlugin::       doDBTreeCollapsed( QTreeWidgetItem * item ){

}


void doDBRelationPlugin::       doDBTreeClicked( QTreeWidgetItem * item, int column ){

// remember selected item
    this->dbTreeSelectedItem = item;


// vars
    doDBtree::treeItemType      itemType;
    QString                     connectionID;

// get Stuff from the selected item
    itemType = this->dbTree->itemType( this->dbTreeSelectedItem );
    connectionID = this->dbTree->itemConnectionID( this->dbTreeSelectedItem );

    if( itemType == doDBtree::typeTable ){
        this->connectionID = connectionID;
        this->btnEditRelations->setVisible(true);
    } else {
        this->btnEditRelations->setVisible(false);
        //this->tableFolderEditor->setVisible( false );
    }


}





void doDBRelationPlugin::       editorShow(){


// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;

// get connection
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return;

// load the relation from db
    this->dbRelationLoad();

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
    this->dbRelationSave();
}


