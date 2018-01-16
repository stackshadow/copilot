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

// load configuration
    this->load();

// health
    this->cmdHealthLock = 0;
    this->cmdHealth = 101;
    etStringAllocLen( cmdHealthDescription, 32 );



// demo
    sysStateCmd freeDisk( "", "df | tail -n +2 | sort -hb -k5 | tail -n 1 | awk -F' ' '{print $5}' | sed 's/%//g'", "free disc space", 100, 0, NULL );
    freeDisk.execute();


// start all commands
    this->commandsStartAll();

// register plugin
	coCore::ptr->plugins->append( this );
}


sysState::                          ~sysState(){

}


bool sysState::                     load(){
    this->jsonConfigObject = coCore::ptr->config->section("sysstate");
    return true;
}


bool sysState::                     save(){
    return coCore::ptr->config->save();
}



int sysState::                      health( int newHealth, void* cmd ){

    if( newHealth >= 0 ){
        lockPthread( this->cmdHealthLock );

        bool sendUpdate = false;

    // set health
        if( newHealth < this->cmdHealth /* - healthBand  || newHealth > this->cmdHealth + healthBand */ ){
            this->cmdHealth = newHealth;
            this->cmdHealthCmd = cmd;
            sendUpdate = true;
        }


    // only the cmd which set the worst value can increase that
        if( newHealth > this->cmdHealth && this->cmdHealthCmd == cmd ){
            this->cmdHealth = newHealth;
            this->cmdHealthCmd = cmd;
            sendUpdate = true;
        }


    // send update ?
        if( sendUpdate == true ){

        // build json-answer-object
            char jsonCharDump[2048];
            snprintf( jsonCharDump, 2048, "{ \"health\": \"%d\", \"name\": \"%s\" }", this->cmdHealth, ((sysStateCmd*)this->cmdHealthCmd)->displayName() );

        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "health", jsonCharDump );

        }



        unlockPthread( this->cmdHealthLock );
    }


    return this->cmdHealth;
}


int sysState::                      running( int newCounter ){

    if( newCounter >= 0 ){
        lockPthread( this->cmdRunningCountLock );
        this->cmdRunningCount = newCounter;
        unlockPthread( this->cmdRunningCountLock );

    // add the message to list
        char cmdRunningChar[10] = "         ";
        snprintf( cmdRunningChar, 10, "%d", this->cmdRunningCount );
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "cmdRunning", cmdRunningChar );
    }


    if( newCounter == 0 ){
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "sysState: Commands stopped" );
    }
    if( newCounter == 1 ){
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "sysState: Commands running" );
    }

    return this->cmdRunningCount;
}




// ################################################### Command store ###################################################

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


    return true;
}


json_t* sysState::                  cmdGet( const char* uuid ){
// vars
    json_t*     jsonTimer = NULL;
    const char* jsonTimerName = NULL;

    json_t*     jsonCmd = NULL;
    const char* jsonCmdUUID = NULL;


// iterate times
    json_object_foreach( this->jsonConfigObject, jsonTimerName, jsonTimer ){

    // iterate commands
        json_object_foreach( jsonTimer, jsonCmdUUID, jsonCmd ){

            if( strncmp( uuid, jsonCmdUUID, strlen(jsonCmdUUID) ) == 0 ){
                return jsonCmd;
            }

        }

    }

    return NULL;
}


