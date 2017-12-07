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

class sysState : public coPlugin
{



    private:
        json_t*                 jsonConfigObject = NULL;
        json_t*                 jsonCommands = NULL;
        json_t*                 jsonTimes = NULL;

        lockID                  healthLock = 0;
        int                     health = 100;


	public:
                                sysState();
                                ~sysState();
        static sysState*        ptr;

        bool                    load();
        bool                    save();

        void                    healthSet( int newHealth );
        static void             updateHealth( int newHealth ){ sysState::ptr->healthSet(newHealth); };

        bool                    commandAppend( json_t* jsonCommand );

        bool                    createBash();

    // cmd thread
        void                    runAllCommands();
        static void*		    cmdThread( void* void_service );

// API
    public:
		coPlugin::t_state	    onBroadcastMessage( coMessage* message );


};





#endif
