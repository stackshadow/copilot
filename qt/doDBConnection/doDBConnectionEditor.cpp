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

#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBTableEditor/doDBTableEditor.h"
#include "doDBPlugin/doDBPlugins.h"

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

    dbConnection = doDBConnections::ptr->connectionGetFirst();
    while( dbConnection != NULL ){

        this->ui.tblConnection->insertRow( this->ui.tblConnection->rowCount() );

        QTableWidgetItem *newItem = NULL;
        newItem = new QTableWidgetItem( dbConnection->UUIDGet() );
        this->ui.tblConnection->setItem( rowIndex, 0, newItem );
        newItem = new QTableWidgetItem( dbConnection->displayNameGet() );
        this->ui.tblConnection->setItem( rowIndex, 1, newItem );

        rowIndex = rowIndex + 1;
        dbConnection = doDBConnections::ptr->connectionGetNext();
    }

    this->ui.tblConnection->resizeColumnsToContents();
}


void doDBConnectionEditor::         showSelectedConnection(){

// get the selection
    int         selectedRow = this->ui.tblConnection->currentRow();
    QString     selectedUUID = this->ui.tblConnection->item( selectedRow, 0 )->text();
    QString     displayName;



// get the connection with id
    this->selectedConnection = doDBConnections::ptr->connectionGet( selectedUUID.toUtf8() );
    if( this->selectedConnection == NULL ) return;

// set values from connections
    this->ui.lineEditConnectionID->setText( selectedUUID );

    displayName = this->selectedConnection->displayNameGet();
    this->ui.lineEditConnectionDisplayName->setText( displayName );
    this->ui.lineEditUserName->setText( this->selectedConnection->username );

// set infos about the type of db
    switch( this->selectedConnection->typeGet() ){

    // SQLITE
        case doDBConnection::CONN_SQLITE:
        {
            QString fileName = this->selectedConnection->fileNameGet();
            this->ui.lineEditSQLiteFileName->setText( fileName );

            this->ui.tabConnections->setCurrentIndex(0);
            break;
        }

        case doDBConnection::CONN_POSTGRES:
        {
            this->ui.lineEditPQServer->setText( this->selectedConnection->hostname );
            this->ui.lineEditPQIP->setText( this->selectedConnection->hostip );
            this->ui.lineEditPQPort->setText( this->selectedConnection->port );
            this->ui.lineEditPQDB->setText( this->selectedConnection->database );
            this->ui.lineEditPQPassword->setText( this->selectedConnection->password );


            this->ui.tabConnections->setCurrentIndex(1);
            break;
        }

        default:
            break;
    }

// check connection state
    if( this->selectedConnection->isConnected() == true ){
        this->ui.btnConnect->setEnabled(false);
        this->ui.btnDisconnect->setEnabled(true);
    } else {
        this->ui.btnConnect->setEnabled(true);
        this->ui.btnDisconnect->setEnabled(false);
    }


// show connection values
    this->ui.btnConnectionRemove->setEnabled(true);
    this->ui.grpConnectionEdit->setVisible(true);

// fire message
    doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgConnectionSelected, this->selectedConnection );

}


void doDBConnectionEditor::         connectionAdd(){

// create connection
    this->selectedConnection = new doDBConnection( doDBConnection::CONN_NOTHING, NULL, "new connection" );

// add connection
    doDBConnections::ptr->connectionAppend( this->selectedConnection );

// refresh list
    this->tblConnectionRefresh();

}


void doDBConnectionEditor::         connectionSave(){

// check
    if( this->selectedConnection == NULL ) return;



// save display-name
    QString     displayName = this->ui.lineEditConnectionDisplayName->text();
    this->selectedConnection->displayNameSet( displayName.toUtf8() );
    this->selectedConnection->username = this->ui.lineEditUserName->text();

// set the type
// SQLITE
    int test = this->ui.tabConnections->currentIndex();
    if( test == 0 ){
        QString fileName = this->ui.lineEditSQLiteFileName->text();
        this->selectedConnection->typeSet( doDBConnection::CONN_SQLITE );
        this->selectedConnection->fileNameSet( fileName );
    }
// postgresql
    if( test == 1 ){
        this->selectedConnection->typeSet( doDBConnection::CONN_POSTGRES );

        this->selectedConnection->hostname = this->ui.lineEditPQServer->text();
        this->selectedConnection->hostip = this->ui.lineEditPQIP->text();
        this->selectedConnection->port = this->ui.lineEditPQPort->text();
        this->selectedConnection->database = this->ui.lineEditPQDB->text();
        this->selectedConnection->password = this->ui.lineEditPQPassword->text();

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
    doDBConnections::ptr->connectionRemove( this->selectedConnection );
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
    if( this->selectedConnection->connect() == true ){
        doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgConnectionConnected, this->selectedConnection );
    }

// check connection state
    if( this->selectedConnection->isConnected() == true ){
        this->ui.btnConnect->setEnabled(false);
        this->ui.btnDisconnect->setEnabled(true);
    } else {
        this->ui.btnConnect->setEnabled(true);
        this->ui.btnDisconnect->setEnabled(false);
    }


}


void doDBConnectionEditor::         dbDisconnect(){

    doDBPlugins::ptr->sendBroadcast( doDBPlugin::msgConnectionDisconnected, this->selectedConnection );
}




void doDBConnectionEditor::         closeClicked(){
    emit finished();
}