bool sysState::                     cmdRemove( const char* uuid ){

// vars
    json_t*     jsonTimer = NULL;
    const char* jsonTimerName = NULL;

    json_t*     jsonCmd = NULL;
    const char* jsonCmdUUID = NULL;


// iterate times
    json_object_foreach( this->jsonConfigObject, jsonTimerName, jsonTimer ){

    // iterate commands
        json_object_foreach( jsonTimer, jsonCmdUUID, jsonCmd ){

            if( strncmp( uuid, jsonCmdUUID, strlen(jsonCmdUUID) ) == 0 ){
                json_object_del( jsonTimer, jsonCmdUUID );
                return true;
            }

        }

    }

    return false;
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



// ################################################### Command-Thread ###################################################

int sysState::                      commandsStartAll(){

// already active
    if( threadedDataArray != NULL ){
        return -1; // already running
    }

// vars - common
    json_t*         jsonValue;

// vars- times
    void*           jsonTimeIterator = NULL;
    int             jsonTimeIndex = 0;
    json_t*         jsonTime = NULL;
    const char*     timeChar = NULL;
    int             iteratorTime = 5000;

// vars - command values
    int             jsonCommandIndex = 0;
    void*           jsonCommandIterator = NULL;
    json_t*         jsonCommand = NULL;
    const char*     cmdUUID = NULL;
    const char*     cmd = NULL;
    const char*     cmdDisplayName = NULL;
    int             cmdValueMin;
    int             cmdValueMax;
    int             cmdDelay = 100;

// how many threads we need ?
// ( normaly one thread per timer )
    this->threadedDataArrayCount = json_object_size( this->jsonConfigObject );
    if( this->threadedDataArrayCount == 0 ){
        return -2; // no commands aviable
    }

// allocate
    this->threadedDataArray = (sysStateThreadData**)malloc( (this->threadedDataArrayCount+1) * sizeof(sysStateThreadData*) );
    this->threadedDataArray[this->threadedDataArrayCount] = NULL;

// reset command counter
    this->running( 0 );

// iterate timers
    jsonTimeIterator = json_object_iter( this->jsonConfigObject );
    for( jsonTimeIndex = 0; jsonTimeIndex < this->threadedDataArrayCount && jsonTimeIterator != NULL; jsonTimeIndex++ ){
        jsonTime = json_object_iter_value( jsonTimeIterator );
        timeChar = json_object_iter_key( jsonTimeIterator );
        iteratorTime = atoi(timeChar);


    // we need a data-array for thread-data
        sysStateThreadData* threadData = (sysStateThreadData*)malloc( sizeof(sysStateThreadData) );
        threadData->cmdArrayCount = json_object_size( jsonTime );
        threadData->cmdArray = (sysStateCmd**)malloc( (threadData->cmdArrayCount+1) * sizeof(sysStateCmd*) );
        threadData->cmdArray[threadData->cmdArrayCount] = NULL;
        threadData->cmdDelay = iteratorTime / (threadData->cmdArrayCount+1);
        threadData->running = false;
        threadData->requestEnd = false;
        this->threadedDataArray[jsonTimeIndex] = threadData;
        this->threadedDataArray[jsonTimeIndex+1] = NULL;

    // iterate commands
        jsonCommandIterator = json_object_iter( jsonTime );
        for( jsonCommandIndex = 0; jsonCommandIndex < threadData->cmdArrayCount && jsonCommandIterator != NULL; jsonCommandIndex++ ){
            jsonCommand = json_object_iter_value( jsonCommandIterator );
            cmdUUID = json_object_iter_key( jsonCommandIterator );

        // cmd
            jsonValue = json_object_get( jsonCommand, "cmd" );
            if( jsonValue != NULL ) cmd = json_string_value(jsonValue);
            else { cmd = NULL; }

        // display name
            jsonValue = json_object_get( jsonCommand, "displayName" );
            if( jsonValue != NULL ){ cmdDisplayName = json_string_value(jsonValue); }
            else { cmdDisplayName = "unknown"; }

        // min
            jsonValue = json_object_get( jsonCommand, "min" );
            if( jsonValue != NULL ) cmdValueMin = json_integer_value(jsonValue);
            else { cmdValueMin = 0; }

        // max
            jsonValue = json_object_get( jsonCommand, "max" );
            if( jsonValue != NULL ) cmdValueMax = json_integer_value(jsonValue);
            else { cmdValueMax = 100; }


            threadData->cmdArray[jsonCommandIndex] = new sysStateCmd( cmdUUID, cmd, cmdDisplayName, cmdValueMin, cmdValueMax, sysState::updateHealthCallback );
            threadData->cmdArray[jsonCommandIndex+1] = NULL;

            jsonCommandIterator = json_object_iter_next( jsonTime, jsonCommandIterator );
        }

    // okay, we now have all infos for the thread :D
        pthread_create( &threadData->thread, NULL, sysState::cmdThread, threadData );
        pthread_detach( threadData->thread );


    // next time
        jsonTimeIterator = json_object_iter_next( this->jsonConfigObject, jsonTimeIterator );
    }


    return 0;
}


int sysState::                      commandsStopAll(){

// already stopped
    if( threadedDataArray == NULL ){
        return -1; // already running
    }

// vars
    sysStateThreadData*     threadData = NULL;
    int                     cmdArraysIndex = 0;
    sysStateCmd*            command;
    int                     commandIndex = 0;

    for( cmdArraysIndex = 0; cmdArraysIndex < this->threadedDataArrayCount; cmdArraysIndex++ ){

    // threaded data
        threadData = this->threadedDataArray[cmdArraysIndex];
        if( threadData == NULL ) continue;

    // wait until thread finished
        threadData->requestEnd = true;

    }

// cleanup
    memset(this->threadedDataArray,0,this->threadedDataArrayCount+1);
    free(this->threadedDataArray);
    this->threadedDataArray = NULL;

    return 0;
}


int sysState::                      commandsStopAllWait(){

// vars
    sysStateThreadData*     threadData = NULL;
    int                     cmdArraysIndex = 0;
    sysStateCmd*            command;
    int                     commandIndex = 0;

    for( cmdArraysIndex = 0; cmdArraysIndex < this->threadedDataArrayCount; cmdArraysIndex++ ){

    // threaded data
        threadData = this->threadedDataArray[cmdArraysIndex];
        if( threadData == NULL ) continue;

    // wait until thread finished
        threadData->requestEnd = true;
        while( threadData->running == true ) sleep(1);

    // destroy all commands
        for( commandIndex = 0; commandIndex < threadData->cmdArrayCount; commandIndex++ ){

        // command
            command = threadData->cmdArray[commandIndex];
            if( command == NULL ) continue;

            delete command;
        }

        free( threadData->cmdArray );
        threadData->cmdArray = NULL;
        threadData->cmdArrayCount = 0;
    }
    free( this->threadedDataArray );
    this->threadedDataArray = NULL;
    this->threadedDataArrayCount = 0;

// reset command counter
    this->running( 0 );
    return 0;
}


void* sysState::                    cmdThread( void* void_service ){

//vars
    sysStateCmd*            cmd = NULL;
    sysStateThreadData*     threadData = (sysStateThreadData*)void_service;
    int                     arrayIndex = 0;
    int                     delay;
    bool                    firstRun = true;

// set thread to running
    threadData->running = true;
    threadData->requestEnd = false;

// thread finished cleanup
    sysState::ptr->running( sysState::ptr->running() + threadData->cmdArrayCount );

    while( threadData->requestEnd == false ){
        arrayIndex = 0;

        cmd = threadData->cmdArray[arrayIndex];
        while( cmd != NULL && threadData->requestEnd == false ){

        // get data
            cmd = threadData->cmdArray[arrayIndex];
            delay = threadData->cmdDelay;

        // run and delay
            cmd->execute();

        // delay
            if( firstRun == false ){ usleep( 1000 * delay ); }
            else { firstRun = false; usleep(100); }

            arrayIndex++;
            cmd = threadData->cmdArray[arrayIndex];
        }

    // sleep
        if( firstRun == false ){ usleep( 1000 * delay ); }
        else { firstRun = false; usleep(100); }


    }

// thread finished cleanup
    sysState::ptr->running( sysState::ptr->running() - threadData->cmdArrayCount );

// free memory
    memset( threadData->cmdArray, 0, threadData->cmdArrayCount+1 );
    free( threadData->cmdArray );
    threadData->cmdArray = NULL;

    memset( threadData, 0, sizeof(threadData) );
    free( threadData );
    threadData = NULL;



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




    if( strncmp(msgCommand,"cmdListGet",10) == 0 ){

        void*       jsonTimerIterator = NULL;
        json_t*     jsonTimer = NULL;

        void*       jsonCmdIterator = NULL;
        json_t*     jsonCmd = NULL;


        jsonTimerIterator = json_object_iter( this->jsonConfigObject );
        while( jsonTimerIterator != NULL ){
            jsonTimer = json_object_iter_value( jsonTimerIterator );

            jsonCmdIterator = json_object_iter( jsonTimer );
            while( jsonCmdIterator != NULL ){
                jsonCmd = json_object_iter_value( jsonCmdIterator );

            // dump
                jsonTempString = json_dumps( jsonCmd, JSON_PRESERVE_ORDER | JSON_COMPACT );

            // add the message to list
                coCore::ptr->plugins->messageQueue->add( this,
                msgTarget, msgSource, msgGroup, "cmdList", jsonTempString );

            // cleanup
                free( jsonTempString );
                //json_decref( jsonAnswer );

                jsonCmdIterator = json_object_iter_next( jsonTimer, jsonCmdIterator );
            }



            jsonTimerIterator = json_object_iter_next( this->jsonConfigObject, jsonTimerIterator );
        }


        return coPlugin::REPLY;
    }


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
        sysStateCmd freeDisk( "test", command, "Test", commandMin, commandMax, NULL );
        freeDisk.execute( commandOut,  &commandOutSize );
        commandHealth = freeDisk.health();

    // build output
        json_decref(jsonPayload);
        jsonPayload = json_object();
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


    if( strncmp(msgCommand,"cmdSave",7) == 0 ){

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

        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "Saved" );

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdDelete",9) == 0 ){

    // remove id
        if( this->cmdRemove( msgPayload ) == true ){
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "Deleted" );
            this->save();
        } else {
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgError", "Could not delete..." );
        }


    }


    if( strncmp(msgCommand,"cmdDetailGet",12) == 0 ){

        json_t*     jsonCmd = this->cmdGet( msgPayload );
        if( jsonCmd == NULL ){
            etDebugMessage( etID_LEVEL_WARNING, "Command not found");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgError", "Command not found" );
        }

    // dump
        jsonTempString = json_dumps( jsonCmd, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        msgTarget, msgSource, msgGroup, "cmdDetail", jsonTempString );

    // cleanup
        free( jsonTempString );

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdStartAll",11) == 0 ){
        int returnCode = commandsStartAll();

        if( returnCode == -1 ){
            etDebugMessage( etID_LEVEL_WARNING, "Commands already started, you need to stop it first!");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgError", "Already running..." );
        }
        if( returnCode == -2 ){
            etDebugMessage( etID_LEVEL_WARNING, "No commands aviable.");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgError", "No commands aviable." );
        }
        if( returnCode == 0 ){
            etDebugMessage( etID_LEVEL_INFO, "Started...");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "sysState: Start reqest" );
        }

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdStopAll",10) == 0 ){
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "msgSuccess", "sysState: Request stop of commands.. this can take a bit" );

        commandsStopAll();

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"cmdRunningGet",13) == 0 ){
    // add the message to list
        char cmdRunningChar[10] = "\0\0\0\0\0\0\0\0\0";
        snprintf( cmdRunningChar, 10, "%d", this->cmdRunningCount );
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "cmdRunning", cmdRunningChar );

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"healthGet",9) == 0 ){

    // get command which was set the health
        sysStateCmd* lastHealthCommand = (sysStateCmd*)this->cmdHealthCmd;

    // if no command was set the health yet
        if( lastHealthCommand == NULL ){
        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "sysstate", "health", "-1" );
            return coPlugin::MESSAGE_FINISHED;
        }

    // build json-answer-object
        char jsonCharDump[2048];
        snprintf( jsonCharDump, 2048, "{ \"health\": \"%d\", \"name\": \"%s\" }", lastHealthCommand->health(), lastHealthCommand->displayName() );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->hostNameGet(), "", "sysstate", "health", jsonCharDump );

        return coPlugin::REPLY;
    }


    if( strncmp(msgCommand,"healthReset",11) == 0 ){
        this->cmdHealth = 101;
        return coPlugin::REPLY;
    }


    return coPlugin::NO_REPLY;
}








#endif
