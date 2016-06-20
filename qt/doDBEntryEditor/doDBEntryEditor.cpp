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

doDBEntryEditor *doDBEntryEditor::ptr = NULL;

doDBEntryEditor::               doDBEntryEditor( QWidget *parent ) : QWidget(parent){
// save pointer
    this->ptr = this;

// setup ui
    this->ui.setupUi( this );

    this->ui.tableWidget->setColumnCount(4);
    this->ui.tableWidget->setColumnHidden( 0, true );
    this->ui.tableWidget->setColumnHidden( 2, true );
    //this->ui.tableWidget->setColumnHidden( 4, true );




    this->tableName = "";
    this->editMode = doDBEntryEditor::modeNothing;
    this->showButtons();

// connections
    connect( this->ui.btnEdit, SIGNAL( toggled(bool) ), this, SLOT( btnEntryEditClicked(bool) ) );
    connect( this->ui.btnNew , SIGNAL (clicked()), this, SLOT( btnEntryCreateClicked() ) );
    connect( this->ui.btnCopy , SIGNAL (clicked()), this, SLOT( btnEntryCopyClicked() ) );
    connect( this->ui.btnSave , SIGNAL (clicked()), this, SLOT( btnEntrySaveClicked() ) );
    connect( this->ui.btnDelete , SIGNAL (clicked()), this, SLOT( btnEntryDeleteReqClicked() ) );
    connect( this->ui.btnDeleteAck , SIGNAL (clicked()), this, SLOT( btnEntryDeleteAckClicked() ) );
    connect( this->ui.btnDeleteCancel , SIGNAL (clicked()), this, SLOT( btnEntryDeleteCancelClicked() ) );


}

