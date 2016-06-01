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

#include <QFileDialog>
#include <QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QStackedWidget>

#include "doDBConnection/doDBConnection.h"
#include "doDBTableEditor/doDBTableEditor.h"

#include "doDBConnectionEditor.h"

#include "main.h"


doDBConnectionEditor::              doDBConnectionEditor( QWidget *parent ) : QWidget(parent){

// save
    this->selectedConnection = NULL;

// setup ui
    this->ui.setupUi( this );
    this->ui.tblConnection->setEditTriggers( QAbstractItemView::NoEditTriggers );
    this->ui.tblConnection->setColumnCount(2);
    this->ui.tblConnection->setColumnHidden(0,true);


// hide stuff
    this->ui.btnConnectionRemove->setEnabled(false);
    this->ui.btnConnect->setEnabled(false);
    this->ui.btnDisconnect->setEnabled(false);
    this->ui.btnTableEdit->setEnabled(false);
    this->ui.grpConnectionEdit->setVisible(false);

// fill the connection table
    this->tblConnectionRefresh();

// set mode to nothing
    this->mode = doDBConnectionEditor::MODE_NOTHING;

// buttons
    connect( this->ui.btnConnectionAdd, SIGNAL( clicked() ), this, SLOT( connectionAdd() ) );
    connect( this->ui.btnConnectionSave, SIGNAL( clicked() ), this, SLOT( connectionSave() ) );
    connect( this->ui.btnConnectionRemove, SIGNAL( clicked() ), this, SLOT( connectionDelete() ) );
    connect( this->ui.btnConnect, SIGNAL( clicked() ), this, SLOT( dbConnect() ) );
    connect( this->ui.btnTableEdit, SIGNAL( clicked() ), this, SLOT( tablesEdit() ) );
    connect( this->ui.btnClose, SIGNAL( clicked() ), this, SLOT( closeClicked() ) );

// default signals
    connect( this->ui.tblConnection, SIGNAL( clicked(const QModelIndex) ), this, SLOT( showSelectedConnection() ) );

// SQLite-Stuff
    connect( this->ui.btnSQLiteFileNameSelect, SIGNAL( clicked() ), this, SLOT( selectSQLiteFilename() ) );


}

doDBConnectionEditor::              ~doDBConnectionEditor(){

}




void doDBConnectionEditor::         tblConnectionRefresh(){

// vars
    doDBConnection      *dbConnection = NULL;
    int                 rowIndex = 0;


// remove table rows
    this->ui.tblConnection->setRowCount(0);

    dbConnection = doDBCore->connectionGetFirst();
    while( dbConnection != NULL ){

        this->ui.tblConnection->insertRow( this->ui.tblConnection->rowCount() );

        QTableWidgetItem *newItem = NULL;
        newItem = new QTableWidgetItem( dbConnection->UUIDGet() );
        this->ui.tblConnection->setItem( rowIndex, 0, newItem );
        newItem = new QTableWidgetItem( dbConnection->displayNameGet() );
        this->ui.tblConnection->setItem( rowIndex, 1, newItem );

        rowIndex = rowIndex + 1;
        dbConnection = doDBCore->connectionGetNext();
    }

    this->ui.tblConnection->resizeColumnsToContents();
}


void doDBConnectionEditor::         showSelectedConnection(){

// get the selection
    int         selectedRow = this->ui.tblConnection->currentRow();
    QString     selectedUUID = this->ui.tblConnection->item( selectedRow, 0 )->text();
    QString     displayName;



// get the connection with id
    this->selectedConnection = doDBCore->connectionGet( selectedUUID.toUtf8() );
    if( this->selectedConnection == NULL ) return;

// set values from connections
    this->ui.lineEditConnectionID->setText( selectedUUID );

    displayName = this->selectedConnection->displayNameGet();
    this->ui.lineEditConnectionDisplayName->setText( displayName );

// set infos about the type of db
    switch( this->selectedConnection->typeGet() ){

    // SQLITE
        case doDBConnection::CONN_SQLITE:
        {
            QString fileName = this->selectedConnection->fileNameGet();

            this->ui.tabConnections->setCurrentIndex(0);
            this->ui.lineEditSQLiteFileName->setText( fileName );
            break;
        }

        default:
            break;
    }

// check connection state
    if( this->selectedConnection->isConnected() == true ){
        this->ui.btnConnect->setEnabled(false);
        this->ui.btnDisconnect->setEnabled(true);
        this->ui.btnTableEdit->setEnabled(true);
    } else {
        this->ui.btnConnect->setEnabled(true);
        this->ui.btnDisconnect->setEnabled(false);
        this->ui.btnTableEdit->setEnabled(false);
    }


// show connection values
    this->ui.btnConnectionRemove->setEnabled(true);
    this->ui.grpConnectionEdit->setVisible(true);

}


