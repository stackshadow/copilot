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

#ifndef nftService_C
#define nftService_C

#include "coCore.h"
#include "plugins/nftService.h"


nftService::                    	nftService() : coPlugin( "nft", coCore::ptr->hostNameGet(), "nft" ) {

    this->jsonRootObject = NULL;

// json
    this->load();

// register plugin
	coCore::ptr->plugins->append( this );

}


nftService::                    	~nftService(){
    if( this->jsonRootObject != NULL ){
        json_decref( this->jsonRootObject );
    }
}





coPlugin::t_state nftService::		onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgTarget = message->nodeNameTarget();
    const char*			msgSource = message->nodeNameSource();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();



    std::string     answerTopic;




    if( strncmp( (char*)msgCommand, "chainsList", 10 ) == 0 ){


    // get the host
        if( this->selectHost(msgTarget) == false ){
            return coPlugin::NO_REPLY;
        }

        char* jsonString = json_dumps( this->jsonChainsObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

        coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource, "nft", "chains", jsonString );

        free( (void*)jsonString );
        return coPlugin::REPLY;
    }


    if( strncmp( (char*)msgCommand, "save", 4 ) == 0 ){

        json_error_t    jsonError;
        int             msgPayloadLen = 0;
        json_t*         jsonChains = NULL;

    // get the host
        if( this->selectHost(msgTarget) == false ) return coPlugin::NO_REPLY;


    // because jsonPayload is a string, we need to reparse it
        if( msgPayload == NULL ) return coPlugin::NO_REPLY;
        msgPayloadLen = strlen(msgPayload);
        jsonChains = json_loads( msgPayload, msgPayloadLen, &jsonError );
        if( jsonError.column >= 0 ) return coPlugin::NO_REPLY;


    // overwrite with new chains
        json_object_set_new( this->jsonHostObject, "chains", jsonChains );

    // save it to the file
        this->save();

        coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource, "nft", "saveok", "" );


        return coPlugin::REPLY;
    }


    if( strncmp( (char*)msgCommand, "apply", 4 ) == 0 ){
        if( this->applyRules(msgTarget, message) == false ){
			return coPlugin::REPLY;
		}

        //coCore::ptr->plugins->messageAdd( this, msgTarget, msgSource, "nft", "applyok", "" );

        return coPlugin::REPLY;
    }


    return coPlugin::NO_REPLY;


}




void nftService::               	load(){

    json_error_t jsonError;
    this->jsonRootObject = json_load_file( baseFilePath "nftrules.json", JSON_PRESERVE_ORDER, &jsonError );
    if( jsonError.position == 0 || jsonError.line >= 0 ){

    // report
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not load json-file: %s", jsonError.text );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

    // there is an error, we create an empty element
        this->jsonRootObject = json_object();
    }




}


