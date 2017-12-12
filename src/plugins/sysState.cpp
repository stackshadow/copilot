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

free -t -m | tail -n1 | awk -F' ' '{print $4}'


maximal used memory:
df | tail -n +2 | sort -hb -k5 | tail -n 1 | awk -F' ' '{print $5}' | sed 's/%//g'
*/

#ifndef sysState_C
#define sysState_C




#include "uuid.h"

#include "sysState.h"
#include "coCore.h"

#include <string>
#include <pthread.h>


sysState* sysState::ptr = NULL;

sysState::                          sysState() : coPlugin( "sysstate", coCore::ptr->hostNameGet(), "sysstate" ) {

// remember pointer
    sysState::ptr = this;


    int             textSize = 1024;
    char            text[textSize];
    int             textSizeOut = textSize;


    sysStateCmd freeDisk( "df | tail -n +2 | sort -hb -k5 | tail -n 1 | awk -F' ' '{print $5}' | sed 's/%//g'", 100, 0, NULL );
    freeDisk.execute();


// load
    this->load();

    this->commandsRunAll();


    //this->runAllCommands();

// register plugin
	coCore::ptr->plugins->append( this );
}


sysState::                          ~sysState(){

}


bool sysState::                     load(){
// vars
    json_error_t    jsonError;
	json_t*			jsonValue;
	bool			saveToFile = false;

// clear
	this->jsonConfigObject = NULL;

// check core-config path
	if( access( baseFilePath, F_OK ) != 0 ){
		system( "mkdir -p " baseFilePath );
	}

// open the file
    this->jsonConfigObject = json_load_file( baseFilePath "sysstate.json", JSON_PRESERVE_ORDER, &jsonError );
    if( this->jsonConfigObject == NULL ){
        this->jsonConfigObject = json_object();
		saveToFile = true;
    }




    if( saveToFile == true ){
        this->save();
    }

    return true;
}


bool sysState::                     save(){

// save the json to file
	if( json_dump_file( this->jsonConfigObject, baseFilePath "sysstate.json", JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
		snprintf( etDebugTempMessage, etDebugTempMessageLen, "Save config to %s%s", baseFilePath, "sysstate.json" );
		etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
		return true;
	}



}



int sysState::                      health( int newHealth ){

    if( newHealth >= 0 ){
        lockPthread( this->healthLock );

    // set health
        if( newHealth < this->cmdHealth - healthBand /* || newHealth > this->cmdHealth + healthBand */ ){
            this->cmdHealth = newHealth;

            char healthChar[10] = "\0\0\0\0\0\0\0\0\0";
            snprintf( healthChar, 10, "%d", this->cmdHealth );

        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "health", healthChar );
        }

        unlockPthread( this->healthLock );
    }


    return this->cmdHealth;
}



bool sysState::                     commandAppend( json_t* jsonCommand ){


// vars - common
    json_t*         jsonValue = NULL;

// vars of command
    const char*     cmdID = NULL;
    int             cmdInterval = 0;
    char            cmdIntervalChar[10] = "         ";

// command: interval
    jsonValue = json_object_get( jsonCommand, "interval" );
    if( jsonValue == NULL ) return false;
    cmdInterval = json_integer_value( jsonValue );
    snprintf( cmdIntervalChar, 10, "%d", cmdInterval );

// command: id
    jsonValue = json_object_get( jsonCommand, "id" );
    if( jsonValue == NULL ){
        uuid_t  newUUID;
        char    newUUIDString[37];
        uuid_generate( newUUID );
        uuid_unparse( newUUID, newUUIDString );
        jsonValue = json_string( newUUIDString );
        json_object_set_new( jsonCommand, "id", jsonValue );
    }
    cmdID = json_string_value( jsonValue );


// commands: interval
    jsonValue = json_object_get( this->jsonConfigObject, cmdIntervalChar );
    if( jsonValue == NULL ){
        jsonValue = json_object();
        json_object_set_new( this->jsonConfigObject, cmdIntervalChar, jsonValue );
    }

// add the command
    json_object_set_new( jsonValue, cmdID, jsonCommand );

// remove the id
    json_object_del( jsonCommand, "id" );

    return true;
}



