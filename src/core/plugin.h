
#include "core/coCore.h"
#include "core/coConfig.h"
#include "pubsub.h"


#ifdef __cplusplus
extern "C" {
#endif

// plugin functions
typedef struct pluginData_s pluginData_t ;


typedef void            (*pluginPrepare_tf)( pluginData_t* pluginData );
typedef int             (*parseCmdLine_tf)( pluginData_t* pluginData, const char* option, const char* value );
typedef void            (*pluginRun_tf)( pluginData_t* pluginData );




// function for plugins
void    pluginSetInfos( pluginData_t* pluginData, const char* name, const char* description );
void    pluginSetConfigSection( pluginData_t* pluginData, const char* configSectionName );
void    pluginSetUserData( pluginData_t* pluginData, void* userdata );
void*   pluginUserData( pluginData_t* pluginData );

// register
void    pluginRegisterCmdParse( pluginData_t* pluginData, parseCmdLine_tf parseCmdLine_fct );
void    pluginRegisterCmdRun( pluginData_t* pluginData, pluginRun_tf pluginRun_fct );




bool    loadPlugin( const char* fileName );
bool    parsePluginOption( const char *arg, const char* argv );
bool    runAllPlugins();


#define pluginVersion 1
#define pluginSetVersion( pluginData ) pluginData.version = pluginVersion


#ifdef __cplusplus
}
#endif


