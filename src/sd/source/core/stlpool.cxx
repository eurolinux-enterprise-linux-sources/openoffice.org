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

#include <com/sun/star/lang/DisposedException.hpp>
#include <svx/eeitem.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/colritem.hxx>
#include <svx/cntritem.hxx>
#include <svx/shdditem.hxx>
#include <svx/crsditem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/postitem.hxx>
#include <svx/fontitem.hxx>
#include <svtools/poolitem.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xlineit0.hxx>
#include <svx/ulspitem.hxx>
#include <svx/numitem.hxx>
#include <svx/brshitem.hxx>
#include <svx/editeng.hxx>
#include <svtools/smplhint.hxx>
#include <svx/langitem.hxx>
#include <svx/charreliefitem.hxx>
#ifndef _SVX_EMPHITEM_HXX
#include <svx/emphitem.hxx>
#endif
#include <svx/sdr/table/tabledesign.hxx>
#include <svx/akrnitem.hxx>

#include <svx/svdattr.hxx>
#include "eetext.hxx"
#include <svx/xtable.hxx>			// fuer RGB_Color
#include <svx/bulitem.hxx>
#include <svx/lrspitem.hxx>
#include <svx/adjitem.hxx>
#include <svtools/itempool.hxx>

#define _SDR_POSITIVE
#define _SDR_ITEMS

#include "stlpool.hxx"
#include "sdresid.hxx"
#include "stlsheet.hxx"
#include "glob.hrc"
#include "glob.hxx"
#include "drawdoc.hxx"
#include "sdmod.hxx"
#include "sdpage.hxx"
#include "helpids.h"
#include <svtools/itemset.hxx>

using ::rtl::OUString;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::style;
using namespace ::com::sun::star::container;

// ----------------------------------------------------------

SdStyleSheetPool::SdStyleSheetPool(SfxItemPool const& _rPool, SdDrawDocument* pDocument)
:	SdStyleSheetPoolBase( _rPool )
,	mpActualStyleSheet(NULL)
,	mpDoc(pDocument)
{
	if( mpDoc )
	{
		rtl::Reference< SfxStyleSheetPool > xPool( this );

		// create graphics family
		mxGraphicFamily = new SdStyleFamily( xPool, SD_STYLE_FAMILY_GRAPHICS );
		mxCellFamily = new SdStyleFamily( xPool, SD_STYLE_FAMILY_CELL );
		
		mxTableFamily = sdr::table::CreateTableDesignFamily();
		Reference< XNamed > xNamed( mxTableFamily, UNO_QUERY );
		if( xNamed.is() )
			msTableFamilyName = xNamed->getName();
		
		// create presentation families, one for each master page
		const sal_uInt16 nCount = mpDoc->GetMasterSdPageCount(PK_STANDARD);
		for( sal_uInt16 nPage = 0; nPage < nCount; ++nPage )
			AddStyleFamily( mpDoc->GetMasterSdPage(nPage,PK_STANDARD) );

//		StartListening( *mpDoc );
	}
}

// ----------------------------------------------------------

SdStyleSheetPool::~SdStyleSheetPool()
{
	DBG_ASSERT( mpDoc == NULL, "sd::SdStyleSheetPool::~SdStyleSheetPool(), dispose me first!" );
}

// ----------------------------------------------------------

SfxStyleSheetBase* SdStyleSheetPool::Create(const String& rName, SfxStyleFamily eFamily, USHORT _nMask )
{
	return new SdStyleSheet(rName, *this, eFamily, _nMask);
}

// ----------------------------------------------------------

SfxStyleSheetBase* SdStyleSheetPool::Create(const SdStyleSheet& rStyle)
{
	return new SdStyleSheet( rStyle );
}

// ----------------------------------------------------------

SfxStyleSheetBase* SdStyleSheetPool::GetTitleSheet(const String& rLayoutName)
{
	String aName(rLayoutName);
	aName.AppendAscii( RTL_CONSTASCII_STRINGPARAM( SD_LT_SEPARATOR ));
	aName += String(SdResId(STR_LAYOUT_TITLE));
	SfxStyleSheetBase* pResult = Find(aName, SD_STYLE_FAMILY_MASTERPAGE);
	return pResult;
}

// ----------------------------------------------------------
// find layout name of first layout
// ----------------------------------------------------------

String SdStyleSheetPool::GetLayoutName() const
{
	String aName( SdResId(STR_LAYOUT_DEFAULT_NAME ) );
	sal_uInt32  nCount = aStyles.size();

	for( sal_uInt32 n = 0; n < nCount; n++ )
	{
		aName = aStyles[n]->GetName();
		sal_uInt16 nPos = aName.SearchAscii( SD_LT_SEPARATOR );
		if( nPos != STRING_NOTFOUND )
			break;
	}

	sal_uInt16 nPos = aName.Search( sal_Unicode( ' ' ));
	if( nPos != STRING_NOTFOUND )
		aName.Erase( nPos );       // removing blanks and number (e.g. "Gliederung 1")

	return aName;
}

/*************************************************************************
|*
|* eine Liste der Gliederungstextvorlagen fuer ein Praesentationlayout
|* erstellen, der Aufrufer muss die Liste wieder loeschen
|*
\************************************************************************/

List* SdStyleSheetPool::CreateOutlineSheetList (const String& rLayoutName)
{
	String aName(rLayoutName);
	aName.AppendAscii( RTL_CONSTASCII_STRINGPARAM( SD_LT_SEPARATOR ));
	aName += String(SdResId(STR_LAYOUT_OUTLINE));
	List* pList = new List;
	for (USHORT nSheet = 1; nSheet < 10; nSheet++)
	{
		String aFullName(aName);
		aFullName.Append( sal_Unicode( ' ' ));
		aFullName.Append( String::CreateFromInt32( (sal_Int32)nSheet ));
		SfxStyleSheetBase* pSheet = Find(aFullName, SD_STYLE_FAMILY_MASTERPAGE);
		pList->Insert(pSheet, LIST_APPEND);
	}
	return pList;
}

/*************************************************************************
|*
|* StyleSheets mit Defaultweren fuer das genannte Praesentationslayout erzeugen
|*
\************************************************************************/

