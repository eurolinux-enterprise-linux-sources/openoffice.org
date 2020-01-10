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

#ifndef _SOT_STORINFO_HXX
#define _SOT_STORINFO_HXX

#include <tools/pstm.hxx>
#include <tools/globname.hxx>
#include <tools/ownlist.hxx>
#include "sot/sotdllapi.h"

class StgDirEntry;
class SvStorageInfo
{
friend class SvStorage;
    String          aName;
    SvGlobalName    aClassName;
    ULONG           nSize;
    BOOL            bStream:1,
                    bStorage:1;

							SvStorageInfo(){}; // Fuer SvStorage
public:
                            SvStorageInfo( const StgDirEntry& );
                            SvStorageInfo( const String& rName, ULONG nSz, BOOL bIsStorage )
                                : aName( rName )
                                , nSize( nSz )
                                , bStream( !bIsStorage )
                                , bStorage( bIsStorage )
                            {}

    const SvGlobalName &    GetClassName() const { return aClassName; }
    const String &          GetName() const { return aName; }
    BOOL                    IsStream() const { return bStream; }
    BOOL                    IsStorage() const { return bStorage; }
    ULONG                   GetSize() const { return nSize;      }
};

class SOT_DLLPUBLIC SvStorageInfoList
{
    PRV_SV_DECL_OWNER_LIST(SvStorageInfoList,SvStorageInfo)
    const SvStorageInfo * Get( const String & rName );
};

SOT_DLLPUBLIC ULONG ReadClipboardFormat( SvStream & rStm );
SOT_DLLPUBLIC void WriteClipboardFormat( SvStream & rStm, ULONG nFormat );

#endif // _STORINFO_HXX
