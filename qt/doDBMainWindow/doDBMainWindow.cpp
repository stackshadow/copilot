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
#include <QSplitter>

#include "main.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnectionEditor.h"
#include "doDBFile/doDBFile.h"

#include "doDBPlugin/doDBPlugins.h"
#include "doDBRelation/doDBRelationPlugin.h"

doDBMainWindow::                doDBMainWindow( QWidget *parent ) : QWidget(parent){

// show ui
    this->ui.setupUi( this);
    this->show();


// set our status bar as debug bar
    doDBDebug::ptr->registerDebugLine( this->ui.lineEditStatusBar );
    doDBDebug::ptr->registerHistroyWidget( this->ui.historyMessages );
    this->ui.historyMessages->setVisible(false);

// left - tree
    this->dataTree = new doDBtree( this );
    this->ui.structuredView->addWidget(this->dataTree);
    connect( this->dataTree, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (treeElementClicked(QTreeWidgetItem*,int)));

// right item view
    this->itemViewLayout = this->ui.itemView->layout();


// setup splitter
    this->ui.layoutWidgetContainer->removeWidget( this->ui.widgetContainerLeft );
    this->ui.layoutWidgetContainer->removeWidget( this->ui.widgetContainerRight );
    QSplitter *splitter = new QSplitter();
    splitter->addWidget( this->ui.widgetContainerLeft );
    splitter->addWidget( this->ui.widgetContainerRight );
    this->ui.layoutWidgetContainer->addWidget( splitter );



// init plugins
//    doDBPlugins::ptr->eventPrepareDashboard( this->ui.page->layout() );
    doDBPlugins::ptr->eventPrepareTree( this->dataTree );
    doDBPlugins::ptr->eventPrepareItemView( this->ui.itemView->layout() );


// load plugins
    doDBFile *dbFile = new doDBFile();
    dbFile->doDBTreeInit( this->dataTree );
    dbFile->doDBItemViewInit( this->itemViewLayout );


// toolbar
    connect( this->ui.btnConnectionsEdit, SIGNAL (clicked()), this, SLOT (connectionEditorShow()));
    connect( this->ui.btnShowMessages, SIGNAL (clicked()), this, SLOT (debugMessagesTrigger()));


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
    tableName = doDBtree::itemTableName( item );
    connectionID = doDBtree::itemConnectionID( item );
    itemID = doDBtree::itemID( item );
    itemType = doDBtree::itemType( item );


// before WE doing something with it, call all plugins
    doDBPlugins::ptr->eventTreeItemClicked( item, column );


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
    this->ui.widgetContainerLeft->setVisible(false);


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
    this->ui.widgetContainerLeft->setVisible(true);
    this->ui.widgetContainerRight->setVisible( true );


// save
    doDBConnections::ptr->connectionsSave();
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
        this->ui.historyMessages->resizeRowsToContents();
    }
}


void doDBMainWindow::           onBtnTableEditClick(){


}




