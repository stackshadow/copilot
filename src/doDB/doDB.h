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

#ifndef DODB_H
#define DODB_H

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

#include "qt/doDBConnection/doDBConnection.h"


class doDB
{
public:
    doDB();
    ~doDB();
    static doDB                 *ptr;

public:
    void                        connectionAppend( doDBConnection *newConnection );
    void                        connectionRemove( doDBConnection *newConnection );
    doDBConnection*             connectionGetFirst();
    doDBConnection*             connectionGetNext();
    doDBConnection*             connectionGet( const char *id );

    void                        connectionsLoad();
    void                        connectionsSave();

public:
    etDBObject                  *dbObjectCore;


private:
    QList<doDBConnection*>      connections;
    int                         connectionIndex;



};

#endif // DODB_H
