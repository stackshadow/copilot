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

#include "doDBDebug.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QTableWidget>

doDBDebug* doDBDebug::ptr = NULL;

doDBDebug::doDBDebug(){
    this->ptr = this;

    this->lineEdit = NULL;
    this->tableWidget = NULL;

    etDebugEvillib->printMessage = doDBDebug::evillibPrint;
}

doDBDebug::~doDBDebug(){

}


void doDBDebug::            registerDebugLine( QLineEdit *lineEdit ){
    this->lineEdit = lineEdit;
}


void doDBDebug::            registerHistroyWidget( QTableWidget *tableWidget ){
    this->tableWidget = tableWidget;


    this->tableWidget->setColumnCount(4);

    QTableWidgetItem *item = NULL;

    item = new QTableWidgetItem();
    item->setText("programm");
    this->tableWidget->setHorizontalHeaderItem( 0, item );

    item = new QTableWidgetItem();
    item->setText("level");
    this->tableWidget->setHorizontalHeaderItem( 1, item );

    item = new QTableWidgetItem();
    item->setText("function");
    this->tableWidget->setHorizontalHeaderItem( 2, item );

    item = new QTableWidgetItem();
    item->setText("message");
    this->tableWidget->setHorizontalHeaderItem( 3, item );

    this->tableWidget->setWordWrap(true);
    this->tableWidget->setTextElideMode(Qt::ElideMiddle);

}


void doDBDebug::            print( QString string ){

// vars
    QTableWidgetItem        *item = NULL;

// show text on line edit
    if( this->lineEdit != NULL ){
        this->lineEdit->setText( string );
    }


    item = new QTableWidgetItem();
    item->setText( string );

    this->tableWidget->insertRow(0);
    this->tableWidget->setItem( 0, 2, item );

}


void doDBDebug::            print( QString programName, QString levelName, QString functionName, QString message ){

// vars
    QTableWidgetItem        *item = NULL;

// show text on line edit
    if( this->lineEdit != NULL ){
        this->lineEdit->setText( message );
    }

    if( this->tableWidget != NULL ){

    // limit debug lines to 200
        if( this->tableWidget->rowCount() > 200 ){
            this->tableWidget->setRowCount(200);
        }

        this->tableWidget->insertRow(0);

        item = new QTableWidgetItem();
        item->setText( programName );
        this->tableWidget->setItem( 0, 0, item );

        item = new QTableWidgetItem();
        item->setText( levelName );
        this->tableWidget->setItem( 0, 1, item );

        item = new QTableWidgetItem();
        item->setText( functionName );
        this->tableWidget->setItem( 0, 2, item );

        item = new QTableWidgetItem();
        item->setText( message );
        this->tableWidget->setItem( 0, 3, item );

    }

}


void doDBDebug::            evillibPrint( etDebug* etDebugActual ){

    doDBDebug *dbDebug = doDBDebug::ptr;

// level name
    const char *levelName = "DETAIL";

    switch( etDebugActual->Level ){

        case etID_LEVEL_INFO:
            levelName = "INFO";

        case etID_LEVEL_WARNING:
            levelName = "WARN";

        case etID_LEVEL_ERR:
            levelName = "ERROR";

    }


    dbDebug->print( etDebugActual->Program, levelName, etDebugActual->Function, etDebugActual->Message );

}


