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
#include "precompiled_sd.hxx"

#include <sal/types.h>
#include <sot/formats.hxx>
#include <sot/storage.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/urihelper.hxx>
#include <svx/svditer.hxx>
#include <sfx2/docfile.hxx>
#include <svx/svdoole2.hxx>
#include <vcl/svapp.hxx>
#include "cusshow.hxx"
#include <sfx2/childwin.hxx>

#include <sfx2/viewfrm.hxx>

#include "strmname.h"
#include "sdtreelb.hxx"
#include "DrawDocShell.hxx"
#include "drawdoc.hxx"
#include "sdpage.hxx"
#include "sdresid.hxx"
#include "navigatr.hxx"
#ifndef _SD_CFGID_HXX
#include "strings.hrc"
#endif
#include "res_bmp.hrc"

#include <com/sun/star/embed/XEmbedPersist.hpp>
#include <svtools/embedtransfer.hxx>
#include <ViewShell.hxx>
    
using namespace com::sun::star;

class SdPageObjsTLB::IconProvider
{
public:
    IconProvider (void);

    // Regular icons.
    Image maImgPage;
    Image maImgPageExcl;
    Image maImgPageObjsExcl;
    Image maImgPageObjs;
    Image maImgObjects;
    Image maImgGroup;

    // High contrast icons.
    Image maImgPageH;
    Image maImgPageExclH;
    Image maImgPageObjsExclH;
    Image maImgPageObjsH;
    Image maImgObjectsH;
    Image maImgGroupH;
};


BOOL SD_DLLPRIVATE SdPageObjsTLB::bIsInDrag = FALSE;

BOOL SdPageObjsTLB::IsInDrag()
{ 
	return bIsInDrag; 
}

sal_uInt32 SdPageObjsTLB::SdPageObjsTransferable::mnListBoxDropFormatId = SAL_MAX_UINT32;

// -----------------------------------------
// - SdPageObjsTLB::SdPageObjsTransferable -
// -----------------------------------------

SdPageObjsTLB::SdPageObjsTransferable::SdPageObjsTransferable( 
    SdPageObjsTLB& rParent, 
        const INetBookmark& rBookmark,
    ::sd::DrawDocShell& rDocShell,
    NavigatorDragType eDragType,
    const ::com::sun::star::uno::Any& rTreeListBoxData )
    : SdTransferable(rDocShell.GetDoc(), NULL, TRUE),
      mrParent( rParent ),
      maBookmark( rBookmark ),
      mrDocShell( rDocShell ),
      meDragType( eDragType ),
      maTreeListBoxData( rTreeListBoxData )
{
}	




SdPageObjsTLB::SdPageObjsTransferable::~SdPageObjsTransferable()
{
}

// -----------------------------------------------------------------------------

void SdPageObjsTLB::SdPageObjsTransferable::AddSupportedFormats()
{ 
    AddFormat(SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK);
    AddFormat(SOT_FORMATSTR_ID_TREELISTBOX);
    AddFormat(GetListBoxDropFormatId());
}

// -----------------------------------------------------------------------------

sal_Bool SdPageObjsTLB::SdPageObjsTransferable::GetData( const ::com::sun::star::datatransfer::DataFlavor& rFlavor )
{
	ULONG nFormatId = SotExchange::GetFormat( rFlavor );
    switch (nFormatId)
    {
        case SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK:
            SetINetBookmark( maBookmark, rFlavor );
            return sal_True;

        case SOT_FORMATSTR_ID_TREELISTBOX:
            SetAny(maTreeListBoxData, rFlavor);
            return sal_True;

        default:
            return sal_False;
    }
}

// -----------------------------------------------------------------------------

void SdPageObjsTLB::SdPageObjsTransferable::DragFinished( sal_Int8 nDropAction )
{
	mrParent.OnDragFinished( nDropAction );
}

// -----------------------------------------------------------------------------

::sd::DrawDocShell& SdPageObjsTLB::SdPageObjsTransferable::GetDocShell() const
{
    return mrDocShell;
}

// -----------------------------------------------------------------------------

NavigatorDragType SdPageObjsTLB::SdPageObjsTransferable::GetDragType() const
{
    return meDragType;
}

// -----------------------------------------------------------------------------

sal_Int64 SAL_CALL SdPageObjsTLB::SdPageObjsTransferable::getSomething( const ::com::sun::star::uno::Sequence< sal_Int8 >& rId ) throw( ::com::sun::star::uno::RuntimeException )
{
    sal_Int64 nRet;

    if( ( rId.getLength() == 16 ) &&
        ( 0 == rtl_compareMemory( getUnoTunnelId().getConstArray(), rId.getConstArray(), 16 ) ) )
    {
        nRet = (sal_Int64)(sal_IntPtr)this;
    }
    else
        nRet = SdTransferable::getSomething(rId);

	return nRet;
}

// -----------------------------------------------------------------------------

const ::com::sun::star::uno::Sequence< sal_Int8 >& SdPageObjsTLB::SdPageObjsTransferable::getUnoTunnelId()
{
    static ::com::sun::star::uno::Sequence< sal_Int8 > aSeq;

	if( !aSeq.getLength() )
	{
		static osl::Mutex   aCreateMutex;
    	osl::MutexGuard     aGuard( aCreateMutex );

		aSeq.realloc( 16 );
    	rtl_createUuid( reinterpret_cast< sal_uInt8* >( aSeq.getArray() ), 0, sal_True );
	}

    return aSeq;
}

// -----------------------------------------------------------------------------

SdPageObjsTLB::SdPageObjsTransferable* SdPageObjsTLB::SdPageObjsTransferable::getImplementation( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >& rxData )
    throw()
{
    try
    {
	    ::com::sun::star::uno::Reference< ::com::sun::star::lang::XUnoTunnel > xUnoTunnel( rxData, ::com::sun::star::uno::UNO_QUERY_THROW );

		return reinterpret_cast<SdPageObjsTLB::SdPageObjsTransferable*>(
				sal::static_int_cast<sal_uIntPtr>(
					xUnoTunnel->getSomething( SdPageObjsTLB::SdPageObjsTransferable::getUnoTunnelId()) ) );
    }
    catch( const ::com::sun::star::uno::Exception& )
	{
	}
	return 0;
}


