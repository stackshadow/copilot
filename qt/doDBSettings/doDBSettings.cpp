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

#include "doDBSettings.h"
doDBSettings* doDBSettings::ptr = NULL;

#include <QDir>

doDBSettings::              doDBSettings() : jsonSettings() {

// save pointer
    this->ptr = this;


    QString configFileName = "";


    configFileName = QDir::homePath();
    configFileName += "/doDB.json";
    if( ! this->open( configFileName.toUtf8() ) ){

    // we need the home directory
        configFileName = QDir::currentPath();
        configFileName += "/doDB.json";

    //
        if( ! this->open( configFileName.toUtf8() ) ){

            if( ! this->createFile( configFileName.toUtf8() ) ) exit(-1);

        }
    }


    this->settingPick( "global", "debugging" );

    this->debugginMode = false;
    const char *debuggingEnabled = this->valueGet( "enabled" );
    if( debuggingEnabled != NULL ){
        if( strncmp( debuggingEnabled, "1", 1 ) == 0 ){
            this->debugginMode = true;
        }
    }

}


bool doDBSettings::         debugModeEnabled(){
    return true;
    return this->debugginMode;
}


QString doDBSettings::      treePictureDirectory(){

    this->settingPick( "directorys", "core" );
    const char* picturePath = this->valueGet( "treeIconPath" );
    if( picturePath != NULL ){
        return QString(picturePath);
    }

    QString configFileName = "./pictures";
    return configFileName;
}

