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

#include "main.h"

#include "doDBTree.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBPlugin/doDBPlugins.h"

doDBtree::                              doDBtree( QWidget *parent ) : QTreeWidget( parent ){

// set the last elemenet-id
    this->treeItemTypeLast = 10;
    this->treeColumnLast = 4;

    this->setColumnCount(5);
    this->setEnabled(false);
    this->setIconSize( QSize(25,25) );
    this->sortByColumn( 0, Qt::AscendingOrder );

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, "displayName" );
    item->setText( 1, "itemID" );
    item->setText( 2, "connectionID" );
    item->setText( 3, "table" );
    item->setText( 4, "type" );
    this->setHeaderItem(item);

    if( ! doDBSettings::ptr->debugModeEnabled() ){
        this->setColumnHidden( 1, true );
        this->setColumnHidden( 2, true );
        this->setColumnHidden( 3, true );
        this->setColumnHidden( 4, true );
        this->setHeaderHidden(true);
    }


// get data directory from settings
    this->picturePath = doDBSettings::ptr->treePictureDirectory();
    //doDBSettingsGlobal
    //this->dataDirectory = doDBSettingsGlobal->

    connect( this, SIGNAL (itemExpanded(QTreeWidgetItem*)), this, SLOT (expand(QTreeWidgetItem*)));
    connect( this, SIGNAL (itemCollapsed(QTreeWidgetItem*)), this, SLOT (collapsed(QTreeWidgetItem*)));
    connect( this, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (clicked(QTreeWidgetItem*,int)));

}

doDBtree::                              ~doDBtree(){
}




QTreeWidgetItem* doDBtree::             append( QTreeWidgetItem *parentItem, QString displayName, QString table, QString connectionID, QString itemId, int itemType ){

// create new item
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, displayName );
    item->setText( 1, itemId );
    item->setText( 2, connectionID );
    item->setText( 3, table );
    item->setText( 4, QString().number(itemType) );
    item->setText( 5, QString("") );


// show an image
    //QString iconFileName = "/media/martin/14d9efa3-5cdf-4657-9b34-8679f90d5529/home/stackshadow/Projects/Programming/dodb/pictures/";
    QString iconFileName = this->picturePath;
    iconFileName += "/";
    iconFileName += table;
    iconFileName += ".png";
    QIcon tableIcon = QIcon(iconFileName);
    item->setIcon( 0, QIcon(iconFileName) );

// we always have childs
    item->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

// top level or child ?
    if( parentItem == NULL ){
        this->addTopLevelItem(item);
    } else {
        parentItem->addChild(item);
    }

    return item;
}


QTreeWidgetItem* doDBtree::             append( QTreeWidgetItem *parentItem, QString displayName, QString table, QString connectionID, QString itemId, treeItemType itemType ){
    return this->append( parentItem, displayName, table, connectionID, itemId, (int)itemType );
}


QTreeWidgetItem* doDBtree::             find( QTreeWidgetItem *parentItem, QString displayName ){

    int                 itemCount = parentItem->childCount();
    int                 itemIndex = 0;
    QTreeWidgetItem     *actualItem = NULL;

    for( itemIndex = 0; itemIndex < itemCount; itemIndex++ ){
        actualItem = parentItem->child(itemIndex);
        if( actualItem->text(0) == displayName ){
            return actualItem;
        }
    }

    return NULL;
}


void doDBtree::                         disableAllTables( bool disable ){

//vars
    QTreeWidgetItem*        topLevelItem = NULL;
    int                     topLevelItemCount = 0;
    int                     topLevelItemIndex = 0;

// iterate items
    topLevelItemCount = this->topLevelItemCount();
    for( topLevelItemIndex = 0; topLevelItemIndex < topLevelItemCount; topLevelItemIndex++ ){

    // get item and disable it
        topLevelItem = this->topLevelItem(topLevelItemIndex);
        topLevelItem->setDisabled(disable);

    }

}


void doDBtree::                         enableTable( QString tableName ){
//vars
    QTreeWidgetItem*        topLevelItem = NULL;
    int                     topLevelItemCount = 0;
    int                     topLevelItemIndex = 0;

// iterate items
    topLevelItemCount = this->topLevelItemCount();
    for( topLevelItemIndex = 0; topLevelItemIndex < topLevelItemCount; topLevelItemIndex++ ){

    // find item and enable it
        topLevelItem = this->topLevelItem(topLevelItemIndex);
        if( doDBtree::itemTableName(topLevelItem) == tableName ){
            topLevelItem->setDisabled(false);
            return;
        }

    }
}