sal_uInt32 SdPageObjsTLB::SdPageObjsTransferable::GetListBoxDropFormatId (void)
{
    if (mnListBoxDropFormatId == SAL_MAX_UINT32)
        mnListBoxDropFormatId = SotExchange::RegisterFormatMimeType(
            ::rtl::OUString::createFromAscii(
                "application/x-openoffice-treelistbox-moveonly;"
                    "windows_formatname=\"SV_LBOX_DD_FORMAT_MOVE\""));
    return mnListBoxDropFormatId;
}

/*************************************************************************
|*
|* Ctor1 SdPageObjsTLB
|*
\************************************************************************/

SdPageObjsTLB::SdPageObjsTLB( Window* pParentWin, const SdResId& rSdResId ) 
:	SvTreeListBox       ( pParentWin, rSdResId )
,	mpParent 		    ( pParentWin )
,	mpDoc			    ( NULL )
,	mpBookmarkDoc	    ( NULL )
,	mpMedium 	    	( NULL )
,	mpOwnMedium		    ( NULL )
,	maImgOle             ( BitmapEx( SdResId( BMP_OLE ) ) )
,	maImgGraphic         ( BitmapEx( SdResId( BMP_GRAPHIC ) ) )
,	maImgOleH            ( BitmapEx( SdResId( BMP_OLE_H ) ) )
,	maImgGraphicH        ( BitmapEx( SdResId( BMP_GRAPHIC_H ) ) )
,	mbLinkableSelected  ( FALSE )
,	mpDropNavWin		( NULL )
,   mbShowAllShapes     ( false )
,   mbShowAllPages      ( false )

{
	// Tree-ListBox mit Linien versehen
	SetWindowBits( WinBits( WB_TABSTOP | WB_BORDER | WB_HASLINES |
							WB_HASBUTTONS | // WB_HASLINESATROOT |
							WB_HSCROLL | // #31562#
							WB_HASBUTTONSATROOT ) );
	SetNodeBitmaps( Bitmap( SdResId( BMP_EXPAND ) ),
					Bitmap( SdResId( BMP_COLLAPSE ) ) );

	SetNodeBitmaps( Bitmap( SdResId( BMP_EXPAND_H ) ),
					Bitmap( SdResId( BMP_COLLAPSE_H ) ),
					BMP_COLOR_HIGHCONTRAST );

    SetDragDropMode(
 		SV_DRAGDROP_CTRL_MOVE | SV_DRAGDROP_CTRL_COPY |
            SV_DRAGDROP_APP_MOVE  | SV_DRAGDROP_APP_COPY  | SV_DRAGDROP_APP_DROP );
}

/*************************************************************************
|*
|* Dtor SdPageObjsTLB
|*
\************************************************************************/

SdPageObjsTLB::~SdPageObjsTLB()
{
    if ( mpBookmarkDoc )
        CloseBookmarkDoc();
    else
        // no document was created from mpMedium, so this object is still the owner of it
        delete mpMedium;
}

/*************************************************************************
|*
|* return name of object
|*
\************************************************************************/

String SdPageObjsTLB::GetObjectName(
    const SdrObject* pObject,
    const bool bCreate) const
{
    String aRet;

    if ( pObject )
    {
        aRet = pObject->GetName();

        if( !aRet.Len() && pObject->ISA( SdrOle2Obj ) )
            aRet = static_cast< const SdrOle2Obj* >( pObject )->GetPersistName();
    }

    if (bCreate
        && mbShowAllShapes
        && aRet.Len() == 0
        && pObject!=NULL)
    {
        aRet = SdResId(STR_NAVIGATOR_SHAPE_BASE_NAME);
        aRet.SearchAndReplaceAscii("%1", String::CreateFromInt32(pObject->GetOrdNum() + 1));
    }

    return aRet;
}

/*************************************************************************
|*
|* In TreeLB Eintrag selektieren
|*
\************************************************************************/

BOOL SdPageObjsTLB::SelectEntry( const String& rName )
{
	BOOL bFound = FALSE;

	if( rName.Len() )
	{
		SvLBoxEntry* pEntry = NULL;
		String aTmp;

		for( pEntry = First(); pEntry && !bFound; pEntry = Next( pEntry ) )
		{
			aTmp = GetEntryText( pEntry );
			if( aTmp == rName )
			{
				bFound = TRUE;
				SetCurEntry( pEntry );
			}
		}
	}
	return( bFound );
}

/*************************************************************************
|*
|* Gibt zurueck, ob Childs des uebergebenen Strings selektiert sind
|*
\************************************************************************/

BOOL SdPageObjsTLB::HasSelectedChilds( const String& rName )
{
	BOOL bFound  = FALSE;
	BOOL bChilds = FALSE;

	if( rName.Len() )
	{
		SvLBoxEntry* pEntry = NULL;
		String aTmp;

		for( pEntry = First(); pEntry && !bFound; pEntry = Next( pEntry ) )
		{
			aTmp = GetEntryText( pEntry );
			if( aTmp == rName )
			{
				bFound = TRUE;
				BOOL bExpanded = IsExpanded( pEntry );
				long nCount = GetChildSelectionCount( pEntry );
				if( bExpanded && nCount > 0 )
					bChilds = TRUE;
			}
		}
	}
	return( bChilds );
}


/*************************************************************************
|*
|* TreeLB mit Seiten und Objekten fuellen
|*
\************************************************************************/

void SdPageObjsTLB::Fill( const SdDrawDocument* pInDoc, BOOL bAllPages,
						  const String& rDocName)
{
	String aSelection;
	if( GetSelectionCount() > 0 )
	{
		aSelection = GetSelectEntry();
		Clear();
	}

	mpDoc = pInDoc;
	maDocName = rDocName;
    mbShowAllPages = (bAllPages == TRUE);
	mpMedium = NULL;

	SdPage* 	 pPage = NULL;

    IconProvider aIconProvider;

	// first insert all pages including objects
	USHORT nPage = 0;
	const USHORT nMaxPages = mpDoc->GetPageCount();

	while( nPage < nMaxPages )
	{
		pPage = (SdPage*) mpDoc->GetPage( nPage );
		if(  (mbShowAllPages || pPage->GetPageKind() == PK_STANDARD)
		     && !(pPage->GetPageKind()==PK_HANDOUT)   ) //#94954# never list the normal handout page ( handout-masterpage is used instead )
		{
			BOOL bPageExluded = pPage->IsExcluded();

            bool bPageBelongsToShow = PageBelongsToCurrentShow (pPage);
            bPageExluded |= !bPageBelongsToShow;

            AddShapeList(*pPage, NULL, pPage->GetName(), bPageExluded, NULL, aIconProvider);
		}
		nPage++;
	}

	// dann alle MasterPages incl. Objekte einfuegen
	if( mbShowAllPages )
	{
		nPage = 0;
		const USHORT nMaxMasterPages = mpDoc->GetMasterPageCount();

		while( nPage < nMaxMasterPages )
		{
			pPage = (SdPage*) mpDoc->GetMasterPage( nPage );
            AddShapeList(*pPage, NULL, pPage->GetName(), false, NULL, aIconProvider);
			nPage++;
		}
	}
	if( aSelection.Len() )
		SelectEntry( aSelection );
}

