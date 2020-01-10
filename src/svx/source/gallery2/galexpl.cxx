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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"

#include <svtools/pathoptions.hxx>
#include <sfx2/viewfrm.hxx>
#include "gallery1.hxx"
#include "galtheme.hxx" 
#include "galbrws.hxx"
#include "gallery.hxx"

// -----------
// - Statics -
// -----------

static SfxListener aLockListener;

// -------------------
// - GalleryExplorer -
// -------------------

Gallery* GalleryExplorer::ImplGetGallery()
{
	static Gallery* pGallery = NULL;
   
    ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );

	if( !pGallery )
		pGallery = Gallery::GetGalleryInstance();

	return pGallery;
}

// ------------------------------------------------------------------------

GalleryExplorer* GalleryExplorer::GetGallery()
{
	static GalleryExplorer* pThis = NULL;

    ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );

	// only create a dummy object which can be accessed
	if( !pThis )
		pThis = new GalleryExplorer;

	return pThis;
}

// ------------------------------------------------------------------------

INetURLObject GalleryExplorer::GetURL() const
{
	return GALLERYBROWSER()->GetURL();
}

String GalleryExplorer::GetFilterName() const
{
	return GALLERYBROWSER()->GetFilterName();
}

// ------------------------------------------------------------------------

Graphic GalleryExplorer::GetGraphic() const
{
	return GALLERYBROWSER()->GetGraphic();
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::GetVCDrawModel( FmFormModel& rModel ) const
{
	return GALLERYBROWSER()->GetVCDrawModel( rModel );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::IsLinkage() const
{
	return GALLERYBROWSER()->IsLinkage();
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::FillThemeList( List& rThemeList )
{
	Gallery* pGal = ImplGetGallery();

	if( pGal )
	{
		for( ULONG i = 0, nCount = pGal->GetThemeCount(); i < nCount; i++ )
		{
			const GalleryThemeEntry* pEntry = pGal->GetThemeInfo( i );

			if( pEntry && !pEntry->IsReadOnly() && !pEntry->IsHidden() )
				rThemeList.Insert( new String( pEntry->GetThemeName() ), LIST_APPEND );
		}
	}

	return( rThemeList.Count() > 0 );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::FillObjList( const String& rThemeName, List& rObjList )
{
	Gallery* pGal = ImplGetGallery();

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{	
			for( ULONG i = 0, nCount = pTheme->GetObjectCount(); i < nCount; i++ )
				rObjList.Insert( new String( pTheme->GetObjectURL( i ).GetMainURL( INetURLObject::NO_DECODE ) ), LIST_APPEND );
			
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return( rObjList.Count() > 0 );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::FillObjList( ULONG nThemeId, List& rObjList )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? FillObjList( pGal->GetThemeName( nThemeId ), rObjList ) : FALSE );
}

// ------------------------------------------------------------------------

sal_Bool GalleryExplorer::FillObjListTitle( const sal_uInt32 nThemeId, std::vector< rtl::OUString >& rList )
{
	Gallery* pGal = ImplGetGallery();
	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( pGal->GetThemeName( nThemeId ), aListener );
		
        if( pTheme )
		{	
			for( ULONG i = 0, nCount = pTheme->GetObjectCount(); i < nCount; i++ )
			{
				SgaObject*	pObj = pTheme->AcquireObject( i );
				if ( pObj )
				{
					rtl::OUString aTitle( pObj->GetTitle() );
					rList.push_back( aTitle );
					pTheme->ReleaseObject( pObj );
				}
			}
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}
	return( rList.size() > 0 );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertURL( const String& rThemeName, const String& rURL )
{
	return InsertURL( rThemeName, rURL, SGA_FORMAT_ALL );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertURL( ULONG nThemeId, const String& rURL )
{
	return InsertURL( nThemeId, rURL, SGA_FORMAT_ALL );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertURL( const String& rThemeName, const String& rURL, const ULONG )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener   aListener;
		GalleryTheme* pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{	
			INetURLObject aURL( rURL );
			DBG_ASSERT( aURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );
			bRet = pTheme->InsertURL( aURL );
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return bRet;
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertURL( ULONG nThemeId, const String& rURL, const ULONG nSgaFormat )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? InsertURL( pGal->GetThemeName( nThemeId ), rURL, nSgaFormat ) : FALSE );
}

// ------------------------------------------------------------------------

ULONG GalleryExplorer::GetObjCount( const String& rThemeName )
{
	Gallery*	pGal = ImplGetGallery();
	ULONG		nRet = 0;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			nRet = pTheme->GetObjectCount();
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return nRet;
}

// ------------------------------------------------------------------------

ULONG GalleryExplorer::GetObjCount( ULONG nThemeId )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? GetObjCount( pGal->GetThemeName( nThemeId ) ) : FALSE );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::GetGraphicObj( const String& rThemeName, ULONG nPos,
									 Graphic* pGraphic, Bitmap* pThumb,
									 BOOL bProgress )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			if( pGraphic )
				bRet = bRet || pTheme->GetGraphic( nPos, *pGraphic, bProgress );
			
			if( pThumb )
				bRet = bRet || pTheme->GetThumb( nPos, *pThumb, bProgress );

			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return bRet;
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::GetGraphicObj( ULONG nThemeId, ULONG nPos,
									 Graphic* pGraphic, Bitmap* pThumb,
									 BOOL bProgress )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? GetGraphicObj( pGal->GetThemeName( nThemeId ), nPos, pGraphic, pThumb, bProgress ) : FALSE );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertGraphicObj( const String& rThemeName, const Graphic& rGraphic )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			bRet = pTheme->InsertGraphic( rGraphic );
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return bRet;
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertGraphicObj( ULONG nThemeId, const Graphic& rGraphic )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? InsertGraphicObj( pGal->GetThemeName( nThemeId ), rGraphic ) : FALSE );
}

// ------------------------------------------------------------------------

ULONG GalleryExplorer::GetSdrObjCount( const String& rThemeName )
{
	Gallery*	pGal = ImplGetGallery();
	ULONG		nRet = 0;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			for( ULONG i = 0, nCount = pTheme->GetObjectCount(); i < nCount; i++ )
				if( SGA_OBJ_SVDRAW == pTheme->GetObjectKind( i ) )
					nRet++;
			
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return nRet;
}

// ------------------------------------------------------------------------

ULONG GalleryExplorer::GetSdrObjCount( ULONG nThemeId  )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? GetSdrObjCount( pGal->GetThemeName( nThemeId ) ) : FALSE );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::GetSdrObj( const String& rThemeName, ULONG nSdrModelPos,
								 SdrModel* pModel, Bitmap* pThumb )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			for( ULONG i = 0, nCount = pTheme->GetObjectCount(), nActPos = 0; ( i < nCount ) && !bRet; i++ )
			{
				if( SGA_OBJ_SVDRAW == pTheme->GetObjectKind( i ) )
				{
					if( nActPos++ == nSdrModelPos )
					{
						if( pModel )
							bRet = bRet || pTheme->GetModel( i, *pModel, FALSE );

						if( pThumb )
							bRet = bRet || pTheme->GetThumb( i, *pThumb );
					}
				}
			}
			
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return bRet;
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::GetSdrObj( ULONG nThemeId, ULONG nSdrModelPos,
								 SdrModel* pModel, Bitmap* pThumb )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? GetSdrObj( pGal->GetThemeName( nThemeId ), nSdrModelPos, pModel, pThumb ) : FALSE );
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertSdrObj( const String& rThemeName, FmFormModel& rModel )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener     aListener;
        GalleryTheme*   pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
			bRet = pTheme->InsertModel( rModel );
			pGal->ReleaseTheme( pTheme, aListener );
		}
	}

	return bRet;
}

