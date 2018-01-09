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

#ifndef syslogd_C
#define syslogd_C


#include "plugins/syslogd.h"
#include "coCore.h"

syslogd* syslogd::ptr = NULL;

syslogd::               			    syslogd() : coPlugin( "syslogd", coCore::ptr->nodeName(), "syslogd" ){


    syslogd::ptr = this;

// register plugin
	coCore::ptr->plugins->append( this );
}


bool syslogd::                          onExecute(){
    this->messageThreadStart();
}


coPlugin::t_state syslogd::	            onBroadcastMessage( coMessage* message ){

// vars
    int                 returnValue = -1;
	const char*			msgTarget = message->nodeNameTarget();
    const char*			msgSource = message->nodeNameSource();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
    int                 msgCommandLen = 0;
	const char*			msgPayload = message->payload();

// check
    if( msgCommand == NULL ) return coPlugin::NO_REPLY;
    msgCommandLen = strlen(msgCommand);


    if( coCore::strIsExact("isRunning",msgCommand,msgCommandLen) == true ){

        if( this->messageThreadRunning == true ){
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "syslogd", "state", "1" );
        } else {
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "syslogd", "state", "0" );
        }

        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("start",msgCommand,msgCommandLen) == true ){

        this->messageThreadGetPrevMsg = true;
        returnValue = this->messageThreadStart();
        if( returnValue == 1 ){
            etDebugMessage( etID_LEVEL_WARNING, "Already running...");
            coCore::ptr->plugins->messageQueue->add( this,
            coCore::ptr->hostNameGet(), "", "syslogd", "msgError", "Already running..." );
        }
        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("stop",msgCommand,msgCommandLen) == true ){
        returnValue = this->messageThreadStop();
        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("refresh",msgCommand,msgCommandLen) == true ){
        this->messageThreadGetPrevMsg = true;
        return coPlugin::REPLY;
    }


}




int syslogd::                           messageThreadStart(){

// already running ?
    if( this->messageThreadRunning == true ){
        return 1;
    }

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, syslogd::messageThread, this );
	pthread_detach( thread );

    return 0;
}


int syslogd::                           messageThreadStop(){
    this->messageThreadStopReq = true;


}


void* syslogd::		                    messageThread( void* void_syslogd ){


// vars
    syslogd*        syslogdInstance = (syslogd*)void_syslogd;
    int             returnVal;
    sd_journal*     journal;

// open the journal
    returnVal = sd_journal_open(&journal, SD_JOURNAL_SYSTEM);

// go to the end
    sd_journal_seek_tail( journal );
    //sd_journal_previous_skip( journal, 10 );

// started
    syslogdInstance->messageThreadRunning = true;
    syslogdInstance->messageThreadStopReq = false;
    coCore::ptr->plugins->messageQueue->add( syslogdInstance, coCore::ptr->nodeName(), coCore::ptr->nodeName(), "syslogd", "msgSuccess", "Running." );

// loop
    while( syslogdInstance->messageThreadStopReq == false ) {

        if( syslogdInstance->messageThreadGetPrevMsg == true ){
            sd_journal_seek_tail( journal );
            sd_journal_previous_skip( journal, 10 );
            syslogdInstance->messageThreadGetPrevMsg = false;
        }

        returnVal = sd_journal_next( journal );
        while( returnVal > 0 ){



        // time
            uint64_t        jTimestampMicrosecond;
            time_t          jTimestamp;
            struct tm*      jTimestampLocal;
            char            jTimestampString[80];

            sd_journal_get_realtime_usec( journal, &jTimestampMicrosecond );
            jTimestamp = jTimestampMicrosecond / 1000 / 1000;
            jTimestampLocal = localtime (&jTimestamp);
            strftime( jTimestampString, 80, "%Y-%m-%d %H:%M:%S", jTimestampLocal);

        // command
            char*           cmdLineString;
            size_t          cmdLineStringLen;
            returnVal = sd_journal_get_data( journal, "_CMDLINE", (const void **)&cmdLineString, &cmdLineStringLen );
            if( returnVal < 0 ) {
                fprintf( stderr, "Failed to read message field: %s\n", strerror(returnVal) );
                //continue;
            }
            if( cmdLineString != NULL ) cmdLineString = &cmdLineString[9];
            else cmdLineString = "unknow";



        // message
            char*           messageString;
            size_t          messageStringLen;
            returnVal = sd_journal_get_data( journal, "MESSAGE", (const void **)&messageString, &messageStringLen );
            if( returnVal < 0 ) {
                fprintf( stderr, "Failed to read message field: %s\n", strerror(returnVal) );
                continue;
            }
            if( messageString != NULL ) messageString = &messageString[8];


        // build json-answer-object
            char jsonCharDump[2048];
            snprintf( jsonCharDump, 2048, "{ \"dt\": \"%s\", \"cmd\": \"%s\", \"msg\": \"%s\" }", jTimestampString, cmdLineString, messageString );

            int jsonCharDumpIndex = 0;
            for( jsonCharDumpIndex = 0; jsonCharDumpIndex < 2048; jsonCharDumpIndex++ ){
                char testChar = jsonCharDump[jsonCharDumpIndex];
                if( jsonCharDump[jsonCharDumpIndex] == '\\' || jsonCharDump[jsonCharDumpIndex] == '\u0003' ){
                    jsonCharDump[jsonCharDumpIndex] = ' ';
                }
                if( jsonCharDump[jsonCharDumpIndex] == 0 ){
                    break;
                }
            }


        // add the message to list
            coCore::ptr->plugins->messageQueue->add( syslogd::ptr,
            coCore::ptr->hostNameGet(), "", "syslogd", "entry", jsonCharDump );


            returnVal = sd_journal_next( journal );
        }


        returnVal = sd_journal_wait( journal, (uint64_t)1000000 );
        if ( returnVal < 0 ) {
            //log_error_errno(r, "Couldn't wait for journal event: %m");
            goto finish;
        }
    }

finish:
    fflush(stdout);
    syslogdInstance->messageThreadStopReq = false;
    syslogdInstance->messageThreadRunning = false;
    coCore::ptr->plugins->messageQueue->add( syslogdInstance, coCore::ptr->nodeName(), coCore::ptr->nodeName(), "syslogd", "msgSuccess", "Stopped." );

}



#endif