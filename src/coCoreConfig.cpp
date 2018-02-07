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


#include "coCoreConfig.h"

coCoreConfig::				coCoreConfig(){
// init lock
	this->threadLock = 0;

// settings
    etStringAlloc( this->configBasePath );
    etStringCharSet( this->configBasePath, "/etc/copilot", -1 );
}


coCoreConfig::				~coCoreConfig(){
    if( this->jsonConfig != NULL ){
        json_decref( this->jsonConfig );
    }
}




bool coCoreConfig::			load( const char* myNodeName ){
	lockMyPthread();

// vars
    const char*     baseConfigPath = NULL;
    std::string     configFile;
    json_error_t    jsonError;
	json_t*			jsonValue;
	bool			saveToFile = false;

// get config path
    this->configPath( &baseConfigPath );
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


// get our own node
    if( myNodeName != NULL ){
        if( this->nodeSelect(myNodeName) == false ){
            saveToFile = true;
        }
    }


	if( saveToFile == true ){
		this->save(NULL);
	}

	unlockMyPthread();
	return true;
}


bool coCoreConfig::			save( const char* jsonString ){
	lockMyPthread();

// vars
	json_t*			jsonObject = NULL;
	json_error_t	jsonError;

	if( jsonString != NULL ){
	// parse json
		jsonObject = json_loads( jsonString, JSON_PRESERVE_ORDER | JSON_INDENT(4), &jsonError );
		if( jsonObject == NULL | jsonError.line > -1 ){
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
    const char*     baseConfigPath = NULL;
    std::string     configFile;
    this->configPath( &baseConfigPath );
    configFile  = baseConfigPath;
    configFile += "/";
    configFile += "core.json";


/// @todo Copy the old config-file with timestamp as a backup


// save the json to file
	if( json_dump_file( this->jsonConfig, configFile.c_str(), JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "Save config to %s%s", configFile.c_str(), "core.json" );
		etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

		unlockMyPthread();
		return true;
	}

	unlockMyPthread();
	return false;
}


json_t* coCoreConfig::      section( const char* sectionName ){

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





bool coCoreConfig::			configPath( const char** path ){
    if( path == NULL ) return false;


// set it
    if( *path != NULL ){
        etStringCharSet( this->configBasePath, *path, -1 );
    } else {
        __etStringCharGet( this->configBasePath, path );
    }

    return true;
}




bool coCoreConfig::			nodesGet( json_t** jsonObject ){
	if( jsonObject == NULL ) return false;
	if( this->jsonNodes == NULL ) return false;

	*jsonObject = this->jsonNodes;
	return true;
}


bool  coCoreConfig::		nodesGetAsArray( json_t* jsonArray ){
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


void coCoreConfig::			nodeStatesRemove(){
	if( this->jsonNodes == NULL ) return;
	lockMyPthread();

// vars
    void*           iterator = NULL;
    json_t*         node = NULL;
    json_t*         jsonString = NULL;
    const char*     jsonStringChar = NULL;


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






bool coCoreConfig::			nodesIterate(){
	lockMyPthread();
	this->jsonNodesIterator = NULL;
	return true;
}


bool coCoreConfig::			nodeAppend( const char* name ){
	if( this->jsonNodes == NULL ) return false;


// create new Object
	this->jsonNode = json_object();
	if( json_object_set_new( this->jsonNodes, name, this->jsonNode ) != 0 ){
		return false;
	}

// return
	return true;
}


bool coCoreConfig::			nodeRemove( const char* name ){
	if( this->jsonNodes == NULL ) return false;

    json_object_del( this->jsonNodes, name );

    return true;
}


bool coCoreConfig::		    nodeSelect( const char* name ){
	if( this->jsonNodes == NULL ) return false;

	this->jsonNode = json_object_get( this->jsonNodes, name );
	if( this->jsonNode != NULL ) return true;

	return false;
}


bool coCoreConfig::		    nodeSelectByHostName( const char* hostName ){
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


bool coCoreConfig::			nodeGet( json_t** jsonNode ){
    if( jsonNode == NULL ) return false;

    *jsonNode = this->jsonNode;
    return true;
}


bool coCoreConfig::			nodeNext(){
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


bool coCoreConfig::			nodeInfo( const char** name, coCoreConfig::nodeType* type, bool set ){
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
				jsonVar = json_integer( coCoreConfig::UNKNOWN );
				json_object_set_new( this->jsonNode, "type", jsonVar );
			}
			*type = (coCoreConfig::nodeType)json_integer_value( jsonVar );
		} else {
			json_object_set_new( this->jsonNode, "type", json_integer(*type) );
		}
	}

	return true;
}


bool coCoreConfig::			nodeConnInfo( const char** host, int* port, bool set ){

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


bool coCoreConfig::			nodesIterateFinish(){
	unlockMyPthread();
	return true;
}




    // user / password
bool coCoreConfig::			authMethode(){
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


bool coCoreConfig::			userAdd( const char* username ){
    if(	this->jsonConfig == NULL ) return false;

// all users
    json_t* jsonUsers = json_object_get( this->jsonConfig, "users" );
    if( jsonUsers == NULL ) return NULL;

// add ( and overwrite ) user
    json_t* jsonUser = json_object();
    json_object_set_new( jsonUsers, username, jsonUser );

    return true;
}


bool coCoreConfig::			userCheck( const char* username, const char* password ){


}



#endif