void doDBtree::                         enableTableItem( QString tableName ){
//vars
    QTreeWidgetItem*        topLevelItem = NULL;
    int                     topLevelItemCount = 0;
    int                     topLevelItemIndex = 0;

// get all items with the table name
    QList<QTreeWidgetItem*>     itemList = this->findItems( tableName, Qt::MatchExactly | Qt::MatchRecursive, 3 );
    QTreeWidgetItem*            item;
    doDBtree::treeItemType      itemType = doDBtree::typeNothing;



    foreach( item, itemList ){

        itemType = doDBtree::itemType(item);
        if( itemType == doDBtree::typeEntry ){

            topLevelItem = item->parent();
            while( topLevelItem != NULL ){
                topLevelItem->setDisabled(false);
                topLevelItem = topLevelItem->parent();
            }

            item->setDisabled(false);

        }

    }


}


int doDBtree::                          newItemType(){
    this->treeItemTypeLast++;
    return this->treeItemTypeLast++;
}

int doDBtree::                          newTreeColumn( QString columnName, bool hide ){

// increment
    this->treeColumnLast++;

// add column
    this->setColumnCount(this->treeColumnLast + 1);
    QTreeWidgetItem *headerItem = this->headerItem();
    headerItem->setText( this->treeColumnLast, columnName );

    if( ! doDBSettings::ptr->debugModeEnabled() ){
        this->setColumnHidden( this->treeColumnLast, hide );
    }

    return this->treeColumnLast;

}


QString doDBtree::                      itemDisplayValue(){
    return doDBtree::itemDisplayValue( this->selectedItem );
}

QString doDBtree::                      itemDisplayValue( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return "";

    return treeItem->text(0);

    return "";
}

QString doDBtree::                      itemID(){
    return doDBtree::itemID( this->selectedItem );
}

QString doDBtree::                      itemID( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return "";

    return treeItem->text(1);

    return "";
}

QString doDBtree::                      itemConnectionID(){
    return doDBtree::itemConnectionID( this->selectedItem );
}

QString doDBtree::                      itemConnectionID( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return "";

    QString             connectionID;
    QTreeWidgetItem*    parentItem = treeItem->parent();

    connectionID = treeItem->text(2);
    while( connectionID == "" && parentItem != NULL ){
        connectionID = parentItem->text(2);
    }

    return connectionID;
}

QString doDBtree::                      itemTableName(){
    return doDBtree::itemTableName( this->selectedItem );
}

QString doDBtree::                      itemTableName( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return "";

    return treeItem->text(3);

    return "";
}

doDBtree::treeItemType doDBtree::       itemType(){
    return doDBtree::itemType( this->selectedItem );
}

doDBtree::treeItemType doDBtree::       itemType( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return doDBtree::typeNothing;

    QString itemText = treeItem->text(4);
    return (doDBtree::treeItemType)itemText.toInt();

}



bool doDBtree::                         tableExist( QTreeWidgetItem *parentItem, QString tableName ){


    QTreeWidgetItem     *currentItem = NULL;
    int                 rowIndex = 0;
    int                 rowCount;


    currentItem = this->itemAt( 0, rowIndex );
    QList<QTreeWidgetItem*> foundItems = this->findItems( tableName, Qt::MatchCaseSensitive, 3 );

    if( foundItems.length() > 0 ) return true;


    return false;
}



void doDBtree::                         refresh(){

    etDBObject              *dbObject = NULL;
    QString                 dbLockID;
    doDBConnection          *dbConnection = NULL;
    const char              *tableName = NULL;
    const char              *tableDisplayName = NULL;
    bool                    tablePresent = false;

    dbConnection = doDBConnections::ptr->connectionGetFirst();
    while( dbConnection != NULL ){

    // connected ?
        if( ! dbConnection->isConnected() ) goto nextConnection;


        if( dbConnection->dbObject != NULL ){
            dbObject = dbConnection->dbObject;

            etDBObjectIterationReset( dbObject );
            while( etDBObjectTableNext( dbObject, tableName ) == etID_YES ){

                etDBObjectTableDisplayNameGet( dbObject, "", tableDisplayName );

                this->append( NULL, tableDisplayName, tableName, dbConnection->UUIDGet(), "", doDBtree::typeTable );
            }

        }

    nextConnection:
        dbConnection = doDBConnections::ptr->connectionGetNext();
    }

    this->tableExist(NULL,"device");

}