void SdStyleSheetPool::CreateLayoutStyleSheets(const String& rLayoutName, sal_Bool bCheck /*= sal_False*/ )
{
    const sal_uInt16 nUsedMask = SFXSTYLEBIT_ALL & ~SFXSTYLEBIT_USERDEF;

    (void)bCheck;
	sal_Bool bCreated = sal_False;

	SfxStyleSheetBase* pSheet = NULL;

	String aPrefix(rLayoutName);
	String aSep( RTL_CONSTASCII_USTRINGPARAM( SD_LT_SEPARATOR ));
	aPrefix.Insert(aSep);

	Font aLatinFont, aCJKFont, aCTLFont;

	mpDoc->getDefaultFonts( aLatinFont, aCJKFont, aCTLFont );

	// Font fuer Titel und Gliederung
	SvxFontItem aSvxFontItem( aLatinFont.GetFamily(), aLatinFont.GetName(), aLatinFont.GetStyleName(), aLatinFont.GetPitch(),
		                      aLatinFont.GetCharSet(), EE_CHAR_FONTINFO );

	SvxFontItem aSvxFontItemCJK( aCJKFont.GetFamily(), aCJKFont.GetName(), aCJKFont.GetStyleName(), aCJKFont.GetPitch(),
		                         aCJKFont.GetCharSet(), EE_CHAR_FONTINFO_CJK );

	SvxFontItem aSvxFontItemCTL( aCTLFont.GetFamily(), aCTLFont.GetName(), aCTLFont.GetStyleName(), aCTLFont.GetPitch(),
		                         aCTLFont.GetCharSet(), EE_CHAR_FONTINFO_CTL );

	Font aBulletFont( GetBulletFont() );

	/**************************************************************************
	* Gliederungsebenen
	**************************************************************************/
	String aName(SdResId(STR_LAYOUT_OUTLINE));
	String aHelpFile;

	SfxStyleSheetBase* pParent = NULL;
    SvxLRSpaceItem aSvxLRSpaceItem( EE_PARA_LRSPACE );
    SvxULSpaceItem aSvxULSpaceItem( EE_PARA_ULSPACE );
	USHORT nLevel;

	for( nLevel = 1; nLevel < 10; nLevel++)
	{
		String aLevelName(aName);
		aLevelName.Append( sal_Unicode( ' ' ));
		aLevelName.Append( String::CreateFromInt32( sal_Int32( nLevel )));

		aLevelName.Insert(aPrefix, 0);

		if (!Find(aLevelName, SD_STYLE_FAMILY_MASTERPAGE))
		{
			bCreated = sal_True;
			pSheet = &Make(aLevelName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
			pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_OUTLINE + nLevel );

			pSheet->SetParent( String() );

			// Attributierung fuer Level 1, die anderen Ebenen "erben"
			if (nLevel == 1)
			{
				SfxItemSet&     rSet = pSheet->GetItemSet();

				rSet.Put(aSvxFontItem);
				rSet.Put(aSvxFontItemCJK);
				rSet.Put(aSvxFontItemCTL);
				rSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC ) );
				rSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CJK ) );
				rSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CTL ) );
				rSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT ) );
				rSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CJK ) );
				rSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CTL ) );
                rSet.Put( SvxUnderlineItem(UNDERLINE_NONE, EE_CHAR_UNDERLINE ) );
                rSet.Put( SvxOverlineItem(UNDERLINE_NONE, EE_CHAR_OVERLINE ) );
                rSet.Put( SvxCrossedOutItem(STRIKEOUT_NONE, EE_CHAR_STRIKEOUT ) );
                rSet.Put( SvxShadowedItem(FALSE, EE_CHAR_SHADOW ) );
                rSet.Put( SvxContourItem(FALSE, EE_CHAR_OUTLINE ) );
                rSet.Put( SvxEmphasisMarkItem(EMPHASISMARK_NONE, EE_CHAR_EMPHASISMARK ) );
                rSet.Put( SvxCharReliefItem(RELIEF_NONE, EE_CHAR_RELIEF) );
                rSet.Put( SvxColorItem( Color(COL_AUTO), EE_CHAR_COLOR) );
				rSet.Put( XLineStyleItem(XLINE_NONE) );
				rSet.Put( XFillStyleItem(XFILL_NONE) );
				// #i16874# enable kerning by default but only for new documents
				rSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );

				if( nLevel == 1 )
				{
					Font f( GetBulletFont() );
					PutNumBulletItem( pSheet, f );
				}
			}

			ULONG nFontSize = 20;
			short nFirstIndent = -600;
//			USHORT nIndent = nLevel * 1200;
			USHORT nLower = 100;

			switch (nLevel)
			{
				case 1:
				{
					nFontSize = 32;
					nLower = 500;
					nFirstIndent = -900;
				}
				break;

				case 2:
				{
					nFontSize = 28;
					nLower = 400;
					nFirstIndent = -800;
				}
				break;

				case 3:
				{
					nFontSize = 24;
					nLower = 300;
				}
				break;

				case 4:
				{
					nLower = 200;
				}
				break;
			}

			// FontSize
			nFontSize = (USHORT)((nFontSize * 2540L) / 72);  // Pt --> 1/100 mm
			SfxItemSet& rOutlineSet = pSheet->GetItemSet();
			rOutlineSet.Put( SvxFontHeightItem( nFontSize, 100, EE_CHAR_FONTHEIGHT ) );
			rOutlineSet.Put( SvxFontHeightItem( nFontSize, 100, EE_CHAR_FONTHEIGHT_CJK ) );
			rOutlineSet.Put( SvxFontHeightItem( SdDrawDocument::convertFontHeightToCTL( nFontSize ), 100, EE_CHAR_FONTHEIGHT_CTL ) );

			// Einzuege
/* i35937
            aSvxLRSpaceItem.SetTxtFirstLineOfst(nFirstIndent);
			aSvxLRSpaceItem.SetTxtLeft(nIndent);
			aSvxLRSpaceItem.SetRight(0);
			aSvxLRSpaceItem.SetBulletFI(TRUE);
			pSheet->GetItemSet().Put(aSvxLRSpaceItem);
*/
			// Zeilendurchschuss (Abstand nach unten)
			aSvxULSpaceItem.SetLower(nLower);
			pSheet->GetItemSet().Put(aSvxULSpaceItem);

/* i35937
			if (nLevel == 1)
			{
				SfxUInt16Item aBulletStateItem(EE_PARA_BULLETSTATE, 1); // Bullets sichtbar
				pSheet->GetItemSet().Put(aBulletStateItem);
			}
*/
		}
	}

	// if we created outline styles, we need to chain them
	if( bCreated )
	{
		pParent = NULL;
		for (nLevel = 1; nLevel < 10; nLevel++)
		{
			String aLevelName(aName);
			aLevelName.Append( sal_Unicode( ' ' ));
			aLevelName.Append( String::CreateFromInt32( sal_Int32( nLevel )));

			aLevelName.Insert(aPrefix, 0);

			pSheet = Find(aLevelName, SD_STYLE_FAMILY_MASTERPAGE);

			DBG_ASSERT( pSheet, "missing layout style!");

			if( pSheet )
			{
				if (pParent)
					pSheet->SetParent(pParent->GetName());
				pParent = pSheet;
			}
		}
	}

	/**************************************************************************
	* Titel
	**************************************************************************/
	aName = String(SdResId(STR_LAYOUT_TITLE));
	aName.Insert(aPrefix, 0);

	if (!Find(aName, SD_STYLE_FAMILY_MASTERPAGE))
	{
		bCreated = sal_True;

		pSheet = &Make(aName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_TITLE );
		pSheet->SetParent(String());
		SfxItemSet& rTitleSet = pSheet->GetItemSet();
		rTitleSet.Put(XLineStyleItem(XLINE_NONE));
		rTitleSet.Put(XFillStyleItem(XFILL_NONE));
		rTitleSet.Put(aSvxFontItem);
		rTitleSet.Put(aSvxFontItemCJK);
		rTitleSet.Put(aSvxFontItemCTL);
		rTitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC ) );
		rTitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CJK ) );
		rTitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CTL ) );
		rTitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT ) );
		rTitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CJK ) );
		rTitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CTL ) );
		rTitleSet.Put(SvxFontHeightItem( 1552, 100, EE_CHAR_FONTHEIGHT ) );		            // 44 pt
		rTitleSet.Put(SvxFontHeightItem( 1552, 100, EE_CHAR_FONTHEIGHT_CJK ) );		            // 44 pt
		rTitleSet.Put(SvxFontHeightItem( SdDrawDocument::convertFontHeightToCTL( 1552 ), 100, EE_CHAR_FONTHEIGHT_CTL ) );		            // 44 pt
        rTitleSet.Put(SvxUnderlineItem(UNDERLINE_NONE, EE_CHAR_UNDERLINE ));
        rTitleSet.Put(SvxOverlineItem(UNDERLINE_NONE, EE_CHAR_OVERLINE ));
        rTitleSet.Put(SvxCrossedOutItem(STRIKEOUT_NONE, EE_CHAR_STRIKEOUT ));
        rTitleSet.Put(SvxShadowedItem(FALSE, EE_CHAR_SHADOW ));
        rTitleSet.Put(SvxContourItem(FALSE, EE_CHAR_OUTLINE ));
        rTitleSet.Put( SvxEmphasisMarkItem(EMPHASISMARK_NONE, EE_CHAR_EMPHASISMARK ) );
        rTitleSet.Put( SvxCharReliefItem(RELIEF_NONE, EE_CHAR_RELIEF ) );
        rTitleSet.Put(SvxColorItem( Color(COL_AUTO), EE_CHAR_COLOR ));
        rTitleSet.Put(SvxAdjustItem(SVX_ADJUST_CENTER, EE_PARA_JUST ));
		rTitleSet.Put( SdrTextVertAdjustItem( SDRTEXTVERTADJUST_CENTER ) );
