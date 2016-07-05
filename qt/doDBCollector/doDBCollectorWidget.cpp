
#include "doDBCollector/doDBCollectorWidget.h"

#include <QTableWidget>
#include <QHeaderView>

doDBCollector::                 doDBCollector( QWidget* parentWidget ) : QTableWidget( parentWidget ) {

// setup the table widget
    this->setColumnCount( this->colCount );
    this->setSizeAdjustPolicy( AdjustToContents );
    this->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Minimum );
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);
}

doDBCollector::                 ~doDBCollector(){

}


void doDBCollector::            entryAppend( doDBEntry* entry ){

// increment entry
    entry->incRef();

// add it to the list
    this->entrys.append( entry );

    this->refreshList();
}


void doDBCollector::            cleanAll(){

// vars
    doDBEntry*      dbEntry = NULL;
    int             rowCount = 0;

// clean list
    this->setRowCount(0);

// iterate
    foreach( dbEntry, this->entrys ){
        doDBEntry::decRef( &dbEntry );
    }

    this->entrys.clear();


}


void doDBCollector::            refreshList(){

// vars
    doDBEntry*      dbEntry = NULL;
    int             rowCount = 0;

// clean list
    this->setRowCount(0);

// iterate
    foreach( dbEntry, this->entrys ){

        QString displayName;

        dbEntry->item( NULL, NULL, NULL, &displayName );

        rowCount = this->rowCount();
        this->setRowCount( rowCount + 1 );
        this->setItem( rowCount, doDBCollector::colDisplayName, new QTableWidgetItem(displayName) );
    }


}


doDBEntry* doDBCollector::      getEntry( int index ){
// ntui
    if( index >= this->entrys.count() ) return NULL;

    return (doDBEntry*)(this->entrys.at(index));

}