/*************************************************************************
|*
|* Es wird nur der erste Eintrag eingefuegt. Childs werden OnDemand erzeugt
|*
\************************************************************************/

void SdPageObjsTLB::Fill( const SdDrawDocument* pInDoc, SfxMedium* pInMedium,
						  const String& rDocName )
{
	mpDoc = pInDoc;

    // this object now owns the Medium
	mpMedium = pInMedium;
	maDocName = rDocName;

	Image aImgDocOpen=Image( BitmapEx( SdResId( BMP_DOC_OPEN ) ) );
	Image aImgDocClosed=Image( BitmapEx( SdResId( BMP_DOC_CLOSED ) ) );
	Image aImgDocOpenH=Image( BitmapEx( SdResId( BMP_DOC_OPEN_H ) ) );
	Image aImgDocClosedH=Image( BitmapEx( SdResId( BMP_DOC_CLOSED_H ) ) );

	// Dokumentnamen einfuegen
	SvLBoxEntry* pFileEntry = InsertEntry( maDocName,
	                          aImgDocOpen,
	                          aImgDocClosed,
	                          NULL,
	                          TRUE,
							  LIST_APPEND,
							  reinterpret_cast< void* >( 1 ) );

	SetExpandedEntryBmp( pFileEntry, aImgDocOpenH, BMP_COLOR_HIGHCONTRAST );
	SetCollapsedEntryBmp( pFileEntry, aImgDocClosedH, BMP_COLOR_HIGHCONTRAST );
}




void SdPageObjsTLB::AddShapeList (
    const SdrObjList& rList,
    SdrObject* pShape,
    const ::rtl::OUString& rsName,
    const bool bIsExcluded,
    SvLBoxEntry* pParentEntry,
    const IconProvider& rIconProvider)
{
    Image aIcon (rIconProvider.maImgPage);
    if (bIsExcluded)
        aIcon = rIconProvider.maImgPageExcl;
    else if (pShape != NULL)
        aIcon = rIconProvider.maImgGroup;

    void* pUserData (reinterpret_cast<void*>(1));
    if (pShape != NULL)
        pUserData = pShape;

	SvLBoxEntry* pEntry = InsertEntry(
        rsName,
        aIcon,
        aIcon,
        pParentEntry,
        FALSE,
        LIST_APPEND,
        pUserData);

    SetExpandedEntryBmp(
        pEntry,
        bIsExcluded ? rIconProvider.maImgPageExclH : rIconProvider.maImgPageH,
        BMP_COLOR_HIGHCONTRAST );
    SetCollapsedEntryBmp(
        pEntry,
        bIsExcluded ? rIconProvider.maImgPageExclH : rIconProvider.maImgPageH,
        BMP_COLOR_HIGHCONTRAST );

    SdrObjListIter aIter(
        rList,
        !rList.HasObjectNavigationOrder() /* use navigation order, if available */,
        IM_FLAT,
        FALSE /*not reverse*/);

    while( aIter.IsMore() )
    {
        SdrObject* pObj = aIter.Next();
        OSL_ASSERT(pObj!=NULL);

        // Get the shape name.
        String aStr (GetObjectName( pObj ) );

        if( aStr.Len() )
        {
            if( pObj->GetObjInventor() == SdrInventor && pObj->GetObjIdentifier() == OBJ_OLE2 )
            {
                SvLBoxEntry* pNewEntry = InsertEntry( aStr, maImgOle, maImgOle, pEntry,
                    FALSE, LIST_APPEND, pObj);

                SetExpandedEntryBmp( pNewEntry, maImgOleH, BMP_COLOR_HIGHCONTRAST );
                SetCollapsedEntryBmp( pNewEntry, maImgOleH, BMP_COLOR_HIGHCONTRAST );
            }
            else if( pObj->GetObjInventor() == SdrInventor && pObj->GetObjIdentifier() == OBJ_GRAF )
            {
                SvLBoxEntry* pNewEntry = InsertEntry( aStr, maImgGraphic, maImgGraphic, pEntry,
                    FALSE, LIST_APPEND, pObj );

                SetExpandedEntryBmp( pNewEntry, maImgGraphicH, BMP_COLOR_HIGHCONTRAST );
                SetCollapsedEntryBmp( pNewEntry, maImgGraphicH, BMP_COLOR_HIGHCONTRAST );
            }
            else if (pObj->IsGroupObject())
            {
                AddShapeList(
                    *pObj->GetSubList(),
                    pObj,
                    aStr,
                    false,
                    pEntry,
                    rIconProvider);
            }
            else
            {
                SvLBoxEntry* pNewEntry = InsertEntry( aStr, rIconProvider.maImgObjects, rIconProvider.maImgObjects, pEntry,
                    FALSE, LIST_APPEND, pObj );

                SetExpandedEntryBmp( pNewEntry, rIconProvider.maImgObjectsH, BMP_COLOR_HIGHCONTRAST );
                SetCollapsedEntryBmp( pNewEntry, rIconProvider.maImgObjectsH, BMP_COLOR_HIGHCONTRAST );
            }
        }
    }

    if( pEntry->HasChilds() )
    {
        SetExpandedEntryBmp(
            pEntry,
            bIsExcluded ? rIconProvider.maImgPageObjsExcl : rIconProvider.maImgPageObjs);
        SetCollapsedEntryBmp(
            pEntry,
            bIsExcluded ? rIconProvider.maImgPageObjsExcl : rIconProvider.maImgPageObjs);
        SetExpandedEntryBmp(
            pEntry,
            bIsExcluded ? rIconProvider.maImgPageObjsExclH : rIconProvider.maImgPageObjsH,
            BMP_COLOR_HIGHCONTRAST);
        SetCollapsedEntryBmp(
            pEntry,
            bIsExcluded ? rIconProvider.maImgPageObjsExclH : rIconProvider.maImgPageObjsH,
            BMP_COLOR_HIGHCONTRAST);
    }
}




