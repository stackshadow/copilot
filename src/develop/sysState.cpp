/* Copyright (C) 2017 by Martin Langlotz

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

#ifndef sysState_C
#define sysState_C

#include <stdio.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include "plugins/sysState.h"
#include "coCore.h"

sysState* sysState::ptr = NULL;
sysState::         							sysState() : coPlugin( "sysState", "", "sýstate" ){

// save
    sysState::ptr = this;

// allocate device-list
	etListAlloc( this->trv_devicesList );

// Initial read of system-state
	this->refreshFreeSpace();
	this->refreshLoad();
	this->refreshRam();

// run threads
	this->stateThreadRun = sysState::THREAD_LOOP;
    pthread_create( &this->stateThread, NULL, sysState::stateRefreshThread, this );
    pthread_setname_np( this->stateThread, "sysState\0" );

// register plugin
	coCore::ptr->plugins->append( this );

// test
	coMessage* message = new coMessage();
	message->command( "getinfos" );
	this->onBroadcastMessage( message );
	message->command( "getHealth" );
	this->onBroadcastMessage( message );
	delete message;

}

sysState:: 									~sysState(){

// wait for finish the thread
	this->stateThreadRun = sysState::THREAD_END;
	pthread_join( this->stateThread, NULL );

	etDebugMessage( etID_LEVEL_DETAIL_APP, "sysState Deleted" );
	fflush( stdout );
	fflush( stderr );
}




void* sysState::							stateRefreshThread( void *userdata ){

// vars
	sysState*		sysInstance = (sysState*)userdata;

	while( sysInstance->stateThreadRun == sysState::THREAD_LOOP ){

		pthread_mutex_lock( &sysInstance->trv_Mutex );
		sysInstance->refreshFreeSpace();
		sysInstance->refreshLoad();
		sysInstance->refreshRam();
		pthread_mutex_unlock( &sysInstance->trv_Mutex );

		sleep( sysInstance->stateThreadRefreshSeconds );
	}

	return NULL;
}


void sysState::    							refreshFreeSpace(){

// vars
	FILE* 				mtab = NULL;
	struct mntent*		entry = NULL;

// start to read mtab
	mtab = setmntent( "/etc/mtab", "r" );

// lock the list
	while( (entry = getmntent(mtab)) != NULL ){
		this->updateDeviceSize(entry);
	}

// release sources
	endmntent( mtab );
}


void sysState::    							refreshLoad(){

// vars
	struct sysinfo		systemInfo;

// get system info
	sysinfo( &systemInfo );

	this->trv_load01 = systemInfo.loads[0] / LINUX_SYSINFO_LOADS_SCALE;
	this->trv_load05 = systemInfo.loads[1] / LINUX_SYSINFO_LOADS_SCALE;
	this->trv_load15 = systemInfo.loads[2] / LINUX_SYSINFO_LOADS_SCALE;

}


void sysState::  	  						refreshRam(){

// vars
	size_t					readBufferSize = 1024 * sizeof(char);
	char*					readBuffer = NULL;
	void*					readBufferPtr = readBuffer;
	unsigned long long 		totalRam = 0L;
	unsigned long long 		freeRam = 0L;

// read /proc/meminfo
	readBuffer = (char*)malloc( readBufferSize );
	FILE* fp = fopen( "/proc/meminfo", "r" );
	if ( fp != NULL ){
		while ( getline( &readBuffer, &readBufferSize, fp ) >= 0 ){


			if ( strncmp( readBuffer, "MemTotal", 8 ) == 0 ){
				sscanf( readBuffer, "%*s%ld", &totalRam );
			}

			if ( strncmp( readBuffer, "MemAvailable", 12 ) == 0 ){
				sscanf( readBuffer, "%*s%ld", &freeRam );
			}

		}
		fclose( fp );
	}
	free(readBuffer);


// calculate free memory in percent
	if( totalRam != 0L ){
		this->trv_ramFreePercent = freeRam * 100.0 / totalRam;
	} else {
		this->trv_ramFreePercent = 0L;
	}

// calculate health
	if( this->trv_health > this->trv_ramFreePercent * this->hmRamSize ){
		this->trv_health = this->trv_ramFreePercent * this->hmRamSize;
	}

}

/** @brief Get a Device by source-name

@warn This function dont lock the mutex !

@param source The source of mount
@return
*/
sysState::mountedDevice_t* sysState::		deviceGet( const char* source ){

	sysState::mountedDevice_t* 		mountedDevice = NULL;
	void*							iterator = NULL;
	const char*						sourceChar;
	size_t							sourceLen = strlen(source);

	etListIterate( this->trv_devicesList, iterator );
	while( etListIterateNext(iterator,mountedDevice) == etID_YES ){

		etStringCharGet( mountedDevice->source, sourceChar );
		if( strncmp( source, sourceChar, sourceLen ) == 0 ){
			return mountedDevice;
		}

	}

	return NULL;
}


