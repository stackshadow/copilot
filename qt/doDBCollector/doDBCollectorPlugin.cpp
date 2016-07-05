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
#include <QLayout>

#include "doDBCollectorPlugin.h"
#include "doDBPlugin/doDBPlugins.h"

doDBCollectorPlugin::               doDBCollectorPlugin(){

    this->collectorWidget = NULL;
    this->dbEntry = NULL;
}

doDBCollectorPlugin::               ~doDBCollectorPlugin(){


}

// overload
QString doDBCollectorPlugin::       valueGet( QString valueName ){

}

void doDBCollectorPlugin::          prepareLayout( QString name, QLayout* layout ){

    if( name == "centralView" ){

        if( this->collectorWidget == NULL ){

            QVBoxLayout* newLayout = new QVBoxLayout();

            this->collectorWidget = new doDBCollector( NULL );
            connect( this->collectorWidget , SIGNAL (cellClicked(int,int)), this, SLOT( itemClicked(int,int) ) );

            this->btnItemRemember = new QPushButton( "Item merken" );
            this->btnItemRemember->setEnabled( false );
            connect( this->btnItemRemember , SIGNAL (clicked()), this, SLOT( rememberItem() ) );

            this->btnItemRemove = new QPushButton( "Item vergessen" );
            this->btnItemRemove->setEnabled( false );

            this->btnClean = new QPushButton( "Alles vergessen" );
            this->btnClean->setEnabled( false );
            connect( this->btnClean , SIGNAL (clicked()), this->collectorWidget, SLOT( cleanAll() ) );


            newLayout->addWidget( this->btnItemRemember );
            newLayout->addWidget( this->btnItemRemove );
            newLayout->addWidget( this->btnClean );
            newLayout->addWidget( this->collectorWidget );

            ((QVBoxLayout*)layout)->addLayout( newLayout );
        }

    }


}

bool doDBCollectorPlugin::          handleAction( QString action, doDBEntry* entry ){

    if( action == "itemSelected" ){

        if( this->dbEntry != NULL ){
            doDBEntry::decRef( &this->dbEntry );
        }

        this->dbEntry = entry;
        this->dbEntry->incRef();

        this->btnItemRemember->setEnabled( true );

        return true;
    }

}



void doDBCollectorPlugin::          rememberItem(){

    this->collectorWidget->entryAppend( this->dbEntry );

    this->btnItemRemove->setEnabled( true );
    this->btnClean->setEnabled( true );

}


void doDBCollectorPlugin::          itemClicked( int row, int column ){

    doDBEntry*      clickedEntry = this->collectorWidget->getEntry( row );
    if( clickedEntry == NULL ) return;

// fire plugins
    doDBPlugins::ptr->handleAction( "itemSelected", clickedEntry );

}




