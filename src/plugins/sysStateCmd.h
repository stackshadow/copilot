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

#ifndef sysStateCmd_H
#define sysStateCmd_H

#include "string/etString.h"
#include "string/etStringChar.h"

#define cmdSize 2048

#include "libio.h"

typedef void sysHealthSetFct( int newHealth, void* cmd );

class sysStateCmd
{
    private:
        sysHealthSetFct*    updateHealth = NULL;
        etString*           uuid = NULL;
        etString*           cmd = NULL;
        etString*           cmdDisplayName = NULL;
        int                 cmdRaw;
        int                 cmdHealth;
        int                 cmdValueMin;
        int                 cmdValueMax;


    public:
        sysStateCmd( const char* id, const char* command, const char* displayName, int min = 0, int max = 100, sysHealthSetFct* updateHealthFunction = NULL );
        ~sysStateCmd();

    public:
        bool            execute( char* commandOutput = NULL, int* commandOutputSize = NULL );
        int             health(){ return this->cmdHealth; };
        const char*     displayName();

};

#endif