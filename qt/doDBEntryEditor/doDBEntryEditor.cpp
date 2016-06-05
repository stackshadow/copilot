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

#include "doDBEntryEditor.h"


#include <QUuid>


doDBEntryEditor::               doDBEntryEditor( QWidget *parent ) : QWidget(parent){

// setup ui
    this->ui.setupUi( this );

    this->ui.tableWidget->setColumnCount(4);
    this->ui.tableWidget->setColumnHidden( 0, true );
    this->ui.tableWidget->setColumnHidden( 2, true );
    //this->ui.tableWidget->setColumnHidden( 4, true );

    this->editMode = doDBEntryEditor::modeChanged;

// connections
    connect( this->ui.btnNew , SIGNAL (clicked()), this, SLOT (entryCreate()));
    connect( this->ui.btnSave , SIGNAL (clicked()), this, SLOT (entrySave()));


}

doDBEntryEditor::               ~doDBEntryEditor(){

}



void doDBEntryEditor::          dbObjectShow( etDBObject *dbObject, QString tableName ){


// vars
    const char      *tableNameActual;
    const char      *columnName;
    const char      *columnDisplayName;     // the human readable column name ;)
    const char      *columnValue;
    const char      *primaryKeyColumnName = NULL;


// get the selected table
    etDBObjectTableNameGet( dbObject, tableNameActual );




// fill columns
    if( tableName != this->ui.lineTableName->text() ){

    // clear
        this->ui.tableWidget->setRowCount(0);
        this->ui.tableWidget->clear();

    // initial set the
        etDBObjectIterationReset( dbObject );
        while( etDBObjectTableColumnNext( dbObject, columnName ) == etID_YES ){

            etDBObjectTableColumnDisplayNameGet( dbObject, "", columnDisplayName );


            this->columnAppend( columnName, columnDisplayName );
        }

    // get the primary key column
        if( etDBObjectTableColumnPrimaryGet(dbObject,primaryKeyColumnName) == etID_YES ){

            int primaryKeyRow = this->columnFind(primaryKeyColumnName);
            if( primaryKeyRow >= 0 ){
                this->primaryKeyRow = primaryKeyRow;
            }


        }

    // set the actual stuff
        this->dbObject = dbObject;
        this->ui.lineTableName->setText(tableName);
    }



// fill selected data
    this->valueCleanAll();

// show values
    etDBObjectIterationReset( this->dbObject );
    while( etDBObjectTableColumnNext(this->dbObject, columnName) == etID_YES ){

        if( etDBObjectValueGet( this->dbObject, columnName, columnValue ) == etID_YES ){
            this->valueSet( this, columnName, columnValue );
        }

    }

// set the mode to change
    this->editMode = modeChanged;

}



void doDBEntryEditor::          valueCleanAll(){
// vars
    int                 rowIndex = 0;
    int                 rowCount = this->ui.tableWidget->rowCount();
    QTableWidgetItem    *tempItem = NULL;
    bool                columnFound = false;

    for( rowIndex = 0; rowIndex < rowCount; rowIndex++ ){
        tempItem = this->ui.tableWidget->item( rowIndex, 2 );
        if( tempItem != NULL ) tempItem->setText( "");
        tempItem = this->ui.tableWidget->item( rowIndex, 3 );
        if( tempItem != NULL ) tempItem->setText( "");
    }
}

void doDBEntryEditor::          valueSet( void *userdata, const char *columnName, const char *columnValue ){

// vars
    doDBEntryEditor     *entryEditor = (doDBEntryEditor*)userdata;
    int                 rowIndex = 0;
    int                 rowCount = entryEditor->ui.tableWidget->rowCount();
    QTableWidgetItem    *tempItem = NULL;
    bool                columnFound = false;

    for( rowIndex = 0; rowIndex < rowCount; rowIndex++ ){
        tempItem = entryEditor->ui.tableWidget->item( rowIndex, 0 );
        if( tempItem->text() == columnName ){
            columnFound = true;
            break;
        }
    }

    if( columnFound == false ){
        return;
    }

// nothing found add a new column
    tempItem = new QTableWidgetItem( QString(columnValue) );
    entryEditor->ui.tableWidget->setItem( rowIndex, 2, tempItem );

    tempItem = new QTableWidgetItem( QString(columnValue) );
    entryEditor->ui.tableWidget->setItem( rowIndex, 3, tempItem );


}

