/*  Copyright (C) 2017 by Martin Langlotz alias stackshadow

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

#ifndef ldapService_C
#define ldapService_C

#include "coCore.h"
#include "plugins/ldapService.h"

ldapService::                   ldapService() : coPlugin( "ldap" ) {


    const char*     nodesText = "nodes/";
    int             nodesLen = 6;
    const char*     topicText = "/co/ldap";
    int             topicLen = 8;
    int             fillLength = nodesLen + coCore::ptr->hostNodeNameLen + topicLen + 1;

// create the full topic
    char topic[fillLength];
    memset( &topic, 0, fillLength );

//    memcpy( topic, "nodes/\0", 7 );
//    memcpy( &topic[6], coCore::ptr->hostInfo.nodename, coCore::ptr->hostNodeNameLen );
    strncpy( topic, nodesText, nodesLen );
    strncat( topic, coCore::ptr->hostInfo.nodename, coCore::ptr->hostNodeNameLen );
    strncat( topic, topicText, topicLen );

// register this plugin
    coCore::ptr->registerPlugin( this, topic );


}








#endif // ldapService_C