//		rTitleSet.Put( SfxUInt16Item(EE_PARA_BULLETSTATE, 0) );
		// #i16874# enable kerning by default but only for new documents
		rTitleSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );

		aBulletFont.SetSize(Size(0,1552));					// 44 pt
		PutNumBulletItem( pSheet, aBulletFont );
	}

	/**************************************************************************
	* Untertitel
	**************************************************************************/
	aName = String(SdResId(STR_LAYOUT_SUBTITLE));
	aName.Insert(aPrefix, 0);

	if (!Find(aName, SD_STYLE_FAMILY_MASTERPAGE))
	{
		bCreated = sal_True;

		pSheet = &Make(aName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_SUBTITLE );
		pSheet->SetParent(String());
		SfxItemSet& rSubtitleSet = pSheet->GetItemSet();
		rSubtitleSet.Put(XLineStyleItem(XLINE_NONE));
		rSubtitleSet.Put(XFillStyleItem(XFILL_NONE));
		rSubtitleSet.Put(aSvxFontItem);
		rSubtitleSet.Put(aSvxFontItemCJK);
		rSubtitleSet.Put(aSvxFontItemCTL);
		rSubtitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC ) );
		rSubtitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CJK ) );
		rSubtitleSet.Put(SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CTL ) );
		rSubtitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT ) );
		rSubtitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CJK ) );
		rSubtitleSet.Put(SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CTL ) );
		rSubtitleSet.Put( SvxFontHeightItem( 1129, 100, EE_CHAR_FONTHEIGHT ) );	    // 32 pt
		rSubtitleSet.Put( SvxFontHeightItem( 1129, 100, EE_CHAR_FONTHEIGHT_CJK ) ); // 32 pt
		rSubtitleSet.Put( SvxFontHeightItem( SdDrawDocument::convertFontHeightToCTL( 1129 ), 100, EE_CHAR_FONTHEIGHT_CTL ) ); // 32 pt
        rSubtitleSet.Put(SvxUnderlineItem(UNDERLINE_NONE, EE_CHAR_UNDERLINE ));
        rSubtitleSet.Put(SvxOverlineItem(UNDERLINE_NONE, EE_CHAR_OVERLINE ));
        rSubtitleSet.Put(SvxCrossedOutItem(STRIKEOUT_NONE, EE_CHAR_STRIKEOUT ));
        rSubtitleSet.Put(SvxShadowedItem(FALSE, EE_CHAR_SHADOW ));
        rSubtitleSet.Put(SvxContourItem(FALSE, EE_CHAR_OUTLINE ));
        rSubtitleSet.Put( SvxEmphasisMarkItem(EMPHASISMARK_NONE, EE_CHAR_EMPHASISMARK ) );
        rSubtitleSet.Put( SvxCharReliefItem(RELIEF_NONE, EE_CHAR_RELIEF ) );
        rSubtitleSet.Put(SvxColorItem( Color(COL_AUTO), EE_CHAR_COLOR ));
        rSubtitleSet.Put(SvxAdjustItem(SVX_ADJUST_CENTER, EE_PARA_JUST ));
		rSubtitleSet.Put( SdrTextVertAdjustItem( SDRTEXTVERTADJUST_CENTER ) );
//		rSubtitleSet.Put( SfxUInt16Item(EE_PARA_BULLETSTATE, 0) );
		// #i16874# enable kerning by default but only for new documents
		rSubtitleSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );
		aSvxLRSpaceItem.SetTxtLeft(0);
		rSubtitleSet.Put(aSvxLRSpaceItem);

		Font aTmpFont( GetBulletFont() );
		aTmpFont.SetSize(Size(0, 1129));		// 32 pt
		PutNumBulletItem( pSheet, aTmpFont );
	}

	/**************************************************************************
	* Notizen
	**************************************************************************/
	aName = String(SdResId(STR_LAYOUT_NOTES));
	aName.Insert(aPrefix, 0);

	if (!Find(aName, SD_STYLE_FAMILY_MASTERPAGE))
	{
		bCreated = sal_True;

		pSheet = &Make(aName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_NOTES );
		pSheet->SetParent(String());
		SfxItemSet& rNotesSet = pSheet->GetItemSet();
		rNotesSet.Put(XLineStyleItem(XLINE_NONE));
		rNotesSet.Put(XFillStyleItem(XFILL_NONE));
		rNotesSet.Put(aSvxFontItem);
		rNotesSet.Put(aSvxFontItemCJK);
		rNotesSet.Put(aSvxFontItemCTL);
		rNotesSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC ) );
		rNotesSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CJK ) );
		rNotesSet.Put( SvxPostureItem( ITALIC_NONE, EE_CHAR_ITALIC_CTL ) );
		rNotesSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT ) );
		rNotesSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CJK ) );
		rNotesSet.Put( SvxWeightItem( WEIGHT_NORMAL, EE_CHAR_WEIGHT_CTL ) );
		rNotesSet.Put( SvxFontHeightItem( 705, 100, EE_CHAR_FONTHEIGHT ) );	    // 20 pt
		rNotesSet.Put( SvxFontHeightItem( 705, 100, EE_CHAR_FONTHEIGHT_CJK ) ); // 20 pt
		rNotesSet.Put( SvxFontHeightItem( SdDrawDocument::convertFontHeightToCTL( 705 ), 100, EE_CHAR_FONTHEIGHT_CTL ) ); // 20 pt
        rNotesSet.Put( SvxUnderlineItem(UNDERLINE_NONE, EE_CHAR_UNDERLINE ) );
        rNotesSet.Put( SvxOverlineItem(UNDERLINE_NONE, EE_CHAR_OVERLINE ) );
        rNotesSet.Put( SvxCrossedOutItem(STRIKEOUT_NONE, EE_CHAR_STRIKEOUT ) );
        rNotesSet.Put( SvxShadowedItem(FALSE, EE_CHAR_SHADOW ) );
        rNotesSet.Put( SvxContourItem(FALSE, EE_CHAR_OUTLINE ) );
        rNotesSet.Put( SvxEmphasisMarkItem(EMPHASISMARK_NONE, EE_CHAR_EMPHASISMARK ) );
        rNotesSet.Put( SvxCharReliefItem(RELIEF_NONE, EE_CHAR_RELIEF) );
        rNotesSet.Put( SvxColorItem( Color(COL_AUTO), EE_CHAR_COLOR ) );
//		rNotesSet.Put( SfxUInt16Item(EE_PARA_BULLETSTATE, 0) );
        rNotesSet.Put( SvxLRSpaceItem( 0, 0, 600, -600, EE_PARA_LRSPACE  ) );
		// #i16874# enable kerning by default but only for new documents
		rNotesSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );

