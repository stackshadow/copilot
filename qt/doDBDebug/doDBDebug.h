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

#ifndef DODBDEBUG_H
#define DODBDEBUG_H


#include <QString>
#include <QLineEdit>
#include <QTableWidget>
#include "core/etDebug.h"

class doDBDebug
{
public:
    doDBDebug();
    ~doDBDebug();

    //static doDBDebug*       ptrGet();

    void                    registerDebugLine( QLineEdit *lineEdit );
    void                    registerHistroyWidget( QTableWidget *tableWidget );

    void                    print( QString string );
    void                    print( QString programName, QString levelName, QString functionName, QString message );

    static void             evillibPrint( etDebug* etDebugActual );

public:
    static doDBDebug        *ptr;

private:
    QLineEdit               *lineEdit;
    QTableWidget            *tableWidget;

};

#endif // DODBDEBUG_H