int doDBEntryEditor::           columnAppend( QString columnName, QString columnDisplayName ){

// vars
    int                     rowIndex = 0;
    QTableWidgetItem        *item = NULL;

// append a column
    rowIndex = this->ui.tableWidget->rowCount();
    this->ui.tableWidget->setRowCount(rowIndex + 1);

// 0 - columnName
    item = new QTableWidgetItem( QString(columnName) );
    this->ui.tableWidget->setItem( rowIndex, 0, item );

// 1 - columnDisplayName
    item = new QTableWidgetItem( QString(columnDisplayName) );
    item->setFlags( Qt::ItemIsEnabled );
    this->ui.tableWidget->setItem( rowIndex, 1, item );

// 2 - oldValue
    item = new QTableWidgetItem( QString("") );
    this->ui.tableWidget->setItem( rowIndex, 2, item );

// 3 - newValue
    item = new QTableWidgetItem( QString("") );
    this->ui.tableWidget->setItem( rowIndex, 3, item );

    return rowIndex;
}

int doDBEntryEditor::           columnFind( QString columnName ){
// vars
    int                 rowIndex = 0;
    int                 rowCount = this->ui.tableWidget->rowCount();
    QTableWidgetItem    *tempItem = NULL;

    for( rowIndex = 0; rowIndex < rowCount; rowIndex++ ){
        tempItem = this->ui.tableWidget->item( rowIndex, 0 );
        if( tempItem != NULL ){
            if( tempItem->text() == columnName ){
                return rowIndex;
            }
        }
    }

    return -1;
}



QString doDBEntryEditor::       columnName( int row ){
    return this->ui.tableWidget->item( row, 0 )->text();
}

QString doDBEntryEditor::       columnDisplayName( int row ){
    return this->ui.tableWidget->item( row, 1 )->text();
}

QString doDBEntryEditor::       oldValue( int row ){
    return this->ui.tableWidget->item( row, 2 )->text();
}

QString doDBEntryEditor::       newValue( int row ){
    return this->ui.tableWidget->item( row, 3 )->text();
}





void doDBEntryEditor::          entryCreate(){

// vars
    int                     rowIndex = 0;
    QTableWidgetItem        *item = NULL;
    QString                 uuid;

// set the mode to create
    this->editMode = modeCreate;

// creat uuid
    uuid = QUuid::createUuid().toString();
    uuid.replace( "{", "" );
    uuid.replace( "}", "" );


    item = new QTableWidgetItem( uuid );
    this->ui.tableWidget->setItem( this->primaryKeyRow, 3, item );


}

void doDBEntryEditor::          entrySave(){

// save widget values to dbObject

// vars
    int                 rowIndex = 0;
    int                 rowCount = this->ui.tableWidget->rowCount();
    QTableWidgetItem    *tempItem = NULL;
    bool                columnFound = false;
    QString             columnName;
    QString             columnValue;

    for( rowIndex = 0; rowIndex < rowCount; rowIndex++ ){
        columnName = this->columnName( rowIndex );
        columnValue = this->newValue( rowIndex );

        etDBObjectValueSet( this->dbObject, columnName.toUtf8(), columnValue.toUtf8() );

    }



// according to the mode, create or change data
    if( this->editMode == doDBEntryEditor::modeCreate ){
        emit this->saveNew( this->dbObject, this->ui.lineTableName->text().toUtf8() );
        return;
    }
    if( this->editMode == doDBEntryEditor::modeChanged ){
        emit this->saveChanged( this->dbObject, this->ui.lineTableName->text().toUtf8() );
        return;
    }

}

void doDBEntryEditor::          entryDeleteStep1(){

}

void doDBEntryEditor::          entryDeleteStep2(){

}