/* #i35937#
		SvxNumBulletItem aNumBullet( (const SvxNumBulletItem&) rNotesSet.Get(EE_PARA_NUMBULLET) );

		EditEngine::ImportBulletItem( aNumBullet, 0, NULL,
								&(const SvxLRSpaceItem&) rNotesSet.Get( EE_PARA_LRSPACE ) );

		( (SfxItemSet&) rNotesSet).Put( aNumBullet );
*/

	}

	/**************************************************************************
	* Hintergrundobjekte
	**************************************************************************/
	aName = String(SdResId(STR_LAYOUT_BACKGROUNDOBJECTS));
	aName.Insert(aPrefix, 0);

	if (!Find(aName, SD_STYLE_FAMILY_MASTERPAGE))
	{
		bCreated = sal_True;

		pSheet = &Make(aName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_BACKGROUNDOBJECTS );
		pSheet->SetParent(String());
		SfxItemSet& rBackgroundObjectsSet = pSheet->GetItemSet();
		rBackgroundObjectsSet.Put(SdrShadowItem(FALSE));
		rBackgroundObjectsSet.Put(SdrShadowColorItem(String(), Color(COL_GRAY)));
		rBackgroundObjectsSet.Put(SdrShadowXDistItem(300));	// 3 mm Schattendistanz
		rBackgroundObjectsSet.Put(SdrShadowYDistItem(300));
		// #i16874# enable kerning by default but only for new documents
		rBackgroundObjectsSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );
	}

	/**************************************************************************
	* Hintergrund
	**************************************************************************/
	aName = String(SdResId(STR_LAYOUT_BACKGROUND));
	aName.Insert(aPrefix, 0);

	if (!Find(aName, SD_STYLE_FAMILY_MASTERPAGE))
	{
		bCreated = sal_True;

		pSheet = &Make(aName, SD_STYLE_FAMILY_MASTERPAGE,nUsedMask);
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_BACKGROUND );
		pSheet->SetParent(String());
		SfxItemSet& rBackgroundSet = pSheet->GetItemSet();
		rBackgroundSet.Put(XLineStyleItem(XLINE_NONE));
		rBackgroundSet.Put(XFillStyleItem(XFILL_NONE));
		// #i16874# enable kerning by default but only for new documents
		rBackgroundSet.Put( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );
	}

	DBG_ASSERT( !bCheck || !bCreated, "missing layout style sheets detected!" );
}

/*************************************************************************
|*
|* StyleSheets des genannten Praesentationslayouts loeschen
|*
\************************************************************************/

void SdStyleSheetPool::EraseLayoutStyleSheets(const String& rLayoutName)
{
	SfxStyleSheetBase* pSheet = NULL;

	List* pNameList = CreateLayoutSheetNames(rLayoutName);

	String* pName = (String*)pNameList->First();
	while (pName)
	{
		pSheet = Find(*pName, SD_STYLE_FAMILY_MASTERPAGE);
		DBG_ASSERT(pSheet, "EraseLayoutStyleSheets: Vorlage nicht gefunden");
		if (pSheet)
			Remove(pSheet);
		delete pName;
		pName = (String*)pNameList->Next();
	}
	delete pNameList;
}


/*************************************************************************
|*
|* Graphik-StyleSheets  aus dem Quellpool in diesen Pool kopieren
|*
|* (rSourcePool kann nicht const sein, weil SfxStyleSheetPoolBase::Find
|*	nicht const ist)
|*
\************************************************************************/

void SdStyleSheetPool::CopyGraphicSheets(SdStyleSheetPool& rSourcePool)
{
	CopySheets( rSourcePool, SD_STYLE_FAMILY_GRAPHICS );
}

void SdStyleSheetPool::CopyCellSheets(SdStyleSheetPool& rSourcePool)
{
	CopySheets( rSourcePool, SD_STYLE_FAMILY_CELL );
}

void SdStyleSheetPool::CopyTableStyles(SdStyleSheetPool& rSourcePool)
{
	Reference< XIndexAccess > xSource( rSourcePool.mxTableFamily, UNO_QUERY );
	Reference< XNameContainer > xTarget( mxTableFamily, UNO_QUERY );
	Reference< XSingleServiceFactory > xFactory( mxTableFamily, UNO_QUERY );

	if( xSource.is() && xFactory.is() && mxTableFamily.is() )
	{
		for( sal_Int32 nIndex = 0; nIndex < xSource->getCount(); nIndex++ ) try
		{	
			Reference< XStyle > xSourceTableStyle( xSource->getByIndex( nIndex ), UNO_QUERY );
			if( xSourceTableStyle.is() )
			{
				Reference< XStyle > xNewTableStyle( xFactory->createInstance(), UNO_QUERY );
				if( xNewTableStyle.is() )
				{
					Reference< XNameAccess> xSourceNames( xSourceTableStyle, UNO_QUERY_THROW );

					Sequence< OUString > aStyleNames( xSourceNames->getElementNames() );
					OUString* pStyleNames( aStyleNames.getArray() );

					Reference< XNameReplace > xTargetNames( xNewTableStyle, UNO_QUERY );

					sal_Int32 nNames = aStyleNames.getLength();
					while( nNames-- )
					{
						const OUString aName( *pStyleNames++ );
						Reference< XStyle > xSourceStyle( xSourceNames->getByName( aName ), UNO_QUERY );
						Reference< XStyle > xTargetStyle;
						if( xSourceStyle.is() ) try
						{
							mxCellFamily->getByName( xSourceStyle->getName() ) >>= xTargetStyle;
						}
						catch( Exception& )
						{
							DBG_ERROR( "sd::SdStyleSheetPool::CopyTableStyles(), exception caught!" );
						}

						if( xTargetStyle.is() )
							xTargetNames->replaceByName( aName, Any( xTargetStyle ) );
					}
				}

                OUString sName( Reference< XNamed >( xSourceTableStyle, UNO_QUERY_THROW )->getName() );
                if( xTarget->hasByName( sName ) )
                    xTarget->replaceByName( sName, Any( xNewTableStyle ) );
                else
				    xTarget->insertByName( sName, Any( xNewTableStyle ) );
			}
		}
		catch( Exception& )
		{
			DBG_ERROR("sd::SdStyleSheetPool::CopyTableStyles(), exception caught!");
		}
	}
}

void SdStyleSheetPool::CopySheets(SdStyleSheetPool& rSourcePool, SfxStyleFamily eFamily )
{
    String aHelpFile;

	sal_uInt32 nCount = rSourcePool.aStyles.size();

	std::vector< std::pair< rtl::Reference< SfxStyleSheetBase >, String > > aNewStyles;

	for (sal_uInt32 n = 0; n < nCount; n++)
	{
		rtl::Reference< SfxStyleSheetBase > xSheet( rSourcePool.aStyles[sal::static_int_cast<sal_uInt16>(n)] );

		if( xSheet->GetFamily() == eFamily )
		{
			String aName( xSheet->GetName() );
			if ( !Find( aName, eFamily ) )
			{
				rtl::Reference< SfxStyleSheetBase > xNewSheet( &Make( aName, eFamily ) );

				xNewSheet->SetMask( xSheet->GetMask() );

                // #91588# Also set parent relation for copied style sheets
                String aParent( xSheet->GetParent() );
                if( aParent.Len() )
					aNewStyles.push_back( std::pair< rtl::Reference< SfxStyleSheetBase >, String >( xNewSheet, aParent ) );

                xNewSheet->SetHelpId( aHelpFile, xSheet->GetHelpId( aHelpFile ) );
				xNewSheet->GetItemSet().Put( xSheet->GetItemSet() );
			}
		}
	}

	// set parents on newly added stylesheets
	std::vector< std::pair< rtl::Reference< SfxStyleSheetBase >, String > >::iterator aIter;
	for( aIter = aNewStyles.begin(); aIter != aNewStyles.end(); aIter++ )
	{
        DBG_ASSERT( rSourcePool.Find( (*aIter).second, eFamily ), "StyleSheet has invalid parent: Family mismatch" );
		(*aIter).first->SetParent( (*aIter).second );
	}
}


