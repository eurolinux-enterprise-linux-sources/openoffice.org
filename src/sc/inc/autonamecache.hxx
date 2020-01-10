/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

#ifndef SC_AUTONAMECACHE_HXX
#define SC_AUTONAMECACHE_HXX

#include <vector>
#include <hash_map>
#include "address.hxx"
#include "global.hxx"

typedef ::std::vector< ScAddress > ScAutoNameAddresses;
typedef ::std::hash_map< String, ScAutoNameAddresses, ScStringHashCode, ::std::equal_to< String > > ScAutoNameHashMap;

//
//  Cache for faster lookup of automatic names during CompileXML
//  (during CompileXML, no document content is changed)
//
 
class ScAutoNameCache
{
    ScAutoNameHashMap   aNames;
    ScDocument*         pDoc;
    SCTAB               nCurrentTab;

public:
            ScAutoNameCache( ScDocument* pD );
            ~ScAutoNameCache();

    const ScAutoNameAddresses& GetNameOccurences( const String& rName, SCTAB nTab );
};

#endif

