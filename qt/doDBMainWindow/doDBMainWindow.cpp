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

#include "doDBMainWindow.h"

//#include "qt/moc_doDBMainWindow.cpp"
#include <QDebug>
#include <QStackedWidget>

#include "main.h"
#include "doDBConnection/doDBConnectionEditor.h"
#include "doDBFile/doDBFile.h"

doDBMainWindow::                doDBMainWindow( QWidget *parent ) : QWidget(parent){

// show ui
    this->ui.setupUi( this);
    this->show();



// left - tree
    this->dataTree = new doDBtree( this );
    this->ui.toolBox->addItem( this->dataTree, "Struktur" );
    connect( this->dataTree, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (treeElementClicked(QTreeWidgetItem*,int)));



// right item view
    this->itemViewLayout = new QVBoxLayout();
    QWidget *itemViewWidget = new QWidget( this );
    itemViewWidget->setLayout( this->itemViewLayout );
    this->ui.widgetContainerRight->addWidget( itemViewWidget );



// set our status bar as debug bar
    doDBDebug::ptr->registerDebugLine( this->ui.lineEditStatusBar );
    doDBDebug::ptr->registerHistroyWidget( this->ui.historyMessages );
    this->ui.historyMessages->setVisible(false);

// load connections from global settings
    doDBCore->connectionsLoad();

// toolbar
    connect( this->ui.btnConnectionsEdit, SIGNAL (clicked()), this, SLOT (connectionEditorShow()));


// debug button
    connect( this->ui.btnShowMessages, SIGNAL (clicked()), this, SLOT (debugMessagesTrigger()));



// entryEditor
    this->entryEditor = new doDBEntryEditor(this);
    this->entryEditor->setEnabled(false);
    this->itemViewLayout->addWidget( this->entryEditor );
    connect( this->entryEditor, SIGNAL (saveNew(etDBObject*,const char*)), this, SLOT (entryEditorItemSaveNew(etDBObject*,const char*)));
    connect( this->entryEditor, SIGNAL (saveChanged(etDBObject*,const char*)), this, SLOT (entryEditorItemChanged(etDBObject*,const char*)));

// load plugins
    doDBFile *dbFile = new doDBFile();
    dbFile->doDBTreeInit( this->dataTree );
    dbFile->doDBItemViewInit( this->itemViewLayout );
}

doDBMainWindow::                ~doDBMainWindow(){
}




void doDBMainWindow::           treeUpdate(){

// clear
    this->dataTree->clear();

// disable sorting
    this->dataTree->setSortingEnabled(false);


// vars
    doDBConnection      *connection;

    this->dataTree->refresh();


// enable tree
    this->dataTree->setSortingEnabled(true);
    this->dataTree->resizeColumnToContents(0);
    this->dataTree->setEnabled( true );
}






void doDBMainWindow::           treeElementClicked( QTreeWidgetItem * item, int column ){

    // vars
    doDBConnection              *connection;
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    etDBObject                  *dbObject;

// get Stuff from the selected item
    tableName = this->dataTree->itemTableName( item );
    connectionID = this->dataTree->itemConnectionID( item );
    itemID = this->dataTree->itemID( item );
    itemType = this->dataTree->itemType( item );

// if selected item is an entry
    if(  itemType == doDBtree::typeTable || itemType == doDBtree::typeEntry ){

        connection = doDBCore->connectionGet( connectionID.toUtf8() );
        if( connection == NULL ) return;

    // get the dbobject
        this->entryEditorConnID = connectionID;
        connection->dbObjectGet( &dbObject, &this->entryEditorLockID );

    // pick the table
        etDBObjectTablePick( dbObject, tableName.toUtf8() );

    // clear values
        etDBObjectValueClean( dbObject );


        if(  itemType == doDBtree::typeEntry ){
            connection->dbDataGet( tableName.toUtf8(), itemID.toUtf8() );
        }

    // show the object
        this->entryEditor->dbObjectShow( dbObject, tableName.toUtf8() );
        this->entryEditor->setEnabled(true);

    // unlock
        connection->dbObjectUnLock( this->entryEditorLockID );

        this->entryEditor->setVisible(true);
    } else {
        this->entryEditor->setVisible(false);
    }


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




void doDBMainWindow::           connectionEditorShow(){

// create the connection editor
    doDBConnectionEditor *editor = new doDBConnectionEditor( this );
    connect( editor, SIGNAL (finished()), this, SLOT (connectionEditorHide()));

// show the widget
    this->ui.toolBox->setVisible(false);


    this->ui.widgetContainerRight->addWidget( editor );
    this->ui.widgetContainerRight->setCurrentWidget( editor );
    this->ui.widgetContainerRight->setVisible(true);
}


void doDBMainWindow::           connectionEditorHide(){

// remove widget
    QWidget *widgetTemp = this->ui.widgetContainerRight->currentWidget();
    this->ui.widgetContainerRight->removeWidget( widgetTemp );
    delete widgetTemp;

// enable the connections-editor-button
    this->ui.toolBox->setVisible(true);
    this->ui.widgetContainerRight->setVisible( true );


// save
    doDBCore->connectionsSave();
    doDBSettingsGlobal->save();
    this->treeUpdate();
}


void doDBMainWindow::           debugMessagesTrigger(){

    if( this->ui.historyMessages->isVisible() ){

        this->ui.historyMessages->setEnabled( false );
        this->ui.historyMessages->setVisible( false );

    } else {
        this->ui.historyMessages->setEnabled( true );
        this->ui.historyMessages->setVisible( true );
        this->ui.historyMessages->resizeColumnsToContents();
    }
}


void doDBMainWindow::           onBtnTableEditClick(){


}





void doDBMainWindow::           entryEditorItemSaveNew( etDBObject *dbObject, const char *tableName ){

    doDBConnection *connection = doDBCore->connectionGet( this->entryEditorConnID.toUtf8() );
    if( connection == NULL ) return;

    connection->dbDataNew( tableName );


}


void doDBMainWindow::           entryEditorItemChanged( etDBObject *dbObject, const char *tableName ){

    doDBConnection *connection = doDBCore->connectionGet( this->entryEditorConnID.toUtf8() );
    if( connection == NULL ) return;

    connection->dbDataChange( tableName );


}



