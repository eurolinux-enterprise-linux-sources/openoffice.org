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

#include <eeng_pch.hxx>

#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif

#include <svx/lspcitem.hxx>

#include <svx/lrspitem.hxx>
#include <svx/ulspitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/colritem.hxx>
#include <svx/fhgtitem.hxx>
#include <fontitem.hxx>
#include <svx/adjitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/postitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/crsditem.hxx>
#include <svx/shdditem.hxx>
#include <svx/escpitem.hxx>
#include <svx/kernitem.hxx>
#include <svx/wrlmitem.hxx>
#include <svx/akrnitem.hxx>
#include <svx/langitem.hxx>
#include <svx/emphitem.hxx>
#include <svx/numitem.hxx>
#include <svx/tstpitem.hxx>
#include <svx/charscaleitem.hxx>
#include <svx/charreliefitem.hxx>
#include <svx/frmdiritem.hxx>

#include <impedit.hxx>
#include <svx/editeng.hxx>
#include <svx/editview.hxx>
#include <editdoc.hxx>
#include <editdbg.hxx>

#if defined( DBG_UTIL ) || ( OSL_DEBUG_LEVEL > 1 )

ByteString DbgOutItem( const SfxItemPool& rPool, const SfxPoolItem& rItem )
{
	ByteString aDebStr;
	switch ( rItem.Which() )
	{
        case EE_PARA_WRITINGDIR:
			aDebStr += "WritingDir=";
			aDebStr += ByteString::CreateFromInt32( ((SvxFrameDirectionItem&)rItem).GetValue() );
        break;
		case EE_PARA_OUTLLRSPACE:
		case EE_PARA_LRSPACE:
			aDebStr += "FI=";
			aDebStr += ByteString::CreateFromInt32( ((SvxLRSpaceItem&)rItem).GetTxtFirstLineOfst() );
			aDebStr += ", LI=";
			aDebStr += ByteString::CreateFromInt32( ((SvxLRSpaceItem&)rItem).GetTxtLeft() );
			aDebStr += ", RI=";
			aDebStr += ByteString::CreateFromInt32( ((SvxLRSpaceItem&)rItem).GetRight() );
		break;
		case EE_PARA_NUMBULLET:
			{
			aDebStr += "NumItem ";
			for ( USHORT nLevel = 0; nLevel < 3; nLevel++ )
			{
				aDebStr += "Level";
				aDebStr += ByteString::CreateFromInt32( nLevel );
				aDebStr += "=";
				const SvxNumberFormat* pFmt = ((const SvxNumBulletItem&)rItem).GetNumRule()->Get( nLevel );
				if ( pFmt )
				{
					aDebStr += "(";
					aDebStr += ByteString::CreateFromInt32( pFmt->GetFirstLineOffset() );
					aDebStr += ",";
					aDebStr += ByteString::CreateFromInt32( pFmt->GetAbsLSpace() );
					aDebStr += ",";
					if ( pFmt->GetNumberingType() == SVX_NUM_BITMAP )
					{
						aDebStr += "Bitmap";
					}
					else if( pFmt->GetNumberingType() != SVX_NUM_CHAR_SPECIAL )
					{
						aDebStr += "Number";
					}
					else
					{
						aDebStr += "Char=[";
						aDebStr += ByteString::CreateFromInt32( pFmt->GetBulletChar() );
						aDebStr += "]";
					}
					aDebStr += ") ";
				}
			}
			}
		break;
		case EE_PARA_BULLETSTATE:
			aDebStr += "ShowBullet=";
			aDebStr += ByteString::CreateFromInt32( ((SfxBoolItem&)rItem).GetValue() );
		break;
		case EE_PARA_HYPHENATE:
			aDebStr += "Hyphenate=";
			aDebStr += ByteString::CreateFromInt32( ((SfxBoolItem&)rItem).GetValue() );
		break;
		case EE_PARA_OUTLLEVEL:
			aDebStr += "Level=";
			aDebStr += ByteString::CreateFromInt32( ((SfxInt16Item&)rItem).GetValue() );
		break;
		case EE_PARA_ULSPACE:
			aDebStr += "SB=";
			aDebStr += ByteString::CreateFromInt32( ((SvxULSpaceItem&)rItem).GetUpper() );
			aDebStr += ", SA=";
			aDebStr += ByteString::CreateFromInt32( ((SvxULSpaceItem&)rItem).GetLower() );
		break;
		case EE_PARA_SBL:
			aDebStr += "SBL=";
			if ( ((SvxLineSpacingItem&)rItem).GetLineSpaceRule() == SVX_LINE_SPACE_MIN )
			{
				aDebStr += "Min: ";
				aDebStr += ByteString::CreateFromInt32( ((SvxLineSpacingItem&)rItem).GetInterLineSpace() );
			}
			else if ( ((SvxLineSpacingItem&)rItem).GetInterLineSpaceRule() == SVX_INTER_LINE_SPACE_PROP )
			{
				aDebStr += "Prop: ";
				aDebStr += ByteString::CreateFromInt32( (ULONG)((SvxLineSpacingItem&)rItem).GetPropLineSpace() );
			}
			else
				aDebStr += "Unsupported Type!";
		break;
		case EE_PARA_JUST:
			aDebStr += "SvxAdust=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxAdjustItem&)rItem).GetAdjust() );
		break;
		case EE_PARA_TABS:
        {
			aDebStr += "Tabs: ";
	        const SvxTabStopItem& rTabs = (const SvxTabStopItem&) rItem;
            aDebStr += ByteString::CreateFromInt32( rTabs.Count() );
            if ( rTabs.Count() )
            {
			    aDebStr += "( ";
	            for ( USHORT i = 0; i < rTabs.Count(); i++ )
	            {
		            const SvxTabStop& rTab = rTabs[i];
                    aDebStr += ByteString::CreateFromInt32( rTab.GetTabPos() );
			        aDebStr += " ";
	            }
			    aDebStr += ")";
            }
        }
		break;
		case EE_CHAR_LANGUAGE:
		case EE_CHAR_LANGUAGE_CJK:
		case EE_CHAR_LANGUAGE_CTL:
			aDebStr += "Language=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxLanguageItem&)rItem).GetLanguage() );
		break;
		case EE_CHAR_COLOR:
		{
			aDebStr += "Color= ";
			Color aColor( ((SvxColorItem&)rItem).GetValue() );
			aDebStr += ByteString::CreateFromInt32( (USHORT)aColor.GetRed() );
			aDebStr += ", ";
			aDebStr += ByteString::CreateFromInt32( (USHORT)aColor.GetGreen() );
			aDebStr += ", ";
			aDebStr += ByteString::CreateFromInt32( (USHORT)aColor.GetBlue() );
		}
		break;
		case EE_CHAR_FONTINFO:
		case EE_CHAR_FONTINFO_CJK:
		case EE_CHAR_FONTINFO_CTL:
		{
			aDebStr += "Font=";
			aDebStr += ByteString( ((SvxFontItem&)rItem).GetFamilyName(), RTL_TEXTENCODING_ASCII_US );
			aDebStr += " (CharSet: ";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxFontItem&)rItem).GetCharSet() );
			aDebStr += ')';
		}
		break;
		case EE_CHAR_FONTHEIGHT:
		case EE_CHAR_FONTHEIGHT_CJK:
		case EE_CHAR_FONTHEIGHT_CTL:
		{
			aDebStr += "Groesse=";
			aDebStr += ByteString::CreateFromInt32( ((SvxFontHeightItem&)rItem).GetHeight() );
			Size aSz( 0, ((SvxFontHeightItem&)rItem).GetHeight() );
			SfxMapUnit eUnit = rPool.GetMetric( rItem.Which() );
			MapMode aItemMapMode( (MapUnit) eUnit );
			MapMode aPntMap( MAP_POINT );
			aSz = OutputDevice::LogicToLogic( aSz, aItemMapMode, aPntMap );
			aDebStr += " Points=";
			aDebStr += ByteString::CreateFromInt32( aSz.Height() );
		}
		break;
		case EE_CHAR_FONTWIDTH:
		{
			aDebStr += "Breite=";
			aDebStr += ByteString::CreateFromInt32( ((SvxCharScaleWidthItem&)rItem).GetValue() );
			aDebStr += "%";
		}
		break;
		case EE_CHAR_WEIGHT:
		case EE_CHAR_WEIGHT_CJK:
		case EE_CHAR_WEIGHT_CTL:
			aDebStr += "FontWeight=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxWeightItem&)rItem).GetWeight() );
		break;
		case EE_CHAR_UNDERLINE:
			aDebStr += "FontUnderline=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxUnderlineItem&)rItem).GetLineStyle() );
		break;
		case EE_CHAR_OVERLINE:
			aDebStr += "FontOverline=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxOverlineItem&)rItem).GetLineStyle() );
		break;
		case EE_CHAR_EMPHASISMARK:
			aDebStr += "FontUnderline=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxEmphasisMarkItem&)rItem).GetEmphasisMark() );
		break;
		case EE_CHAR_RELIEF:
			aDebStr += "FontRelief=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxCharReliefItem&)rItem).GetValue() );
		break;
		case EE_CHAR_STRIKEOUT:
			aDebStr += "FontStrikeout=";
			aDebStr +=ByteString::CreateFromInt32(  (USHORT)((SvxCrossedOutItem&)rItem).GetStrikeout() );
		break;
		case EE_CHAR_ITALIC:
		case EE_CHAR_ITALIC_CJK:
		case EE_CHAR_ITALIC_CTL:
			aDebStr += "FontPosture=";
			aDebStr +=ByteString::CreateFromInt32(  (USHORT)((SvxPostureItem&)rItem).GetPosture() );
		break;
		case EE_CHAR_OUTLINE:
			aDebStr += "FontOutline=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxContourItem&)rItem).GetValue() );
		break;
		case EE_CHAR_SHADOW:
			aDebStr += "FontShadowed=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxShadowedItem&)rItem).GetValue() );
		break;
		case EE_CHAR_ESCAPEMENT:
			aDebStr += "Escape=";
			aDebStr += ByteString::CreateFromInt32( (short)((SvxEscapementItem&)rItem).GetEsc() );
			aDebStr += ", ";
			aDebStr += ByteString::CreateFromInt32( (short)((SvxEscapementItem&)rItem).GetProp() );
		break;
		case EE_CHAR_PAIRKERNING:
			aDebStr += "PairKerning=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxAutoKernItem&)rItem).GetValue() );
		break;
		case EE_CHAR_KERNING:
		{
			aDebStr += "Kerning=";
			aDebStr += ByteString::CreateFromInt32( (short)((SvxKerningItem&)rItem).GetValue() );
			Size aSz( 0, (short)((SvxKerningItem&)rItem).GetValue() );
			SfxMapUnit eUnit = rPool.GetMetric( rItem.Which() );
			MapMode aItemMapMode( (MapUnit) eUnit );
			MapMode aPntMap( MAP_POINT );
			aSz = OutputDevice::LogicToLogic( aSz, aItemMapMode, aPntMap );
			aDebStr += " Points=";
			aDebStr += ByteString::CreateFromInt32( aSz.Height() );
		}
		break;
		case EE_CHAR_WLM:
			aDebStr += "WordLineMode=";
			aDebStr += ByteString::CreateFromInt32( (USHORT)((SvxWordLineModeItem&)rItem).GetValue() );
		break;
		case EE_CHAR_XMLATTRIBS:
			aDebStr += "XMLAttribs=...";
		break;
	}
	return aDebStr;
}

