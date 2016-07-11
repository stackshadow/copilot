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

#include "doDBDebug/doDBDebug.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBPlugin/doDBPlugins.h"

#include "doDBTableEditorPlugin.h"

#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"

doDBTableEditorPlugin::             doDBTableEditorPlugin() : doDBPlugin() {


// register for messages
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitToolBar, true );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgInitStackedRight, true );

    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionSelected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionConnected );
    doDBPlugins::ptr->registerListener( this, doDBPlugin::msgConnectionDisconnected );


    this->tableEditor = NULL;

}

doDBTableEditorPlugin::             ~doDBTableEditorPlugin(){

}




bool doDBTableEditorPlugin::        recieveMessage( messageID type, void* payload ){

    QLayout*            layout = (QLayout*)payload;
    doDBConnection*     dbConnection = (doDBConnection*)payload;

// Show/hide editor-button
    if( type == doDBPlugin::msgConnectionConnected ){

    // remember selected Connection
        this->connectionSelected = dbConnection;

        this->toolBarGroup->setVisible(true);
        return true;
    }

// Show/hide editor-button
    if( type == doDBPlugin::msgConnectionDisconnected ){

    // forget connection
        this->connectionSelected = NULL;

        this->toolBarGroup->setVisible(false);
        return true;
    }

// Show/hide editor-button
    if( type == doDBPlugin::msgConnectionSelected ){

    // remember selected Connection
        this->connectionSelected = dbConnection;

        if( this->connectionSelected == NULL ){
            this->toolBarGroup->setVisible(false);
            return true;
        }

    // show editor button if we are connected
        if( this->connectionSelected->isConnected() == true ){
            this->toolBarGroup->setVisible(true);
        } else {
            this->toolBarGroup->setVisible(false);
        }

        return true;
    }



    if( type == doDBPlugin::msgInitToolBar ){
        this->toolBarGroup = new QGroupBox( "Tabellen" );
        this->toolBarGroup->setVisible(false);
        this->toolBarGroup->setLayout( new QHBoxLayout() );
        this->toolBarGroup->setLayoutDirection( Qt::LeftToRight );
        this->toolBarGroup->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );

        this->btnTableEdit = new QPushButton( "Tabelle\nBearbeiten" );
        connect( this->btnTableEdit, SIGNAL(clicked()), this, SLOT(editorShow()) );
        this->toolBarGroup->layout()->setMargin(1);
        this->toolBarGroup->layout()->addWidget( this->btnTableEdit );


        layout->addWidget( this->toolBarGroup );
        return true;
    }


    if( type == doDBPlugin::msgInitStackedRight ){
        this->stackedWidget = (QStackedWidget*)payload;
    }





    return true;
}




void doDBTableEditorPlugin::        editorShow(){

    if( this->tableEditor == NULL ){
        this->tableEditor = new doDBTableEditor(0);
        connect( this->tableEditor, SIGNAL( newTable(etDBObject*, QString) ), this, SLOT( tableNew(etDBObject*, QString) ) );
        connect( this->tableEditor, SIGNAL( newColumn(etDBObject*, QString, QString) ), this, SLOT( columnNew(etDBObject*, QString, QString) ) );
        connect( this->tableEditor, SIGNAL( closed() ), this, SLOT( tablesEditorClosed() ) );
    }

// show
    this->tableEditor->showObject( this->connectionSelected->dbObject );


    this->stackedWidget->addWidget( this->tableEditor );
    this->stackedWidget->setCurrentWidget( this->tableEditor );

}



void doDBTableEditorPlugin::        tableNew( etDBObject *dbObject, QString newTableName ){
    this->connectionSelected->tableAppend( newTableName.toUtf8() );
}


void doDBTableEditorPlugin::        columnNew(  etDBObject *dbObject, QString newTableName, QString newColumnName  ){
    this->connectionSelected->columnAppend( newTableName.toUtf8(), newColumnName.toUtf8() );
}


void doDBTableEditorPlugin::        tablesEditorClosed(){
// check
    if( this->connectionSelected == NULL ) return;

// just save it ;)
    this->connectionSelected->dbObjectSave();

}














