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

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif

// include ---------------------------------------------------------------

#include "srchxtra.hxx"
#include <tools/rcid.h>
#include <vcl/msgbox.hxx>
#include <svtools/cjkoptions.hxx>
#include <svtools/whiter.hxx>
#include <sfx2/objsh.hxx>

#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#ifndef _SVX_SRCHXTRA_HRC
#include "srchxtra.hrc"
#endif
#ifndef _SVXITEMS_HRC
#include <svx/svxitems.hrc>
#endif

#ifndef _SVX_FLSTITEM_HXX

#include "flstitem.hxx"
#endif
#include "chardlg.hxx"
#include "paragrph.hxx"
#include <svx/dialmgr.hxx>
#include "backgrnd.hxx"

// class SvxSearchFormatDialog -------------------------------------------

SvxSearchFormatDialog::SvxSearchFormatDialog( Window* pParent, const SfxItemSet& rSet ) :

	SfxTabDialog( pParent, SVX_RES( RID_SVXDLG_SEARCHFORMAT ), &rSet ),

	pFontList( NULL )

{
	FreeResource();

	AddTabPage( RID_SVXPAGE_CHAR_NAME, SvxCharNamePage::Create, 0 );
	AddTabPage( RID_SVXPAGE_CHAR_EFFECTS, SvxCharEffectsPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_CHAR_POSITION, SvxCharPositionPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_CHAR_TWOLINES, SvxCharTwoLinesPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_STD_PARAGRAPH, SvxStdParagraphTabPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_ALIGN_PARAGRAPH, SvxParaAlignTabPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_EXT_PARAGRAPH, SvxExtParagraphTabPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_PARA_ASIAN, SvxAsianTabPage::Create, 0 );
	AddTabPage( RID_SVXPAGE_BACKGROUND, SvxBackgroundTabPage::Create, 0 );

	// remove asian tabpages if necessary
    SvtCJKOptions aCJKOptions;
    if ( !aCJKOptions.IsDoubleLinesEnabled() )
        RemoveTabPage( RID_SVXPAGE_CHAR_TWOLINES );
    if ( !aCJKOptions.IsAsianTypographyEnabled() )
		RemoveTabPage( RID_SVXPAGE_PARA_ASIAN );
}

// -----------------------------------------------------------------------

SvxSearchFormatDialog::~SvxSearchFormatDialog()
{
	delete pFontList;
}

// -----------------------------------------------------------------------

void SvxSearchFormatDialog::PageCreated( USHORT nId, SfxTabPage& rPage )
{
	switch ( nId )
	{
        case RID_SVXPAGE_CHAR_NAME:
		{
			const FontList*	pAppFontList = 0;
			SfxObjectShell* pSh = SfxObjectShell::Current();

			if ( pSh )
			{
				const SvxFontListItem* pFLItem = (const SvxFontListItem*)
					pSh->GetItem( SID_ATTR_CHAR_FONTLIST );
				if ( pFLItem )
					pAppFontList = pFLItem->GetFontList();
			}

			const FontList* pList = pAppFontList;

			if ( !pList )
			{
				if ( !pFontList )
					pFontList = new FontList( this );
				pList = pFontList;
			}

			if ( pList )
                ( (SvxCharNamePage&)rPage ).
                    SetFontList( SvxFontListItem( pList, SID_ATTR_CHAR_FONTLIST ) );
            ( (SvxCharNamePage&)rPage ).EnableSearchMode();
			break;
		}

		case RID_SVXPAGE_STD_PARAGRAPH:
			( (SvxStdParagraphTabPage&)rPage ).EnableAutoFirstLine();
			break;

		case RID_SVXPAGE_ALIGN_PARAGRAPH:
			( (SvxParaAlignTabPage&)rPage ).EnableJustifyExt();
			break;
		case RID_SVXPAGE_BACKGROUND :
			( (SvxBackgroundTabPage&)rPage ).ShowParaControl(TRUE);
			break;
	}
}

// class SvxSearchFormatDialog -------------------------------------------

SvxSearchAttributeDialog::SvxSearchAttributeDialog( Window* pParent,
													SearchAttrItemList& rLst,
													const USHORT* pWhRanges ) :

	ModalDialog( pParent, SVX_RES( RID_SVXDLG_SEARCHATTR )  ),

    aAttrFL ( this, SVX_RES( FL_ATTR ) ),
    aAttrLB ( this, SVX_RES( LB_ATTR ) ),
    aOKBtn  ( this, SVX_RES( BTN_ATTR_OK ) ),
	aEscBtn	( this, SVX_RES( BTN_ATTR_CANCEL ) ),
	aHelpBtn( this, SVX_RES( BTN_ATTR_HELP ) ),

	rList( rLst )