void sysState::								updateDeviceSize( struct mntent* mountEntry ){

// first check if device exist
	sysState::mountedDevice_t* 		mountedDevice = NULL;
	struct statvfs					deviceStat;


// read statistic
	if( statvfs( mountEntry->mnt_dir, &deviceStat ) != 0 ){
		return;
	}

// if filesystem has noch blocks, its virtual ( like /dev /tmp etc. )
	if( deviceStat.f_blocks == 0 ){
		return;
	}

// try to find the device
	mountedDevice = this->deviceGet( mountEntry->mnt_fsname );

// create new
	if( mountedDevice == NULL ){

		etMemoryAlloc( mountedDevice, sizeof(sysState::mountedDevice_t) );
		etStringAlloc( mountedDevice->source );
		etStringCharSet( mountedDevice->source, mountEntry->mnt_fsname, -1 );
		mountedDevice->f_bsize = deviceStat.f_bsize;
		mountedDevice->f_frsize = deviceStat.f_frsize;

		etListAppend( this->trv_devicesList, mountedDevice );
	}

// update
	mountedDevice->f_blocks = deviceStat.f_blocks;
	mountedDevice->f_bfree = deviceStat.f_bfree;

// calculate percentage
	if( mountedDevice->f_blocks != 0 ){
		mountedDevice->percentFree = deviceStat.f_bfree * 100 / deviceStat.f_blocks;
	} else {
		mountedDevice->percentFree = 0.0;
	}

// calculate health
	if( this->trv_health > mountedDevice->percentFree * this->hmDeviceSize ){
		this->trv_health = mountedDevice->percentFree * this->hmDeviceSize;
	}

}




void sysState::								mountedDeviceAppendToJson( mountedDevice_t* mountedDevice, json_t* outJson ){

// vars
	json_t*							jsonDevice = NULL;
	const char*						tempConstChar = NULL;
	char							tempIntChar[10] = { 0,0,0,0,0,0,0,0,0,0 };

// single device
	jsonDevice = json_object();
	snprintf( tempIntChar, sizeof(tempIntChar), "%3.2f\0", mountedDevice->percentFree );
	json_object_set_new( jsonDevice, "freepercent", json_string(tempIntChar) );

// add device to global object
	etStringCharGet( mountedDevice->source, tempConstChar );
	json_object_set_new( outJson, tempConstChar, jsonDevice );

}


void sysState::								freeRamAppendToJson( json_t* outJson ){

// convert time to char
	char	tempChar[10] = { 0,0,0,0,0,0,0,0,0,0 };
	snprintf( tempChar, 10, "%3.2f", this->trv_ramFreePercent );

	json_object_set_new( outJson, "freepercent", json_string(tempChar) );

}


void sysState::								cpuLoadAppendToJson( json_t* outJson ){

// convert time to char
	char	tempChar[10] = { 0,0,0,0,0,0,0,0,0,0 };


	snprintf( tempChar, 10, "%3.2f", this->trv_load01 );
	json_object_set_new( outJson, "load01", json_string(tempChar) );

	snprintf( tempChar, 10, "%3.2f", this->trv_load05 );
	json_object_set_new( outJson, "load05", json_string(tempChar) );

	snprintf( tempChar, 10, "%3.2f", this->trv_load15 );
	json_object_set_new( outJson, "load15", json_string(tempChar) );
}




coPlugin::t_state sysState:: 				onBroadcastMessage( coMessage* message ){


// vars
    int                 			msgPayloadLen;
	const char*						msgHostName = message->hostName();
	const char*						msgGroup = message->group();
	const char*						msgCommand = message->command();
	const char*						fullTopic = message->topic();
	const char*						msgPayload = message->payload();
	sysState::mountedDevice_t* 		mountedDevice = NULL;
	void*							mountedDeviceIterator = NULL;
	const char*						tempConstChar = NULL;
	bool							messageForThisHost = false;
	bool							messageForAllHost = false;


// to this host
	if( coCore::ptr->isHostName(msgHostName) == false ){
		messageForThisHost = true;
	}
// to all hosts
    if( strncmp( (char*)msgHostName, "all", 3 ) == 0 ){
		messageForAllHost = true;
	}



// for all or this
	if( messageForThisHost || messageForAllHost ){

		if( strncmp( msgCommand,"getHealth", 9 ) == 0 ){

		// temp value
			char jsonValue[64];
			memset( jsonValue, 0, 64 );


			pthread_mutex_lock( &this->trv_Mutex );
			snprintf( jsonValue, 64, "{ \"value\": %3.2f }", this->trv_health );
			pthread_mutex_unlock( &this->trv_Mutex );
			message->replyCommand( "health" );
			message->replyPayload( jsonValue );
			return coPlugin::REPLY;
		}

	}


// only for this host
	if( messageForThisHost == false ){
		return coPlugin::NO_REPLY;
	}



// infos of system
	if( strncmp( msgCommand,"getInfos", 8 ) == 0 ){

		json_t*		jsonObject = json_object();


	// #################### Devices
	// lock
		pthread_mutex_lock(&this->trv_Mutex);

		json_t* jsonDevices = json_object();
		etListIterate( this->trv_devicesList, mountedDeviceIterator );
		while( etListIterateNext(mountedDeviceIterator,mountedDevice) == etID_YES ){
			mountedDeviceAppendToJson( mountedDevice, jsonDevices );
		}
		json_object_set_new( jsonObject, "devices", jsonDevices );

	// #################### RAM
		json_t* jsonRam = json_object();
		freeRamAppendToJson( jsonRam );
		json_object_set_new( jsonObject, "ram", jsonRam );

	// #################### CPU
		json_t* jsonCPULoad = json_object();
		cpuLoadAppendToJson( jsonCPULoad );
		json_object_set_new( jsonObject, "cpu", jsonCPULoad );


	// unlock
		pthread_mutex_unlock(&this->trv_Mutex);

	// dump
	/*
		const char * jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
		etDebugMessage( etID_LEVEL_DETAIL, jsonObjectChar );
		free( (void*)jsonObjectChar );
	*/

	// clean
		json_decref( jsonObject );

		return coPlugin::REPLY;
	}





	return coPlugin::NO_REPLY;
}




#endif

