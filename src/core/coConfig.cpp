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


#ifndef coCoreConfig_C
#define coCoreConfig_C



#include "coConfig.h"
#include "coCore.h"

coConfig* coConfig::ptr = NULL;

coConfig::              coConfig(){
    coConfig::ptr = this;

// init lock
    this->threadLock = 0;

// options for config
    coCore::addOption( "configpath", "c", "<path>: Sets the path where the config of copilotd lives", required_argument );
    coCore::addOption( "nodename", "n", "<nodename>: Sets the name of this node", required_argument );
    coCore::addOption( "connect", "o", "<host:port>: Connect to another copilotd", required_argument );
    coCore::addOption( "accept", "a", "<port>: Listen on port", required_argument );
}


coConfig::              ~coConfig(){
    if( this->jsonConfig != NULL ){
        json_decref( this->jsonConfig );
    }
}




bool coConfig::         parseOpt( int argc, char *argv[] ){
// reset getopt
    optind = 1;

    int optionSelected = 0;
    while( optionSelected >= 0 ) {
        optionSelected = getopt_long(argc, argv, "", coCore::ptr->options, NULL);
        if( optionSelected < 0 ) break;

        if( coCore::isOption( "help", optionSelected ) == true ){
            fprintf( stdout, "\n====== config ======\n" );
            fprintf( stdout, "--configpath <path>: Sets the path where the config of copilotd lives\n" );
            fprintf( stdout, "--nodename <nodename>: Sets the name of this node\n" );
            break;
        }

        if( coCore::isOption( "nodename", optionSelected ) == true ){
            this->load();
            this->nodeName( optarg );
            this->save();
            continue;
        }

        if( coCore::isOption( "accept", optionSelected ) == true ){

            int     port = atoi( optarg );

            this->load();
            this->nodesIterate();
            if( this->nodeSelect( coCore::ptr->nodeName() ) == false ){
                nodeAppend( coCore::ptr->nodeName() );
            }
            this->nodeConnInfo( NULL, &port, true );
            this->nodesIterateFinish();
            this->save();

            continue;
        }

        if( coCore::isOption( "connect", optionSelected ) == true ){
            char*   host = strtok( optarg, ":" );
            char*   portChar = strtok( NULL, ":" );
            int     port = atoi( portChar );
            coConfig::nodeType  type = coConfig::CLIENT;

            this->load();
            this->nodesIterate();
            if( this->nodeSelectByHostName( host ) == false ){
                nodeAppend( host );
            }
            this->nodeInfo( NULL, &type, true );
            this->nodeConnInfo( (const char**)&host, &port, true );
            this->nodesIterateFinish();
            this->save();

            continue;
        }

    }

    return true;
}


bool coConfig::         load(){
	lockMyPthread();

// vars
    const char*     baseConfigPath = NULL;
    std::string     configFile;
    json_error_t    jsonError;
	json_t*			jsonValue;
	bool			saveToFile = false;

// get config path
    baseConfigPath = coCore::ptr->configPath();
    configFile  = baseConfigPath;
    configFile += "/";
    configFile += "core.json";

// already loaded ? free it
    if( this->jsonConfig != NULL ){
        json_decref( this->jsonConfig );
    }

// clear
	this->jsonConfig = NULL;
	this->jsonNodes = NULL;
	this->jsonNodesIterator = NULL;

// check core-config path
	if( access( baseConfigPath, F_OK ) != 0 ){
        std::string createDirCmd = "mkdir -p ";
        createDirCmd += baseConfigPath;
		system( createDirCmd.c_str() );
	}

// open the file
    this->jsonConfig = json_load_file( configFile.c_str(), JSON_PRESERVE_ORDER, &jsonError );
    if( this->jsonConfig == NULL ){
        this->jsonConfig = json_object();
		saveToFile = true;
    }

// get nodes
	this->jsonNodes = json_object_get( this->jsonConfig, "nodes" );
	if( this->jsonNodes == NULL ){
		this->jsonNodes = json_object();
		json_object_set_new( this->jsonConfig, "nodes", this->jsonNodes );
		saveToFile = true;
	}

// users
    json_t* jsonUsers = json_object_get( this->jsonConfig, "users" );
    if( jsonUsers == NULL ){
        jsonUsers = json_object();
        json_object_set_new( this->jsonConfig, "users", jsonUsers );
        saveToFile = true;
    }



// save if needed
	if( saveToFile == true ){
		this->save(NULL);
	}

	unlockMyPthread();
	return true;
}