void SdPageObjsTLB::SetShowAllShapes (
    const bool bShowAllShapes,
    const bool bFillList)
{
    mbShowAllShapes = bShowAllShapes;
    if (bFillList)
    {
        if (mpMedium == NULL)
            Fill(mpDoc, mbShowAllPages, maDocName);
        else
            Fill(mpDoc, mpMedium, maDocName);
    }
}




bool SdPageObjsTLB::GetShowAllShapes (void) const
{
    return mbShowAllShapes;
}




/*************************************************************************
|*
|* Prueft, ob die Seiten (PK_STANDARD) und die darauf befindlichen Objekte
|* des Docs und der TreeLB identisch sind.
|* Wird ein Doc uebergeben, wird dieses zum aktuellem Doc (Wichtig bei
|* mehreren Documenten).
|*
\************************************************************************/

BOOL SdPageObjsTLB::IsEqualToDoc( const SdDrawDocument* pInDoc )
{
	if( pInDoc )
		mpDoc = pInDoc;

	if( !mpDoc )
		return( FALSE );

	SdrObject*	 pObj = NULL;
	SdPage* 	 pPage = NULL;
	SvLBoxEntry* pEntry = First();
	String		 aName;

	// Alle Pages incl. Objekte vergleichen
	USHORT nPage = 0;
	const USHORT nMaxPages = mpDoc->GetPageCount();

	while( nPage < nMaxPages )
	{
		pPage = (SdPage*) mpDoc->GetPage( nPage );
		if( pPage->GetPageKind() == PK_STANDARD )
		{
			if( !pEntry )
				return( FALSE );
			aName = GetEntryText( pEntry );

			if( pPage->GetName() != aName )
				return( FALSE );

			pEntry = Next( pEntry );

			SdrObjListIter aIter(
                *pPage,
                !pPage->HasObjectNavigationOrder() /* use navigation order, if available */,
                IM_DEEPWITHGROUPS );

			while( aIter.IsMore() )
			{
				pObj = aIter.Next();

                const String aObjectName( GetObjectName( pObj ) );

				if( aObjectName.Len() )
				{
					if( !pEntry )
						return( FALSE );

                	aName = GetEntryText( pEntry );

					if( aObjectName != aName )
						return( FALSE );

					pEntry = Next( pEntry );
				}
			}
		}
		nPage++;
	}
	// Wenn noch Eintraege in der Listbox vorhanden sind, wurden
	// Objekte (mit Namen) oder Seiten geloescht
	return( !pEntry );
}

/*************************************************************************
|*
|* Selectierten String zurueckgeben
|*
\************************************************************************/

String SdPageObjsTLB::GetSelectEntry()
{
	return( GetEntryText( GetCurEntry() ) );
}

/*************************************************************************
|*
|* Selektierte Eintrage zurueckgeben
|* nDepth == 0 -> Seiten
|* nDepth == 1 -> Objekte
|*
\************************************************************************/

List* SdPageObjsTLB::GetSelectEntryList( USHORT nDepth )
{
	List*		 pList	= NULL;
	SvLBoxEntry* pEntry = FirstSelected();

	while( pEntry )
	{
		USHORT nListDepth = GetModel()->GetDepth( pEntry );
		if( nListDepth == nDepth )
		{
			if( !pList )
				pList = new List();

			const String aEntryText( GetEntryText( pEntry ) );
			pList->Insert( new String( aEntryText ), LIST_APPEND );
		}
		pEntry = NextSelected( pEntry );
	}

	return( pList );
}

/*************************************************************************
|*
|* Alle Pages (und Objekte) des Docs zurueckgeben
|* nType == 0 -> Seiten
|* nType == 1 -> Objekte
|*
\************************************************************************/

List* SdPageObjsTLB::GetBookmarkList( USHORT nType )
{
	List* pList	= NULL;

	if( GetBookmarkDoc() )
	{
		SdPage* 	 pPage = NULL;
		String*		 pName = NULL;
		USHORT 		 nPage = 0;
		const USHORT nMaxPages = mpBookmarkDoc->GetSdPageCount( PK_STANDARD );

		while( nPage < nMaxPages )
		{
			pPage = mpBookmarkDoc->GetSdPage( nPage, PK_STANDARD );

			if( nType == 0 ) // Seitennamen einfuegen
			{
				if( !pList )
					pList = new List();

				pName = new String( pPage->GetRealName() );
				pList->Insert( pName, LIST_APPEND );
			}
			else // Objektnamen einfuegen
			{
				// Ueber Objekte der Seite iterieren
				SdrObjListIter aIter( *pPage, IM_DEEPWITHGROUPS );
				while( aIter.IsMore() )
				{
					SdrObject* pObj = aIter.Next();
					String aStr( GetObjectName( pObj ) );
					if( aStr.Len() )
					{
						if( !pList )
							pList = new List();

						pName = new String( aStr );
						pList->Insert( pName, LIST_APPEND );
					}
				}
			}
			nPage++;
		}
	}
	return( pList );
}

/*************************************************************************
|*
|* Eintraege werden erst auf Anforderung (Doppelklick) eingefuegt
|*
\************************************************************************/

