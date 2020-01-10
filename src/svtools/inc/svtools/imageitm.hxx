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

#ifndef _SVT_IMAGEITM_HXX
#define _SVT_IMAGEITM_HXX

#include "svtools/svtdllapi.h"
#include <svtools/intitem.hxx>

class String;

struct SfxImageItem_Impl;
class SVT_DLLPUBLIC SfxImageItem : public SfxInt16Item
{
    SfxImageItem_Impl*      pImp;
public:
							TYPEINFO();
                            SfxImageItem( USHORT nWhich = 0, UINT16 nImage = 0 );
                            SfxImageItem( USHORT nWhich, const String& rURL );
                            SfxImageItem( const SfxImageItem& );
    virtual                 ~SfxImageItem();

	virtual SfxPoolItem*	Clone( SfxItemPool* pPool = 0 ) const;
	virtual int 			operator==( const SfxPoolItem& ) const;
    virtual BOOL            QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
    virtual BOOL            PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

    void                    SetRotation( long nValue );
    long                    GetRotation() const;
    void                    SetMirrored( BOOL bSet );
    BOOL                    IsMirrored() const;
    String                  GetURL() const;
};

#endif // _SFX_IMAGEITM_HXX