bool nftService::               	save(){
    int state = json_dump_file( this->jsonRootObject,
                    baseFilePath "nftrules.json", JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    if( state == 0 ){
        return true;
    }

    return false;
}




void nftService::               	iterate(){

// reset
    this->jsonHostsObject = this->jsonRootObject;

// get the first host
    this->jsonHostsIterator = json_object_iter( this->jsonHostsObject );


    this->jsonChainsObject = NULL;

}


bool nftService::               	nextHost( const char** host ){



// no more hosts host, we finished
    if( this->jsonHostsIterator == NULL ){
        return false;
    }

// remember the host object
    this->jsonHostObject = json_object_iter_value( this->jsonHostsIterator );

// reset the rest
    this->jsonChainsObject = NULL;
    this->jsonChainsIterator = NULL;
    this->jsonRulesArray = NULL;
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = 0;
    this->jsonRuleObject = NULL;

// return the hostname
    *host = json_object_iter_key( this->jsonHostsIterator );

// next
    this->jsonHostsIterator = json_object_iter_next( this->jsonHostsObject, this->jsonHostsIterator );


    return true;
}


bool nftService::               	nextChain( const char** chainName ){
// precheck
    if( this->jsonHostObject == NULL ) return false;

// get the "chains"-object
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
        if( this->jsonChainsObject == NULL ) {
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Chain '%s' not aviable in host.", chainName );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }

    // get the iter and the first chain
        this->jsonChainsIterator = json_object_iter( this->jsonChainsObject );

    }

// the end of the chains
    if( this->jsonChainsIterator == NULL ) return false;

// remember the chain object
    this->jsonRulesArray = json_object_iter_value( this->jsonChainsIterator );
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = json_array_size( this->jsonRulesArray );
    this->jsonRuleObject = NULL;


// return the hostname
    const char* tmpChainName = json_object_iter_key( this->jsonChainsIterator );
    if( chainName != NULL ){
        *chainName = tmpChainName;
    }

// next
    this->jsonChainsIterator = json_object_iter_next( this->jsonChainsObject, this->jsonChainsIterator );


    return true;
}


bool nftService::               	nextRule( const char **type ){

// end of the list
    if( this->jsonRulesArrayIndex >= this->jsonRulesArrayLen ){
        this->jsonRulesArrayLen = json_array_size( this->jsonRulesArray );
        this->jsonRulesArrayIndex = 0;
        return false;
    }

// get the rule
    this->jsonRuleObject = json_array_get( this->jsonRulesArray, this->jsonRulesArrayIndex );
    if( this->jsonRuleObject == NULL ) return false;

// get the type
    json_t*     jsonRuleType = json_object_get( this->jsonRuleObject, "rule" );
    if( jsonRuleType == NULL ) return false;
    if( type != NULL ){
        *type = json_string_value( jsonRuleType );
    }

// iterate
    this->jsonRulesArrayIndex++;

    return true;
}




bool nftService::               	selectHost( const char* hostName ){

    if( this->jsonRootObject == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No root in json-file" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }




// the host-object
    this->jsonHostObject = json_object_get( this->jsonRootObject, hostName );
    if( this->jsonHostObject == NULL ){
        this->jsonHostObject = json_object();

	/** @todo This is maybe useless, because we WANT to select a host
	not use only the local hostname.
	*/
	// vars
		const char* localHostName = NULL;
		coCore::ptr->hostNameGet( &localHostName, NULL );

    // add the host
        json_object_set_new( this->jsonRootObject, localHostName, this->jsonHostObject );
        json_object_set_new( this->jsonHostObject, "displayName", json_string(localHostName) );
    }


// the chains-object inside the host
    this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object();
        json_object_set_new( this->jsonHostObject, "chains", this->jsonChainsObject );

    // add default chains
        json_object_set_new( this->jsonChainsObject, "prerouting", json_array() );
        json_object_set_new( this->jsonChainsObject, "input", json_array() );
        json_object_set_new( this->jsonChainsObject, "output", json_array() );
        json_object_set_new( this->jsonChainsObject, "forward", json_array() );
        json_object_set_new( this->jsonChainsObject, "postrouting", json_array() );
    }

// reset the rest
    this->jsonChainsIterator = NULL;
    this->jsonRulesArray = NULL;
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = 0;
    this->jsonRuleObject = NULL;

    return true;
}


bool nftService::               	selectChain( const char* chainName ){
// precheck
    if( this->jsonHostObject == NULL ) return false;

// get the "chains"-object
    if( this->jsonChainsObject == NULL ){
        this->jsonChainsObject = json_object_get( this->jsonHostObject, "chains" );
        if( this->jsonChainsObject == NULL ) {
            return false;
        }
    }

// get the selected chain-array
    this->jsonRulesArray = json_object_get( this->jsonChainsObject, chainName );
    if( this->jsonRulesArray == NULL ) return false;

// reset the rest
    this->jsonRulesArrayIndex = 0;
    this->jsonRulesArrayLen = json_array_size(this->jsonRulesArray);


    return true;
}





bool nftService::               	createRuleCommand( std::string* nftCommand, const char* chainName, json_t* jsonRule ){

    // vars
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;

// rule active ?
    jsonString = json_object_get( jsonRule, "active" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );
    if( strncmp(jsonStringChar,"1",1) != 0 ){
        return false;
    }

// create
    *nftCommand = "sudo nft add rule ";

// add family
    jsonString = json_object_get( jsonRule, "family" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );
    if( strncmp(jsonStringChar,"ip4",3) == 0 ){
        *nftCommand += "ip ";
    } else {
        *nftCommand += "ip6 ";
    }

    *nftCommand += "default ";

// add chain
    *nftCommand += chainName;
    *nftCommand += " ";

// add the rule
    jsonString = json_object_get( jsonRule, "rule" );
    if( jsonString == NULL ) return false;
    jsonStringChar = json_string_value( jsonString );

    *nftCommand += jsonStringChar;

    return true;
}


bool nftService::               	applyChain( const char* chainName, const char* chainType, coMessage* message ){

// vars
    std::string     command;
    int             returnValue = -1;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;

    if( message != NULL ){
        msgSource = message->nodeNameSource();
        msgTarget = message->nodeNameTarget();
    } else {
        msgSource = "";
        msgTarget = "";
    }

    if( this->selectChain(chainName) == true ){

    // add ip ( v4 ) chain
        command  = "sudo nft add chain ip default ";
        command += chainName;
        command += " { type ";
        command += chainType;
        command += " hook ";
        command += chainName;
        command += " priority 100 \\; }";
        returnValue = system( command.c_str() );
        fprintf( stdout, "nft: %s\n", command.c_str() );
        if( returnValue != 0 ){
            coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
            "nft", "msgError", "Could not create chain" );
            return false;
        }

    // add ip ( v6 ) chain
        command  = "sudo nft add chain ip6 default ";
        command += chainName;
        command += " { type ";
        command += chainType;
        command += " hook ";
        command += chainName;
        command += " priority 100 \\; }";
        returnValue = system( command.c_str() );
        fprintf( stdout, "nft: %s\n", command.c_str() );
        if( returnValue != 0 ){
            coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
            "nft", "msgError", "Could not create chain" );
            return false;
        }

    // apply rules
        while( this->nextRule(NULL) == true ){
            if( nftService::createRuleCommand( &command, chainName, this->jsonRuleObject ) == true ){
                fprintf( stdout, "nft: %s\n", command.c_str() );
                returnValue = system( command.c_str() );
                if( returnValue != 0 ){

					command = "Could not apply " + command;

                    coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
                    "nft", "msgError", command.c_str() );

                    return false;
                }
            }
        }
    }

    return true;
}