void SdPageObjsTLB::RequestingChilds( SvLBoxEntry* pFileEntry )
{
	if( !pFileEntry->HasChilds() )
	{
		if( GetBookmarkDoc() )
		{
			SdrObject*	 pObj = NULL;
			SdPage* 	 pPage = NULL;
			SvLBoxEntry* pPageEntry = NULL;

			Image aImgPage=Image( BitmapEx( SdResId( BMP_PAGE ) ) );
			Image aImgPageObjs=Image( BitmapEx( SdResId( BMP_PAGEOBJS ) ) );
			Image aImgObjects=Image( BitmapEx( SdResId( BMP_OBJECTS ) ) );
			Image aImgPageH=Image( BitmapEx( SdResId( BMP_PAGE_H ) ) );
			Image aImgPageObjsH=Image( BitmapEx( SdResId( BMP_PAGEOBJS_H ) ) );
			Image aImgObjectsH=Image( BitmapEx( SdResId( BMP_OBJECTS_H ) ) );

			// document name already inserted

			// only insert all "normal" ? slides with objects
			USHORT nPage = 0;
			const USHORT nMaxPages = mpBookmarkDoc->GetPageCount();

			while( nPage < nMaxPages )
			{
				pPage = (SdPage*) mpBookmarkDoc->GetPage( nPage );
				if( pPage->GetPageKind() == PK_STANDARD )
				{
					pPageEntry = InsertEntry( pPage->GetName(),
					                          aImgPage,
					                          aImgPage,
					                          pFileEntry,
							                  FALSE,
								              LIST_APPEND,
								              reinterpret_cast< void* >( 1 ) );

					SetExpandedEntryBmp( pPageEntry, aImgPageH, BMP_COLOR_HIGHCONTRAST );
					SetCollapsedEntryBmp( pPageEntry, aImgPageH, BMP_COLOR_HIGHCONTRAST );

					SdrObjListIter aIter( *pPage, IM_DEEPWITHGROUPS );

					while( aIter.IsMore() )
					{
						pObj = aIter.Next();
						String aStr( GetObjectName( pObj ) );
						if( aStr.Len() )
						{
							if( pObj->GetObjInventor() == SdrInventor && pObj->GetObjIdentifier() == OBJ_OLE2 )
							{
								SvLBoxEntry* pNewEntry = InsertEntry(aStr, maImgOle, maImgOle, pPageEntry);


								SetExpandedEntryBmp( pNewEntry, maImgOleH, BMP_COLOR_HIGHCONTRAST );
								SetCollapsedEntryBmp( pNewEntry, maImgOleH, BMP_COLOR_HIGHCONTRAST );
							}
							else if( pObj->GetObjInventor() == SdrInventor && pObj->GetObjIdentifier() == OBJ_GRAF )
							{
								SvLBoxEntry* pNewEntry = InsertEntry(aStr, maImgGraphic, maImgGraphic, pPageEntry);

								SetExpandedEntryBmp( pNewEntry, maImgGraphicH, BMP_COLOR_HIGHCONTRAST );
								SetCollapsedEntryBmp( pNewEntry, maImgGraphicH, BMP_COLOR_HIGHCONTRAST );
							}
							else
							{
								SvLBoxEntry* pNewEntry = InsertEntry(aStr, aImgObjects, aImgObjects, pPageEntry);

								SetExpandedEntryBmp( pNewEntry, aImgObjectsH, BMP_COLOR_HIGHCONTRAST );
								SetCollapsedEntryBmp( pNewEntry, aImgObjectsH, BMP_COLOR_HIGHCONTRAST );
							}
						}
					}
					if( pPageEntry->HasChilds() )
					{
						SetExpandedEntryBmp( pPageEntry, aImgPageObjs );
						SetCollapsedEntryBmp( pPageEntry, aImgPageObjs );
						SetExpandedEntryBmp( pPageEntry, aImgPageObjsH, BMP_COLOR_HIGHCONTRAST );
						SetCollapsedEntryBmp( pPageEntry, aImgPageObjsH, BMP_COLOR_HIGHCONTRAST );
					}
				}
				nPage++;
			}
		}
	}
	else
		SvTreeListBox::RequestingChilds( pFileEntry );
}

/*************************************************************************
|*
|*	Prueft, ob es sich um eine Draw-Datei handelt und oeffnet anhand des
|*	uebergebenen Docs das BookmarkDoc
|*
\************************************************************************/

SdDrawDocument* SdPageObjsTLB::GetBookmarkDoc(SfxMedium* pMed)
{
	if (
       !mpBookmarkDoc ||
		 (pMed && (!mpOwnMedium || mpOwnMedium->GetName() != pMed->GetName()))
      )
	{
        // create a new BookmarkDoc if now one exists or if a new Medium is provided
        if (mpOwnMedium != pMed)
		{
			CloseBookmarkDoc();
		}

		if (pMed)
		{
            // it looks that it is undefined if a Medium was set by Fill() allready
            DBG_ASSERT( !mpMedium, "SfxMedium confusion!" );
            delete mpMedium;
            mpMedium = NULL;

            // take over this Medium (currently used only be Navigator)
			mpOwnMedium = pMed;
        }

        DBG_ASSERT( mpMedium || pMed, "No SfxMedium provided!" );

        if( pMed )
        {
            // in this mode the document is also owned and controlled by this instance
            mxBookmarkDocShRef = new ::sd::DrawDocShell(SFX_CREATE_MODE_STANDARD, TRUE);
            if (mxBookmarkDocShRef->DoLoad(pMed))
                mpBookmarkDoc = mxBookmarkDocShRef->GetDoc();
            else
                mpBookmarkDoc = NULL;
        }
        else if ( mpMedium )
            // in this mode the document is owned and controlled by the SdDrawDocument
            // it can be released by calling the corresponding CloseBookmarkDoc method
            // successfull creation of a document makes this the owner of the medium
            mpBookmarkDoc = ((SdDrawDocument*) mpDoc)->OpenBookmarkDoc(*mpMedium);

        if ( !mpBookmarkDoc )
		{
			ErrorBox aErrorBox( this, WB_OK, String( SdResId( STR_READ_DATA_ERROR ) ) );
			aErrorBox.Execute();
            mpMedium = 0; //On failure the SfxMedium is invalid
		}
	}

	return( mpBookmarkDoc );
}

/*************************************************************************
|*
|* Bookmark-Dokument schlieŠen und loeschen
|*
\************************************************************************/

