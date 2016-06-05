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

#include <QGroupBox>
#include <QListView>
#include <QStackedWidget>
#include <QFileDialog>

#include "doDBFile/doDBTableFoldersEditor.h"


doDBTableFoldersEditor::            doDBTableFoldersEditor( QWidget *parent ) : QWidget( parent ) {

// vars
    this->ui.setupUi(this);
    this->tableName = "";
    this->json = NULL;

// setup
    this->ui.tableWidget->setColumnCount(2);
    this->ui.tableWidget->horizontalHeader()->setVisible(false);
    this->ui.tableWidget->setSelectionMode( QAbstractItemView::SingleSelection );
    this->ui.tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    this->ui.tableWidget->setEditTriggers( QAbstractItemView::NoEditTriggers );

// append to os
    this->ui.cBoxOS->addItem( "Linux", "linux" );


    connect( this->ui.btnTableFolderSelect, SIGNAL (clicked()), this, SLOT (appendFolder()));



}


doDBTableFoldersEditor::            ~doDBTableFoldersEditor(){


}


void doDBTableFoldersEditor::       loadJson( json_t *tableFolders, const char *table ){

    this->json = tableFolders;
    this->tableName = table;

    this->refreshTable();
}


void doDBTableFoldersEditor::       refreshTable(){

// check
    if( this->json == NULL || this->tableName.length() <= 0 ){
        return;
    }

// clear the table
    this->ui.tableWidget->setRowCount(0);

// get table from root-json
    json_t *jsonTable = json_object_get( this->json, this->tableName.toUtf8() );
    if( jsonTable == NULL ){
        return;
    }

// iterate over all folder
    json_t              *jsonValue = NULL;
    const char          *jsonValueChar = NULL;
    void                *jsonFolderIterator = NULL;
    json_t              *jsonFolder = NULL;
    const char          *jsonFolderChar = NULL;
    QTableWidgetItem    *item = NULL;

// iterate folders
    jsonFolderIterator = json_object_iter( jsonTable );
    while( jsonFolderIterator != NULL ){

    // the object
        jsonFolder = json_object_iter_value(jsonFolderIterator);

    // the os
        jsonValue = json_object_get( jsonFolder, "os" );
        jsonValueChar = json_string_value(jsonValue);

    // the folder iterator
        jsonFolderChar = json_object_iter_key(jsonFolderIterator);

        this->ui.tableWidget->insertRow(0);

        item = new QTableWidgetItem();
        item->setText(jsonValueChar);
        this->ui.tableWidget->setItem( 0, 0, item );

        item = new QTableWidgetItem();
        item->setText(jsonFolderChar);
        this->ui.tableWidget->setItem( 0, 1, item );

        jsonFolderIterator = json_object_iter_next(jsonTable,jsonFolderIterator);
    }

    this->ui.tableWidget->resizeColumnsToContents();

}


void doDBTableFoldersEditor::       appendFolder(){

// load it from the db
    if( this->json == NULL ){
        this->json = json_object();
    }

// get table from root-json
    json_t *jsonTable = json_object_get( this->json, this->tableName.toUtf8() );
    if( jsonTable == NULL ){
        jsonTable = json_object();
        if( json_object_set_new( this->json, this->tableName.toUtf8(), jsonTable ) != 0 ){
            return;
        }
    }

// read infos from editor
    QString operationSystem = this->ui.cBoxOS->itemData( this->ui.cBoxOS->currentIndex() ).toString();
    QString folder = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                             "/home",
                                             QFileDialog::ShowDirsOnly
                                             | QFileDialog::DontResolveSymlinks);

// append the folder
    json_t *jsonFolder = json_object();
    json_object_set_new( jsonFolder, "os", json_string(operationSystem.toUtf8()) );
    json_object_set_new( jsonTable, folder.toUtf8(), jsonFolder );


    this->refreshTable();
}





