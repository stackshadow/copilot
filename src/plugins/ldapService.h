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

//#include "coPlugin.h"

#include <ldap.h>
#include <string.h>

class ldapService 
{

private:
    timeval         searchTimeout;
    LDAP*           ldapConnectionAdmin;
    bool            ldapConnectionAdminActive = false;
    LDAP*           ldapConnection;
    bool            ldapConnectionActive = false;

// credentials
    std::string     uri;

    std::string     admindn;
    std::string     adminpass;

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

public:
// API
    static int          onSubscriberMessage( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata );



private:
    bool                configLoad();
    bool                configToJson( json_t* jsonObject );
    bool                configFromJson( json_t* jsonObject );
    bool                configSave();
    bool                connect( LDAP** connection, int* version, int* timeout, const char* uri, const char* logindn, const char* pass );
    //bool            disconnect();

// ldap
    bool                ldapModAppend( LDAPMod*** mod, int* LDAPModLen, int op, const char* property, const char* value1, const char* value2 = NULL, const char* value3 = NULL );
    bool                ldapModMemFree( LDAPMod*** mod );

    bool                exist( LDAP* connection, const char* dn );
    bool                find( LDAP* connection, const char* searchBase, const char* attribute, const char* value, std::string* dn );
    bool                removeDN( const char* dn );
    bool                iterate( const char* searchBase );
    bool                dumpElement( LDAP* connection, LDAPMessage* actualMessage, json_t* jsonObjectOutput = NULL );
    bool                dumpAll( LDAP* connection, const char* basedn );
    bool                dumpChilds( LDAP* connection, const char* basedn, json_t* jsonObjectOutput );
    bool                dumpDBs( LDAP* connection, json_t* jsonObjectOutput );
    bool                dump( json_t* jsonObjectOutput, const char **attributes, const char* searchDN, bool singleDN = false, const char* LDAPFilter = NULL );



    bool                purge();

    bool                attributeAdd( const char* dn, const char* property, const char* value );

    bool                dbAdd( const char* suffix, const char* dbType = "bdb" );
    bool                dbChangeCreds( const char* suffix, const char* username, const char* password );
    bool                dbAddHash( const char* hash );
    bool                dbRemove( const char* configdn );

    bool                orgaAdd( const char* basedn );
    bool                orgaUnitAdd( const char* name, const char* base = NULL );

    bool                groupDump( json_t* jsonObjectOutput );
    bool                groupMembersDump( const char* groupName, json_t* jsonObjectOutput );
    bool                groupAdd( const char* name );
    bool                groupChange( const char* name, const char* description );
    bool                groupAddMember( const char* groupName, const char* memberUserName );
    bool                groupRemoveMember( const char* groupName, const char* memberUserName );

    bool                userDump( LDAP* connection, json_t* jsonObjectOutput );
    bool                userDumpMembership( const char* uid, json_t* jsonObjectOutput );
    bool                userAdd( const char* accountName );
    bool                userChange( const char* accountName, const char* passwordClearText, const char* mail = NULL );
    bool                userDelete( const char* uid );


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