void SdPageObjsTLB::CloseBookmarkDoc()
{
	if (mxBookmarkDocShRef.Is())
	{
		mxBookmarkDocShRef->DoClose();
        mxBookmarkDocShRef.Clear();

        // Medium is owned by document, so it's destroyed already
        mpOwnMedium = 0;
	}
    else if ( mpBookmarkDoc )
	{
        DBG_ASSERT( !mpOwnMedium, "SfxMedium confusion!" );
        if ( mpDoc )
        {
            // The document owns the Medium, so the Medium will be invalid after closing the document
			((SdDrawDocument*) mpDoc)->CloseBookmarkDoc();
            mpMedium = 0;
        }
	}
    else
    {
        // perhaps mpOwnMedium provided, but no successfull creation of BookmarkDoc
        delete mpOwnMedium;
        mpOwnMedium = NULL;
    }

	mpBookmarkDoc = NULL;
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

void SdPageObjsTLB::SelectHdl()
{
	SvLBoxEntry* pEntry = FirstSelected();

	mbLinkableSelected = TRUE;

	while( pEntry && mbLinkableSelected )
	{
	    if( NULL == pEntry->GetUserData() )
	        mbLinkableSelected = FALSE;

		pEntry = NextSelected( pEntry );
	}

	SvTreeListBox::SelectHdl();
}

/*************************************************************************
|*
|* Ueberlaedt RETURN mit der Funktionsweise von DoubleClick
|*
\************************************************************************/

void SdPageObjsTLB::KeyInput( const KeyEvent& rKEvt )
{
	if( rKEvt.GetKeyCode().GetCode() == KEY_RETURN )
	{
		// Auskommentierter Code aus svtools/source/contnr/svimpbox.cxx
		SvLBoxEntry* pCursor = GetCurEntry();
		if( pCursor->HasChilds() || pCursor->HasChildsOnDemand() )
		{
			if( IsExpanded( pCursor ) )
				Collapse( pCursor );
			else
				Expand( pCursor );
		}

		DoubleClickHdl();
	}
	else
		SvTreeListBox::KeyInput( rKEvt );
}

/*************************************************************************
|*
|* StartDrag-Request
|*
\************************************************************************/

void SdPageObjsTLB::StartDrag( sal_Int8 nAction, const Point& rPosPixel)
{
    (void)nAction;
    (void)rPosPixel;
    
	SdNavigatorWin* pNavWin = NULL;
    SvLBoxEntry* pEntry = GetEntry(rPosPixel);
    
	if( mpFrame->HasChildWindow( SID_NAVIGATOR ) )
		pNavWin = (SdNavigatorWin*) ( mpFrame->GetChildWindow( SID_NAVIGATOR )->GetContextWindow( SD_MOD() ) );

	if (pEntry != NULL
        && pNavWin !=NULL
        && pNavWin == mpParent
        && pNavWin->GetNavigatorDragType() != NAVIGATOR_DRAGTYPE_NONE )
	{
        // Mark only the children of the page under the mouse as drop
        // targets.  This prevents moving shapes from one page to another.

        // Select all entries and disable them as drop targets.
        SetSelectionMode(MULTIPLE_SELECTION);
        SetCursor(NULL, FALSE);
        SelectAll(TRUE, FALSE);
        EnableSelectionAsDropTarget(FALSE, TRUE);

        // Enable only the entries as drop targets that are children of the
        // page under the mouse.
        SvLBoxEntry* pParent = GetRootLevelParent(pEntry);
        if (pParent != NULL)
        {
            SelectAll(FALSE, FALSE);
            Select(pParent, TRUE);
            //            for (SvLBoxEntry*pChild=FirstChild(pParent); pChild!=NULL; pChild=NextSibling(pChild))
            //                Select(pChild, TRUE);
            EnableSelectionAsDropTarget(TRUE, TRUE);//FALSE);
        }

        // Set selection back to the entry under the mouse.
        SelectAll(FALSE,FALSE);
        SetSelectionMode(SINGLE_SELECTION);
        Select(pEntry, TRUE);

		//  Aus dem ExecuteDrag heraus kann der Navigator geloescht werden
		//  (beim Umschalten auf einen anderen Dokument-Typ), das wuerde aber
		//  den StarView MouseMove-Handler, der Command() aufruft, umbringen.
		//  Deshalb Drag&Drop asynchron:
        Application::PostUserEvent( STATIC_LINK( this, SdPageObjsTLB, ExecDragHdl ) );
	}
}

/*************************************************************************
|*
|* Begin drag
|*
\************************************************************************/

void SdPageObjsTLB::DoDrag()
{
	mpDropNavWin = ( mpFrame->HasChildWindow( SID_NAVIGATOR ) ) ?
				  (SdNavigatorWin*)( mpFrame->GetChildWindow( SID_NAVIGATOR )->GetContextWindow( SD_MOD() ) ) :
				  NULL;

	if( mpDropNavWin )
	{
		::sd::DrawDocShell* pDocShell = mpDoc->GetDocSh();
        String aURL = INetURLObject( pDocShell->GetMedium()->GetPhysicalName(), INET_PROT_FILE ).GetMainURL( INetURLObject::NO_DECODE );
		NavigatorDragType	eDragType = mpDropNavWin->GetNavigatorDragType();

		aURL.Append( '#' );
		aURL.Append( GetSelectEntry() );

		INetBookmark	aBookmark( aURL, GetSelectEntry() );
		sal_Int8		nDNDActions = DND_ACTION_COPYMOVE;

		if( eDragType == NAVIGATOR_DRAGTYPE_LINK )
			nDNDActions = DND_ACTION_LINK;	// #93240# Either COPY *or* LINK, never both!

		SvTreeListBox::ReleaseMouse();

		bIsInDrag = TRUE;

        SvLBoxDDInfo aDDInfo;
        memset(&aDDInfo,0,sizeof(SvLBoxDDInfo));
        aDDInfo.pApp = GetpApp();
        aDDInfo.pSource = this;
        //            aDDInfo.pDDStartEntry = pEntry;
		::com::sun::star::uno::Sequence<sal_Int8> aSequence (sizeof(SvLBoxDDInfo));
		memcpy(aSequence.getArray(), (sal_Char*)&aDDInfo, sizeof(SvLBoxDDInfo));
		::com::sun::star::uno::Any aTreeListBoxData (aSequence);
        
		// object is destroyed by internal reference mechanism
		SdTransferable* pTransferable = new SdPageObjsTLB::SdPageObjsTransferable(
            *this, aBookmark, *pDocShell, eDragType, aTreeListBoxData);
        OSL_TRACE("created new SdPageObjsTransferable at %x", pTransferable);

        // Get the view.
        sd::View* pView = NULL;
        if (pDocShell != NULL)
        {
            ::sd::ViewShell* pViewShell = pDocShell->GetViewShell();
            if (pViewShell != NULL)
                pView = pViewShell->GetView();
        }
        if (pView == NULL)
        {
            OSL_ASSERT(pView!=NULL);
            return;
        }

        SdrObject* pObject = NULL;
        void* pUserData = GetCurEntry()->GetUserData();
        if (pUserData != NULL && pUserData != (void*)1)
            pObject = reinterpret_cast<SdrObject*>(pUserData);
        if (pObject == NULL)
            return;

        // For shapes without a user supplied name (the automatically
        // created name does not count), a different drag and drop technique
        // is used.
        if (GetObjectName(pObject, false).Len() == 0)
        {
            AddShapeToTransferable(*pTransferable, *pObject);
            pTransferable->SetView(pView);
            SD_MOD()->pTransferDrag = pTransferable;
        }

        // Unnamed shapes have to be selected to be recognized by the
        // current drop implementation.  In order to have a consistent
        // behaviour for all shapes, every shape that is to be dragged is
        // selected first.
        SdrPageView* pPageView = pView->GetSdrPageView();
        pView->UnmarkAllObj(pPageView);
        pView->MarkObj(pObject, pPageView);

        pTransferable->StartDrag( this, nDNDActions );
	}
}

/*************************************************************************
|*
|* Drag finished
|*
\************************************************************************/

void SdPageObjsTLB::OnDragFinished( sal_uInt8 )
{
	if( mpFrame->HasChildWindow( SID_NAVIGATOR ) )
	{
		SdNavigatorWin* pNewNavWin = (SdNavigatorWin*) ( mpFrame->GetChildWindow( SID_NAVIGATOR )->GetContextWindow( SD_MOD() ) );

		if( mpDropNavWin == pNewNavWin)
		{
			MouseEvent aMEvt( mpDropNavWin->GetPointerPosPixel() );
			SvTreeListBox::MouseButtonUp( aMEvt );
		}
	}

	mpDropNavWin = NULL;
	bIsInDrag = FALSE;
}

/*************************************************************************
|*
|* AcceptDrop-Event
|*
\************************************************************************/

sal_Int8 SdPageObjsTLB::AcceptDrop (const AcceptDropEvent& rEvent)
{
    sal_Int8 nResult (DND_ACTION_NONE);
    
	if ( !bIsInDrag && IsDropFormatSupported( FORMAT_FILE ) )
    {
        nResult = rEvent.mnAction;
    }
    else
    {
        SvLBoxEntry* pEntry = GetDropTarget(rEvent.maPosPixel);
        if (rEvent.mbLeaving || !CheckDragAndDropMode( this, rEvent.mnAction ))
        {
            ImplShowTargetEmphasis( pTargetEntry, FALSE );
        }
        else if( !nDragDropMode )
        {
            DBG_ERRORFILE( "SdPageObjsTLB::AcceptDrop(): no target" );
        }
        else if (IsDropAllowed(pEntry))
        {
            nResult = DND_ACTION_MOVE;

            // Draw emphasis.
            if (pEntry != pTargetEntry || !(nImpFlags & SVLBOX_TARGEMPH_VIS))
            {
                ImplShowTargetEmphasis( pTargetEntry, FALSE );
                pTargetEntry = pEntry;
                ImplShowTargetEmphasis( pTargetEntry, TRUE );
            }
        }
    }
    
    // Hide emphasis when there is no valid drop action.
    if (nResult == DND_ACTION_NONE)
        ImplShowTargetEmphasis(pTargetEntry, FALSE);
    
    return nResult;
}

/*************************************************************************
|*
|* ExecuteDrop-Event
|*
\************************************************************************/

sal_Int8 SdPageObjsTLB::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
	sal_Int8 nRet = DND_ACTION_NONE;

    try
    {
        if( !bIsInDrag )
        {
            SdNavigatorWin* pNavWin = NULL;
            USHORT			nId = SID_NAVIGATOR;
            
            if( mpFrame->HasChildWindow( nId ) )
                pNavWin = (SdNavigatorWin*)( mpFrame->GetChildWindow( nId )->GetContextWindow( SD_MOD() ) );
            
            if( pNavWin && ( pNavWin == mpParent ) )
            {
                TransferableDataHelper	aDataHelper( rEvt.maDropEvent.Transferable );
                String					aFile;
                
                if( aDataHelper.GetString( FORMAT_FILE, aFile ) &&
                    ( (SdNavigatorWin*) mpParent)->InsertFile( aFile ) )
                {
                    nRet = rEvt.mnAction;
                }
            }
        }
    }
    catch (com::sun::star::uno::Exception&)
    {
        OSL_ASSERT(false);
    }

    if (nRet == DND_ACTION_NONE)
        SvTreeListBox::ExecuteDrop(rEvt, this);


	return nRet;
}

