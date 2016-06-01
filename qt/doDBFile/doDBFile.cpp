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

#include "doDBFile.h"

#include <QDebug>
#include <QUuid>

#include "main.h"
#include "doDBDebug/doDBDebug.h"

#include "doDBConnection/doDBConnection.h"


doDBFile::doDBFile() : QObject() {


// setup ui
    this->btnTableFolderEdit = NULL;
    this->tableFolderEditor = NULL;

// setup dbTree
    this->dbTree = NULL;
    this->dbTreeItemTypeFolder = -1;
    this->dbTreeItemTypeFile = -1;
    this->dbTreeItemColumnFolder = -1;

// remember stuff
    this->connectionID = "-1";
    this->tableName = "";
    this->actualJSON = NULL;

}

doDBFile::~doDBFile(){

}


void doDBFile::             doDBTreeInit( doDBtree *dbTree ){

// save
    this->dbTree = dbTree;

// setup types
    this->dbTreeItemTypeFolder = dbTree->newItemType();
    this->dbTreeItemTypeFile = dbTree->newItemType();

// setup column in tree
    this->dbTreeItemColumnFolder = dbTree->newTreeColumn( "folder", true );


    connect( this->dbTree, SIGNAL (itemExpanded(QTreeWidgetItem*)), this, SLOT (doDBTreeExpand(QTreeWidgetItem*)));
    connect( this->dbTree, SIGNAL (itemCollapsed(QTreeWidgetItem*)), this, SLOT (doDBTreeCollapsed(QTreeWidgetItem*)));
    connect( this->dbTree, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (doDBTreeClicked(QTreeWidgetItem*,int)));

    //connect( dbTree, SIGNAL (itemClicked(QTreeWidgetItem*,int)), this, SLOT (clicked(QTreeWidgetItem*,int)));
}


void doDBFile::             doDBItemViewInit( QLayout *itemView ){


// edit button
    this->btnTableFolderEdit = new QPushButton( "Ordner Editieren" );
    this->btnTableFolderEdit->setVisible( false );
    connect( this->btnTableFolderEdit, SIGNAL (clicked()), this, SLOT (tableFoldersEditorShow()));
    itemView->addWidget( this->btnTableFolderEdit );

// the editor itselfe
    this->tableFolderEditor = new doDBTableFoldersEditor(NULL);
    this->tableFolderEditor->setVisible( false );
    itemView->addWidget( this->tableFolderEditor );

}


void doDBFile::             doDBActionAreaInit( QLayout *actionArea ){



}