bool sysState::                     createBash(){

// vars
    char            tempFileNameIn[] = "/tmp/copilotd_health.bash";
    char*           tempFileName;
    FILE*           tempFile = NULL;

// json
    json_t*         jsonCommands = NULL;
    void*           jsonCommandIterator = NULL;
    json_t*         jsonCommand = NULL;
    json_t*         jsonValue = NULL;
    char*           jsonValueChar = NULL;
    int             jsonArrayIndex = 0;
    int             jsonArrayLen = 0;

// vars - command
    const char*     command = NULL;
    int             commandMin = 0;
    int             commandMax = 0;
    int             commandHealth = 0;
    int             commandInterval = 0;


// check
    if( this->jsonCommands == NULL ) return false;


    tempFile = fopen( tempFileNameIn, "w+" );
    fprintf( tempFile, "#!/bin/bash\n");
    fprintf( tempFile, "\n");

    fprintf( tempFile, "while true; do \n");

    fprintf( tempFile, "    health=100 \n");

    jsonCommandIterator = json_object_iter(this->jsonCommands);
    while( jsonCommandIterator != NULL ){
        jsonCommand = json_object_iter_value(jsonCommandIterator);

    // command
        jsonValue = json_object_get( jsonCommand, "cmd" );
        command = json_string_value( jsonValue );

    // min
        jsonValue = json_object_get( jsonCommand, "min" );
        commandMin = json_integer_value( jsonValue );
    // max
        jsonValue = json_object_get( jsonCommand, "max" );
        commandMax = json_integer_value( jsonValue );

        jsonValue = json_object_get( jsonCommand, "interval" );
        commandInterval = json_integer_value( jsonValue );

        fprintf( tempFile, "var=$(%s) \n", command );
        fprintf( tempFile, "varPercent=$(( (var - %d) * 100 / (%d - %d) )) \n", commandMin, commandMax, commandMin );
        fprintf( tempFile, "if [ $varPercent -lt $health ]; then \n");
        fprintf( tempFile, "    health=$varPercent \n");
        fprintf( tempFile, "fi \n");

        fprintf( tempFile, "sleep %.3f \n", (double)0.001 * (double)commandInterval );

        jsonCommandIterator = json_object_iter_next( this->jsonCommands, jsonCommandIterator );
    }

    fprintf( tempFile, "    echo $health > /tmp/health \n");
    fprintf( tempFile, "done \n");

    fflush( tempFile );
    fclose( tempFile );
/*
health=100
var=$(free -t -m | tail -n1 | awk -F' ' '{print $4}')

echo $var
min=1000
max=3000
rangeDiff=$(( (var - min) * 100 / (max - min) ))
echo $rangeDiff
*/

// sleep 0.001
/*
if [ $this -gt $that ]
then
fi
*/


}




void sysState::                     commandsRunAll(){

// vars - common
    json_t*         jsonValue;

// vars- times
    void*           jsonTimeIterator = NULL;
    int             jsonTimeIndex = 0;
    int             jsonTimeCounter = 0;
    json_t*         jsonTime = NULL;
    const char*     timeChar = NULL;
    int             iteratorTime = 5000;

// vars - command values
    int             jsonCommandCounter = 0;
    int             jsonCommandIndex = 0;
    void*           jsonCommandIterator = NULL;
    json_t*         jsonCommand = NULL;
    const char*     cmd = NULL;
    int             cmdValueMin;
    int             cmdValueMax;
    int             cmdDelay = 100;

// we need to remember all command arrays
    jsonTimeCounter = json_object_size( this->jsonConfigObject );
    this->cmdArrays = (sysStateThreadData**)malloc( (jsonTimeCounter+1) * sizeof(sysStateThreadData) );
    this->cmdArrays[jsonTimeCounter] = NULL;

    jsonTimeIterator = json_object_iter( this->jsonConfigObject );
    for( jsonTimeIndex = 0; jsonTimeIndex < jsonTimeCounter && jsonTimeIterator != NULL; jsonTimeIndex++ ){
        jsonTime = json_object_iter_value( jsonTimeIterator );
        timeChar = json_object_iter_key( jsonTimeIterator );
        iteratorTime = atoi(timeChar);


    // calculate delay
        jsonCommandCounter = json_object_size( jsonTime );
        cmdDelay = iteratorTime / ( jsonCommandCounter + 1 );

    // we need a data-array for thread-data
        sysStateThreadData* threadData = (sysStateThreadData*)malloc( sizeof(sysStateThreadData) );
        threadData->cmdArray = (sysStateCmd**)malloc( (jsonCommandCounter + 1) * sizeof(sysStateCmd) );
        threadData->cmdDelay = iteratorTime / ( jsonCommandCounter + 1 );
        threadData->running = false;
        threadData->requestEnd = false;
        this->cmdArrays[jsonTimeIndex] = threadData;

    // iterate commands
        jsonCommandIterator = json_object_iter( jsonTime );
        for( jsonCommandIndex = 0; jsonCommandIndex < jsonCommandCounter && jsonCommandIterator != NULL; jsonCommandIndex++ ){
            jsonCommand = json_object_iter_value( jsonCommandIterator );

        // cmd
            jsonValue = json_object_get( jsonCommand, "cmd" );
            if( jsonValue != NULL ) cmd = json_string_value(jsonValue);
            else { cmd = NULL; }

        // min
            jsonValue = json_object_get( jsonCommand, "min" );
            if( jsonValue != NULL ) cmdValueMin = json_integer_value(jsonValue);
            else { cmdValueMin = 0; }

        // max
            jsonValue = json_object_get( jsonCommand, "max" );
            if( jsonValue != NULL ) cmdValueMax = json_integer_value(jsonValue);
            else { cmdValueMax = 100; }


            threadData->cmdArray[jsonCommandIndex] = new sysStateCmd( cmd, cmdValueMin, cmdValueMax, sysState::updateHealth );
            threadData->cmdArray[jsonCommandIndex+1] = NULL;


            jsonCommandIterator = json_object_iter_next( jsonTime, jsonCommandIterator );
        }

    // okay, we now have all infos for the thread :D
        pthread_create( &threadData->thread, NULL, sysState::cmdThread, threadData );
        pthread_detach( threadData->thread );


    // next time
        jsonTimeIterator = json_object_iter_next( this->jsonConfigObject, jsonTimeIterator );
    }





}