void DbgOutItemSet( FILE* fp, const SfxItemSet& rSet, BOOL bSearchInParent, BOOL bShowALL )
{
	for ( USHORT nWhich = EE_PARA_START; nWhich <= EE_CHAR_END; nWhich++ )
	{
		fprintf( fp, "\nWhich: %i\t", nWhich );
		if ( rSet.GetItemState( nWhich, bSearchInParent ) == SFX_ITEM_OFF )
			fprintf( fp, "ITEM_OFF   " );
		else if ( rSet.GetItemState( nWhich, bSearchInParent ) == SFX_ITEM_DONTCARE )
			fprintf( fp, "ITEM_DC    " );
		else if ( rSet.GetItemState( nWhich, bSearchInParent ) == SFX_ITEM_ON )
			fprintf( fp, "ITEM_ON   *" );

		if ( !bShowALL && ( rSet.GetItemState( nWhich, bSearchInParent ) != SFX_ITEM_ON ) )
			continue;

		const SfxPoolItem& rItem = rSet.Get( nWhich, bSearchInParent );
		ByteString aDebStr = DbgOutItem( *rSet.GetPool(), rItem );
		fprintf( fp, "%s", aDebStr.GetBuffer() );
	}
}

void EditDbg::ShowEditEngineData( EditEngine* pEE, BOOL bInfoBox )
{
#if defined UNX
	FILE* fp = fopen( "/tmp/debug.log", "w" );
#else
	FILE* fp = fopen( "d:\\debug.log", "w" );
#endif
	if ( fp == 0 )
	{
		DBG_ERROR( "Log-File konnte nicht angelegt werden!" );
		return;
	}

	const SfxItemPool& rPool = *pEE->GetEmptyItemSet().GetPool();

	fprintf( fp, "================================================================================" );
	fprintf( fp, "\n==================   Dokument   ================================================" );
	fprintf( fp, "\n================================================================================" );
	for ( USHORT nPortion = 0; nPortion < pEE->pImpEditEngine->GetParaPortions(). Count(); nPortion++)
	{

		ParaPortion* pPPortion = pEE->pImpEditEngine->GetParaPortions().GetObject(nPortion );
		fprintf( fp, "\nAbsatz %i: Laenge = %i, Invalid = %i\nText = '%s'", nPortion, pPPortion->GetNode()->Len(), pPPortion->IsInvalid(), ByteString( *pPPortion->GetNode(), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
		fprintf( fp, "\nVorlage:" );
		SfxStyleSheet* pStyle = pPPortion->GetNode()->GetStyleSheet();
		if ( pStyle )
			fprintf( fp, " %s", ByteString( pStyle->GetName(), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
		fprintf( fp, "\nAbsatzattribute:" );
		DbgOutItemSet( fp, pPPortion->GetNode()->GetContentAttribs().GetItems(), FALSE, FALSE );

		fprintf( fp, "\nZeichenattribute:" );
		BOOL bZeroAttr = FALSE;
		USHORT z;
		for ( z = 0; z < pPPortion->GetNode()->GetCharAttribs().Count(); z++ )
		{
			EditCharAttrib* pAttr = pPPortion->GetNode()->GetCharAttribs().GetAttribs().GetObject( z );
			ByteString aCharAttribs;
			aCharAttribs += "\nA";
			aCharAttribs += ByteString::CreateFromInt32( nPortion );
			aCharAttribs += ":  ";
			aCharAttribs += ByteString::CreateFromInt32( pAttr->GetItem()->Which() );
			aCharAttribs += '\t';
			aCharAttribs += ByteString::CreateFromInt32( pAttr->GetStart() );
			aCharAttribs += '\t';
			aCharAttribs += ByteString::CreateFromInt32( pAttr->GetEnd() );
			if ( pAttr->IsEmpty() )
				bZeroAttr = TRUE;
			fprintf( fp, "%s => ", aCharAttribs.GetBuffer() );

			ByteString aDebStr = DbgOutItem( rPool, *pAttr->GetItem() );
			fprintf( fp, "%s", aDebStr.GetBuffer() );
		}
		if ( bZeroAttr )
			fprintf( fp, "\nNULL-Attribute!" );

		USHORT nTextPortions = pPPortion->GetTextPortions().Count();
		ByteString aPortionStr("\nTextportions: #");
		aPortionStr += ByteString::CreateFromInt32( nTextPortions );
		aPortionStr += " \nA";
		aPortionStr += ByteString::CreateFromInt32( nPortion );
		aPortionStr += ": Absatzlaenge = ";
		aPortionStr += ByteString::CreateFromInt32( pPPortion->GetNode()->Len() );
		aPortionStr += "\nA";
		aPortionStr += ByteString::CreateFromInt32( nPortion );
		aPortionStr += ": ";
		ULONG n = 0;
		for ( z = 0; z < nTextPortions; z++ )
		{
			TextPortion* pPortion = pPPortion->GetTextPortions().GetObject( z );
			aPortionStr += " ";
			aPortionStr += ByteString::CreateFromInt32( pPortion->GetLen() );
			aPortionStr += "(";
			aPortionStr += ByteString::CreateFromInt32( pPortion->GetSize().Width() );
			aPortionStr += ")";
			aPortionStr += "[";
			aPortionStr += ByteString::CreateFromInt32( (USHORT)pPortion->GetKind() );
			aPortionStr += "]";
			aPortionStr += ";";
			n += pPortion->GetLen();
		}
		aPortionStr += "\nA";
		aPortionStr += ByteString::CreateFromInt32( nPortion );
		aPortionStr += ": Gesamtlaenge: ";
		aPortionStr += ByteString::CreateFromInt32( n );
		if ( pPPortion->GetNode()->Len() != n )
			aPortionStr += " => Fehler !!!";
		fprintf( fp, "%s", aPortionStr.GetBuffer() );


		fprintf( fp, "\n\nZeilen:" );
		// Erstmal die Inhalte...
		USHORT nLine;
		for ( nLine = 0; nLine < pPPortion->GetLines().Count(); nLine++ )
		{
			EditLine* pLine = pPPortion->GetLines().GetObject( nLine );

			ByteString aLine( *(pPPortion->GetNode()), pLine->GetStart(), pLine->GetEnd() - pLine->GetStart(), RTL_TEXTENCODING_ASCII_US );
			fprintf( fp, "\nZeile %i\t>%s<", nLine, aLine.GetBuffer() );
		}
		// dann die internen Daten...
		for ( nLine = 0; nLine < pPPortion->GetLines().Count(); nLine++ )
		{
			EditLine* pLine = pPPortion->GetLines().GetObject( nLine );
			fprintf( fp, "\nZeile %i:\tStart: %i,\tEnd: %i", nLine, pLine->GetStart(), pLine->GetEnd() );
			fprintf( fp, "\t\tPortions: %i - %i.\tHoehe: %i, Ascent=%i", pLine->GetStartPortion(), pLine->GetEndPortion(), pLine->GetHeight(), pLine->GetMaxAscent() );
		}

		fprintf( fp, "\n-----------------------------------------------------------------------------" );
	}

	if ( pEE->pImpEditEngine->GetStyleSheetPool() )
	{
		ULONG nStyles = pEE->pImpEditEngine->GetStyleSheetPool() ? pEE->pImpEditEngine->GetStyleSheetPool()->Count() : 0;
		fprintf( fp, "\n\n================================================================================" );
		fprintf( fp, "\n==================   Stylesheets   =============================================" );
		fprintf( fp, "\n================================================================================" );
		fprintf( fp, "\n#Vorlagen:   %lu\n", nStyles );
		SfxStyleSheetIterator aIter( pEE->pImpEditEngine->GetStyleSheetPool(), SFX_STYLE_FAMILY_ALL );
		SfxStyleSheetBase* pStyle = aIter.First();
		while ( pStyle )
		{
			fprintf( fp, "\nVorlage:   %s", ByteString( pStyle->GetName(), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
			fprintf( fp, "\nParent:    %s", ByteString( pStyle->GetParent(), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
			fprintf( fp, "\nFollow:    %s", ByteString( pStyle->GetFollow(), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
			DbgOutItemSet( fp, pStyle->GetItemSet(), FALSE, FALSE );
			fprintf( fp, "\n----------------------------------" );

			pStyle = aIter.Next();
		}
	}

	fprintf( fp, "\n\n================================================================================" );
	fprintf( fp, "\n==================   Defaults   ================================================" );
	fprintf( fp, "\n================================================================================" );
	DbgOutItemSet( fp, pEE->pImpEditEngine->GetEmptyItemSet(), TRUE, TRUE );

	fprintf( fp, "\n\n================================================================================" );
	fprintf( fp, "\n==================   EditEngine & Views   ======================================" );
	fprintf( fp, "\n================================================================================" );
	fprintf( fp, "\nControl: %lx", pEE->GetControlWord() );
	fprintf( fp, "\nRefMapMode: %i", pEE->pImpEditEngine->pRefDev->GetMapMode().GetMapUnit() );
	fprintf( fp, "\nPaperSize: %li x %li", pEE->GetPaperSize().Width(), pEE->GetPaperSize().Height() );
	fprintf( fp, "\nMaxAutoPaperSize: %li x %li", pEE->GetMaxAutoPaperSize().Width(), pEE->GetMaxAutoPaperSize().Height() );
	fprintf( fp, "\nMinAutoPaperSize: %li x %li", pEE->GetMinAutoPaperSize().Width(), pEE->GetMinAutoPaperSize().Height() );
	fprintf( fp, "\nUpdate: %i", pEE->GetUpdateMode() );
	fprintf( fp, "\nAnzahl der Views: %i", pEE->GetViewCount() );
	for	( USHORT nView = 0; nView < pEE->GetViewCount(); nView++ )
	{
		EditView* pV = pEE->GetView( nView );
		DBG_ASSERT( pV, "View nicht gefunden!" );
		fprintf( fp, "\nView %i: Focus=%i", nView, pV->GetWindow()->HasFocus() );
		Rectangle aR( pV->GetOutputArea() );
		fprintf( fp, "\n  OutputArea: nX=%li, nY=%li, dX=%li, dY=%li, MapMode = %i", aR.TopLeft().X(), aR.TopLeft().Y(), aR.GetSize().Width(), aR.GetSize().Height() , pV->GetWindow()->GetMapMode().GetMapUnit() );
		aR = pV->GetVisArea();
		fprintf( fp, "\n  VisArea: nX=%li, nY=%li, dX=%li, dY=%li", aR.TopLeft().X(), aR.TopLeft().Y(), aR.GetSize().Width(), aR.GetSize().Height() );
		ESelection aSel = pV->GetSelection();
		fprintf( fp, "\n  Selektion: Start=%u,%u, End=%u,%u", aSel.nStartPara, aSel.nStartPos, aSel.nEndPara, aSel.nEndPos );
	}
	if ( pEE->GetActiveView() )
	{
		fprintf( fp, "\n\n================================================================================" );
		fprintf( fp, "\n==================   Aktuelle View   ===========================================" );
		fprintf( fp, "\n================================================================================" );
		DbgOutItemSet( fp, pEE->GetActiveView()->GetAttribs(), TRUE, FALSE );
	}
	fclose( fp );
	if ( bInfoBox )
		InfoBox(0, String( RTL_CONSTASCII_USTRINGPARAM( "D:\\DEBUG.LOG !" ) ) ).Execute();
}

ByteString EditDbg::GetPortionInfo( ParaPortion* pPPortion )
{
	USHORT z;

	ByteString aDebStr( "Absatzlaenge = " );
	aDebStr += ByteString::CreateFromInt32( pPPortion->GetNode()->Len() );

	aDebStr += "\nZeichenattribute:";
	for ( z = 0; z < pPPortion->GetNode()->GetCharAttribs().Count(); z++ )
	{
		EditCharAttrib* pAttr = pPPortion->GetNode()->GetCharAttribs().GetAttribs().GetObject( z );
		aDebStr += "\n  ";
		aDebStr += ByteString::CreateFromInt32( pAttr->GetItem()->Which() );
		aDebStr += '\t';
		aDebStr += ByteString::CreateFromInt32( pAttr->GetStart() );
		aDebStr += '\t';
		aDebStr += ByteString::CreateFromInt32( pAttr->GetEnd() );
	}

	aDebStr += "\nTextportions:";
	USHORT n = 0;
	for ( z = 0; z < pPPortion->GetTextPortions().Count(); z++ )
	{
		TextPortion* pPortion = pPPortion->GetTextPortions().GetObject( z );
		aDebStr += " ";
		aDebStr += ByteString::CreateFromInt32( pPortion->GetLen() );
		aDebStr += "(";
		aDebStr += ByteString::CreateFromInt32( pPortion->GetSize().Width() );
		aDebStr += ")";
		aDebStr += ";";
		n = n + pPortion->GetLen();
	}
	aDebStr += "\nGesamtlaenge: ";
	aDebStr += ByteString::CreateFromInt32( n );
	aDebStr += "\nSortiert nach Start:";
	for ( USHORT x = 0; x < pPPortion->GetNode()->GetCharAttribs().Count(); x++ )
	{
		EditCharAttrib* pCurAttrib = pPPortion->GetNode()->GetCharAttribs().GetAttribs().GetObject( x );
		aDebStr += "\nStart: ";
		aDebStr += ByteString::CreateFromInt32( pCurAttrib->GetStart() );
		aDebStr += "\tEnde: ";
		aDebStr += ByteString::CreateFromInt32( pCurAttrib->GetEnd() );
	}
	return aDebStr;
}

ByteString EditDbg::GetTextPortionInfo( TextPortionList& rPortions )
{
	ByteString aDebStr;
	for ( USHORT z = 0; z < rPortions.Count(); z++ )
	{
		TextPortion* pPortion = rPortions.GetObject( z );
		aDebStr += " ";
		aDebStr += ByteString::CreateFromInt32( pPortion->GetLen() );
		aDebStr += "(";
		aDebStr += ByteString::CreateFromInt32( pPortion->GetSize().Width() );
		aDebStr += ")";
		aDebStr += ";";
	}
	return aDebStr;
}

void EditDbg::ShowPortionData( ParaPortion* pPortion )
{
	ByteString aDebStr( GetPortionInfo( pPortion ) );
	InfoBox( 0, String( aDebStr, RTL_TEXTENCODING_ASCII_US )  ).Execute();
}


BOOL ParaPortion::DbgCheckTextPortions()
{
	// pruefen, ob Portionlaenge ok:
	USHORT nXLen = 0;
	for ( USHORT nPortion = 0; nPortion < aTextPortionList.Count(); nPortion++  )
		nXLen = nXLen + aTextPortionList[nPortion]->GetLen();
	return nXLen == pNode->Len() ? TRUE : FALSE;
}

BOOL CheckOrderedList( CharAttribArray& rAttribs, BOOL bStart )
{
	USHORT nPrev = 0;
	for ( USHORT nAttr = 0; nAttr < rAttribs.Count(); nAttr++ )
	{
		EditCharAttrib* pAttr = rAttribs[nAttr];
		USHORT nCur = bStart ? pAttr->GetStart() : pAttr->GetEnd();
		if ( nCur < nPrev )
			return FALSE;

		nPrev = nCur;
	}
	return TRUE;
}

#endif