bool coConfig::         save( const char* jsonString ){
	lockMyPthread();

// vars
	json_t*			jsonObject = NULL;
	json_error_t	jsonError;

	if( jsonString != NULL ){
	// parse json
		jsonObject = json_loads( jsonString, JSON_PRESERVE_ORDER | JSON_INDENT(4), &jsonError );
		if( jsonObject == NULL || jsonError.line > -1 ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "json-error: %s", jsonError.text );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

			unlockMyPthread();
			return false;
		}

	// cool, we parse the json, now we can save it
	// but before we release the old memory
		if( this->jsonConfig != NULL ){
			json_decref( this->jsonConfig );
		}

	// set
		this->jsonConfig = jsonObject;
	}

// remove states
    this->nodeStatesRemove();

// get config path
    const char*     baseConfigPath = coCore::ptr->configPath();
    std::string     configFile;
    configFile  = baseConfigPath;
    configFile += "/";
    configFile += "core.json";


/// @todo Copy the old config-file with timestamp as a backup


// save the json to file
    if( json_dump_file( this->jsonConfig, configFile.c_str(), JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Save config to %s", configFile.c_str() );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        unlockMyPthread();
        return true;
    }

// error
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not save config-file to %s%s", configFile.c_str(), "core.json" );
    etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    unlockMyPthread();
    return false;
}




/**
 * @brief Get a section inside the config file
 * @param sectionName
 * @return A json_t object which should NOT be freed !
 */
json_t* coConfig::      section( const char* sectionName ){

// vars
    json_t* jsonSection;

// try to get section
	jsonSection = json_object_get( this->jsonConfig, sectionName );
	if( jsonSection == NULL ){
		jsonSection = json_object();
		json_object_set_new( this->jsonConfig, sectionName, jsonSection );
	}

// return
    return jsonSection;
}




const char* coConfig::  nodeName( const char* nodeName ){

// Get
    if( nodeName == NULL ){

    // try to get the nodeName
        json_t* jsonNodeName = json_object_get( this->jsonConfig, "nodeName" );

    // not found
        if( jsonNodeName == NULL ) return NULL;

    // found, return it
        return json_string_value(jsonNodeName);
    }

// set
    else {
        json_object_set_new( this->jsonConfig, "nodeName", json_string(nodeName) );
        return nodeName;
    }

    return NULL;
}





bool coConfig::         nodesGet( json_t** jsonObject ){
	if( jsonObject == NULL ) return false;
	if( this->jsonNodes == NULL ) return false;

	*jsonObject = this->jsonNodes;
	return true;
}


bool  coConfig::        nodesGetAsArray( json_t* jsonArray ){
	if( jsonArray == NULL ) return false;
	if( this->jsonNodes == NULL ) return false;
	lockMyPthread();


// vars
	void*		iterator = NULL;

	iterator = json_object_iter( this->jsonNodes );
	while( iterator != NULL ){
		json_array_append_new( jsonArray, json_string(json_object_iter_key(iterator)) );
		iterator = json_object_iter_next( this->jsonNodes, iterator );
	}

// return
	unlockMyPthread();
	return true;
}


void coConfig::         nodeStatesRemove(){
	if( this->jsonNodes == NULL ) return;
	lockMyPthread();

// vars
    void*           iterator = NULL;
    json_t*         node = NULL;


    iterator = json_object_iter( this->jsonNodes );
    while( iterator != NULL ){

    // get the node
        node = json_object_iter_value(iterator);

    // remove the state
        json_object_del( node, "state" );

    // next
        iterator = json_object_iter_next( this->jsonNodes, iterator );
    }


// return
	unlockMyPthread();
	return;
}






bool coConfig::			nodesIterate(){
	lockMyPthread();
	this->jsonNodesIterator = NULL;
	return true;
}


bool coConfig::			nodeAppend( const char* name ){
	if( this->jsonNodes == NULL ) return false;


// create new Object
	this->jsonNode = json_object();
	if( json_object_set_new( this->jsonNodes, name, this->jsonNode ) != 0 ){
		return false;
	}

// return
	return true;
}


bool coConfig::			nodeRemove( const char* name ){
	if( this->jsonNodes == NULL ) return false;

    json_object_del( this->jsonNodes, name );

    return true;
}


bool coConfig::		    nodeSelect( const char* name ){
	if( this->jsonNodes == NULL ) return false;

	this->jsonNode = json_object_get( this->jsonNodes, name );
	if( this->jsonNode != NULL ) return true;

	return false;
}


bool coConfig::		    nodeSelectByHostName( const char* hostName ){
	if( this->jsonNodes == NULL ) return false;

// vars
    void*           iterator = NULL;
    json_t*         node = NULL;
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;


    iterator = json_object_iter( this->jsonNodes );
    while( iterator != NULL ){

    // get the node
        node = json_object_iter_value(iterator);

    // get the host
        jsonString = json_object_get( node, "host" );
        if( jsonString != NULL ) {

        // compare to parameter
            jsonStringChar = json_string_value(jsonString);
            if( strncmp(hostName,jsonStringChar,strlen(hostName)) == 0 ){
                this->jsonNode = node;
                return true;
            }

        }

    // next
        iterator = json_object_iter_next( this->jsonNodes, iterator );
    }

// unlock
	return false;
}