void sysState::                     commandsStopAllWait(){



}


void* sysState::                    cmdThread( void* void_service ){

//vars
    sysStateCmd*           cmd = NULL;
    sysStateThreadData*     threadData = (sysStateThreadData*)void_service;
    int                     arrayIndex = 0;
    int                     delay;

// set thread to running
    threadData->running = true;
    threadData->requestEnd = false;

    while( threadData->requestEnd == false ){
        arrayIndex = 0;

        cmd = threadData->cmdArray[arrayIndex];
        while( cmd != NULL && threadData->requestEnd == false ){

        // get data
            cmd = threadData->cmdArray[arrayIndex];
            delay = threadData->cmdDelay;

        // run and delay
            cmd->execute();
            usleep( 1000 * delay );

            arrayIndex++;
            cmd = threadData->cmdArray[arrayIndex];
        }

    // sleep
        usleep( 1000 * delay );

    }

// thread finished
    threadData->running = false;

    return NULL;
}




coPlugin::t_state sysState::        onBroadcastMessage( coMessage* message ){



// vars
	const char*			        msgSource = message->nodeNameSource();
    const char*			        msgTarget = message->nodeNameTarget();
    const char*		            msgGroup = message->group();
	const char*		            msgCommand = message->command();
	const char*		            msgPayload = message->payload();

	json_error_t	            jsonError;
	json_t*			            jsonPayload = NULL;
	json_t*                     jsonValue;
    char*                       jsonTempString = NULL;
    void*                       jsonIterator = NULL;




	if( strncmp(msgCommand,"cmdTry",6) == 0 ){

	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

        const char*         command;
        int                 commandMin = 0;
        int                 commandMax = 0;
        int                 commandOutSize = 1024;
        char                commandOut[commandOutSize];
        int                 commandHealth = 0;

    // cmd
        jsonValue = json_object_get( jsonPayload, "cmd" );
        if( jsonValue == NULL ) return coPlugin::NO_REPLY;
        command = json_string_value( jsonValue );

    // min
        jsonValue = json_object_get( jsonPayload, "min" );
        if( jsonValue == NULL ) return coPlugin::NO_REPLY;
        commandMin = json_integer_value( jsonValue );

    // max
        jsonValue = json_object_get( jsonPayload, "max" );
        if( jsonValue == NULL ) return coPlugin::NO_REPLY;
        commandMax = json_integer_value( jsonValue );

    // run
        sysStateCmd freeDisk( command, commandMin, commandMax );
        freeDisk.execute( commandOut,  &commandOutSize );
        commandHealth = freeDisk.health();

    // build output
        json_decref(jsonPayload);
        jsonPayload = json_object();
        json_object_set_new( jsonPayload, "cmd", json_string(command) );
        json_object_set_new( jsonPayload, "out", json_string(commandOut) );
        json_object_set_new( jsonPayload, "health", json_integer(commandHealth) );

    // create the answer payloer
        msgPayload = json_dumps( jsonPayload, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, msgGroup, "cmdTryOut", msgPayload );

    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonPayload);
        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdAppend",9) == 0 ){

    // vars
        json_t*         jsonID = NULL;
        const char*     jsonIDChar = NULL;
        json_t*         jsonInterval = NULL;
        int             intervalMilliseconds = 0;
        char            intervalMillisecondsChar[20]; memset( intervalMillisecondsChar, 0, 20 );
        char*           id;

	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

        this->commandAppend( jsonPayload );
        this->save();


    }


    if( strncmp(msgCommand,"healthGet",9) == 0 ){

        char healthChar[10] = "\0\0\0\0\0\0\0\0\0";
        snprintf( healthChar, 10, "%d", this->health() );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, msgGroup, "health", healthChar );

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"healthReset",11) == 0 ){
        this->cmdHealth = 100;
        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdListGet",10) == 0 ){


        jsonIterator = json_object_iter( this->jsonConfigObject );
        while( jsonIterator != NULL ){
            jsonValue = json_object_iter_value( jsonIterator );

        // dump
            jsonTempString = json_dumps( jsonValue, JSON_PRESERVE_ORDER | JSON_COMPACT );

        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            msgTarget, msgSource, msgGroup, "cmdList", jsonTempString );

        // cleanup
            free( jsonTempString );

            jsonIterator = json_object_iter_next( this->jsonConfigObject, jsonIterator );
        }


        return coPlugin::REPLY;
    }



    return coPlugin::NO_REPLY;
}








#endif
