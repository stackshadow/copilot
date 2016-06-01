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

#ifndef doDBtree_H
#define doDBtree_H

#include <QTreeWidget>

class doDBtree :
public QTreeWidget
{
    Q_OBJECT

public:
    typedef enum treeItemType {
        typeNothing = 0,
        typeTable = 1,
        typeEntry = 2,
        typeRelatedTable = 3,
    } treeItemType;

public:
    doDBtree( QWidget *parent );
    ~doDBtree();

private:
    QTreeWidgetItem*        findItem( QString parentTableName );
    QTreeWidgetItem*        findItem( QString parentID, QString table );

public:
    QTreeWidgetItem*        append( QTreeWidgetItem *parentItem, QString displayName, QString table, QString connectionID, QString itemId, treeItemType itemType );
    QTreeWidgetItem*        find( QTreeWidgetItem *parentItem, QString displayName );

// request an new type or an new column
    int                     newItemType();
    int                     newTreeColumn( QString columnName, bool hide );

// handle an single item
    QString                 itemDisplayValue( QTreeWidgetItem *treeItem = NULL );
    QString                 itemID( QTreeWidgetItem *treeItem = NULL );
    QString                 itemConnectionID( QTreeWidgetItem *treeItem = NULL );
    QString                 itemTableName( QTreeWidgetItem *treeItem = NULL );
    treeItemType            itemType( QTreeWidgetItem *treeItem = NULL );

    bool                    tableExist( QTreeWidgetItem *parentItem, QString tableName );



    void                    refresh();

private slots:
    void                    expand( QTreeWidgetItem * item );
    void                    collapsed( QTreeWidgetItem * item );

public:
    static void             callbackTableAdd( void *userdata, const char *connID, const char *tableName, const char *tableDisplayName );
    static void             callbackEntryAdd( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue );


private:
    QString                 picturePath;
    int                     treeItemTypeLast;
    int                     treeColumnLast;


public:
    QTreeWidgetItem*        selectedItem;

};

#endif // DODBTREE_H
