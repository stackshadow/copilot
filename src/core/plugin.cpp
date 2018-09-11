
#include <dlfcn.h>
#include "plugin.h"
#include "core/etDebug.h"


struct pluginData_s {

// version of pluginData
    unsigned int        version;

// common stuff
    const char*         name;
    const char*         description;
    const char*         configSectionName;

// data to handle plugin
    void*               dlHandle;
    bool                finishWithInit;

// function pointer
    parseCmdLine_tf     parseCmdLine;
    pluginRun_tf        pluginRun;

// data for plugin
    void*               userdata;

};


static etList* pluginList = NULL;
static int     g_argc;
static char**  g_argv;



void    pluginSetInfos( pluginData_t* pluginData, const char* name, const char* description ){
    pluginData->name = strdup( name );
    pluginData->description = strdup( description );
}

void    pluginSetConfigSection( pluginData_t* pluginData, const char* configSectionName ){
    pluginData->configSectionName = strdup( configSectionName );
}

void    pluginSetUserData( pluginData_t* pluginData, void* userdata ){
    pluginData->userdata = userdata;
}

void*   pluginUserData( pluginData_t* pluginData ){
    return pluginData->userdata;
}


void    pluginRegisterCmdParse( pluginData_t* pluginData, parseCmdLine_tf parseCmdLine_fct ){
    pluginData->parseCmdLine = parseCmdLine_fct;
}

void    pluginRegisterCmdRun( pluginData_t* pluginData, pluginRun_tf pluginRun_fct ){
    pluginData->pluginRun = pluginRun_fct;
}









bool    loadPlugin( const char* fileName ){


    void*               handle = NULL;
    char*               error;

// init list if needed
    if( pluginList == NULL ){
        etListAlloc( pluginList );
    }

// open plugin
    std::string     fillFileName = PLUGINPATH;
    fillFileName += "/";
    fillFileName += fileName;
    fillFileName += ".so";
    handle = dlopen( fillFileName.c_str(), RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }


// prepare
    pluginData_s* pluginData = NULL;
    etMemoryAlloc( pluginData, sizeof(pluginData_s) );


    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Prepare '%s'", fileName );
    etDebugMessage( etID_LEVEL_DETAIL_THREAD, etDebugTempMessage );

    pluginPrepare_tf pluginPrepare = (pluginPrepare_tf)dlsym( handle, "pluginPrepare" );
    if ( pluginPrepare == NULL ) {
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s", dlerror() );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        exit(EXIT_FAILURE);
    }
    pluginPrepare( pluginData );

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Prepare '%s' finished", pluginData->name );
    etDebugMessage( etID_LEVEL_DETAIL_THREAD, etDebugTempMessage );


// add plugin to the list
    etListAppend( pluginList, (void*)pluginData );

    return true;
}


bool    parsePluginOption( const char *arg, const char* argv ){

//vars
    void*               pluginListIterator = NULL;
    pluginData_s*       pluginData = NULL;

// iterate and init
    etListIterate( pluginList, pluginListIterator );
    while( etListIterateNext( pluginListIterator, pluginData ) == etID_YES ){
        if( pluginData->parseCmdLine == NULL ) continue;
        pluginData->parseCmdLine( pluginData, arg, argv );
    }

    fflush( stdout );
    return true;
}




void*   runPluginThread( void* pluginData_v ){

// vars
    pluginData_t*       pluginData = (pluginData_t*)pluginData_v;
    pluginRun_tf        pluginRun = NULL;

// init plugin
    if( pluginData->pluginRun == NULL ) return NULL;

// run
    pluginData->pluginRun( pluginData );

    return NULL;
}


bool    runAllPlugins(){

//vars
    void*               pluginListIterator = NULL;
    pluginData_s*       pluginData = NULL;

// iterate and init
    etListIterate( pluginList, pluginListIterator );
    while( etListIterateNext( pluginListIterator, pluginData ) == etID_YES ){

        if( pluginData->pluginRun == NULL ) continue;

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Run Plugin '%s'", pluginData->name );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

    // create thread
        pthread_t   initThread;
        pthread_create( &initThread, NULL, runPluginThread, pluginData );
        pthread_detach( initThread );

    }

// return
    return true;
}



