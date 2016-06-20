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

#include "doDBTableEditor.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBConnection/doDBConnectionEditor.h"



doDBTableEditor::                       doDBTableEditor( QWidget *parent, etDBObject *dbObject ) : QWidget(parent){

// save
    this->dbObject = dbObject;
    this->mode = MODE_NONE;

// setup ui
    this->ui.setupUi( this );

// hide table / column
    this->ui.groupTable->setVisible( false );
    this->ui.groupColumns->setVisible( false );
    this->ui.groupColumn->setVisible( false );

// setup columntable
    this->ui.listColumns->setEditTriggers( QAbstractItemView::NoEditTriggers );
    this->ui.listColumns->setColumnCount(2);
    this->ui.listColumns->setColumnHidden(0,true);

    this->cBoxTablesRefresh();

// hide "create of table in db" button
    this->ui.btnTableCreate->setVisible(false);

// fill the
    this->ui.columnType->clear();
    this->ui.columnType->addItem( "STRING" );
    this->ui.columnType->addItem( "INT" );
    this->ui.columnType->addItem( "FLOAT" );
    this->ui.columnType->addItem( "BLOB" );

// connect
    connect( this->ui.tablesCBox, SIGNAL( activated(int) ), this, SLOT( cBoxTableSelected(int) ) );

    connect( this->ui.btnTableNew, SIGNAL( clicked() ), this, SLOT( tableNew() ) );
    connect( this->ui.btnTableSave, SIGNAL( clicked() ), this, SLOT( tableSave() ) );

    connect( this->ui.listColumns, SIGNAL(cellClicked(int,int)), this, SLOT( listColumnsSelected(int,int) ) );
    connect( this->ui.btnColumnAdd, SIGNAL( clicked() ), this, SLOT( columnNew() ) );
    connect( this->ui.btnColumnSave, SIGNAL( clicked() ), this, SLOT( columnSave() ) );

    connect( this->ui.btnClose, SIGNAL( clicked() ), this, SLOT( close() ) );


}

doDBTableEditor::                       ~doDBTableEditor(){
}




void doDBTableEditor::                  cBoxTablesRefresh(){

// clear
    this->ui.tablesCBox->clear();

    const char      *tableName = NULL;
    const char      *tableDisplayName = NULL;
    bool            tablePresent = false;


    etDBObjectIterationReset( this->dbObject );
    while( etDBObjectTableNext( this->dbObject, tableName ) == etID_YES ){

        etDBObjectTableDisplayNameGet( this->dbObject, "", tableDisplayName );

        this->ui.tablesCBox->addItem( QString(tableDisplayName), QString(tableName) );
    }

}


void doDBTableEditor::                  cBoxTablesClean(){
    this->ui.tablesCBox->clear();
}


void doDBTableEditor::                  cBoxTableSelected( int index ){

// infos
    QString tableName = this->ui.tablesCBox->itemData(index).toString();
    QString tableDisplayName = this->ui.tablesCBox->itemText(index);

// visible / enable
    this->ui.groupTable->setVisible(true);
    this->ui.tableName->setEnabled(false);
    this->ui.groupColumns->setVisible(true);

// set values
    this->ui.tableName->setText( tableName );
    this->ui.tableDisplayName->setText( tableDisplayName );

// pick table
    if( etDBObjectTablePick( this->dbObject, tableName.toUtf8() ) != etID_YES ){
        return;
    }

// refresh columns
    this->listColumnsRefresh();


}




void doDBTableEditor::                  tableNew(){

// visible / enable
    this->ui.groupTable->setVisible(true);
    this->ui.tableName->setEnabled(true);
    this->ui.groupColumns->setVisible(false);
    this->ui.groupColumn->setVisible(false);
    this->ui.btnTableCreate->setVisible(false);

// clear entrys
    this->ui.tableName->setText( "" );
    this->ui.tableDisplayName->setText( "" );

// clear columns
    this->ui.listColumns->setRowCount(0);

}