/*************************************************************************
|*
|* Handler fuers Dragging
|*
\************************************************************************/

IMPL_STATIC_LINK(SdPageObjsTLB, ExecDragHdl, void*, EMPTYARG)
{
	//	als Link, damit asynchron ohne ImpMouseMoveMsg auf dem Stack auch der
	//	Navigator geloescht werden darf
	pThis->DoDrag();
	return 0;
}


bool SdPageObjsTLB::PageBelongsToCurrentShow (const SdPage* pPage) const
{
    // Return <TRUE/> as default when there is no custom show or when none
    // is used.  The page does then belong to the standard show.
    bool bBelongsToShow = true;

    if (mpDoc->getPresentationSettings().mbCustomShow)
    {
        // Get the current custom show.
        SdCustomShow* pCustomShow = NULL;
        List* pShowList = const_cast<SdDrawDocument*>(mpDoc)->GetCustomShowList();
        if (pShowList != NULL)
        {
            ULONG nCurrentShowIndex = pShowList->GetCurPos();
            void* pObject = pShowList->GetObject(nCurrentShowIndex);
            pCustomShow = static_cast<SdCustomShow*>(pObject);
        }

        // Check whether the given page is part of that custom show.
        if (pCustomShow != NULL)
        {
            bBelongsToShow = false;
            ULONG nPageCount = pCustomShow->Count();
            for (USHORT i=0; i<nPageCount && !bBelongsToShow; i++)
                if (pPage == static_cast<SdPage*>(pCustomShow->GetObject (i)))
                    bBelongsToShow = true;
        }
    }

    return bBelongsToShow;
}




