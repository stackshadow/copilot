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

#ifndef ldapService_H
#define ldapService_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

#include "coPlugin.h"

#include <ldap.h>
#include <string.h>

class ldapService : public coPlugin
{

private:
    timeval         searchTimeout;
    LDAP*           ldapConnection;
    bool            connected = false;

// credentials
    std::string     uri;
    
    std::string     logindn;
    std::string     loginpass;
    
    std::string     basedn;
    std::string     userdn;
    std::string     groupdn;

// config
    json_t*         jsonObjectConfig;

// iterator-stuff
    LDAPMessage*    resultMessages;


public:
                    ldapService();
                    ~ldapService();




private:
    bool            configLoad();
    bool            configToJson( json_t* jsonObject );
    bool            configFromJson( json_t* jsonObject );
    bool            connect();
    //bool            disconnect();

    bool            exist( const char* dn );
    bool            iterate( const char* searchBase );
    bool            dump( LDAPMessage* actualMessage );
    bool            dumpAll();


    bool            purge();


    bool            orgaAdd( const char *name, const char* base = NULL );
    
    bool            groupAdd( const char *name, const char* description, const char *firstMember );
    bool            groupAddToUser( const char* userdn, const char* groupdn );

    bool            userAdd( const char *name );



// overloaded functions
public:
/*
    bool            onBroadcastMessage(  const char*     msgHostName, 
                                const char*     msgGroup, 
                                const char*     msgCommand, 
                                json_t*         jsonData, 
                                json_t*         jsonAnswerObject );
*/
};

#endif // DODB_H


