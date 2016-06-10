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

#include "doDBRelation/doDBRelationEditor.h"

#include "db/etDBObject.h"
#include "db/etDBObjectTable.h"

doDBRelationEditor::            doDBRelationEditor( QWidget *parent ) : QWidget( parent ) {

// setup ui
    this->ui.setupUi(this);


// setup
    this->ui.tblRelations->setColumnCount(4);
    this->ui.tblRelations->setHorizontalHeaderItem(0, new QTableWidgetItem("Tabelle A") );
    this->ui.tblRelations->setHorizontalHeaderItem(1, new QTableWidgetItem("Spalte A") );
    this->ui.tblRelations->setHorizontalHeaderItem(2, new QTableWidgetItem("Tabelle B") );
    this->ui.tblRelations->setHorizontalHeaderItem(3, new QTableWidgetItem("Spalte B") );
    this->ui.tblRelations->setSelectionMode( QAbstractItemView::SingleSelection );
    this->ui.tblRelations->setSelectionBehavior( QAbstractItemView::SelectRows );
    this->ui.tblRelations->setEditTriggers( QAbstractItemView::NoEditTriggers );


// setup
    this->ui.tblSrcColumns->setColumnCount(2);
    this->ui.tblSrcColumns->horizontalHeader()->setVisible(false);
    this->ui.tblSrcColumns->setColumnHidden( 0, true );
    this->ui.tblSrcColumns->horizontalHeader()->setStretchLastSection(true);
    this->ui.tblSrcColumns->setSelectionMode( QAbstractItemView::SingleSelection );
    this->ui.tblSrcColumns->setSelectionBehavior( QAbstractItemView::SelectRows );
    this->ui.tblSrcColumns->setEditTriggers( QAbstractItemView::NoEditTriggers );

// setup
    this->ui.tblRelColumns->setColumnCount(2);
    this->ui.tblRelColumns->horizontalHeader()->setVisible(false);
    this->ui.tblRelColumns->setColumnHidden( 0, true );
    this->ui.tblRelColumns->horizontalHeader()->setStretchLastSection(true);
    this->ui.tblRelColumns->setSelectionMode( QAbstractItemView::SingleSelection );
    this->ui.tblRelColumns->setSelectionBehavior( QAbstractItemView::SelectRows );
    this->ui.tblRelColumns->setEditTriggers( QAbstractItemView::NoEditTriggers );



// connections
    connect( this->ui.cBoxSrcTable, SIGNAL( activated(int) ), this, SLOT( srcTableSelected(int) ) );
    connect( this->ui.cBoxRelTable, SIGNAL( activated(int) ), this, SLOT( relTableSelected(int) ) );
    connect( this->ui.btnRelationCreate, SIGNAL( clicked() ), this, SLOT( relationAppend() ) );
    connect( this->ui.btnRelationDelete, SIGNAL( clicked() ), this, SLOT( relationRemove() ) );
    connect( this->ui.btnClose, SIGNAL( clicked() ), this, SLOT( closeEditor() ) );

}


doDBRelationEditor::            ~doDBRelationEditor(){


}


void doDBRelationEditor::       showRelation( etDBObject *dbObject, doDBRelation *dbRelation ){

// save relation
    this->dbObject = dbObject;
    this->dbRelation = dbRelation;

// vars
    const char      *tableName = NULL;
    const char      *tableDisplayName = NULL;

// clean combobox
    this->ui.cBoxSrcTable->clear();
    this->ui.cBoxRelTable->clear();

// appent all tables
    if( etDBObjectIterationReset( dbObject ) != etID_YES ) return;
    while( etDBObjectTableNext(dbObject,tableName) == etID_YES ){

        etDBObjectTableDisplayNameGet(dbObject,"",tableDisplayName);

    // append to comboboxen
        this->ui.cBoxSrcTable->addItem( tableDisplayName, tableName );
        this->ui.cBoxRelTable->addItem( tableDisplayName, tableName );

    }

    this->refreshRelation();
}




void doDBRelationEditor::       fillTableWithColumns( const char *tableName, QTableWidget *tableWidget ){


// select table
    if( etDBObjectTablePick(this->dbObject,tableName) != etID_YES ) return;


// vars
    const char              *columnName = NULL;
    const char              *columnDisplayName = NULL;
    QTableWidgetItem        *item = NULL;

// appent all tables
    if( etDBObjectIterationReset( dbObject ) != etID_YES ) return;
    while( etDBObjectTableColumnNext(dbObject,columnName) == etID_YES ){

        etDBObjectTableColumnDisplayNameGet(dbObject,"",columnDisplayName);

        tableWidget->insertRow(0);

        item = new QTableWidgetItem();
        item->setText(columnName);
        tableWidget->setItem( 0, 0, item );

        item = new QTableWidgetItem();
        item->setText(columnDisplayName);
        tableWidget->setItem( 0, 1, item );

    }

    this->refreshRelation();


}


