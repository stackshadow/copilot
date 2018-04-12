/*  Copyright (C) 2017 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef threadList_C
#define threadList_C

#ifdef __cplusplus
extern "C" {
#endif

#include "threadList.h"



etID_STATE 		etThreadListAlloc( threadList_t** threadList ){
	
// vars
	threadList_t*	newThreadList = NULL;
	
// allocate the list itselfe
	etMemoryAlloc( newThreadList, sizeof(threadList_t) );
	
// setup
	newThreadList->count = 0;
	newThreadList->items = NULL;
	
	*threadList = newThreadList;
	return etID_YES;
}


etID_STATE 		etThreadListFree( threadList_t** threadList ){
	
}


etID_STATE 		etThreadListAdd( threadList_t* threadList, const char* name, threadFunctionStart startFunction, threadFunctionStop stopFunction, void* userdata ){
	
// vars
	threadListItem_t*	newThreadListItem;
	threadListItem_t**	newThreadListItems;
	int					newThreadListItemsCount;
	
// list-item: alloc and set
	etMemoryAlloc( newThreadListItem, sizeof(threadListItem_t) );
	newThreadListItem->functionStart = startFunction;
	newThreadListItem->functionCancel = stopFunction;
	newThreadListItem->functionData = userdata;
	
// list-array: alloc and copy
	newThreadListItemsCount = threadList->count + 1;
	etMemoryAlloc( newThreadListItems, sizeof(threadListItem_t*) * newThreadListItemsCount );
	memcpy( newThreadListItems, threadList->items, sizeof(threadListItem_t*) * threadList->count );
	
// list-array: set new list-item
	newThreadListItems[threadList->count] = newThreadListItem;
	
// list-array: replace
	if( threadList->items != NULL )	etMemoryRelease( threadList->items );
	threadList->items = newThreadListItems;
	threadList->count = newThreadListItemsCount;

// start the thread
	pthread_create( &newThreadListItem->thread, NULL, startFunction, newThreadListItem->functionData );
	snprintf( newThreadListItem->threadName, 16, "%s\0", name );
	pthread_setname_np( newThreadListItem->thread, newThreadListItem->threadName );
	pthread_detach( newThreadListItem->thread );

	return etID_YES;
}


etID_STATE 		etThreadListCancelAll( threadList_t* threadList ){
	
// vars
	int						index = 0;
	threadListItem_t*		threadListItem;
	
	
	for( index = 0; index < threadList->count; index++ ){
		
	// get element
		threadListItem = threadList->items[index];
		if( threadListItem == NULL ) continue;
		
	// call 
		if( threadListItem->functionCancel != NULL ){
			threadListItem->functionCancel( threadListItem->functionData );
		}

	// cancel thread
		pthread_cancel( threadListItem->thread );
		
	// release memory
		etMemoryRelease( threadListItem );
		threadList->items[index] = NULL;
	}

// release item-array
	etMemoryRelease( threadList->items );
	threadList->items = NULL;
	threadList->count = 0;
	
}





#ifdef __cplusplus
}
#endif





#endif