bool nftService::               	applyRules( const char* hostName, coMessage* message ){

// vars
    int             returnValue = -1;
    std::string     command;
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;

    if( message != NULL ){
        msgSource = message->nodeNameSource();
        msgTarget = message->nodeNameTarget();
    } else {
        msgSource = "";
        msgTarget = "";
    }

// select host
    if( this->selectHost( hostName ) == false ) return false;

// save the original rules
    //sudo nft list ruleset > /tmp/fw.rules
    //sudo nft -f /tmp/fw.rules


//
    returnValue = system( "sudo nft flush ruleset" );
    fprintf( stdout, "nft: sudo nft flush ruleset\n" );
    if( returnValue != 0 ){
        coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
        "nft", "msgError", "Could not flush rules" );
        return false;
    }

// create tables
    returnValue = system( "sudo nft add table ip default" );
    fprintf( stdout, "nft: sudo nft add table ip default\n" );
    if( returnValue != 0 ){
        coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
        "nft", "msgError", "Could not create ip table" );
        return false;
    }
    returnValue = system( "sudo nft add table ip6 default" );
    fprintf( stdout, "nft: sudo nft add table ip6 default\n" );
    if( returnValue != 0 ){
        coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
        "nft", "msgError", "Could not create ip6 table" );
        return false;
    }

// prerouting
    if( this->applyChain( "prerouting",     "nat",      message ) == false ) return false;
    if( this->applyChain( "input",          "filter",   message ) == false ) return false;
    if( this->applyChain( "forward",        "filter",   message ) == false ) return false;
    if( this->applyChain( "output",         "filter",   message ) == false ) return false;
    if( this->applyChain( "postrouting",    "nat",      message ) == false ) return false;


    coCore::ptr->plugins->messageQueue->add( this, msgTarget, msgSource,
    "nft", "msgInfo", "Rules active." );

    return true;
}


bool nftService::               	applyRules(){
	const char* hostName = NULL;
	coCore::ptr->hostNameGet( &hostName, NULL );
	this->applyRules( hostName );
}



#endif // ldapService_C