BOOL SdPageObjsTLB::NotifyMoving(
    SvLBoxEntry* pTarget,
    SvLBoxEntry* pEntry,
    SvLBoxEntry*& rpNewParent,
    ULONG& rNewChildPos)
{
    SvLBoxEntry* pDestination = pTarget;
    while (GetParent(pDestination) != NULL && GetParent(GetParent(pDestination)) != NULL)
        pDestination = GetParent(pDestination);

    SdrObject* pTargetObject = reinterpret_cast<SdrObject*>(pDestination->GetUserData());
    SdrObject* pSourceObject = reinterpret_cast<SdrObject*>(pEntry->GetUserData());
    if (pSourceObject == reinterpret_cast<SdrObject*>(1))
        pSourceObject = NULL;

    if (pTargetObject != NULL && pSourceObject != NULL)
    {
        SdrPage* pObjectList = pSourceObject->GetPage();
        if (pObjectList != NULL)
        {
            sal_uInt32 nNewPosition;
            if (pTargetObject == reinterpret_cast<SdrObject*>(1))
                nNewPosition = 0;
            else
                nNewPosition = pTargetObject->GetNavigationPosition() + 1;
            pObjectList->SetObjectNavigationPosition(*pSourceObject, nNewPosition);
        }

        // Update the tree list.
        if (pTarget == NULL)
        {
            rpNewParent = 0;
            rNewChildPos = 0;
            return TRUE;
        }
        else if (GetParent(pDestination) == NULL)
        {
            rpNewParent = pDestination;
            rNewChildPos = 0;
        }
        else
        {
            rpNewParent = GetParent(pDestination);
            rNewChildPos = pModel->GetRelPos(pDestination) + 1;
            rNewChildPos += nCurEntrySelPos;
            nCurEntrySelPos++;
        }
        return TRUE;
    }
    else
        return FALSE;
}




SvLBoxEntry* SdPageObjsTLB::GetDropTarget (const Point& rLocation)
{
    SvLBoxEntry* pEntry = SvTreeListBox::GetDropTarget(rLocation);
    if (pEntry == NULL)
        return NULL;

        OSL_TRACE("entry is %s", 
            ::rtl::OUStringToOString(GetEntryText(pEntry), RTL_TEXTENCODING_UTF8).getStr());
    if (GetParent(pEntry) == NULL)
    {
        // Use page entry as insertion position.
    }
    else
    {
        // Go to second hierarchy level, i.e. top level shapes,
        // i.e. children of pages.
        while (GetParent(pEntry) != NULL && GetParent(GetParent(pEntry)) != NULL)
            pEntry = GetParent(pEntry);

        // Advance to next sibling.
        SvLBoxEntry* pNext;
        sal_uInt16 nDepth (0);
        while (pEntry != NULL)
        {
            pNext = dynamic_cast<SvLBoxEntry*>(NextVisible(pEntry, &nDepth));
            if (pNext != NULL && nDepth > 0 && nDepth!=0xffff)
                pEntry = pNext;
            else
                break;
        }
        OSL_TRACE("returning %s", 
            ::rtl::OUStringToOString(GetEntryText(pEntry), RTL_TEXTENCODING_UTF8).getStr());
    }

    return pEntry;
}




bool SdPageObjsTLB::IsDropAllowed (SvLBoxEntry* pEntry)
{
    if (pEntry == NULL)
        return false;
    
    if ( ! IsDropFormatSupported(SdPageObjsTransferable::GetListBoxDropFormatId()))
        return false;

    if ((pEntry->GetFlags() & SV_ENTRYFLAG_DISABLE_DROP) != 0)
        return false;

    return true;
}




void SdPageObjsTLB::AddShapeToTransferable (
    SdTransferable& rTransferable,
    SdrObject& rObject) const
{
	TransferableObjectDescriptor aObjectDescriptor;
    bool bIsDescriptorFillingPending (true);

    const SdrOle2Obj* pOleObject = dynamic_cast<const SdrOle2Obj*>(&rObject);
    if (pOleObject != NULL && pOleObject->GetObjRef().is())
    {
        // If object has no persistence it must be copied as part of the document
        try
        {
            uno::Reference< embed::XEmbedPersist > xPersObj (pOleObject->GetObjRef(), uno::UNO_QUERY );
            if (xPersObj.is() && xPersObj->hasEntry())
            {
                SvEmbedTransferHelper::FillTransferableObjectDescriptor(
                    aObjectDescriptor,
                    pOleObject->GetObjRef(),
                    pOleObject->GetGraphic(),
                    pOleObject->GetAspect());
                bIsDescriptorFillingPending = false;
            }
        }
        catch( uno::Exception& )
        {
        }
	}

    ::sd::DrawDocShell* pDocShell = mpDoc->GetDocSh();
	if (bIsDescriptorFillingPending && pDocShell!=NULL)
    {
        bIsDescriptorFillingPending = false;
        pDocShell->FillTransferableObjectDescriptor(aObjectDescriptor);
    }

    Point aDragPos (rObject.GetCurrentBoundRect().Center());
    //Point aDragPos (0,0);
	aObjectDescriptor.maDragStartPos = aDragPos;
    //	aObjectDescriptor.maSize = GetAllMarkedRect().GetSize();
    if (pDocShell != NULL)
        aObjectDescriptor.maDisplayName = pDocShell->GetMedium()->GetURLObject().GetURLNoPass();
    else
        aObjectDescriptor.maDisplayName = String();
	aObjectDescriptor.mbCanLink = FALSE;

	rTransferable.SetStartPos(aDragPos);
	rTransferable.SetObjectDescriptor( aObjectDescriptor );
}




//===== IconProvider ==========================================================

SdPageObjsTLB::IconProvider::IconProvider (void)
    : maImgPage( BitmapEx( SdResId( BMP_PAGE ) ) ),
      maImgPageExcl( BitmapEx( SdResId( BMP_PAGE_EXCLUDED ) ) ),
      maImgPageObjsExcl( BitmapEx( SdResId( BMP_PAGEOBJS_EXCLUDED ) ) ),
      maImgPageObjs( BitmapEx( SdResId( BMP_PAGEOBJS ) ) ),
      maImgObjects( BitmapEx( SdResId( BMP_OBJECTS ) ) ),
      maImgGroup( BitmapEx( SdResId( BMP_GROUP ) ) ),

      maImgPageH( BitmapEx( SdResId( BMP_PAGE_H ) ) ),
      maImgPageExclH( BitmapEx( SdResId( BMP_PAGE_EXCLUDED_H ) ) ),
      maImgPageObjsExclH( BitmapEx( SdResId( BMP_PAGEOBJS_EXCLUDED_H ) ) ),
      maImgPageObjsH( BitmapEx( SdResId( BMP_PAGEOBJS_H ) ) ),
      maImgObjectsH( BitmapEx( SdResId( BMP_OBJECTS_H ) ) ),
      maImgGroupH( BitmapEx( SdResId( BMP_GROUP_H ) ) )
{
}