bool coConfig::			nodeGet( json_t** jsonNode ){
    if( jsonNode == NULL ) return false;

    *jsonNode = this->jsonNode;
    return true;
}


bool coConfig::			nodeNext(){
	if( this->jsonNodes == NULL ) return false;



// iterate from beginning
	if( this->jsonNodesIterator == NULL ){
		this->jsonNodesIterator = json_object_iter( this->jsonNodes );
	} else {
// next node
		this->jsonNodesIterator = json_object_iter_next( this->jsonNodes, this->jsonNodesIterator );
		if( this->jsonNodesIterator == NULL ){
			return false;
		}
	}

// get json-object
	this->jsonNode = json_object_iter_value( this->jsonNodesIterator );
	if( this->jsonNode == NULL ) return false;

	return true;
}


bool coConfig::			nodeInfo( const char** name, coConfig::nodeType* type, bool set ){
    if( this->jsonNode == NULL ) return false;

// vars
	json_t*		jsonVar = NULL;

// name
	if( name != NULL ){
		if( set == false ){
            if( this->jsonNodesIterator != NULL ){
                *name = json_object_iter_key( this->jsonNodesIterator );
            }
		}
	}

// type
	if( type != NULL ){
		if( set == false ){
			jsonVar = json_object_get( this->jsonNode, "type" );
			if( jsonVar == NULL ){
				jsonVar = json_integer( coConfig::UNKNOWN );
				json_object_set_new( this->jsonNode, "type", jsonVar );
			}
			*type = (coConfig::nodeType)json_integer_value( jsonVar );
		} else {
			json_object_set_new( this->jsonNode, "type", json_integer(*type) );
		}
	}

	return true;
}


bool coConfig::			nodeConnInfo( const char** host, int* port, bool set ){

// vars
	json_t*		jsonVar = NULL;

// host
	if( host != NULL ){

	// get values
		if( set == false ){
			jsonVar = json_object_get( this->jsonNode, "host" );
			if( jsonVar == NULL ){
				*host = NULL;
			} else {
				*host = json_string_value( jsonVar );
			}
		}
	// save values
		else {
			if( *host != NULL ){
				json_object_set_new( this->jsonNode, "host", json_string(*host) );
			}
		}

	}

// port
	if( port != NULL ){

	// get values
		if( set == false ){
			jsonVar = json_object_get( this->jsonNode, "port" );
			if( jsonVar == NULL ){
				*port = 9876;
			} else {
				*port = (int)json_integer_value( jsonVar );
			}
		}
	// save values
		else {
			json_object_set_new( this->jsonNode, "port", json_integer(*port) );
		}

	}


// return
	return true;
}


bool coConfig::         nodeIsServer( const char* name ){
    lockMyPthread();

// vars
	json_t*		                jsonVar = NULL;
    coConfig::nodeType      type;


    if( this->nodeSelect(name) == false ){
        unlockMyPthread();
        return false;
    }

    jsonVar = json_object_get( this->jsonNode, "type" );
    if( jsonVar == NULL ){
        unlockMyPthread();
        return false;
    }

    jsonVar = json_integer( coConfig::UNKNOWN );
    type = (coConfig::nodeType)json_integer_value( jsonVar );

// check
    if( type == coConfig::SERVER ){
        unlockMyPthread();
        return true;
    }


    unlockMyPthread();
    return true;
}


bool coConfig::         isServer( const char* nodeName ){
    if( coConfig::ptr == NULL ) return false;

    return coConfig::ptr->nodeIsServer( nodeName );
}


bool coConfig::			nodesIterateFinish(){
	unlockMyPthread();
	return true;
}




    // user / password
bool coConfig::         authMethode(){
    if(	this->jsonConfig == NULL ) return false;

// get the methode
    json_t* methode = json_object_get( this->jsonConfig, "authMethode" );
    if( methode == NULL ){
        methode = json_string("none");
        json_object_set_new( this->jsonConfig, "authMethode", methode );
    }

// if none we return false
    if( strncmp(json_string_value(methode),"none",4) == 0 ){
        return false;
    }

    return true;
}


bool coConfig::         userAdd( const char* username ){
    if(	this->jsonConfig == NULL ) return false;

// all users
    json_t* jsonUsers = json_object_get( this->jsonConfig, "users" );
    if( jsonUsers == NULL ) return NULL;

// add ( and overwrite ) user
    json_t* jsonUser = json_object();
    json_object_set_new( jsonUsers, username, jsonUser );

    return true;
}


bool coConfig::         userCheck( const char* username, const char* password ){

    return true;
}



#endif
