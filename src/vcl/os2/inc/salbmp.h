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

#ifndef _SV_SALBMP_H
#define _SV_SALBMP_H

#include <tools/gen.hxx>
#include <vcl/sv.h>
#include <vcl/salbmp.hxx>

// --------------
// - SalBitmap    -
// --------------

struct    BitmapBuffer;
class    BitmapColor;
class    BitmapPalette;
class    SalGraphics;

#define HANDLE ULONG
#define HBITMAP ULONG

class Os2SalBitmap : public SalBitmap
{
private:

    Size                maSize;
    HANDLE				mhDIB;
    HANDLE				mhDIB1Subst;
    HBITMAP             mhDDB;
    USHORT              mnBitCount;

public:

    HANDLE              ImplGethDIB() const { return mhDIB; }
    HBITMAP             ImplGethDDB() const { return mhDDB; }
    HANDLE				ImplGethDIB1Subst() const { return mhDIB1Subst; }

    void				ImplReplacehDIB1Subst( HANDLE hDIB1Subst );

    static HANDLE       ImplCreateDIB( const Size& rSize, USHORT nBitCount, const BitmapPalette& rPal );
	static HANDLE		ImplCreateDIB4FromDIB1( HANDLE hDIB1 );
    static HANDLE       ImplCopyDIBOrDDB( HANDLE hHdl, BOOL bDIB );
    static USHORT       ImplGetDIBColorCount( HANDLE hDIB );
    static void         ImplDecodeRLEBuffer( const BYTE* pSrcBuf, BYTE* pDstBuf,
                                             const Size& rSizePixel, BOOL bRLE4 );

    //BOOL                Create( HANDLE hBitmap, BOOL bDIB, BOOL bCopyHandle );
	
public:

                        Os2SalBitmap();
                        ~Os2SalBitmap();

public:

    //BOOL                Create( const Size& rSize, USHORT nBitCount, const BitmapPalette& rPal );
    //BOOL                Create( const SalBitmap& rSalBmpImpl );
    //BOOL                Create( const SalBitmap& rSalBmpImpl, SalGraphics* pGraphics );
    //BOOL                Create( const SalBitmap& rSalBmpImpl, USHORT nNewBitCount );

    //void                Destroy();

    //Size                GetSize() const { return maSize; }
    //USHORT              GetBitCount() const { return mnBitCount; }

	//BitmapBuffer*		AcquireBuffer( bool bReadOnly );
	//void				ReleaseBuffer( BitmapBuffer* pBuffer, bool bReadOnly );
	bool                        Create( HANDLE hBitmap, bool bDIB, bool bCopyHandle );
	virtual bool                Create( const Size& rSize, USHORT nBitCount, const BitmapPalette& rPal );
	virtual bool                Create( const SalBitmap& rSalBmpImpl );
	virtual bool                Create( const SalBitmap& rSalBmpImpl, SalGraphics* pGraphics );
	virtual bool                Create( const SalBitmap& rSalBmpImpl, USHORT nNewBitCount );

	virtual void                Destroy();

	virtual Size                GetSize() const { return maSize; }
	virtual USHORT              GetBitCount() const { return mnBitCount; }

	virtual BitmapBuffer*		AcquireBuffer( bool bReadOnly );
	virtual void                ReleaseBuffer( BitmapBuffer* pBuffer, bool bReadOnly );
    virtual bool                GetSystemData( BitmapSystemData& rData );
};

#endif // _SV_SALBMP_H
