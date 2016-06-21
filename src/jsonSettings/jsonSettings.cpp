/*  Copyright (C) 2016 by Martin Langlotz alias stackshadow

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

#include "jsonSettings.h"

#include <errno.h>
#include <string.h>



jsonSettings::                  jsonSettings(){

    this->jsonFilename = NULL;
    this->langCodeActive = false;
    this->jsonRoot = json_object();
    this->jsonSettingIterator = NULL;

}

jsonSettings::                  ~jsonSettings(){

}



bool jsonSettings::             open( const char *fileName ){


// correctly opened, save filename
    this->jsonFilename = json_string(fileName);


// check if file exist
    FILE *fp = fopen( fileName, "r" );
    if( fp == NULL ){
        return false;
    }
    fclose(fp);

    json_error_t jsonError;
    this->jsonRoot = json_load_file( fileName, JSON_PRESERVE_ORDER, &jsonError );
    if( this->jsonRoot == NULL ) return false;

    return true;
}


bool jsonSettings::             createFile( const char *fileName ){

// okay try to create the file
    FILE *fp = fopen( fileName, "w" );
    if( fp == NULL ){
        fprintf( stderr, "Error opening config file '%s': %s", fileName, strerror(errno) );
        exit(-1);
    }

    fclose(fp);

// create json root
    this->jsonRoot = json_object();

// save it one time
    if( json_dump_file( this->jsonRoot, json_string_value(this->jsonFilename), JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
        return true;
    }

    return false;
}


bool jsonSettings::             saveToFile(){
    if( this->jsonRoot == NULL ) return false;
    if( this->jsonFilename == NULL ) return false;

// dump to stdout
    json_dumpf( this->jsonRoot, stdout, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    if( json_dump_file( this->jsonRoot, json_string_value(this->jsonFilename), JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
        return true;
    }

    return false;
}


void jsonSettings::             langCodeSet( const char langCode[3] ){
    strncpy( this->langCode, langCode, 3 );
}




bool jsonSettings::             groupPick( const char *group ){

    this->jsonGroup = json_object_get( this->jsonRoot, group );
    if( this->jsonGroup == NULL ){
        this->jsonGroup = json_object();
        json_object_set_new( this->jsonRoot, group, this->jsonGroup );
    }

    return true;
}




void jsonSettings::             settingIterateReset(){
// check
    if( this->jsonGroup == NULL ) return;

    this->jsonSettingIterator = json_object_iter( this->jsonGroup );
}


bool jsonSettings::             settingIterateNext(){
// check
    if( this->jsonGroup == NULL ) return false;

    this->jsonSettingIterator = json_object_iter_next( this->jsonGroup, this->jsonSettingIterator );
    if( this->jsonSettingIterator != NULL ){
        this->jsonSetting = json_object_iter_value(this->jsonSettingIterator);
        if( this->jsonSetting != NULL ){
            return true;
        }
    }

    return false;
}


bool jsonSettings::             settingPick( const char *group, const char *name ){

// first pick the grouo
    if( groupPick(group) == false ){
        return false;
    }

// try to get the object
    this->jsonSetting = json_object_get( this->jsonGroup, name );
    if( this->jsonSetting == NULL ){

    // create a new setting
        this->jsonSetting = json_object();
        json_object_set_new( this->jsonSetting, "name", json_string(name) );

    // append it to the group
        json_object_set_new( this->jsonGroup, name, this->jsonSetting );
    }

    return true;
}




void jsonSettings::             nameGet( const char **name ){
// check
    if( this->jsonSetting == NULL ){
        *name = NULL;
        return;
    }

// get
    json_t *jsonName = json_object_get( this->jsonSetting, "name" );
    if( jsonName == NULL ){
        *name = NULL;
        return;
    }


    *name = json_string_value( jsonName );
    return;
}


void jsonSettings::             valueSet( const char *valueName, const char *value ){
// check
    if( this->jsonSetting == NULL ){
        return;
    }

// save the value
    json_object_set_new( this->jsonSetting, valueName, json_string(value) );

}


const char* jsonSettings::      valueGet( const char *valueName ){
// check
    if( this->jsonSetting == NULL ){
        return NULL;
    }


    json_t *jsonValue = NULL;
    jsonValue = json_object_get( this->jsonSetting, valueName );
    if( jsonValue != NULL ){
        return json_string_value(jsonValue);
    }

    return NULL;
}


void jsonSettings::             descriptionSet( const char *desciption ){
    if( this->jsonSetting == NULL ) return;

    json_object_set_new( this->jsonSetting, "description", json_string(desciption) );
}


void jsonSettings::             descriptionGet( const char **desciption ){
    if( this->jsonSetting == NULL ){
        *desciption = NULL;
        return;
    }

    json_t *jsonDescription = json_object_get( this->jsonSetting, "description" );
    if( jsonDescription == NULL ){
        *desciption = NULL;
        return;
    }

    *desciption = json_string_value( jsonDescription );
    return;
}

