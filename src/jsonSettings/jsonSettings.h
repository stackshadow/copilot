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

#ifndef DODBSETTINGS_H
#define DODBSETTINGS_H

#include <jansson.h>

class jsonSettings
{
public:
    jsonSettings();
    ~jsonSettings();

// default stuff
    bool            open( const char *fileName );
    bool            createFile( const char *fileName );
    bool            save();
    void            langCodeSet( const char langCode[3] );

// group
    bool            groupPick( const char *group );

// Iteration
    void            settingIterateReset();
    bool            settingIterateNext();
    bool            settingPick( const char *group, const char *name );

// get / set selected setting
    void            nameGet( const char **name );
    void            valueSet( const char *valueName, const char *value );
    const char*     valueGet( const char *valueName );
    void            descriptionSet( const char *desciption );
    void            descriptionGet( const char **desciption );


public:
    bool            langCodeActive;
    char            langCode[3];

// the filename as json
    json_t          *jsonFilename;

    json_t          *jsonRoot;
    json_t          *jsonGroup;
    void            *jsonSettingIterator;
    json_t          *jsonSetting;
};





#endif // DODBSETTINGS_H