bool doDBFile::             loadFromDB(){


// vars
    doDBConnection          *connection = NULL;
    const char              *jsonCharDoDBFolder = NULL;
    json_error_t            jsonError;

// get connection
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return false;

// load from db
    if( ! connection->dbDoDBValueGet( "doDBFolder", &jsonCharDoDBFolder ) ) return false;

// free an existing one
    if( this->actualJSON != NULL ){
        json_decref( this->actualJSON );
        this->actualJSON = NULL;
    }

// parse json
    this->actualJSON = json_loads( jsonCharDoDBFolder, JSON_PRESERVE_ORDER, &jsonError );
    if( this->actualJSON == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s", jsonError.text );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }


/*

// vars
    json_t      *jsonTable = NULL;
    json_t      *jsonValue = NULL;
    const char  *jsonValueChar = NULL;
    void        *jsonFolderIterator = NULL;
    json_t      *jsonFolder = NULL;
    const char  *jsonFolderChar = NULL;

// OS ?
#ifdef __gnu_linux__
    QString         os = "linux";
#endif

// get table from root-json
    jsonTable = json_object_get( this->actualJSON, tableName );
    if( jsonTable == NULL ){
        return false;
    }


// iterate folders
    jsonFolderIterator = json_object_iter( jsonTable );
    while( jsonFolderIterator != NULL ){

        jsonFolder = json_object_iter_value(jsonFolderIterator);
        jsonValue = json_object_get( jsonFolder, "os" );
        jsonValueChar = json_string_value(jsonValue);

    // check if the entry is of our os
        if( os == jsonValueChar ){

            jsonFolderChar = json_object_iter_key(jsonFolderIterator);


        }


        jsonFolderIterator = json_object_iter_next(jsonTable,jsonFolderIterator);
    }


*/

}


bool doDBFile::             saveToDB(){
// check
    if( this->actualJSON == NULL ) return false;


// vars
    doDBConnection          *connection = NULL;

//dump json to char
    const char *jsonDump = json_dumps( this->actualJSON, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

// save it to the db
    connection = doDBCore->connectionGet( this->connectionID.toUtf8() );
    if( connection == NULL ) return false;

// load from db
    if( ! connection->dbDoDBValueSet( "doDBFolder", jsonDump ) ) return false;



    free( (void*)jsonDump );



}




bool doDBFile::             getFolderFromTable( const char *tableName, const char **folderName ){
// check
    if( this->actualJSON == NULL ) return false;

// OS ?
#ifdef __gnu_linux__
    QString         os = "linux";
#endif

// get table from root-json
    json_t *jsonTable = json_object_get( this->actualJSON, tableName );
    if( jsonTable == NULL ){
        return false;
    }

// iterate over all folder
    json_t      *jsonValue = NULL;
    const char  *jsonValueChar = NULL;
    void        *jsonFolderIterator = NULL;
    json_t      *jsonFolder = NULL;
    const char  *jsonFolderChar = NULL;

// iterate folders
    jsonFolderIterator = json_object_iter( jsonTable );
    while( jsonFolderIterator != NULL ){

        jsonFolder = json_object_iter_value(jsonFolderIterator);
        jsonValue = json_object_get( jsonFolder, "os" );
        jsonValueChar = json_string_value(jsonValue);

    // check if the entry is of our os
        if( os == jsonValueChar ){

            jsonFolderChar = json_object_iter_key(jsonFolderIterator);

        // check if we can write to the folder
            DIR *directory = opendir( jsonFolderChar );
            if( directory != NULL ){
                closedir( directory );
                *folderName = jsonFolderChar;

                doDBDebug::ptr->print( QString("%1: found folder '%2' for table '%3' !").arg(__PRETTY_FUNCTION__).arg(jsonFolderChar).arg(tableName) );

                return true;
            }

        }


        jsonFolderIterator = json_object_iter_next(jsonTable,jsonFolderIterator);
    }




    doDBDebug::ptr->print( QString("%1: no folder for table '%2' !").arg(__PRETTY_FUNCTION__).arg(jsonFolderChar) );
    return false;

}





QString doDBFile::          doDBTreeItemFolderNameGet( QTreeWidgetItem *treeItem ){
// if no item is passed, we use the selecte
    if( treeItem == NULL ) return "";

    return treeItem->text(this->dbTreeItemColumnFolder);
}

/**
@short Return the full Path of an doDBTree-Element
@param treeItem The actual tree item
@return an QString which holds the full tree of is "" if no folder/file is set
*/
QString doDBFile::          doDBTreeFullPathGet( QTreeWidgetItem *treeItem ){
// check
    if( treeItem == NULL ) return "";

// rootfolder
    QTreeWidgetItem     *actualItem = treeItem;
    QString             parentFolder = "";
    while( this->doDBTreeItemFolderNameGet(actualItem).length() > 0 ){
        #ifdef __gnu_linux__
        parentFolder = this->doDBTreeItemFolderNameGet(actualItem) + "/" + parentFolder;
        #endif

        actualItem = actualItem->parent();
        if( actualItem == NULL ) break;
    }

    //parentFolder = parentFolder + "/" + this->folderName(treeItem);



    return parentFolder;
}


bool doDBFile::             doDBTreeSetPathOfExistingItems( QTreeWidgetItem *treeItem, const char *folderName ){

    bool                returnValue = false;
    int                 itemCount = treeItem->childCount();
    int                 itemIndex = 0;
    QTreeWidgetItem     *actualItem = NULL;

    for( itemIndex = 0; itemIndex < itemCount; itemIndex++ ){
        actualItem = treeItem->child(itemIndex);
        if( actualItem->text(0) == folderName ){
            actualItem->setText( this->dbTreeItemColumnFolder , folderName );
            returnValue = true;
        }
    }


    return returnValue;
}


void doDBFile::             doDBTreeAppendFolder( QTreeWidgetItem *parentItem, const char *folderName ){

// ignore . and ..
#ifdef __gnu_linux__
    QString qFolderName = QString(folderName);
    if( qFolderName == "." ) return;
    if( qFolderName == ".." ) return;
#endif


// vars
    QTreeWidgetItem* item = NULL;

// check if the item already exist
    if( this->doDBTreeSetPathOfExistingItems( parentItem, folderName ) ){
        return;
    }

// create new
    item = new QTreeWidgetItem();

// icon-filename
    QString iconFileName = doDBSettingsGlobal->treePictureDirectory();
    iconFileName += "/folder.png";
    QIcon tableIcon = QIcon(iconFileName);
    item->setIcon( 0, QIcon(iconFileName) );

// create new item
    item->setText( 0, folderName );
    item->setText( 4, QString().number(this->dbTreeItemTypeFolder) );
    item->setText( this->dbTreeItemColumnFolder, folderName );

// we always have childs
    item->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

    parentItem->addChild(item);

    return;
}


void doDBFile::             doDBTreeAppendFile( QTreeWidgetItem *parentItem, const char *fileName ){


// check if the item already exist
    if( this->doDBTreeSetPathOfExistingItems( parentItem, fileName ) ){
        return;
    }

// create new item
    QTreeWidgetItem* item = new QTreeWidgetItem();

// icon-filename
    QString iconFileName = doDBSettingsGlobal->treePictureDirectory();
    iconFileName += "/file.png";
    QIcon tableIcon = QIcon(iconFileName);
    item->setIcon( 0, QIcon(iconFileName) );

// create new item
    item->setText( 0, fileName );
    item->setText( 4, QString().number(this->dbTreeItemTypeFile) );
    item->setText( this->dbTreeItemColumnFolder, fileName );

    parentItem->addChild(item);

    return;
}


void doDBFile::             doDBTreeExpand( QTreeWidgetItem * item ){

// vars
    //doDBConnection              *connection;
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    QString                     itemFolder;
    const char                  *tempChar = NULL;

// get Stuff from the selected item
    tableName = this->dbTree->itemTableName();
    connectionID = this->dbTree->itemConnectionID();
    itemID = this->dbTree->itemID();
    itemType = this->dbTree->itemType();
    itemFolder = this->doDBTreeItemFolderNameGet(item);

    if( itemType == doDBtree::typeTable ){

    // an table with another connection ID
        if( this->connectionID != connectionID || actualJSON == NULL ){
            this->connectionID = connectionID;
            this->loadFromDB();
        }

    // load filename for table
        if( this->getFolderFromTable( tableName.toUtf8(), &tempChar ) ){

            itemFolder = tempChar;

        // set the tale-folder
            item->setText( this->dbTreeItemColumnFolder, itemFolder );

        }

    }

    if( itemFolder.length() > 0 ){
        QString qFolderName = this->doDBTreeFullPathGet(item);
        const char *tempChar = qFolderName.toUtf8();

    // open dir
        DIR *directory = opendir( qFolderName.toUtf8() );
        if( directory != NULL ){

            dirent *dirEntry = readdir(directory);
            while( dirEntry != NULL ){

                if( dirEntry->d_type == DT_DIR ){
                    this->doDBTreeAppendFolder( item, dirEntry->d_name );
                }
                if( dirEntry->d_type == DT_REG ){
                    this->doDBTreeAppendFile( item, dirEntry->d_name );
                }

                dirEntry = readdir(directory);
            }

            closedir(directory);
        }


    }



}


void doDBFile::             doDBTreeCollapsed( QTreeWidgetItem * item ){

// vars
    doDBtree::treeItemType      itemType;

    itemType = this->dbTree->itemType();

    if( itemType == this->dbTreeItemTypeFolder ){
        item->takeChildren();
    }
}


void doDBFile::             doDBTreeClicked( QTreeWidgetItem * item, int column ){

// vars
    //doDBConnection              *connection;
    QString                     tableName;
    QString                     connectionID;
    QString                     itemID;
    doDBtree::treeItemType      itemType;
    QString                     itemFolder;
    const char                  *tempChar = NULL;

// get Stuff from the selected item
    tableName = this->dbTree->itemTableName( item );
    connectionID = this->dbTree->itemConnectionID( item );
    itemID = this->dbTree->itemID( item );
    itemType = this->dbTree->itemType( item );
    itemFolder = this->doDBTreeItemFolderNameGet( item );

    if( itemType == doDBtree::typeTable ){
        this->connectionID = connectionID;
        this->tableName = tableName;
        this->btnTableFolderEdit->setVisible(true);
    } else {
        this->btnTableFolderEdit->setVisible(false);
        this->tableFolderEditor->setVisible( false );
    }


}





void doDBFile::             tableFoldersEditorShow(){

// load from db
    this->loadFromDB();
    this->tableFolderEditor->loadJson( this->actualJSON, this->tableName.toUtf8() );

// show editor
    this->tableFolderEditor->setVisible( true );




}