void doDBtree::                         expand( QTreeWidgetItem * item ){

// vars
    QString                     lockID = "doDBTree";
    doDBConnection              *connection;
    doDBtree::treeItemType      itemType;

    this->selectedItem = item;


// save the entry to global
    doDBEntry *dbEntry = new doDBEntry();
    doDBEntry::connectionIDSet( &dbEntry, doDBtree::itemConnectionID( item ) );
    doDBEntry::itemSet( &dbEntry, doDBtree::itemTableName( item ), doDBtree::itemID( item ), doDBtree::itemType( item ) );
    doDBEntry::treeWidgetItemSet( &dbEntry, item );
    doDBEntry::treeWidgetItemEnabledSet( &dbEntry, ! item->isDisabled() );
    dbEntry->incRef();


    itemType = (doDBtree::treeItemType)doDBtree::itemType( item );

// if selected item is a table
    if( itemType == doDBtree::typeTable ){

    // item is selected
        connection = dbEntry->connection();
        if( connection == NULL ){
            return;
        }

        connection->dbDataGet( doDBtree::itemTableName( item ).toUtf8(), this, doDBtree::callbackEntryAdd );


    }

// call the plugins
    doDBPlugins::ptr->handleAction( "itemExpanded", dbEntry );

    dbEntry->decRef();
}

void doDBtree::                         collapsed( QTreeWidgetItem * item ){


// vars
    QString                     lockID = "doDBTree";
    doDBConnection              *connection;
    doDBtree::treeItemType      itemType;

    this->selectedItem = item;



// save the entry to global
    doDBEntry *dbEntry = new doDBEntry();
    doDBEntry::connectionIDSet( &dbEntry, doDBtree::itemConnectionID( item ) );
    doDBEntry::itemSet( &dbEntry, doDBtree::itemTableName( item ), doDBtree::itemID( item ), doDBtree::itemType( item ) );
    doDBEntry::treeWidgetItemSet( &dbEntry, item );
    doDBEntry::treeWidgetItemEnabledSet( &dbEntry, ! item->isDisabled() );
    dbEntry->incRef();

    itemType = (doDBtree::treeItemType)doDBtree::itemType( item );

    if( itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){
        item->takeChildren();
    }

// fire all plugins
    doDBPlugins::ptr->handleAction( "itemCollapsed", dbEntry );
    dbEntry->decRef();
}

void doDBtree::                         clicked( QTreeWidgetItem * item, int column ){



// save the entry to global
    doDBEntry *dbEntry = new doDBEntry();

    doDBEntry::connectionIDSet( &dbEntry, doDBtree::itemConnectionID( item ) );
    doDBEntry::itemSet( &dbEntry, doDBtree::itemTableName( item ), doDBtree::itemID( item ), doDBtree::itemType( item ) );
    doDBEntry::treeWidgetItemSet( &dbEntry, item );
    doDBEntry::treeWidgetItemEnabledSet( &dbEntry, ! item->isDisabled() );
    dbEntry->incRef();


// before WE doing something with it, call all plugins
    doDBPlugins::ptr->handleAction( "itemClicked", dbEntry );
    dbEntry->decRef();

/*
    this->entryEditor->setEnabled(true);

// basic info about selected item in the tree
    QString connectionID = doDBtree::connectionID( item );
    QString primaryKeyValue = doDBtree::itemID( item );
    QString tableName = doDBtree::tableName( item );

// get connection id
    doDBConnection      *connection = doDBCore->connectionGet( connectionID.toUtf8() );
    if( connection == NULL ) return;

// show it in the editor
    this->entryEditor->showEntry( connection, tableName, primaryKeyValue );

*/




}

QTreeWidgetItem* doDBtree::             findItem( QString parentTableName ){

}



void doDBtree::                         callbackEntryAdd( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue ){

// the userdata are our doDBTree instance
    doDBtree *dbTree = (doDBtree*)userdata;

// append it
    dbTree->append(     dbTree->selectedItem,
                        displayValue,
                        tableName,
                        connID,
                        primaryValue,
                        doDBtree::typeEntry );



}



