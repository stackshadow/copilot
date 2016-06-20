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

#include "doDBEntry.h"
#include "qt/doDBDebug/doDBDebug.h"
#include "qt/doDBConnection/doDBConnections.h"

///doDBEntry* doDBEntry::ptr = NULL;


doDBEntry::doDBEntry(){

// remember this item
//    this->ptr = this;

// unlock
    this->referenceCount = 0;

// set default values
    this->connectionIdent = "";
    this->dbConnection = NULL;
    this->itemTableName = "";
    this->itemID = "";
    this->itemType = 0;

    this->treeItem = NULL;
    this->treeItemEnabled = false;
}


doDBEntry::~doDBEntry(){

}



void doDBEntry::                    incRef(){
    this->referenceCount = this->referenceCount + 1;
}


void doDBEntry::                    decRef(){
    this->referenceCount = this->referenceCount - 1;

    if( this->referenceCount <= 0 ){
        doDBDebug::ptr->print( "doDB", "INFO", __PRETTY_FUNCTION__, QString("%1: Entry not used anymore, delete it.").arg((long long int)this) );
        delete this;
    }
}


int doDBEntry::                     refCount(){
    return this->referenceCount;
}


bool doDBEntry::                    isWriteable( doDBEntry *dbEntry ){
    if( dbEntry->referenceCount == 0 ) return true;

    doDBDebug::ptr->print( "doDB", "WARNING", __PRETTY_FUNCTION__, QString("Can not write to entry, its in use...") );
    return false;
}




doDBEntry* doDBEntry::              requestWrite(){
// nobody use this, so we can write to it
    if( this->referenceCount == 0 ) return this;

// somebody use this, create a new one
    doDBEntry* newDBEntry = new doDBEntry();

// copy the stuff
    newDBEntry->connectionIdent = this->connectionIdent;
    newDBEntry->dbConnection = this->dbConnection;
    newDBEntry->itemTableName = this->itemTableName;
    newDBEntry->itemID = this->itemID;
    newDBEntry->itemType = this->itemType;
    newDBEntry->treeItem = this->treeItem;
    newDBEntry->treeItemEnabled = this->treeItemEnabled;

    doDBDebug::ptr->print( "doDB", "WARNING", __PRETTY_FUNCTION__, QString("Entry was in use, create a copy.") );

    return newDBEntry;
}




bool doDBEntry::                    connectionIDSet( doDBEntry **p_dbEntry, QString connectionID ){

    doDBEntry* dbEntry = *p_dbEntry;

// we would like to write to it
    dbEntry = dbEntry->requestWrite();

    const char *forDebug = connectionID.toUtf8();

// get connection pointer
    if( dbEntry->connectionIdent != connectionID ){
        dbEntry->connectionIdent = connectionID;
        dbEntry->dbConnection = doDBConnections::ptr->connectionGet( connectionID.toUtf8() );
    }

// return
    *p_dbEntry = dbEntry;
    return true;
}

QString doDBEntry::                 connectionID(){
    return this->connectionIdent;
}

doDBConnection* doDBEntry::         connection(){
    return this->dbConnection;
}




bool doDBEntry::                    itemSet( doDBEntry **p_dbEntry, QString tableName, QString itemID, int type ){

    doDBEntry* dbEntry = *p_dbEntry;

// we would like to write to it
    dbEntry = dbEntry->requestWrite();

// get connection pointer
    dbEntry->itemTableName = tableName;
    dbEntry->itemID = itemID;
    dbEntry->itemType = type;


// return
    *p_dbEntry = dbEntry;
    return true;
}

void doDBEntry::                    item( QString* tableName, QString* itemID, int* type ){

    if( tableName != NULL ){
        *tableName = this->itemTableName;
    }

    if( itemID != NULL ){
        *itemID = this->itemID;
    }

    if( type != NULL ){
        *type = this->itemType;
    }
}




bool doDBEntry::                    treeWidgetItemSet( doDBEntry **p_dbEntry, QTreeWidgetItem* treeItem ){

    doDBEntry* dbEntry = *p_dbEntry;

// we would like to write to it
    dbEntry = dbEntry->requestWrite();

// save
    dbEntry->treeItem = treeItem;

// return
    *p_dbEntry = dbEntry;
    return true;
}

QTreeWidgetItem* doDBEntry::        treeWidgetItem(){
    return this->treeItem;
}




bool doDBEntry::                    treeWidgetItemEnabledSet( doDBEntry **p_dbEntry, bool enabled ){

    doDBEntry* dbEntry = *p_dbEntry;

// we would like to write to it
    dbEntry = dbEntry->requestWrite();

// save
    dbEntry->treeItemEnabled = enabled;

// return
    *p_dbEntry = dbEntry;
    return true;
}

bool doDBEntry::                    treeWidgetItemEnabled(){
    return this->treeItemEnabled;
}


















