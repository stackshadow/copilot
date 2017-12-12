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

#ifndef sysState_H
#define sysState_H


#include "coPlugin.h"
#include "coCoreConfig.h"

#include "lockPthread.h"

#include "sysStateCmd.h"

#define healthBand 2

class sysState : public coPlugin
{
private:
        typedef struct sysStateThreadData_s {
            pthread_t           thread;
            bool                running;
            bool                requestEnd;

            sysStateCmd**      cmdArray;
            int                 cmdDelay;
        } sysStateThreadData;


    private:
        json_t*                 jsonConfigObject = NULL;
        json_t*                 jsonCommands = NULL;
        json_t*                 jsonTimes = NULL;

        lockID                  healthLock = 0;
        int                     cmdHealth = 100;

        sysStateThreadData**    cmdArrays = NULL;


	public:
                                sysState();
                                ~sysState();
        static sysState*        ptr;

        bool                    load();
        bool                    save();

        int                     health( int newHealth = -1 );
        static void             updateHealth( int newHealth ){ sysState::ptr->health(newHealth); };

        bool                    commandAppend( json_t* jsonCommand );

        bool                    createBash();

    // cmd thread
        void                    commandsRunAll();
        void                    commandsStopAllWait();
        static void*		    cmdThread( void* void_service );

// API
    public:
		coPlugin::t_state	    onBroadcastMessage( coMessage* message );


};





#endif
