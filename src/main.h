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

#include <evillib_defines.h>
#include <evillib_depends.h>
#include <evillib-extra_depends.h>
#include <core/etIDState.h>
#include "db/etDBObject.h"
#include "jsonSettings/jsonSettings.h"
#include "doDBDebug/doDBDebug.h"
#include "qt/doDBSettings/doDBSettings.h"

extern doDBSettings     *doDBSettingsGlobal;
extern doDBDebug        *debug;

typedef struct          t_globalSettings {
    bool    debuggingEnabled;
} t_globalSettings;

extern t_globalSettings globalSettings;