void doDBTableEditor::                  tableSave(){

// visible / enable
    this->ui.groupColumns->setVisible(true);

// infos
    QString         tableName = this->ui.tableName->text();
    QString         tableDisplayName = this->ui.tableDisplayName->text();

// check
    if( tableName.length() <= 0 ){
        return;
    }


    if( this->ui.tableName->isEnabled() ){
    // append a new table
        etDBObjectTableAdd( this->dbObject, tableName.toUtf8() );
        etDBObjectTableDisplayNameSet( this->dbObject, "", tableDisplayName.toUtf8() );

        this->mode = MODE_TABLE_NEW;
    }
// change
    else {
        etDBObjectTableDisplayNameSet( this->dbObject, "", tableDisplayName.toUtf8() );
    }


// refresh columns
    this->cBoxTablesRefresh();

}


void doDBTableEditor::                  tableCreateInDB(){

}




void doDBTableEditor::                  listColumnsRefresh(){

    this->ui.listColumns->setRowCount(0);

// infos
    QString tableName = this->ui.tableName->text();

    const char *columnName = NULL;
    const char *columnDisplayName = NULL;

    etDBObjectIterationReset( this->dbObject );
    while( etDBObjectTableColumnNext( this->dbObject, columnName ) == etID_YES ){

        etDBObjectTableColumnDisplayNameGet( this->dbObject, "", columnDisplayName );

        this->listColumnsAppend( columnName, columnDisplayName );

    }


}


void doDBTableEditor::                  listColumnsAppend( const char *columnName, const char *displayName ){

    this->ui.listColumns->insertRow(0);
    this->ui.listColumns->setItem( 0, 0, new QTableWidgetItem(columnName) );
    this->ui.listColumns->setItem( 0, 1, new QTableWidgetItem(displayName) );


}


void doDBTableEditor::                  listColumnsSelected( int row, int column ){

// visible / enable
    this->ui.groupColumn->setVisible(true);
    this->ui.groupColumn->setEnabled(true);


// vars
    QString             columnName = this->ui.listColumns->item( row, 0 )->text();
    QString             columnDisplayName = this->ui.listColumns->item( row, 1 )->text();
    etDBColumnType      columnType = etDBCOLUMN_TYPE_STRING;
    int                 columnOption = etDBCOLUMN_OPTION_NOTHING;


// clean input boxes
    this->ui.columnName->setText("");
    this->ui.columnDisplayName->setText("");
    this->ui.columnType->setCurrentIndex(0);
    this->ui.columnOptNotNull->setChecked(false);
    this->ui.columnOptPrimaryKey->setChecked(false);
    this->ui.columnOptUnique->setChecked(false);
    this->ui.ckBoxDisplayColumn->setChecked(false);

// read column info
    if( etDBObjectTableColumnPick( this->dbObject, columnName.toUtf8() ) != etID_YES ){
        return;
    }
    etDBObjectTableColumnTypeGet( this->dbObject, columnType );
    etDBObjectTableColumnOptionGet( this->dbObject, columnOption );

// set values
    this->ui.columnName->setText(columnName);
    this->ui.columnName->setEnabled(false);
    this->ui.columnDisplayName->setText(columnDisplayName);

// set the columnType
    if( columnType == etDBCOLUMN_TYPE_STRING ){
        this->ui.columnType->setCurrentIndex(0);
    }
    if( columnType == etDBCOLUMN_TYPE_INT ){
        this->ui.columnType->setCurrentIndex(1);
    }
    if( columnType == etDBCOLUMN_TYPE_FLOAT ){
        this->ui.columnType->setCurrentIndex(2);
    }
    if( columnType == etDBCOLUMN_TYPE_BLOB ){
        this->ui.columnType->setCurrentIndex(3);
    }

// set the column option
    if( columnOption & etDBCOLUMN_OPTION_NOTNULL ){
        this->ui.columnOptNotNull->setChecked(true);
    }
    if( columnOption & etDBCOLUMN_OPTION_PRIMARY ){
        this->ui.columnOptPrimaryKey->setChecked(true);
    }
    if( columnOption & etDBCOLUMN_OPTION_UNIQUE ){
        this->ui.columnOptUnique->setChecked(true);
    }

// get display column
    const char *displayColumn;
    if( etDBObjectTableColumnMainGet( this->dbObject, displayColumn ) == etID_YES ){
        if( columnName == displayColumn ){
            this->ui.ckBoxDisplayColumn->setChecked(true);
        }
    }


}