doDBEntryEditor::               ~doDBEntryEditor(){
    this->ptr = NULL;
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




// read all columns from dbObject and add it to the table
    if( this->tableName != tableName ){

    // clear
        this->ui.tableWidget->setRowCount(0);
        this->ui.tableWidget->clear();
        this->ui.tableWidget->setSortingEnabled( false );

    // initial set the
        etDBObjectIterationReset( dbObject );
        while( etDBObjectTableColumnNext( dbObject, columnName ) == etID_YES ){

            etDBObjectTableColumnDisplayNameGet( dbObject, "", columnDisplayName );


            this->columnAppend( columnName, columnDisplayName );
        }
        this->ui.tableWidget->setSortingEnabled( true );
        this->ui.tableWidget->sortItems( 0, Qt::AscendingOrder );

    // get the primary key column
        this->primaryKeyRow = -1;
        if( etDBObjectTableColumnPrimaryGet(dbObject,primaryKeyColumnName) == etID_YES ){

            int primaryKeyRow = this->columnFind(primaryKeyColumnName);
            if( primaryKeyRow >= 0 ){
                this->primaryKeyRow = primaryKeyRow;
            }


        }

    // set the actual stuff
        this->dbObject = dbObject;
        this->tableName = tableName;
    }


// clean all values in the table
    this->valueCleanAll();

// show values
    etDBObjectIterationReset( this->dbObject );
    while( etDBObjectTableColumnNext(this->dbObject, columnName) == etID_YES ){

        if( etDBObjectValueGet( this->dbObject, columnName, columnValue ) == etID_YES ){
            this->valueSet( this, columnName, columnValue );
        }

    }

// resize to content
    this->ui.tableWidget->resizeColumnsToContents();

// set the mode to change
    this->editMode = doDBEntryEditor::modeView;
    this->showButtons();

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

void doDBEntryEditor::          showButtons(){

// default
    this->ui.btnNew->setVisible(true);
    this->ui.btnCopy->setVisible(true);
    this->ui.btnEdit->setVisible(true);
    this->ui.btnDelete->setVisible(true);
    this->ui.btnDeleteAck->setVisible(false);
    this->ui.btnDeleteCancel->setVisible(false);
    this->ui.btnSave->setVisible(false);

    switch( this->editMode ){

        case doDBEntryEditor::modeView:
            this->ui.btnNew->setChecked(false);
            this->ui.btnEdit->setChecked(false);
            this->ui.tableWidget->setEditTriggers( QAbstractItemView::NoEditTriggers );
            break;

        case doDBEntryEditor::modeCreate:
            this->ui.btnNew->setVisible(false);
            this->ui.btnCopy->setVisible(false);
            this->ui.btnEdit->setVisible(false);
            this->ui.btnDelete->setVisible(false);
            this->ui.btnSave->setVisible(true);
            this->ui.tableWidget->setEditTriggers( QAbstractItemView::AllEditTriggers );
            break;

        case doDBEntryEditor::modeEdit:
            this->ui.btnNew->setVisible(false);
            this->ui.btnCopy->setVisible(false);
            this->ui.btnDelete->setVisible(false);
            this->ui.tableWidget->setEditTriggers( QAbstractItemView::AllEditTriggers );
            this->ui.btnSave->setVisible(true);
            break;

        case doDBEntryEditor::modeRemove:
            this->ui.btnNew->setVisible(false);
            this->ui.btnCopy->setVisible(false);
            this->ui.btnEdit->setVisible(false);
            this->ui.btnDeleteAck->setVisible(false);
            this->ui.btnDeleteCancel->setVisible(true);
            break;

        default:
            this->ui.btnNew->setVisible(false);
            this->ui.btnCopy->setVisible(false);
            this->ui.btnEdit->setVisible(false);
            this->ui.btnDelete->setVisible(false);
            break;
    }



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





void doDBEntryEditor::          btnEntryEditClicked( bool checked ){

    if( checked == true ){
        this->editMode = doDBEntryEditor::modeEdit;
        this->showButtons();

        emit this->entryEditStart( this->dbObject, this->tableName.toUtf8() );
    } else {
        this->editMode = doDBEntryEditor::modeView;
        this->showButtons();

        emit this->entryEditAbort( this->dbObject, this->tableName.toUtf8() );
    }


}

void doDBEntryEditor::          btnEntryCreateClicked(){

// clear all values
    this->valueCleanAll();

// copy the empty stuff
    this->btnEntryCopyClicked();

}

void doDBEntryEditor::          btnEntryCopyClicked(){
// vars
    int                     rowIndex = 0;
    QTableWidgetItem        *item = NULL;
    QString                 uuid;

// creat uuid
    uuid = QUuid::createUuid().toString();
    uuid.replace( "{", "" );
    uuid.replace( "}", "" );

    item = new QTableWidgetItem( uuid );
    this->ui.tableWidget->setItem( this->primaryKeyRow, 3, item );

// set the mode to create
    this->editMode = modeCreate;
    this->showButtons();
}

void doDBEntryEditor::          btnEntrySaveClicked(){

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
        emit this->entryNew( this->dbObject, this->tableName.toUtf8() );
    }
    if( this->editMode == doDBEntryEditor::modeEdit ){
        emit this->entryChanged( this->dbObject, this->tableName.toUtf8() );
    }

// set the mode to create
    this->editMode = modeView;
    this->showButtons();
}

void doDBEntryEditor::          btnEntryDeleteReqClicked(){
// ack
    this->ui.btnDelete->setVisible(false);
    this->ui.btnDeleteAck->setVisible(true);
    this->ui.btnDeleteCancel->setVisible(true);
}

void doDBEntryEditor::          btnEntryDeleteAckClicked(){

    if( this->primaryKeyRow < 0 ){
        return;
    }

// get the primary key value
    QString primaryKeyValue = this->newValue( this->primaryKeyRow );

    emit this->entryDelete( this->dbObject, this->tableName.toUtf8(), primaryKeyValue.toUtf8() );

// close
    this->btnEntryDeleteCancelClicked();

// set the mode to create
    this->editMode = modeNothing;
    this->showButtons();
}

void doDBEntryEditor::          btnEntryDeleteCancelClicked(){
// hide new/save
    this->ui.btnNew->setVisible(true);
    this->ui.btnSave->setVisible(true);

// ack
    this->ui.btnDeleteAck->setVisible(false);
    this->ui.btnDeleteCancel->setVisible(false);
}