{
	FreeResource();

	aAttrLB.SetWindowBits( GetStyle() | WB_CLIPCHILDREN | WB_HSCROLL | WB_SORT );
	aAttrLB.GetModel()->SetSortMode( SortAscending );

	aOKBtn.SetClickHdl( LINK( this, SvxSearchAttributeDialog, OKHdl ) );

	SfxObjectShell* pSh = SfxObjectShell::Current();
	DBG_ASSERT( pSh, "No DocShell" );

	SfxItemPool& rPool = pSh->GetPool();
	SfxItemSet aSet( rPool, pWhRanges );
	SfxWhichIter aIter( aSet );
	USHORT nWhich = aIter.FirstWhich();

	while ( nWhich )
	{
		USHORT nSlot = rPool.GetSlotId( nWhich );
		if ( nSlot >= SID_SVX_START )
		{
			BOOL bChecked = FALSE, bFound = FALSE;
			for ( USHORT i = 0; !bFound && i < rList.Count(); ++i )
			{
				if ( nSlot == rList[i].nSlot )
				{
					bFound = TRUE;
					if ( IsInvalidItem( rList[i].pItem ) )
						bChecked = TRUE;
				}
			}

			USHORT nResId = nSlot - SID_SVX_START + RID_ATTR_BEGIN;
			SvLBoxEntry* pEntry = NULL;
    		ResId aId( nResId, DIALOG_MGR() );
    		aId.SetRT( RSC_STRING );
    		if ( DIALOG_MGR().IsAvailable( aId ) )
				pEntry = aAttrLB.SvTreeListBox::InsertEntry( SVX_RESSTR( nResId ) );
			else
			{
				ByteString sError( "no resource for slot id\nslot = " );
				sError += ByteString::CreateFromInt32( nSlot );
				sError += ByteString( "\nresid = " );
				sError += ByteString::CreateFromInt32( nResId );
				DBG_ERRORFILE( sError.GetBuffer() );
			}

			if ( pEntry )
			{
				aAttrLB.SetCheckButtonState( pEntry, bChecked ? SV_BUTTON_CHECKED : SV_BUTTON_UNCHECKED );
				pEntry->SetUserData( (void*)(ULONG)nSlot );
			}
		}
		nWhich = aIter.NextWhich();
	}

	aAttrLB.SetHighlightRange();
	aAttrLB.SelectEntryPos( 0 );
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchAttributeDialog, OKHdl, Button *, EMPTYARG )
{
	SearchAttrItem aInvalidItem;
	aInvalidItem.pItem = (SfxPoolItem*)-1;

	for ( USHORT i = 0; i < aAttrLB.GetEntryCount(); ++i )
	{
		USHORT nSlot = (USHORT)(ULONG)aAttrLB.GetEntryData(i);
		BOOL bChecked = aAttrLB.IsChecked(i);

		USHORT j;
		for ( j = rList.Count(); j; )
		{
			SearchAttrItem& rItem = rList[ --j ];
			if( rItem.nSlot == nSlot )
			{
				if( bChecked )
				{
					if( !IsInvalidItem( rItem.pItem ) )
						delete rItem.pItem;
					rItem.pItem = (SfxPoolItem*)-1;
				}
				else if( IsInvalidItem( rItem.pItem ) )
					rItem.pItem = 0;
				j = 1;
				break;
			}
		}

		if ( !j && bChecked )
		{
			aInvalidItem.nSlot = nSlot;
			rList.Insert( aInvalidItem );
		}
	}

	// remove invalid items (pItem == NULL)
	for ( USHORT n = rList.Count(); n; )
		if ( !rList[ --n ].pItem )
			rList.Remove( n );

	EndDialog( RET_OK );
	return 0;
}

// class SvxSearchSimilarityDialog ---------------------------------------

SvxSearchSimilarityDialog::SvxSearchSimilarityDialog
(
	Window* pParent,
	BOOL bRelax,
	USHORT nOther,
	USHORT nShorter,
	USHORT nLonger
) :
	ModalDialog( pParent, SVX_RES( RID_SVXDLG_SEARCHSIMILARITY ) ),

    aFixedLine  ( this, SVX_RES( FL_SIMILARITY ) ),
    aOtherTxt   ( this, SVX_RES( FT_OTHER ) ),
	aOtherFld	( this, SVX_RES( NF_OTHER	) ),
    aLongerTxt  ( this, SVX_RES( FT_LONGER ) ),
    aLongerFld  ( this, SVX_RES( NF_LONGER ) ),
    aShorterTxt ( this, SVX_RES( FT_SHORTER ) ),
	aShorterFld	( this, SVX_RES( NF_SHORTER ) ),
    aRelaxBox   ( this, SVX_RES( CB_RELAX ) ),

	aOKBtn		( this, SVX_RES( BTN_ATTR_OK ) ),
	aEscBtn		( this, SVX_RES( BTN_ATTR_CANCEL ) ),
	aHelpBtn	( this, SVX_RES( BTN_ATTR_HELP ) )

{
	FreeResource();

	aOtherFld.SetValue( nOther );
	aShorterFld.SetValue( nShorter );
	aLongerFld.SetValue( nLonger );
	aRelaxBox.Check( bRelax );
}