void doDBTableEditor::                  columnNew(){

// visible / enable
    this->ui.groupColumn->setVisible(true);
    this->ui.groupColumn->setEnabled(true);

// clean input boxes
    this->ui.columnName->setText("new");
    this->ui.columnName->setEnabled(true);
    this->ui.columnDisplayName->setText("new");
    this->ui.columnType->setCurrentIndex(0);
    this->ui.columnOptNotNull->setChecked(false);
    this->ui.columnOptPrimaryKey->setChecked(false);
    this->ui.columnOptUnique->setChecked(false);

}


void doDBTableEditor::                  columnSave(){


// vars
    QString             tableName = this->ui.tableName->text();
    QString             columnName = this->ui.columnName->text();
    QString             columnDisplayName = this->ui.columnDisplayName->text();
    etDBColumnType      columnType = etDBCOLUMN_TYPE_STRING;
    int                 columnOption = etDBCOLUMN_OPTION_NOTHING;

// pick selected table
    if( etDBObjectTablePick( this->dbObject, tableName.toUtf8() ) != etID_YES ) return;

// get column type
    if( this->ui.columnType->currentIndex() == 0 ){
        columnType = etDBCOLUMN_TYPE_STRING;
    }
    if( this->ui.columnType->currentIndex() == 1 ){
        columnType = etDBCOLUMN_TYPE_INT;
    }
    if( this->ui.columnType->currentIndex() == 2 ){
        columnType = etDBCOLUMN_TYPE_FLOAT;
    }
    if( this->ui.columnType->currentIndex() == 3 ){
        columnType = etDBCOLUMN_TYPE_BLOB;
    }

// columnOption
    if( this->ui.columnOptNotNull->isChecked() ){
        columnOption |= etDBCOLUMN_OPTION_NOTNULL;
    }
    if( this->ui.columnOptPrimaryKey->isChecked() ){
        columnOption |= etDBCOLUMN_OPTION_PRIMARY;
        etDBObjectTableColumnPrimarySet( this->dbObject, columnName.toUtf8() );
    }
    if( this->ui.columnOptUnique->isChecked() ){
        columnOption |= etDBCOLUMN_OPTION_UNIQUE;
    }




// new
    if( this->ui.columnName->isEnabled() ){
        etDBObjectTableColumnAdd( this->dbObject, columnName.toUtf8(), columnType, columnOption );
        etDBObjectTableColumnDisplayNameSet( this->dbObject, "", columnDisplayName.toUtf8() );

        if( this->ui.ckBoxDisplayColumn->isChecked() ){
            etDBObjectTableColumnMainSet( this->dbObject, columnName.toUtf8() );
        }

        if( this->mode == MODE_TABLE_NEW ){
            emit newTable( this->dbObject, tableName.toUtf8() );
            this->mode = MODE_TABLE_CHANGE;
        } else {
            emit newColumn( this->dbObject, tableName.toUtf8(), columnName.toUtf8() );
        }

    }
// change
    else {
        etDBObjectTableColumnAdd( this->dbObject, columnName.toUtf8(), columnType, columnOption );
        etDBObjectTableColumnDisplayNameSet( this->dbObject, "", columnDisplayName.toUtf8() );

        if( this->ui.ckBoxDisplayColumn->isChecked() ){
            etDBObjectTableColumnMainSet( this->dbObject, columnName.toUtf8() );
        }
    }

// refresh
    this->listColumnsRefresh();

// visible / enable
    this->ui.groupColumn->setVisible(false);

}


void doDBTableEditor::                  close(){

    QStackedWidget          *container = (QStackedWidget*)this->parentWidget();

// remove widget
    QWidget *widgetTemp = container->currentWidget();
    container->removeWidget( widgetTemp );

// emit
    emit closed();


    delete this;
}