void doDBRelationEditor::       refreshRelation(){




// vars
    void                    *jsonSrcTableIterator = NULL;
    json_t                  *jsonSrcTableArray = NULL;
    int                     jsonRelTableIndex = 0;
    int                     jsonRelTableLen = 0;
    json_t                  *jsonRelTable = NULL;
    json_t                  *jsonValue = NULL;
    const char              *jsonValueChar = NULL;
    QTableWidgetItem        *item = NULL;

    const char              *srcTable = NULL;
    const char              *srcColumn = NULL;
    const char              *relatedTable = NULL;
    const char              *relatedColumn = NULL;

// clear tables
    this->ui.tblRelations->setRowCount(0);


    this->dbRelation->relationGetReset();
    while( this->dbRelation->relationGetNext( &srcTable, &srcColumn, &relatedTable, &relatedColumn ) ){

    // append a column
        this->ui.tblRelations->insertRow(0);

    // src-table
        item = new QTableWidgetItem();
        item->setText( srcTable );
        this->ui.tblRelations->setItem( 0, 0, item );

    // src-Column
        item = new QTableWidgetItem();
        item->setText( srcColumn );
        this->ui.tblRelations->setItem( 0, 1, item );

    // src-Column
        item = new QTableWidgetItem();
        item->setText( relatedTable );
        this->ui.tblRelations->setItem( 0, 2, item );

    // rel-Column
        item = new QTableWidgetItem();
        item->setText( relatedColumn );
        this->ui.tblRelations->setItem( 0, 3, item );


    }


    this->ui.tblRelations->resizeColumnsToContents();
}




void doDBRelationEditor::       srcTableSelected( int selectedItem ){

// get table name
    QString             tableName = this->ui.cBoxSrcTable->currentData().toString();

    this->fillTableWithColumns( tableName.toUtf8(), this->ui.tblSrcColumns );

}


void doDBRelationEditor::       relTableSelected( int selectedItem ){

// get table name
    QString             tableName = this->ui.cBoxRelTable->currentData().toString();

    this->fillTableWithColumns( tableName.toUtf8(), this->ui.tblRelColumns );

}





void doDBRelationEditor::       relationAppend(){

// vars
    QString             srcTableName;
    QString             srcColumnName;
    QString             relTableName;
    QString             relColumnName;
    QTableWidgetItem    *item = NULL;

// src-table
    srcTableName = this->ui.cBoxSrcTable->currentData().toString();

// src-column
    if( this->ui.tblSrcColumns->selectedItems().count() <= 0 ) return;
    item = this->ui.tblSrcColumns->item( this->ui.tblSrcColumns->currentRow(), 0 );
    srcColumnName = item->text();

// rel-table
    relTableName = this->ui.cBoxRelTable->currentData().toString();

// rel-column
    if( this->ui.tblRelColumns->selectedItems().count() <= 0 ) return;
    item = this->ui.tblRelColumns->item( this->ui.tblRelColumns->currentRow(), 0 );
    relColumnName = item->text();


// append
    this->dbRelation->relationAppend( srcTableName.toUtf8(), relTableName.toUtf8(), srcColumnName.toUtf8(), relColumnName.toUtf8() );

// refresh table
    this->refreshRelation();

}


void doDBRelationEditor::       relationRemove(){


// vars
    QString             srcTableName;
    QString             srcColumnName;
    QString             relTableName;
    QString             relColumnName;
    QTableWidgetItem    *item = NULL;

// src-table
    srcTableName = this->ui.tblRelations->item( this->ui.tblRelations->currentRow(), 0 )->text();
    srcColumnName = this->ui.tblRelations->item( this->ui.tblRelations->currentRow(), 1 )->text();
    relTableName = this->ui.tblRelations->item( this->ui.tblRelations->currentRow(), 2 )->text();
    relColumnName = this->ui.tblRelations->item( this->ui.tblRelations->currentRow(), 3 )->text();


// remove
    this->dbRelation->relationRemove( srcTableName.toUtf8(), relTableName.toUtf8(), srcColumnName.toUtf8(), relColumnName.toUtf8() );

// refresh table
    this->refreshRelation();



}




void doDBRelationEditor::       closeEditor(){
    this->setVisible(false);
    emit closed();
}







