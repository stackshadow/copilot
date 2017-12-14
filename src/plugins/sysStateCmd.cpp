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

#ifndef sysStateCmd_C
#define sysStateCmd_C

#include "sysStateCmd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#include "core/etDebug.h"

sysStateCmd::              sysStateCmd( const char* id, const char* command, int min, int max, sysHealthSetFct* updateHealthFunction ){


// init
    this->cmdRaw = 0;
    this->cmdHealth = 0;
    this->cmdValueMin = min;
    this->cmdValueMax = max;

// id
    etStringAllocLen( this->uuid, 37 );
    etStringCharSet( this->uuid, id, -1 );

// command
    etStringAllocLen( this->cmd, 1024 );
    etStringCharSet( this->cmd, command, -1 );


    this->updateHealth = updateHealthFunction;
}


sysStateCmd::              ~sysStateCmd(){

}




bool sysStateCmd::         execute( char* commandOutput, int* commandOutputSize ){


// vars
    const char*     command = NULL;
    int*            cmdOutSize = 0;
    char*           cmdOutChar = NULL;
    int             tmpCmdOutSize = 1024;
    char            tmpCmdOutChar[tmpCmdOutSize];

// temp ?
    if( commandOutput == NULL ){
        cmdOutSize = &tmpCmdOutSize;
        cmdOutChar = tmpCmdOutChar;
    } else {
        cmdOutSize = commandOutputSize;
        cmdOutChar = commandOutput;
    }

// clean
    memset( cmdOutChar, 0, *cmdOutSize );
    int rangeDiff = this->cmdValueMax - this->cmdValueMin;

// check
    if( rangeDiff == 0 ) rangeDiff = 1;

// open the command
    etStringCharGet( this->cmd, command );
    FILE* pf = popen( command, "r" );
    while( fgets(cmdOutChar, *cmdOutSize, pf) != NULL ){
        cmdOutChar[strlen(cmdOutChar)-1] = 0;
    }

// rangeDiff == 100 %
// health = ???? + min
    this->cmdRaw = atoi(cmdOutChar);

// calculate percentage
    this->cmdHealth = (this->cmdRaw - this->cmdValueMin) * 100 / rangeDiff;

// clean
    fclose(pf);

// health bigger
    if( this->cmdHealth > 100 ){
        this->cmdHealth = 100;
    }

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Command: %s Result: %s Health: %d", this->cmd, cmdOutChar, this->cmdHealth );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// update health if needed
    if( this->updateHealth != NULL ){
        this->updateHealth( this->cmdHealth );
    }


    return true;
}




#endif