/*************************************************************************
|*
|* StyleSheets des genannten Praesentationslayouts aus dem Quellpool in diesen
|* Pool kopieren. Kopiert werden nur solche StyleSheets, die in diesem Pool
|* noch nicht vorhanden sind.
|* pCreatedSheets wird - wenn ungleich NULL - mit Zeigern auf die erzeugten
|* StyleSheets gefuellt.
|*
|* (rSourcePool kann nicht const sein, weil SfxStyleSheetPoolBase::Find
|*	nicht const ist)
|*
\************************************************************************/

void SdStyleSheetPool::CopyLayoutSheets(const String& rLayoutName, SdStyleSheetPool& rSourcePool, SdStyleSheetVector& rCreatedSheets)
{
	SfxStyleSheetBase* pSheet = NULL;

	String aOutlineTag(SdResId(STR_LAYOUT_OUTLINE));

	List* pNameList = CreateLayoutSheetNames(rLayoutName);

    String sEmpty;
	String* pName = (String*)pNameList->First();
	while (pName)
	{
		pSheet = Find(*pName, SD_STYLE_FAMILY_MASTERPAGE);
		if (!pSheet)
		{
			SfxStyleSheetBase* pSourceSheet = rSourcePool.Find(*pName, SD_STYLE_FAMILY_MASTERPAGE);
			DBG_ASSERT(pSourceSheet, "CopyLayoutSheets: Quellvorlage nicht gefunden");
			if (pSourceSheet)
			{
				// falls einer mit Methusalem-Doks. ankommt
				SfxStyleSheetBase& rNewSheet = Make(*pName, SD_STYLE_FAMILY_MASTERPAGE);
                rNewSheet.SetHelpId( sEmpty, pSourceSheet->GetHelpId( sEmpty ) );
				rNewSheet.GetItemSet().Put(pSourceSheet->GetItemSet());
				rCreatedSheets.push_back( SdStyleSheetRef( static_cast< SdStyleSheet* >( &rNewSheet ) ) );
			}
		}
		delete pName;
		pName = (String*)pNameList->Next();
	}
	delete pNameList;

	// Sonderbehandlung fuer Gliederungsvorlagen: Parentbeziehungen aufbauen
	List* pOutlineSheets = CreateOutlineSheetList(rLayoutName);
	SfxStyleSheetBase* pParent = (SfxStyleSheetBase*)pOutlineSheets->First();
	pSheet = (SfxStyleSheetBase*)pOutlineSheets->Next();
	while (pSheet)
	{
		// kein Parent?
		if (pSheet->GetParent().Len() == 0)
			pSheet->SetParent(pParent->GetName());
		pParent = pSheet;
		pSheet = (SfxStyleSheetBase*)pOutlineSheets->Next();
	}
	delete pOutlineSheets;
}

/*************************************************************************
|*
|* Liste mit den Namen der Praesentationsvorlagen eines Layouts erzeugen.
|* Die Liste und die enthaltenen Strings gehoeren dem Caller!
|*
\************************************************************************/

List* SdStyleSheetPool::CreateLayoutSheetNames(const String& rLayoutName) const
{
	String aPrefix(rLayoutName);
	String aSep( RTL_CONSTASCII_USTRINGPARAM( SD_LT_SEPARATOR ));
	aPrefix.Insert(aSep);

	List* pNameList = new List;

	String aName(SdResId(STR_LAYOUT_OUTLINE));
	String* pName = NULL;

	for (USHORT nLevel = 1; nLevel < 10; nLevel++)
	{
		pName = new String(aName);
		pName->Append( sal_Unicode( ' ' ));
		pName->Append( String::CreateFromInt32( sal_Int32( nLevel )));
		pName->Insert(aPrefix, 0);
		pNameList->Insert(pName, LIST_APPEND);
	}

	pName = new String(SdResId(STR_LAYOUT_TITLE));
	pName->Insert(aPrefix, 0);
	pNameList->Insert(pName, LIST_APPEND);

	pName = new String(SdResId(STR_LAYOUT_SUBTITLE));
	pName->Insert(aPrefix, 0);
	pNameList->Insert(pName, LIST_APPEND);

	pName = new String(SdResId(STR_LAYOUT_NOTES));
	pName->Insert(aPrefix, 0);
	pNameList->Insert(pName, LIST_APPEND);

	pName = new String(SdResId(STR_LAYOUT_BACKGROUNDOBJECTS));
	pName->Insert(aPrefix, 0);
	pNameList->Insert(pName, LIST_APPEND);

	pName = new String(SdResId(STR_LAYOUT_BACKGROUND));
	pName->Insert(aPrefix, 0);
	pNameList->Insert(pName, LIST_APPEND);

	return pNameList;
}

/*************************************************************************
|*
|* Liste mit Zeigern auf Praesentationsvorlagen eines Layouts erzeugen.
|* Die Liste gehoert dem Caller!
|*
\************************************************************************/

void SdStyleSheetPool::CreateLayoutSheetList(const String& rLayoutName, SdStyleSheetVector& rLayoutSheets )
{
	String aLayoutNameWithSep(rLayoutName);
	aLayoutNameWithSep.AppendAscii( RTL_CONSTASCII_STRINGPARAM( SD_LT_SEPARATOR ));
	USHORT nLen = aLayoutNameWithSep.Len();

	SfxStyleSheetIterator aIter(this, SD_STYLE_FAMILY_MASTERPAGE);
	SfxStyleSheetBase* pSheet = aIter.First();

	while (pSheet)
	{
		if (pSheet->GetName().Match(aLayoutNameWithSep) == nLen)
			rLayoutSheets.push_back( SdStyleSheetRef( static_cast< SdStyleSheet* >( pSheet ) ) );
		pSheet = aIter.Next();
	}
}

/*************************************************************************
|*
|* ggfs. PseudoStyleSheets erzeugen
|*
\************************************************************************/

