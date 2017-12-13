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

#define LINUX_SYSINFO_LOADS_SCALE 65536.0

#include <stdio.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "memory/etList.h"

#include "coPlugin.h"

class sysState : public coPlugin {

	public:
		typedef struct mountedDevice_s {
			etString*			source;
			unsigned long		f_bsize;		/**< file system block size */
			unsigned long  		f_frsize;   	/**< fragment size */
			fsblkcnt_t     		f_blocks;   	/**< size of fs in f_frsize units */
			fsblkcnt_t     		f_bfree;    	/**< # free blocks */
			float				percentFree;
		} mountedDevice_t;

		typedef enum threadAction_e {
			THREAD_LOOP,
			THREAD_END
		} threadAction_t;

private:
// from here the vars should be thread save
		pthread_t           stateThread;
		threadAction_t		stateThreadRun = THREAD_LOOP;
		int					stateThreadRefreshSeconds = 10;

		// (h)ealth (m)ultiplikator
		/**
		This multiply the free size of an device ( in percent ) and compare it to the actual trv_health.
		For example if you use 2.0 the size of an device must be lower than 50% to effekt the trv_health.
		*/
		float				hmDeviceSize = 2.0;
		float				hmRamSize = 1.0;

		// trv ( Threaded values )
		pthread_mutex_t 	trv_Mutex = PTHREAD_MUTEX_INITIALIZER;
		float				trv_health = 100.0;

		etList*				trv_devicesList = NULL;
		float				trv_ramFreePercent = 0.0;
		float				trv_load01 = 0.0;
		float				trv_load05 = 0.0;
		float				trv_load15 = 0.0;



// public functions
	public:
							sysState();
							~sysState();
		static sysState* 	ptr;

	private:
		static void*		stateRefreshThread( void *userdata );

// functions for refresh infos
		void    			refreshFreeSpace();
		void		    	refreshLoad();
		void    			refreshRam();

// device
		mountedDevice_t*	deviceGet( const char* source );
		void				updateDeviceSize( struct mntent* mountEntry );

// json-stuff
		void				mountedDeviceAppendToJson( mountedDevice_t* mountedDevice, json_t* outJson );
		void				freeRamAppendToJson( json_t* outJson );
		void				cpuLoadAppendToJson( json_t* outJson );

// callbacks
	public:
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		//bool        		onBroadcastReply( coMessage* message );


};
















#endif