void doDBConnectionEditor::         connectionAdd(){

// create connection
    this->selectedConnection = new doDBConnection( doDBConnection::CONN_NOTHING, NULL, "new connection" );

// add connection
    doDBCore->connectionAppend( this->selectedConnection );

// refresh list
    this->tblConnectionRefresh();

}


void doDBConnectionEditor::         connectionSave(){

// check
    if( this->selectedConnection == NULL ) return;



// save display-name
    QString     displayName = this->ui.lineEditConnectionDisplayName->text();
    this->selectedConnection->displayNameSet( displayName.toUtf8() );

// set the type
// SQLITE
    int test = this->ui.tabConnections->currentIndex();
    if( test == 0 ){
        QString fileName = this->ui.lineEditSQLiteFileName->text();
        this->selectedConnection->typeSet( doDBConnection::CONN_SQLITE );
        this->selectedConnection->fileNameSet( fileName );
    }


// refresh list
    this->tblConnectionRefresh();



}


void doDBConnectionEditor::         connectionDelete(){
// check
    if( this->selectedConnection == NULL ) return;

// should be disconnected
    if( this->selectedConnection->isConnected() ) return;

// remove it from settings
    doDBCore->connectionRemove( this->selectedConnection );
    this->selectedConnection = NULL;

// refresh list
    this->tblConnectionRefresh();

}




void doDBConnectionEditor::         selectSQLiteFilename(){

    QString fileName =  QFileDialog::getSaveFileName(
                            this, tr("Save File"),
                            "",
                            tr("SQL-Database (*.sqlite)") );

    this->ui.lineEditSQLiteFileName->setText( fileName );
}


void doDBConnectionEditor::         dbConnect(){

// check
    if( this->selectedConnection == NULL ) return;

// set infos about the type of db
    if( this->selectedConnection->typeGet() == doDBConnection::CONN_SQLITE ){
        QString sqliteFileName = this->ui.lineEditSQLiteFileName->text();
        this->selectedConnection->fileNameSet( sqliteFileName );
    }

// connect
    this->selectedConnection->connect();


// check connection state
    if( this->selectedConnection->isConnected() == true ){
        this->ui.btnConnect->setEnabled(false);
        this->ui.btnDisconnect->setEnabled(true);
        this->ui.btnTableEdit->setEnabled(true);
    } else {
        this->ui.btnConnect->setEnabled(true);
        this->ui.btnDisconnect->setEnabled(false);
        this->ui.btnTableEdit->setEnabled(false);
    }


}


void doDBConnectionEditor::         dbDisconnect(){

}


void doDBConnectionEditor::         tablesEdit(){
// check
    if( this->selectedConnection == NULL ) return;

// vars
    etDBObject          *dbObject;
    QString             dbObjectLockID;

    if( ! this->selectedConnection->dbObjectGet(&dbObject, &dbObjectLockID) ) return;
    this->selectedConnection->dbObjectUnLock( dbObjectLockID );

    QStackedWidget      *stackedContainer = (QStackedWidget*)this->parentWidget();

    doDBTableEditor     *tableEditor = new doDBTableEditor( stackedContainer, dbObject );
    stackedContainer->addWidget( tableEditor );
    stackedContainer->setCurrentWidget( tableEditor );
    stackedContainer->setVisible( true );

    connect( tableEditor, SIGNAL( newTable(etDBObject*, QString) ), this, SLOT( tableNew(etDBObject*, QString) ) );
    connect( tableEditor, SIGNAL( newColumn(etDBObject*, QString, QString) ), this, SLOT( columnNew(etDBObject*, QString, QString) ) );
    connect( tableEditor, SIGNAL( closed() ), this, SLOT( tablesEditorClosed() ) );



}


void doDBConnectionEditor::         tableNew( etDBObject *dbObject, QString newTableName ){
    this->selectedConnection->tableAppend( newTableName.toUtf8() );
}


void doDBConnectionEditor::         columnNew(  etDBObject *dbObject, QString newTableName, QString newColumnName  ){
    this->selectedConnection->columnAppend( newTableName.toUtf8(), newColumnName.toUtf8() );
}


void doDBConnectionEditor::         tablesEditorClosed(){
// check
    if( this->selectedConnection == NULL ) return;

// just save it ;)
    this->selectedConnection->dbObjectSave();

}


void doDBConnectionEditor::         closeClicked(){
    emit finished();
}