void SdStyleSheetPool::CreatePseudosIfNecessary()
{
	String aName;
	String aHelpFile;
	SfxStyleSheetBase* pSheet = NULL;
	SfxStyleSheetBase* pParent = NULL;

	//USHORT nUsedMask = SFXSTYLEBIT_ALL & ~SFXSTYLEBIT_USERDEF;
	USHORT nUsedMask = SFXSTYLEBIT_USED;

	aName = String(SdResId(STR_PSEUDOSHEET_TITLE));
	if( (pSheet = Find(aName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
	{
		pSheet = &Make(aName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);
		pSheet->SetParent( String() );
		((SfxStyleSheet*)pSheet)->StartListening(*this);
	}
	pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_TITLE );

	aName = String(SdResId(STR_PSEUDOSHEET_SUBTITLE));
	if( (pSheet = Find(aName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
	{
		pSheet = &Make(aName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);
		pSheet->SetParent(String());
		((SfxStyleSheet*)pSheet)->StartListening(*this);
	}
	pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_SUBTITLE );

	aName = String(SdResId(STR_PSEUDOSHEET_BACKGROUNDOBJECTS));
	if( (pSheet = Find(aName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
	{
		pSheet = &Make(aName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);
		pSheet->SetParent( String() );
		((SfxStyleSheet*)pSheet)->StartListening(*this);
	}
	pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_BACKGROUNDOBJECTS );

	aName = String(SdResId(STR_PSEUDOSHEET_BACKGROUND));
	if( (pSheet = Find(aName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
	{
		pSheet = &Make(aName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);
		pSheet->SetParent( String() );
		((SfxStyleSheet*)pSheet)->StartListening(*this);
	}
	pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_BACKGROUND );

	aName = String(SdResId(STR_PSEUDOSHEET_NOTES));
	if( (pSheet = Find(aName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
	{
		pSheet = &Make(aName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);
		pSheet->SetParent( String() );
		((SfxStyleSheet*)pSheet)->StartListening(*this);
	}
	pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_NOTES );

	pParent = NULL;
	SetSearchMask(SD_STYLE_FAMILY_PSEUDO);
	aName = String(SdResId(STR_PSEUDOSHEET_OUTLINE));
	for (USHORT nLevel = 1; nLevel < 10; nLevel++)
	{
		String aLevelName(aName);
		aLevelName.Append( sal_Unicode( ' ' ));
		aLevelName.Append( String::CreateFromInt32( sal_Int32( nLevel )));

		if( (pSheet = Find(aLevelName, SD_STYLE_FAMILY_PSEUDO)) == 0 )
		{
			pSheet = &Make(aLevelName, SD_STYLE_FAMILY_PSEUDO, nUsedMask);

			if (pSheet)
			{
				if (pParent)
					pSheet->SetParent(pParent->GetName());
				pParent = pSheet;
				((SfxStyleSheet*)pSheet)->StartListening(*this);
			}
		}
		pSheet->SetHelpId( aHelpFile, HID_PSEUDOSHEET_OUTLINE + nLevel );
	}
}


/*************************************************************************
|*
|* Standard-Styles den richtigen Namen in der Programm-Sprache geben
|*
\************************************************************************/

void SdStyleSheetPool::UpdateStdNames()
{
	String aHelpFile;
	sal_uInt32  nCount = aStyles.size();
	List* pEraseList = NULL;

	for( sal_uInt32 n=0; n < nCount; n++ )
	{
		SfxStyleSheetBase* pStyle = aStyles[ n ].get();

		if( !pStyle->IsUserDefined() )
		{
			String aOldName		= pStyle->GetName();
			ULONG nHelpId		= pStyle->GetHelpId( aHelpFile );
			SfxStyleFamily eFam	= pStyle->GetFamily();

			BOOL bHelpKnown = TRUE;
			String aNewName;
			USHORT nNameId = 0;
			switch( nHelpId )
			{
				case HID_STANDARD_STYLESHEET_NAME:	nNameId = STR_STANDARD_STYLESHEET_NAME;	break;
				case HID_POOLSHEET_OBJWITHARROW:	nNameId = STR_POOLSHEET_OBJWITHARROW;	break;
				case HID_POOLSHEET_OBJWITHSHADOW:	nNameId = STR_POOLSHEET_OBJWITHSHADOW;	break;
				case HID_POOLSHEET_OBJWITHOUTFILL:	nNameId = STR_POOLSHEET_OBJWITHOUTFILL;	break;
				case HID_POOLSHEET_TEXT:			nNameId = STR_POOLSHEET_TEXT;			break;
				case HID_POOLSHEET_TEXTBODY:		nNameId = STR_POOLSHEET_TEXTBODY;		break;
				case HID_POOLSHEET_TEXTBODY_JUSTIFY:nNameId = STR_POOLSHEET_TEXTBODY_JUSTIFY;break;
				case HID_POOLSHEET_TEXTBODY_INDENT:	nNameId = STR_POOLSHEET_TEXTBODY_INDENT;break;
				case HID_POOLSHEET_TITLE:			nNameId = STR_POOLSHEET_TITLE;			break;
				case HID_POOLSHEET_TITLE1:			nNameId = STR_POOLSHEET_TITLE1;			break;
				case HID_POOLSHEET_TITLE2:			nNameId = STR_POOLSHEET_TITLE2;			break;
				case HID_POOLSHEET_HEADLINE:		nNameId = STR_POOLSHEET_HEADLINE;		break;
				case HID_POOLSHEET_HEADLINE1:		nNameId = STR_POOLSHEET_HEADLINE1;		break;
				case HID_POOLSHEET_HEADLINE2:		nNameId = STR_POOLSHEET_HEADLINE2;		break;
				case HID_POOLSHEET_MEASURE:			nNameId = STR_POOLSHEET_MEASURE;		break;

				case HID_PSEUDOSHEET_TITLE:			nNameId = STR_PSEUDOSHEET_TITLE; 		break;
				case HID_PSEUDOSHEET_SUBTITLE:	 	nNameId = STR_PSEUDOSHEET_SUBTITLE; 	break;
				case HID_PSEUDOSHEET_OUTLINE1:
				case HID_PSEUDOSHEET_OUTLINE2:
				case HID_PSEUDOSHEET_OUTLINE3:
				case HID_PSEUDOSHEET_OUTLINE4:
				case HID_PSEUDOSHEET_OUTLINE5:
				case HID_PSEUDOSHEET_OUTLINE6:
				case HID_PSEUDOSHEET_OUTLINE7:
				case HID_PSEUDOSHEET_OUTLINE8:
				case HID_PSEUDOSHEET_OUTLINE9:		nNameId = STR_PSEUDOSHEET_OUTLINE; 		break;
				case HID_PSEUDOSHEET_BACKGROUNDOBJECTS:	nNameId = STR_PSEUDOSHEET_BACKGROUNDOBJECTS; break;
				case HID_PSEUDOSHEET_BACKGROUND:	nNameId = STR_PSEUDOSHEET_BACKGROUND; 	break;
				case HID_PSEUDOSHEET_NOTES:			nNameId = STR_PSEUDOSHEET_NOTES; 		break;

				case HID_SD_CELL_STYLE_DEFAULT:			nNameId = STR_STANDARD_STYLESHEET_NAME; break;
				case HID_SD_CELL_STYLE_BANDED:			nNameId = STR_POOLSHEET_BANDED_CELL; break;
				case HID_SD_CELL_STYLE_HEADER:			nNameId = STR_POOLSHEET_HEADER; break;
				case HID_SD_CELL_STYLE_TOTAL:			nNameId = STR_POOLSHEET_TOTAL; break;
				case HID_SD_CELL_STYLE_FIRST_COLUMN:	nNameId = STR_POOLSHEET_FIRST_COLUMN; break;
				case HID_SD_CELL_STYLE_LAST_COLUMN:		nNameId = STR_POOLSHEET_LAST_COLUMN; break;

				default:
					// 0 oder falsche (alte) HelpId
					bHelpKnown = FALSE;
			}
			if( bHelpKnown )
			{
				if( nNameId )
				{
					aNewName = String( SdResId( nNameId ) );
					if( nNameId == STR_PSEUDOSHEET_OUTLINE )
					{
						aNewName.Append( sal_Unicode( ' ' ));
						aNewName.Append( String::CreateFromInt32( sal_Int32( nHelpId - HID_PSEUDOSHEET_OUTLINE )));
					}
				}

				if( aNewName.Len() && aNewName != aOldName )
				{
					SfxStyleSheetBase* pSheetFound = Find( aNewName, eFam );

					if ( !pSheetFound )
					{
						// Sheet existiert noch nicht: Altes Sheet wird umbenannt
						pStyle->SetName( aNewName );	// setzt auch Parents um
					}
					else
					{
						// Sheet existiert schon: Altes Sheet muss entfernt werden
						if( !pEraseList )
						{
							pEraseList = new List();
						}

						pEraseList->Insert( pStyle );
					}
				}
			}
		}
	}

	if ( pEraseList )
	{
		// Styles, welche nicht umbenannt werden konnten, muessen entfernt werden
		for ( ULONG i = 0; i < pEraseList->Count(); i++ )
		{
			SfxStyleSheetBase* pEraseSheet = ( SfxStyleSheetBase* ) pEraseList->GetObject( i );
			Remove( pEraseSheet );
		}

		delete pEraseList;
		pEraseList = NULL;
	}
}
// --------------------------------------------------------------------
// Neues SvxNumBulletItem fuer das jeweilige StyleSheet setzen
// --------------------------------------------------------------------

void SdStyleSheetPool::PutNumBulletItem( SfxStyleSheetBase* pSheet,
										 Font& rBulletFont )
{
	String aHelpFile;
	ULONG nHelpId = pSheet->GetHelpId( aHelpFile );
	SfxItemSet& rSet = pSheet->GetItemSet();

	switch ( nHelpId )
	{
		case HID_STANDARD_STYLESHEET_NAME :
		{
			// Standard-Vorlage
			SvxNumberFormat aNumberFormat(SVX_NUM_CHAR_SPECIAL);
			aNumberFormat.SetBulletFont(&rBulletFont);
			aNumberFormat.SetBulletChar( 0x25CF ); // StarBats: 0xF000 + 34
			aNumberFormat.SetBulletRelSize(45);
			aNumberFormat.SetBulletColor(Color(COL_AUTO));
			aNumberFormat.SetStart(1);
			aNumberFormat.SetNumAdjust(SVX_ADJUST_LEFT);

			SvxNumRule aNumRule( NUM_BULLET_REL_SIZE|NUM_BULLET_COLOR|NUM_CHAR_TEXT_DISTANCE, 10 , FALSE);
			aNumberFormat.SetLSpace( 0 );
			aNumberFormat.SetAbsLSpace( 0 );
			aNumberFormat.SetFirstLineOffset( 0 );
			aNumRule.SetLevel( 0, aNumberFormat );

			for( USHORT i = 1; i < aNumRule.GetLevelCount(); i++ )
			{
				const short nLSpace = (i + 1) * 600;
				aNumberFormat.SetLSpace(nLSpace);
				aNumberFormat.SetAbsLSpace(nLSpace);
				aNumberFormat.SetFirstLineOffset(-600);
				aNumRule.SetLevel( i, aNumberFormat );
			}

			rSet.Put( SvxNumBulletItem( aNumRule, EE_PARA_NUMBULLET ) );
			((SfxStyleSheet*)pSheet)->Broadcast(SfxSimpleHint( SFX_HINT_DATACHANGED ) );
		}
		break;

		case HID_PSEUDOSHEET_TITLE:
			/* #84013# title gets same bullet as subtitle and not that page symbol anymore */
		case HID_PSEUDOSHEET_SUBTITLE :
		{
			// Untertitel-Vorlage
			SvxNumRule* pDefaultRule = ((SvxNumBulletItem*) rSet.GetPool()->GetSecondaryPool()->GetPoolDefaultItem(EE_PARA_NUMBULLET))->GetNumRule();
			DBG_ASSERT( pDefaultRule, "Wo ist mein Default? [CL]" );

			if(pDefaultRule)
			{
				SvxNumRule aNumRule(pDefaultRule->GetFeatureFlags(), 10, FALSE);
				for(USHORT i=0; i < aNumRule.GetLevelCount(); i++)
				{
					SvxNumberFormat aFrmt( pDefaultRule->GetLevel(i) );
					aFrmt.SetNumberingType(SVX_NUM_CHAR_SPECIAL);
					aFrmt.SetStart(1);
					aFrmt.SetBulletRelSize(45);
					aFrmt.SetBulletChar( 0x25CF );  // StarBats: 0xF000 + 34
					aFrmt.SetBulletFont(&rBulletFont);
					aNumRule.SetLevel(i, aFrmt);
				}

				rSet.Put( SvxNumBulletItem( aNumRule, EE_PARA_NUMBULLET ) );
				((SfxStyleSheet*)pSheet)->Broadcast(SfxSimpleHint( SFX_HINT_DATACHANGED ) );
			}
		}
		break;

		case HID_PSEUDOSHEET_OUTLINE + 1 :
		{
			// Gliederungs-Vorlage
			SvxNumberFormat aNumberFormat(SVX_NUM_CHAR_SPECIAL);
			aNumberFormat.SetBulletColor(Color(COL_AUTO));
			aNumberFormat.SetStart(1);
			aNumberFormat.SetNumAdjust(SVX_ADJUST_LEFT);

			SvxNumRule aNumRule( NUM_BULLET_REL_SIZE|NUM_BULLET_COLOR|NUM_CHAR_TEXT_DISTANCE|NUM_SYMBOL_ALIGNMENT,
								 10, FALSE );
			for( UINT16 i = 0; i < aNumRule.GetLevelCount(); i++ )
			{
				aNumberFormat.SetBulletChar( 0x25CF );  // StarBats: 0xF000 + 34
				aNumberFormat.SetBulletRelSize(45);
				const short nLSpace = (i + 1) * 1200;
				aNumberFormat.SetLSpace(nLSpace);
				aNumberFormat.SetAbsLSpace(nLSpace);
				short nFirstLineOffset = -600;

				ULONG nFontSize = 20;
				switch(i)
				{
					case 0:
					case 1:
					{
						nFontSize = 32;
						nFirstLineOffset = -900;
					}
					break;

					case 2:
					{
						nFontSize = 28;
						nFirstLineOffset = -800;
						aNumberFormat.SetBulletChar( 0x2013 );  // StarBats: 0xF000 + 150
						aNumberFormat.SetBulletRelSize(75);
					}
					break;

					case 3:
					{
						nFontSize = 24;
					}
					break;

					case 4:
					{
						aNumberFormat.SetBulletChar( 0x2013 ); // StarBats: 0xF000 + 150
						aNumberFormat.SetBulletRelSize(75);
					}
					break;
				}

				aNumberFormat.SetFirstLineOffset(nFirstLineOffset);
				nFontSize = (USHORT)((nFontSize * 2540L) / 72);  // Pt --> 1/100 mm
				rBulletFont.SetSize(Size(0,846));		// 24 pt
				aNumberFormat.SetBulletFont(&rBulletFont);
				aNumRule.SetLevel( i, aNumberFormat );
			}

			rSet.Put( SvxNumBulletItem( aNumRule, EE_PARA_NUMBULLET ) );
			((SfxStyleSheet*)pSheet)->Broadcast(SfxSimpleHint( SFX_HINT_DATACHANGED ) );
		}
		break;
	}
}

/*************************************************************************
|*
|* Standard Bullet-Font erzeugen (ohne Groesse)
|*
\************************************************************************/

Font SdStyleSheetPool::GetBulletFont() const
{
	Font aBulletFont( String( RTL_CONSTASCII_USTRINGPARAM( "StarSymbol" )), Size(0, 1000) );
	aBulletFont.SetCharSet(RTL_TEXTENCODING_UNICODE);
	aBulletFont.SetWeight(WEIGHT_NORMAL);
	aBulletFont.SetUnderline(UNDERLINE_NONE);
	aBulletFont.SetOverline(UNDERLINE_NONE);
	aBulletFont.SetStrikeout(STRIKEOUT_NONE);
	aBulletFont.SetItalic(ITALIC_NONE);
	aBulletFont.SetOutline(FALSE);
	aBulletFont.SetShadow(FALSE);
	aBulletFont.SetColor(Color(COL_AUTO));
	aBulletFont.SetTransparent(TRUE);

	return aBulletFont;
}

// --------------------------------------------------------------------

void SdStyleSheetPool::AddStyleFamily( const SdPage* pPage )
{
	rtl::Reference< SfxStyleSheetPool > xPool( this );
	maStyleFamilyMap[pPage] = new SdStyleFamily( xPool, pPage );
}

// --------------------------------------------------------------------

void SdStyleSheetPool::RemoveStyleFamily( const SdPage* pPage )
{
	SdStyleFamilyMap::iterator iter( maStyleFamilyMap.find( pPage ) );
	if( iter != maStyleFamilyMap.end() )
	{
		SdStyleFamilyRef xStyle( (*iter).second );
		maStyleFamilyMap.erase( iter );

		if( xStyle.is() ) try
		{
			xStyle->dispose();
		}
		catch( Exception& )
		{
		}
	}
}

// --------------------------------------------------------------------

void SdStyleSheetPool::throwIfDisposed() throw(::com::sun::star::uno::RuntimeException)
{
	if( mpDoc == NULL )
		throw DisposedException();
}

// --------------------------------------------------------------------
// XServiceInfo
// --------------------------------------------------------------------

OUString SAL_CALL SdStyleSheetPool::getImplementationName() throw(RuntimeException)
{
	return OUString( RTL_CONSTASCII_USTRINGPARAM("SdStyleSheetPool") );
}

// --------------------------------------------------------------------

static const sal_Char* gpServiceName = "com.sun.star.style.StyleFamilies";

sal_Bool SAL_CALL SdStyleSheetPool::supportsService( const OUString& ServiceName ) throw(RuntimeException)
{
	return ServiceName.equalsAscii( gpServiceName );
}

// --------------------------------------------------------------------

Sequence< OUString > SAL_CALL SdStyleSheetPool::getSupportedServiceNames() throw(RuntimeException)
{
	OUString aStr( OUString::createFromAscii( gpServiceName ) );
	return Sequence< OUString >( &aStr, 1 );
}

// --------------------------------------------------------------------
// XNameAccess
// --------------------------------------------------------------------

Any SAL_CALL SdStyleSheetPool::getByName( const OUString& aName ) throw(NoSuchElementException, WrappedTargetException, RuntimeException)
{
	throwIfDisposed();

	if( mxGraphicFamily->getName() == aName )
		return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( mxGraphicFamily.get() ) ) );

	if( mxCellFamily->getName() == aName )
		return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( mxCellFamily.get() ) ) );

	if( msTableFamilyName == aName )
		return Any( mxTableFamily );

	for( SdStyleFamilyMap::iterator iter( maStyleFamilyMap.begin() ); iter != maStyleFamilyMap.end(); iter++ )
	{
		if( (*iter).second->getName() == aName )
			return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( (*iter).second.get() ) ) );
	}

	throw NoSuchElementException();
}

// --------------------------------------------------------------------

Sequence< OUString > SAL_CALL SdStyleSheetPool::getElementNames() throw(RuntimeException)
{
	throwIfDisposed();
	
	Sequence< OUString > aNames( maStyleFamilyMap.size() + 3 );
	OUString* pNames = aNames.getArray();

	*pNames++ = mxGraphicFamily->getName();
	*pNames++ = mxCellFamily->getName();
	*pNames++ = msTableFamilyName;

	for( SdStyleFamilyMap::iterator iter( maStyleFamilyMap.begin() ); iter != maStyleFamilyMap.end(); iter++ )
	{
		*pNames++ = (*iter).second->getName();
	}

	return aNames;
}

// --------------------------------------------------------------------

sal_Bool SAL_CALL SdStyleSheetPool::hasByName( const OUString& aName ) throw(RuntimeException)
{
	throwIfDisposed();

	if( mxGraphicFamily->getName() == aName )
		return sal_True;

	if( mxCellFamily->getName() == aName )
		return sal_True;

	if( msTableFamilyName == aName )
		return sal_True;

	for( SdStyleFamilyMap::iterator iter( maStyleFamilyMap.begin() ); iter != maStyleFamilyMap.end(); iter++ )
	{
		if( (*iter).second->getName() == aName )
			return sal_True;
	}

	return sal_False;
}

// --------------------------------------------------------------------
// XElementAccess
// --------------------------------------------------------------------

Type SAL_CALL SdStyleSheetPool::getElementType() throw(RuntimeException)
{
	throwIfDisposed();

	return XNameAccess::static_type();
}

// --------------------------------------------------------------------

sal_Bool SAL_CALL SdStyleSheetPool::hasElements() throw(RuntimeException)
{
	return sal_True;
}

// --------------------------------------------------------------------
// XIndexAccess
// --------------------------------------------------------------------

sal_Int32 SAL_CALL SdStyleSheetPool::getCount() throw(RuntimeException)
{
	throwIfDisposed();

	return maStyleFamilyMap.size() + 3;
}

// --------------------------------------------------------------------

Any SAL_CALL SdStyleSheetPool::getByIndex( sal_Int32 Index ) throw(IndexOutOfBoundsException, WrappedTargetException, RuntimeException)
{
	switch( Index )
	{
	case 0:
		return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( mxGraphicFamily.get() ) ) );

	case 1:
		return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( mxCellFamily.get() ) ) );

	case 2:
		return Any( mxTableFamily );

	default:
		{
			Index -= 3;
			if( (Index < 0) || (Index >= sal::static_int_cast<sal_Int32>(maStyleFamilyMap.size())) )
				throw IndexOutOfBoundsException();
			SdStyleFamilyMap::iterator iter( maStyleFamilyMap.begin() );
			while( Index-- )
				iter++;

			return Any( Reference< XNameAccess >( static_cast< XNameAccess* >( (*iter).second.get() ) ) );
		}
	}
}

