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

#include <salstd.hxx>
#ifndef _SV_SALGTYPE
#include <vcl/salgtype.hxx>
#endif
#include <saldisp.hxx>
#include <vcl/salbmp.hxx>
#include <vcl/dllapi.h>

struct	BitmapBuffer;
class	BitmapPalette;
class	SalGraphics;
class	ImplSalDDB;
class	ImplSalBitmapCache;

// -------------
// - SalBitmap -
// -------------

class VCL_DLLPUBLIC X11SalBitmap : public SalBitmap
{
private:

	static BitmapBuffer*		ImplCreateDIB( const Size& rSize,
											   USHORT nBitCount, 
											   const BitmapPalette& rPal );
	static BitmapBuffer*		ImplCreateDIB( Drawable aDrawable,
                                               int nScreen,
											   long nDrawableDepth,
											   long nX, long nY,
											   long nWidth, long nHeight );

public:

	static ImplSalBitmapCache*	mpCache;
	static ULONG				mnCacheInstCount;

	static void					ImplCreateCache();
	static void					ImplDestroyCache();
    void				        ImplRemovedFromCache();

    bool                        SnapShot (Display* pDisplay, XLIB_Window hWindow);
    bool                        ImplCreateFromXImage( Display* pDisplay,
                                                      XLIB_Window hWindow,
                                                      int nScreen,
                                                      XImage* pImage);
private:


	BitmapBuffer*	mpDIB;
	ImplSalDDB*		mpDDB;
								
public:

	SAL_DLLPRIVATE bool    ImplCreateFromDrawable( Drawable aDrawable,
                                                  int nScreen,
                                                  long nDrawableDepth,
                                                  long nX, long nY, 
                                                  long nWidth, long nHeight );

	SAL_DLLPRIVATE XImage* ImplCreateXImage( SalDisplay* pSalDisp,
                                            int nScreen, long nDepth, 
									  		const SalTwoRect& rTwoRect ) const;

    SAL_DLLPRIVATE ImplSalDDB* ImplGetDDB( Drawable, int nScreen, long nDrawableDepth,
                                           const SalTwoRect& ) const;
	void    ImplDraw( Drawable aDrawable, int nScreen, long nDrawableDepth, 
                      const SalTwoRect& rTwoRect, const GC& rGC ) const;
						
public:					
								
    X11SalBitmap();
    virtual ~X11SalBitmap();

    // overload pure virtual methods
	virtual bool			Create( const Size& rSize, 
                            USHORT nBitCount, 
							const BitmapPalette& rPal );
	virtual bool			Create( const SalBitmap& rSalBmp );
	virtual bool			Create( const SalBitmap& rSalBmp, 
                                    SalGraphics* pGraphics );
	virtual bool			Create( const SalBitmap& rSalBmp,
                                    USHORT nNewBitCount );
						
	virtual void			Destroy();
						
	virtual Size			GetSize() const;
	virtual USHORT			GetBitCount() const;
						
	virtual BitmapBuffer*	AcquireBuffer( bool bReadOnly );
	virtual void			ReleaseBuffer( BitmapBuffer* pBuffer, bool bReadOnly );
    virtual bool            GetSystemData( BitmapSystemData& rData );
};

// --------------
// - ImplSalDDB -
// --------------

class ImplSalDDB
{
private:

	Pixmap			maPixmap;
	SalTwoRect		maTwoRect;
	long			mnDepth;
    int             mnScreen;

					ImplSalDDB() {}

    static void	ImplDraw( Drawable aSrcDrawable, long nSrcDrawableDepth,
                          Drawable aDstDrawable, long nDstDrawableDepth,
                          long nSrcX, long nSrcY, 
                          long nDestWidth, long nDestHeight, 
                          long nDestX, long nDestY, const GC& rGC );
					
public:				
					
					ImplSalDDB( XImage* pImage,
                                Drawable aDrawable, int nScreen, 
								const SalTwoRect& rTwoRect );
					ImplSalDDB( Drawable aDrawable,
                                int nScreen,
                                long nDrawableDepth,
								long nX, long nY, long nWidth, long nHeight );
                    ImplSalDDB( Display* pDisplay,
                                XLIB_Window hWindow,
                                int nScreen,
                                XImage* pImage); 
					~ImplSalDDB();
					
	Pixmap			ImplGetPixmap() const { return maPixmap; }
	long			ImplGetWidth() const { return maTwoRect.mnDestWidth; }
	long			ImplGetHeight() const { return maTwoRect.mnDestHeight; }
	long			ImplGetDepth() const { return mnDepth; }
	ULONG			ImplGetMemSize() const { return( ( maTwoRect.mnDestWidth * maTwoRect.mnDestHeight * mnDepth ) >> 3 ); }
    int             ImplGetScreen() const { return mnScreen; }
					
	bool			ImplMatches( int nScreen, long nDepth, const SalTwoRect& rTwoRect ) const;
	void			ImplDraw( Drawable aDrawable, long nDrawableDepth, 
                              const SalTwoRect& rTwoRect, const GC& rGC ) const;
};

// ----------------------
// - ImplSalBitmapCache -
// ----------------------

class ImplSalBitmapCache
{
private:

	List			maBmpList;
	ULONG			mnTotalSize;

public:				
					
					ImplSalBitmapCache();
					~ImplSalBitmapCache();

	void			ImplAdd( X11SalBitmap* pBmp, ULONG nMemSize = 0UL, ULONG nFlags = 0UL );
	void			ImplRemove( X11SalBitmap* pBmp );
	void			ImplClear();
};

#endif // _SV_SALBMP_HXX












































