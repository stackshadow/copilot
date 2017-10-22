/*
Copyright (C) 2017 by Martin Langlotz

This file is part of copilot.

copilot is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3 of this License

copilot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with copilot.  If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef nftService_H
#define nftService_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

#include "coPlugin.h"

#include <string>
#include "jansson.h"

/*
// create custom rule
	var customRule = {};
	customRule["rule"] = customRuleText;

// create rule
	var newRule = {};
	newRule["active"] = "0";
	newRule["descr"] = customDescription;
	newRule["type"] = "custom";
	newRule["rule"] = customRule;
*/
class nftService : public coPlugin
{
private:
    json_t*         jsonRootObject;
    json_t*         jsonHostsObject;
    void*           jsonHostsIterator;
    json_t*         jsonHostObject;
    json_t*         jsonChainsObject;
    void*           jsonChainsIterator;
    json_t*         jsonRulesArray;
    int             jsonRulesArrayLen;
    int             jsonRulesArrayIndex;
    json_t*         jsonRuleObject;



public:
                    nftService();
                    ~nftService();
    bool            applyRules();

private:
    void            load();
    bool            save();

    void            iterate();
    bool            nextHost( const char** host );
    bool            nextChain( const char** chainName );
    bool            nextRule( const char **type );

    bool            selectHost( const char* hostName );
    bool            selectChain( const char* chainName );

    static bool     createRuleCommand( std::string* nftCommand, const char* chainName, json_t* jsonRule );
    bool            applyChain( const char* chainName, const char* chainType, coMessage* message = NULL );
    bool            applyRules( const char* hostName, coMessage* message = NULL );



// overloaded functions
public:
	coPlugin::t_state 		onBroadcastMessage( coMessage* message );

};












#endif // nftService_H