// --------------------------------------------------------------------
// XComponent
// --------------------------------------------------------------------

void SAL_CALL SdStyleSheetPool::dispose() throw (RuntimeException)
{
	if( mpDoc )
	{
		mxGraphicFamily->dispose();
		mxGraphicFamily.clear();
		mxCellFamily->dispose();
		mxCellFamily.clear();

		Reference< XComponent > xComp( mxTableFamily, UNO_QUERY );
		if( xComp.is() )
			xComp->dispose();
		mxTableFamily = 0;

		SdStyleFamilyMap aTempMap;
		aTempMap.swap( maStyleFamilyMap );

		for( SdStyleFamilyMap::iterator iter( aTempMap.begin() ); iter != aTempMap.end(); iter++ ) try
		{
			(*iter).second->dispose();
		}
		catch( Exception& )
		{
		}

//		EndListening( *mpDoc );
		mpDoc = 0;
	}
}

// --------------------------------------------------------------------

void SAL_CALL SdStyleSheetPool::addEventListener( const Reference< XEventListener >& /*xListener*/ ) throw (RuntimeException)
{
}

// --------------------------------------------------------------------

void SAL_CALL SdStyleSheetPool::removeEventListener( const Reference< XEventListener >& /*aListener*/ ) throw (RuntimeException)
{
}

// --------------------------------------------------------------------

SdStyleSheetVector SdStyleSheetPool::CreateChildList( SdStyleSheet* pSheet )
{
	SdStyleSheetVector aResult;

	USHORT nListenerCount = pSheet->GetListenerCount();
	if (nListenerCount > 0)
	{
		for (USHORT n = 0; n < nListenerCount; n++)
		{
			SdStyleSheet* pChild = dynamic_cast< SdStyleSheet* >( pSheet->GetListener(n) );
			if(pChild && pChild->GetParent() == pSheet->GetName())
			{
				aResult.push_back( SdStyleSheetRef( pChild ) );
			}
		}
	}

	return aResult;
}

// --------------------------------------------------------------------