// ------------------------------------------------------------------------

BOOL GalleryExplorer::InsertSdrObj( ULONG nThemeId, FmFormModel& rModel )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? InsertSdrObj( pGal->GetThemeName( nThemeId ), rModel ) : FALSE );
}

// -----------------------------------------------------------------------------

BOOL GalleryExplorer::BeginLocking( const String& rThemeName )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
		GalleryTheme* pTheme = pGal->AcquireTheme( rThemeName, aLockListener );

		if( pTheme )
        {      
            pTheme->LockTheme();
			bRet = TRUE;
        }         
	}

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GalleryExplorer::BeginLocking( ULONG nThemeId )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? BeginLocking( pGal->GetThemeName( nThemeId ) ) : FALSE );
}

// -----------------------------------------------------------------------------

BOOL GalleryExplorer::EndLocking( const String& rThemeName )
{
	Gallery*	pGal = ImplGetGallery();
	BOOL		bRet = FALSE;

	if( pGal )
	{
        SfxListener   aListener;
		GalleryTheme* pTheme = pGal->AcquireTheme( rThemeName, aListener );

		if( pTheme )
		{
            const BOOL bReleaseLockedTheme = pTheme->UnlockTheme();
        
			// release acquired theme
			pGal->ReleaseTheme( pTheme, aListener );
            
            if( bReleaseLockedTheme )
            {
                // release locked theme
                pGal->ReleaseTheme( pTheme, aLockListener );
                bRet = TRUE;
            }
		}
	}

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GalleryExplorer::EndLocking( ULONG nThemeId )
{
	Gallery* pGal = ImplGetGallery();
	return( pGal ? EndLocking( pGal->GetThemeName( nThemeId ) ) : FALSE );
}

// -----------------------------------------------------------------------------

BOOL GalleryExplorer::DrawCentered( OutputDevice* pOut, const FmFormModel& rModel )
{
	return SgaObjectSvDraw::DrawCentered( pOut, rModel );
}
