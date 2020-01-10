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

/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */

#include "flstitem.hxx"
#include "fontitem.hxx"
#include <svx/postitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/fhgtitem.hxx>
#include "fwdtitem.hxx"
#include <svx/udlnitem.hxx>
#include <svx/crsditem.hxx>
#include <svx/shdditem.hxx>
#include <svx/akrnitem.hxx>
#include <svx/wrlmitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/prszitem.hxx>
#include <svx/colritem.hxx>
#include <svx/cscoitem.hxx>
#include <svx/kernitem.hxx>
#include <svx/cmapitem.hxx>
#include <svx/escpitem.hxx>
#include <svx/langitem.hxx>
#include "nlbkitem.hxx"
#include <svx/nhypitem.hxx>
#include <svx/lcolitem.hxx>
#include <svx/blnkitem.hxx>
#include <svx/emphitem.hxx>
#include <svx/twolinesitem.hxx>

#include <svx/pbinitem.hxx>
#include <svx/sizeitem.hxx>
#include <svx/lrspitem.hxx>
#include <svx/ulspitem.hxx>
#include "prntitem.hxx"
#include "opaqitem.hxx"
#include "protitem.hxx"
#include <svx/shaditem.hxx>
#include <svx/boxitem.hxx>
#include <svx/brkitem.hxx>
#include <svx/keepitem.hxx>
#include "bolnitem.hxx"
#include <svx/brshitem.hxx>
#include <svx/lspcitem.hxx>
#include <svx/adjitem.hxx>
#include <svx/orphitem.hxx>
#include <svx/widwitem.hxx>
#include <svx/tstpitem.hxx>
#include <svx/pmdlitem.hxx>
#include <svx/spltitem.hxx>
#include <svx/hyznitem.hxx>
#include <svx/charscaleitem.hxx>
#include <svx/charrotateitem.hxx>
#include <svx/charreliefitem.hxx>
#include <svx/paravertalignitem.hxx>
#include <svx/forbiddenruleitem.hxx>
#include <svx/hngpnctitem.hxx>
#include <svx/scriptspaceitem.hxx>
#include <svx/frmdiritem.hxx>
#include "charhiddenitem.hxx"


#include <svtools/rtftoken.h>
#include <svtools/itempool.hxx>
#include <svtools/itemiter.hxx>

#include "svxrtf.hxx"


#define BRACELEFT	'{'
#define BRACERIGHT	'}'


// einige Hilfs-Funktionen
// char
inline const SvxEscapementItem& GetEscapement(const SfxItemSet& rSet,USHORT nId,BOOL bInP=TRUE)
	{ return (const SvxEscapementItem&)rSet.Get( nId,bInP); }
inline const SvxLineSpacingItem& GetLineSpacing(const SfxItemSet& rSet,USHORT nId,BOOL bInP=TRUE)
	{ return (const SvxLineSpacingItem&)rSet.Get( nId,bInP); }
// frm
inline const SvxLRSpaceItem& GetLRSpace(const SfxItemSet& rSet,USHORT nId,BOOL bInP=TRUE)
	{ return (const SvxLRSpaceItem&)rSet.Get( nId,bInP); }
inline const SvxULSpaceItem& GetULSpace(const SfxItemSet& rSet,USHORT nId,BOOL bInP=TRUE)
	{ return (const SvxULSpaceItem&)rSet.Get( nId,bInP); }

#define PARDID		((RTFPardAttrMapIds*)aPardMap.GetData())
#define PLAINID		((RTFPlainAttrMapIds*)aPlainMap.GetData())

void SvxRTFParser::SetScriptAttr( RTF_CharTypeDef eType, SfxItemSet& rSet,
									SfxPoolItem& rItem )
{
	const USHORT *pNormal = 0, *pCJK = 0, *pCTL = 0;
	const RTFPlainAttrMapIds* pIds = (RTFPlainAttrMapIds*)aPlainMap.GetData();
	switch( rItem.Which() )
	{
	case SID_ATTR_CHAR_FONT:
		pNormal = &pIds->nFont;
		pCJK = &pIds->nCJKFont;
		pCTL = &pIds->nCTLFont;
		break;

	case SID_ATTR_CHAR_FONTHEIGHT:
		pNormal = &pIds->nFontHeight;
		pCJK = &pIds->nCJKFontHeight;
		pCTL = &pIds->nCTLFontHeight;
		break;

	case SID_ATTR_CHAR_POSTURE:
		pNormal = &pIds->nPosture;
		pCJK = &pIds->nCJKPosture;
		pCTL = &pIds->nCTLPosture;
		break;

	case SID_ATTR_CHAR_WEIGHT:
		pNormal = &pIds->nWeight;
		pCJK = &pIds->nCJKWeight;
		pCTL = &pIds->nCTLWeight;
		break;

	case SID_ATTR_CHAR_LANGUAGE:
		pNormal = &pIds->nLanguage;
		pCJK = &pIds->nCJKLanguage;
		pCTL = &pIds->nCTLLanguage;
		break;

	case 0:
		// it exist no WhichId - don't set this item
		break;

	default:
	   rSet.Put( rItem );
	   break;
	}


	if( DOUBLEBYTE_CHARTYPE == eType )
	{
		if( bIsLeftToRightDef && *pCJK )
		{
			rItem.SetWhich( *pCJK );
			rSet.Put( rItem );
		}
	}
	else if( !bIsLeftToRightDef )
	{
		if( *pCTL )
		{
			rItem.SetWhich( *pCTL );
			rSet.Put( rItem );
		}
	}
	else
	{
		if( LOW_CHARTYPE == eType )
		{
			if( *pNormal )
			{
				rItem.SetWhich( *pNormal );
				rSet.Put( rItem );
			}
		}
		else if( HIGH_CHARTYPE == eType )
		{
			if( *pCTL )
			{
				rItem.SetWhich( *pCTL );
				rSet.Put( rItem );
			}
		}
		else
		{
			if( *pCJK )
			{
				rItem.SetWhich( *pCJK );
				rSet.Put( rItem );
			}
			if( *pCTL )
			{
				rItem.SetWhich( *pCTL );
				rSet.Put( rItem );
			}
			if( *pNormal )
			{
				rItem.SetWhich( *pNormal );
				rSet.Put( rItem );
			}
		}
	}
}

// --------------------

void SvxRTFParser::ReadAttr( int nToken, SfxItemSet* pSet )
{
	DBG_ASSERT( pSet, "Es muss ein SfxItemSet uebergeben werden!" );
	int bFirstToken = TRUE, bWeiter = TRUE;
	USHORT nStyleNo = 0; 		// default
	FontUnderline eUnderline;
	FontUnderline eOverline;
	FontEmphasisMark eEmphasis;
	bPardTokenRead = FALSE;
	RTF_CharTypeDef eCharType = NOTDEF_CHARTYPE;
	USHORT nFontAlign;

	int bChkStkPos = !bNewGroup && aAttrStack.Top();

	while( bWeiter && IsParserWorking() )			// solange bekannte Attribute erkannt werden
	{
		switch( nToken )
		{
		case RTF_PARD:
			RTFPardPlain( TRUE, &pSet );
            ResetPard();
			nStyleNo = 0;
			bPardTokenRead = TRUE;
			break;

		case RTF_PLAIN:
			RTFPardPlain( FALSE, &pSet );
			break;

		default:
			do {		// middle checked loop
				if( !bChkStkPos )
					break;

				SvxRTFItemStackType* pAkt = aAttrStack.Top();
				if( !pAkt || (pAkt->pSttNd->GetIdx() == pInsPos->GetNodeIdx() &&
					pAkt->nSttCnt == pInsPos->GetCntIdx() ))
					break;

				int nLastToken = GetStackPtr(-1)->nTokenId;
				if( RTF_PARD == nLastToken || RTF_PLAIN == nLastToken )
					break;

				if( pAkt->aAttrSet.Count() || pAkt->pChildList ||
					pAkt->nStyleNo )
				{
					// eine neue Gruppe aufmachen
					SvxRTFItemStackType* pNew = new SvxRTFItemStackType(
												*pAkt, *pInsPos, TRUE );
					pNew->SetRTFDefaults( GetRTFDefaults() );

					// alle bis hierher gueltigen Attribute "setzen"
					AttrGroupEnd();
					pAkt = aAttrStack.Top();  // can be changed after AttrGroupEnd!
					pNew->aAttrSet.SetParent( pAkt ? &pAkt->aAttrSet : 0 );

					aAttrStack.Push( pNew );
					pAkt = pNew;
				}
				else
					// diesen Eintrag als neuen weiterbenutzen
					pAkt->SetStartPos( *pInsPos );

				pSet = &pAkt->aAttrSet;
			} while( FALSE );

			switch( nToken )
			{
			case RTF_INTBL:
			case RTF_PAGEBB:
			case RTF_SBYS:
			case RTF_CS:
			case RTF_LS:
			case RTF_ILVL:
					UnknownAttrToken( nToken, pSet );
					break;

			case RTF_S:
				if( bIsInReadStyleTab )
				{
					if( !bFirstToken )
						SkipToken( -1 );
					bWeiter = FALSE;
				}
				else
				{
					nStyleNo = -1 == nTokenValue ? 0 : USHORT(nTokenValue);
					// setze am akt. auf dem AttrStack stehenden Style die
					// StyleNummer
					SvxRTFItemStackType* pAkt = aAttrStack.Top();
					if( !pAkt )
						break;

					pAkt->nStyleNo = USHORT( nStyleNo );

#if 0
// JP 05.09.95: zuruecksetzen der Style-Attribute fuehrt nur zu Problemen.
//				Es muss reichen, wenn das ueber pard/plain erfolgt
//	ansonsten Bugdoc 15304.rtf - nach nur "\pard" falscher Font !!

					SvxRTFStyleType* pStyle = aStyleTbl.Get( pAkt->nStyleNo );
					if( pStyle && pStyle->aAttrSet.Count() )
					{
						//JP 07.07.95:
						// alle Attribute, die in der Vorlage gesetzt werden
						// auf defaults setzen. In RTF werden die Attribute
						// der Vorlage danach ja wiederholt.
						// WICHTIG: Attribute die in der Vorlage definiert
						//			sind, werden zurueckgesetzt !!!!
						// pAkt->aAttrSet.Put( pStyle->aAttrSet );

						SfxItemIter aIter( pStyle->aAttrSet );
						SfxItemPool* pPool = pStyle->aAttrSet.GetPool();
						USHORT nWh = aIter.GetCurItem()->Which();
						while( TRUE )
						{
							pAkt->aAttrSet.Put( pPool->GetDefaultItem( nWh ));
							if( aIter.IsAtEnd() )
								break;
							nWh = aIter.NextItem()->Which();
						}
					}
#endif
				}
				break;

			case RTF_KEEP:
				if( PARDID->nSplit )
				{
					pSet->Put( SvxFmtSplitItem( FALSE, PARDID->nSplit ));
				}
				break;

			case RTF_KEEPN:
				if( PARDID->nKeep )
				{
					pSet->Put( SvxFmtKeepItem( TRUE, PARDID->nKeep ));
				}
				break;

			case RTF_LEVEL:
				if( PARDID->nOutlineLvl )
				{
					pSet->Put( SfxUInt16Item( PARDID->nOutlineLvl,
												(UINT16)nTokenValue ));
				}
				break;

			case RTF_QL:
				if( PARDID->nAdjust )
				{
					pSet->Put( SvxAdjustItem( SVX_ADJUST_LEFT, PARDID->nAdjust ));
				}
				break;
			case RTF_QR:
				if( PARDID->nAdjust )
				{
					pSet->Put( SvxAdjustItem( SVX_ADJUST_RIGHT, PARDID->nAdjust ));
				}
				break;
			case RTF_QJ:
				if( PARDID->nAdjust )
				{
					pSet->Put( SvxAdjustItem( SVX_ADJUST_BLOCK, PARDID->nAdjust ));
				}
				break;
			case RTF_QC:
				if( PARDID->nAdjust )
				{
					pSet->Put( SvxAdjustItem( SVX_ADJUST_CENTER, PARDID->nAdjust ));
				}
				break;

			case RTF_FI:
				if( PARDID->nLRSpace )
				{
					SvxLRSpaceItem aLR( GetLRSpace(*pSet, PARDID->nLRSpace ));
					USHORT nSz = 0;
					if( -1 != nTokenValue )
					{
						if( IsCalcValue() )
							CalcValue();
						nSz = USHORT(nTokenValue);
					}
					aLR.SetTxtFirstLineOfst( nSz );
					pSet->Put( aLR );
				}
				break;

			case RTF_LI:
			case RTF_LIN:
				if( PARDID->nLRSpace )
				{
					SvxLRSpaceItem aLR( GetLRSpace(*pSet, PARDID->nLRSpace ));
					USHORT nSz = 0;
					if( 0 < nTokenValue )
					{
						if( IsCalcValue() )
							CalcValue();
						nSz = USHORT(nTokenValue);
					}
					aLR.SetTxtLeft( nSz );
					pSet->Put( aLR );
				}
				break;

			case RTF_RI:
			case RTF_RIN:
				if( PARDID->nLRSpace )
				{
					SvxLRSpaceItem aLR( GetLRSpace(*pSet, PARDID->nLRSpace ));
					USHORT nSz = 0;
					if( 0 < nTokenValue )
					{
						if( IsCalcValue() )
							CalcValue();
						nSz = USHORT(nTokenValue);
					}
					aLR.SetRight( nSz );
					pSet->Put( aLR );
				}
				break;

			case RTF_SB:
				if( PARDID->nULSpace )
				{
					SvxULSpaceItem aUL( GetULSpace(*pSet, PARDID->nULSpace ));
					USHORT nSz = 0;
					if( 0 < nTokenValue )
					{
						if( IsCalcValue() )
							CalcValue();
						nSz = USHORT(nTokenValue);
					}
					aUL.SetUpper( nSz );
					pSet->Put( aUL );
				}
				break;

			case RTF_SA:
				if( PARDID->nULSpace )
				{
					SvxULSpaceItem aUL( GetULSpace(*pSet, PARDID->nULSpace ));
					USHORT nSz = 0;
					if( 0 < nTokenValue )
					{
						if( IsCalcValue() )
							CalcValue();
						nSz = USHORT(nTokenValue);
					}
					aUL.SetLower( nSz );
					pSet->Put( aUL );
				}
				break;

			case RTF_SLMULT:
				if( PARDID->nLinespacing && 1 == nTokenValue )
				{
					// dann wird auf mehrzeilig umgeschaltet!
					SvxLineSpacingItem aLSpace( GetLineSpacing( *pSet,
												PARDID->nLinespacing, FALSE ));

					// wieviel bekommt man aus dem LineHeight Wert heraus

					// Proportionale-Groesse:
					// D.H. das Verhaeltnis ergibt sich aus ( n / 240 ) Twips

					nTokenValue = 240;
					if( IsCalcValue() )
						CalcValue();

					nTokenValue = short( 100L * aLSpace.GetLineHeight()
											/ long( nTokenValue ) );

					if( nTokenValue > 200 )		// Datenwert fuer PropLnSp
						nTokenValue = 200;		// ist ein BYTE !!!

					aLSpace.SetPropLineSpace( (const BYTE)nTokenValue );
					aLSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;

					pSet->Put( aLSpace );
				}
				break;

			case RTF_SL:
				if( PARDID->nLinespacing )
				{
					// errechne das Verhaeltnis aus dem default Font zu der
					// Size Angabe. Der Abstand besteht aus der Zeilenhoehe
					// (100%) und dem Leerraum ueber der Zeile (20%).
					SvxLineSpacingItem aLSpace(0, PARDID->nLinespacing);

					nTokenValue = !bTokenHasValue ? 0 : nTokenValue;
					if (1000 == nTokenValue )
						nTokenValue = 240;

					SvxLineSpace eLnSpc;
					if (nTokenValue < 0)
					{
						eLnSpc = SVX_LINE_SPACE_FIX;
						nTokenValue = -nTokenValue;
					}
					else if (nTokenValue == 0)
                    {
                        //if \sl0 is used, the line spacing is automatically
                        //determined
                        eLnSpc = SVX_LINE_SPACE_AUTO;
                    }
                    else
						eLnSpc = SVX_LINE_SPACE_MIN;

					if (IsCalcValue())
						CalcValue();

                    if (eLnSpc != SVX_LINE_SPACE_AUTO)
					    aLSpace.SetLineHeight( (const USHORT)nTokenValue );

					aLSpace.GetLineSpaceRule() = eLnSpc;
					pSet->Put(aLSpace);
				}
				break;

			case RTF_NOCWRAP:
				if( PARDID->nForbRule )
				{
					pSet->Put( SvxForbiddenRuleItem( FALSE,
													PARDID->nForbRule ));
				}
				break;
			case RTF_NOOVERFLOW:
				if( PARDID->nHangPunct )
				{
					pSet->Put( SvxHangingPunctuationItem( FALSE,
													PARDID->nHangPunct ));
				}
				break;

			case RTF_ASPALPHA:
				if( PARDID->nScriptSpace )
				{
					pSet->Put( SvxScriptSpaceItem( TRUE,
												PARDID->nScriptSpace ));
				}
				break;

			case RTF_FAFIXED:
			case RTF_FAAUTO:	nFontAlign = SvxParaVertAlignItem::AUTOMATIC;
								goto SET_FONTALIGNMENT;
			case RTF_FAHANG:	nFontAlign = SvxParaVertAlignItem::TOP;
								goto SET_FONTALIGNMENT;
			case RTF_FAVAR:     nFontAlign = SvxParaVertAlignItem::BOTTOM;
								goto SET_FONTALIGNMENT;
			case RTF_FACENTER:  nFontAlign = SvxParaVertAlignItem::CENTER;
								goto SET_FONTALIGNMENT;
			case RTF_FAROMAN:   nFontAlign = SvxParaVertAlignItem::BASELINE;
								goto SET_FONTALIGNMENT;
SET_FONTALIGNMENT:
			if( PARDID->nFontAlign )
			{
				pSet->Put( SvxParaVertAlignItem( nFontAlign,
												PARDID->nFontAlign ));
			}
			break;

/*  */
			case RTF_B:
			case RTF_AB:
				if( IsAttrSttPos() )	// nicht im Textfluss ?
				{

					SvxWeightItem aTmpItem(
									nTokenValue ? WEIGHT_BOLD : WEIGHT_NORMAL,
									SID_ATTR_CHAR_WEIGHT );
					SetScriptAttr( eCharType, *pSet, aTmpItem);
				}
				break;

			case RTF_CAPS:
			case RTF_SCAPS:
				if( PLAINID->nCaseMap &&
					IsAttrSttPos() )		// nicht im Textfluss ?
				{
					SvxCaseMap eCaseMap;
					if( !nTokenValue )
						eCaseMap = SVX_CASEMAP_NOT_MAPPED;
					else if( RTF_CAPS == nToken )
						eCaseMap = SVX_CASEMAP_VERSALIEN;
					else
						eCaseMap = SVX_CASEMAP_KAPITAELCHEN;

					pSet->Put( SvxCaseMapItem( eCaseMap, PLAINID->nCaseMap ));
				}
				break;

			case RTF_DN:
			case RTF_SUB:
				if( PLAINID->nEscapement )
				{
					const USHORT nEsc = PLAINID->nEscapement;
					if( -1 == nTokenValue || RTF_SUB == nToken )
						nTokenValue = 6;
					if( IsCalcValue() )
						CalcValue();
					const SvxEscapementItem& rOld = GetEscapement( *pSet, nEsc, FALSE );
					short nEs;
					BYTE nProp;
					if( DFLT_ESC_AUTO_SUPER == rOld.GetEsc() )
					{
						nEs = DFLT_ESC_AUTO_SUB;
						nProp = rOld.GetProp();
					}
					else
					{
						nEs = (short)-nTokenValue;
						nProp = (nToken == RTF_SUB) ? DFLT_ESC_PROP : 100;
					}
					pSet->Put( SvxEscapementItem( nEs, nProp, nEsc ));
				}
				break;

			case RTF_NOSUPERSUB:
				if( PLAINID->nEscapement )
				{
					const USHORT nEsc = PLAINID->nEscapement;
					pSet->Put( SvxEscapementItem( nEsc ));
				}
				break;

			case RTF_EXPND:
				if( PLAINID->nKering )
				{
					if( -1 == nTokenValue )
						nTokenValue = 0;
					else
						nTokenValue *= 5;
					if( IsCalcValue() )
						CalcValue();
					pSet->Put( SvxKerningItem( (short)nTokenValue, PLAINID->nKering ));
				}
				break;

			case RTF_KERNING:
				if( PLAINID->nAutoKerning )
				{
					if( -1 == nTokenValue )
						nTokenValue = 0;
					else
						nTokenValue *= 10;
					if( IsCalcValue() )
						CalcValue();
					pSet->Put( SvxAutoKernItem( 0 != nTokenValue,
												PLAINID->nAutoKerning ));
				}
				break;

			case RTF_EXPNDTW:
				if( PLAINID->nKering )
				{
					if( -1 == nTokenValue )
						nTokenValue = 0;
					if( IsCalcValue() )
						CalcValue();
					pSet->Put( SvxKerningItem( (short)nTokenValue, PLAINID->nKering ));
				}
				break;

			case RTF_F:
			case RTF_AF:
				{
					const Font& rSVFont = GetFont( USHORT(nTokenValue) );
					SvxFontItem aTmpItem( rSVFont.GetFamily(),
									rSVFont.GetName(), rSVFont.GetStyleName(),
									rSVFont.GetPitch(), rSVFont.GetCharSet(),
									SID_ATTR_CHAR_FONT );
					SetScriptAttr( eCharType, *pSet, aTmpItem );
					if( RTF_F == nToken )
					{
						SetEncoding( rSVFont.GetCharSet() );
						RereadLookahead();
					}
				}
				break;

			case RTF_FS:
			case RTF_AFS:
				{
					if( -1 == nTokenValue )
						nTokenValue = 240;
					else
						nTokenValue *= 10;
// #i66167# 
// for the SwRTFParser 'IsCalcValue' will be false and for the EditRTFParser
// the converiosn takes now place in EditRTFParser since for other reasons
// the wrong MapUnit might still be use there
//                   if( IsCalcValue() )
//                       CalcValue();
					SvxFontHeightItem aTmpItem(
							(const USHORT)nTokenValue, 100,
							SID_ATTR_CHAR_FONTHEIGHT );
					SetScriptAttr( eCharType, *pSet, aTmpItem );
				}
				break;

			case RTF_I:
			case RTF_AI:
				if( IsAttrSttPos() )		// nicht im Textfluss ?
				{
					SvxPostureItem aTmpItem(
							        nTokenValue ? ITALIC_NORMAL : ITALIC_NONE,
							        SID_ATTR_CHAR_POSTURE );
					SetScriptAttr( eCharType, *pSet, aTmpItem );
				}
				break;

			case RTF_OUTL:
				if( PLAINID->nContour &&
					IsAttrSttPos() )		// nicht im Textfluss ?
				{
					pSet->Put( SvxContourItem( nTokenValue ? TRUE : FALSE,
								PLAINID->nContour ));
				}
				break;

			case RTF_SHAD:
				if( PLAINID->nShadowed &&
					IsAttrSttPos() )		// nicht im Textfluss ?
				{
					pSet->Put( SvxShadowedItem( nTokenValue ? TRUE : FALSE,
								PLAINID->nShadowed ));
				}
				break;

			case RTF_STRIKE:
				if( PLAINID->nCrossedOut &&
					IsAttrSttPos() )		// nicht im Textfluss ?
				{
					pSet->Put( SvxCrossedOutItem(
						nTokenValue ? STRIKEOUT_SINGLE : STRIKEOUT_NONE,
						PLAINID->nCrossedOut ));
				}
				break;

			case RTF_STRIKED:
				if( PLAINID->nCrossedOut )		// nicht im Textfluss ?
				{
					pSet->Put( SvxCrossedOutItem(
						nTokenValue ? STRIKEOUT_DOUBLE : STRIKEOUT_NONE,
						PLAINID->nCrossedOut ));
				}
				break;

			case RTF_UL:
				if( !IsAttrSttPos() )
					break;
				eUnderline = nTokenValue ? UNDERLINE_SINGLE : UNDERLINE_NONE;
				goto ATTR_SETUNDERLINE;

			case RTF_ULD:
				eUnderline = UNDERLINE_DOTTED;
				goto ATTR_SETUNDERLINE;
			case RTF_ULDASH:
				eUnderline = UNDERLINE_DASH;
				goto ATTR_SETUNDERLINE;
			case RTF_ULDASHD:
				eUnderline = UNDERLINE_DASHDOT;
				goto ATTR_SETUNDERLINE;
			case RTF_ULDASHDD:
				eUnderline = UNDERLINE_DASHDOTDOT;
				goto ATTR_SETUNDERLINE;
			case RTF_ULDB:
				eUnderline = UNDERLINE_DOUBLE;
				goto ATTR_SETUNDERLINE;
			case RTF_ULNONE:
				eUnderline = UNDERLINE_NONE;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTH:
				eUnderline = UNDERLINE_BOLD;
				goto ATTR_SETUNDERLINE;
			case RTF_ULWAVE:
				eUnderline = UNDERLINE_WAVE;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTHD:
				eUnderline = UNDERLINE_BOLDDOTTED;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTHDASH:
				eUnderline = UNDERLINE_BOLDDASH;
				goto ATTR_SETUNDERLINE;
			case RTF_ULLDASH:
				eUnderline = UNDERLINE_LONGDASH;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTHLDASH:
				eUnderline = UNDERLINE_BOLDLONGDASH;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTHDASHD:
				eUnderline = UNDERLINE_BOLDDASHDOT;
				goto ATTR_SETUNDERLINE;
			case RTF_ULTHDASHDD:
				eUnderline = UNDERLINE_BOLDDASHDOTDOT;
				goto ATTR_SETUNDERLINE;
			case RTF_ULHWAVE:
				eUnderline = UNDERLINE_BOLDWAVE;
				goto ATTR_SETUNDERLINE;
			case RTF_ULULDBWAVE:
				eUnderline = UNDERLINE_DOUBLEWAVE;
				goto ATTR_SETUNDERLINE;

			case RTF_ULW:
				eUnderline = UNDERLINE_SINGLE;

				if( PLAINID->nWordlineMode )
				{
					pSet->Put( SvxWordLineModeItem( TRUE, PLAINID->nWordlineMode ));
				}
				goto ATTR_SETUNDERLINE;

ATTR_SETUNDERLINE:
				if( PLAINID->nUnderline )
				{
					pSet->Put( SvxUnderlineItem( eUnderline, PLAINID->nUnderline ));
				}
				break;

			case RTF_ULC:
				if( PLAINID->nUnderline )
				{
					SvxUnderlineItem aUL( UNDERLINE_SINGLE, PLAINID->nUnderline );
					const SfxPoolItem* pItem;
					if( SFX_ITEM_SET == pSet->GetItemState(
						PLAINID->nUnderline, FALSE, &pItem ) )
					{
						// is switched off ?
						if( UNDERLINE_NONE ==
							((SvxUnderlineItem*)pItem)->GetLineStyle() )
							break;
						aUL = *(SvxUnderlineItem*)pItem;
					}
					else
						aUL = (const SvxUnderlineItem&)pSet->Get( PLAINID->nUnderline, FALSE );

					if( UNDERLINE_NONE == aUL.GetLineStyle() )
						aUL.SetLineStyle( UNDERLINE_SINGLE );
					aUL.SetColor( GetColor( USHORT(nTokenValue) ));
					pSet->Put( aUL );
				}
				break;

			case RTF_OL:
				if( !IsAttrSttPos() )
					break;
				eOverline = nTokenValue ? UNDERLINE_SINGLE : UNDERLINE_NONE;
				goto ATTR_SETOVERLINE;

			case RTF_OLD:
				eOverline = UNDERLINE_DOTTED;
				goto ATTR_SETOVERLINE;
			case RTF_OLDASH:
				eOverline = UNDERLINE_DASH;
				goto ATTR_SETOVERLINE;
			case RTF_OLDASHD:
				eOverline = UNDERLINE_DASHDOT;
				goto ATTR_SETOVERLINE;
			case RTF_OLDASHDD:
				eOverline = UNDERLINE_DASHDOTDOT;
				goto ATTR_SETOVERLINE;
			case RTF_OLDB:
				eOverline = UNDERLINE_DOUBLE;
				goto ATTR_SETOVERLINE;
			case RTF_OLNONE:
				eOverline = UNDERLINE_NONE;
				goto ATTR_SETOVERLINE;
			case RTF_OLTH:
				eOverline = UNDERLINE_BOLD;
				goto ATTR_SETOVERLINE;
			case RTF_OLWAVE:
				eOverline = UNDERLINE_WAVE;
				goto ATTR_SETOVERLINE;
			case RTF_OLTHD:
				eOverline = UNDERLINE_BOLDDOTTED;
				goto ATTR_SETOVERLINE;
			case RTF_OLTHDASH:
				eOverline = UNDERLINE_BOLDDASH;
				goto ATTR_SETOVERLINE;
			case RTF_OLLDASH:
				eOverline = UNDERLINE_LONGDASH;
				goto ATTR_SETOVERLINE;
			case RTF_OLTHLDASH:
				eOverline = UNDERLINE_BOLDLONGDASH;
				goto ATTR_SETOVERLINE;
			case RTF_OLTHDASHD:
				eOverline = UNDERLINE_BOLDDASHDOT;
				goto ATTR_SETOVERLINE;
			case RTF_OLTHDASHDD:
				eOverline = UNDERLINE_BOLDDASHDOTDOT;
				goto ATTR_SETOVERLINE;
			case RTF_OLHWAVE:
				eOverline = UNDERLINE_BOLDWAVE;
				goto ATTR_SETOVERLINE;
			case RTF_OLOLDBWAVE:
				eOverline = UNDERLINE_DOUBLEWAVE;
				goto ATTR_SETOVERLINE;

			case RTF_OLW:
				eOverline = UNDERLINE_SINGLE;

				if( PLAINID->nWordlineMode )
				{
					pSet->Put( SvxWordLineModeItem( TRUE, PLAINID->nWordlineMode ));
				}
				goto ATTR_SETOVERLINE;

ATTR_SETOVERLINE:
				if( PLAINID->nUnderline )
				{
					pSet->Put( SvxOverlineItem( eOverline, PLAINID->nOverline ));
				}
				break;

			case RTF_OLC:
				if( PLAINID->nOverline )
				{
					SvxOverlineItem aOL( UNDERLINE_SINGLE, PLAINID->nOverline );
					const SfxPoolItem* pItem;
					if( SFX_ITEM_SET == pSet->GetItemState(
						PLAINID->nOverline, FALSE, &pItem ) )
					{
						// is switched off ?
						if( UNDERLINE_NONE ==
							((SvxOverlineItem*)pItem)->GetLineStyle() )
							break;
						aOL = *(SvxOverlineItem*)pItem;
					}
					else
						aOL = (const SvxOverlineItem&)pSet->Get( PLAINID->nUnderline, FALSE );

					if( UNDERLINE_NONE == aOL.GetLineStyle() )
						aOL.SetLineStyle( UNDERLINE_SINGLE );
					aOL.SetColor( GetColor( USHORT(nTokenValue) ));
					pSet->Put( aOL );
				}
				break;

			case RTF_UP:
			case RTF_SUPER:
				if( PLAINID->nEscapement )
				{
					const USHORT nEsc = PLAINID->nEscapement;
					if( -1 == nTokenValue || RTF_SUPER == nToken )
						nTokenValue = 6;
					if( IsCalcValue() )
						CalcValue();
					const SvxEscapementItem& rOld = GetEscapement( *pSet, nEsc, FALSE );
					short nEs;
					BYTE nProp;
					if( DFLT_ESC_AUTO_SUB == rOld.GetEsc() )
					{
						nEs = DFLT_ESC_AUTO_SUPER;
						nProp = rOld.GetProp();
					}
					else
					{
						nEs = (short)nTokenValue;
						nProp = (nToken == RTF_SUPER) ? DFLT_ESC_PROP : 100;
					}
					pSet->Put( SvxEscapementItem( nEs, nProp, nEsc ));
				}
				break;

			case RTF_CF:
				if( PLAINID->nColor )
				{
					pSet->Put( SvxColorItem( GetColor( USHORT(nTokenValue) ),
								PLAINID->nColor ));
				}
				break;
#if 0
			//#i12501# While cb is clearly documented in the rtf spec, word
            //doesn't accept it at all
			case RTF_CB:
				if( PLAINID->nBgColor )
				{
					pSet->Put( SvxBrushItem( GetColor( USHORT(nTokenValue) ),
								PLAINID->nBgColor ));
				}
				break;
#endif
			case RTF_LANG:
				if( PLAINID->nLanguage )
				{
					pSet->Put( SvxLanguageItem( (LanguageType)nTokenValue,
								PLAINID->nLanguage ));
				}
				break;

			case RTF_LANGFE:
				if( PLAINID->nCJKLanguage )
				{
					pSet->Put( SvxLanguageItem( (LanguageType)nTokenValue,
												PLAINID->nCJKLanguage ));
				}
				break;
			case RTF_ALANG:
				{
					SvxLanguageItem aTmpItem( (LanguageType)nTokenValue,
									SID_ATTR_CHAR_LANGUAGE );
					SetScriptAttr( eCharType, *pSet, aTmpItem );
				}
				break;

			case RTF_RTLCH:
                bIsLeftToRightDef = FALSE;
                break;
			case RTF_LTRCH:
                bIsLeftToRightDef = TRUE;
                break;
            case RTF_RTLPAR:
                if (PARDID->nDirection)
                {
                    pSet->Put(SvxFrameDirectionItem(FRMDIR_HORI_RIGHT_TOP,
                        PARDID->nDirection));
                }
                break;
            case RTF_LTRPAR:
                if (PARDID->nDirection)
                {
                    pSet->Put(SvxFrameDirectionItem(FRMDIR_HORI_LEFT_TOP,
                        PARDID->nDirection));
                }
                break;
			case RTF_LOCH:  	eCharType = LOW_CHARTYPE;			break;
			case RTF_HICH:  	eCharType = HIGH_CHARTYPE;			break;
			case RTF_DBCH:  	eCharType = DOUBLEBYTE_CHARTYPE;	break;


			case RTF_ACCNONE:
				eEmphasis = EMPHASISMARK_NONE;
				goto ATTR_SETEMPHASIS;
			case RTF_ACCDOT:
				eEmphasis = EMPHASISMARK_DOTS_ABOVE;
				goto ATTR_SETEMPHASIS;

			case RTF_ACCCOMMA:
				eEmphasis = EMPHASISMARK_SIDE_DOTS;
ATTR_SETEMPHASIS:
				if( PLAINID->nEmphasis )
				{
					pSet->Put( SvxEmphasisMarkItem( eEmphasis,
											   		PLAINID->nEmphasis ));
				}
				break;

			case RTF_TWOINONE:
				if( PLAINID->nTwoLines )
				{
					sal_Unicode cStt, cEnd;
					switch ( nTokenValue )
					{
					case 1:	cStt = '(', cEnd = ')';	break;
					case 2:	cStt = '[', cEnd = ']';	break;
					case 3:	cStt = '<', cEnd = '>';	break;
					case 4:	cStt = '{', cEnd = '}';	break;
					default: cStt = 0, cEnd = 0; break;
					}

					pSet->Put( SvxTwoLinesItem( TRUE, cStt, cEnd,
											   		PLAINID->nTwoLines ));
				}
				break;

			case RTF_CHARSCALEX :
				if (PLAINID->nCharScaleX)
				{
                    //i21372
                    if (nTokenValue < 1 || nTokenValue > 600)
                        nTokenValue = 100;
					pSet->Put( SvxCharScaleWidthItem( USHORT(nTokenValue),
											   		PLAINID->nCharScaleX ));
				}
				break;

			case RTF_HORZVERT:
				if( PLAINID->nHorzVert )
				{
                    // RTF knows only 90deg
					pSet->Put( SvxCharRotateItem( 900, 1 == nTokenValue,
											   		PLAINID->nHorzVert ));
				}
				break;

			case RTF_EMBO:
				if (PLAINID->nRelief)
				{
					pSet->Put(SvxCharReliefItem(RELIEF_EMBOSSED,
                        PLAINID->nRelief));
				}
				break;
			case RTF_IMPR:
				if (PLAINID->nRelief)
				{
					pSet->Put(SvxCharReliefItem(RELIEF_ENGRAVED,
                        PLAINID->nRelief));
				}
				break;
			case RTF_V:
				if (PLAINID->nHidden)
                {
					pSet->Put(SvxCharHiddenItem(nTokenValue != 0,
                        PLAINID->nHidden));
                }
				break;
			case RTF_CHBGFDIAG:
			case RTF_CHBGDKVERT:
			case RTF_CHBGDKHORIZ:
			case RTF_CHBGVERT:
			case RTF_CHBGHORIZ:
			case RTF_CHBGDKFDIAG:
			case RTF_CHBGDCROSS:
			case RTF_CHBGCROSS:
			case RTF_CHBGBDIAG:
			case RTF_CHBGDKDCROSS:
			case RTF_CHBGDKCROSS:
			case RTF_CHBGDKBDIAG:
			case RTF_CHCBPAT:
			case RTF_CHCFPAT:
			case RTF_CHSHDNG:
				if( PLAINID->nBgColor )
					ReadBackgroundAttr( nToken, *pSet );
				break;


/*  */

			case BRACELEFT:
				{
					// teste auf Swg-Interne Tokens
                    bool bHandled = false;
					short nSkip = 0;
					if( RTF_IGNOREFLAG != GetNextToken())
						nSkip = -1;
					else if( (nToken = GetNextToken() ) & RTF_SWGDEFS )
					{
                        bHandled = true;
						switch( nToken )
						{
						case RTF_PGDSCNO:
						case RTF_PGBRK:
						case RTF_SOUTLVL:
							UnknownAttrToken( nToken, pSet );
							// ueberlese die schliessende Klammer
							break;

						case RTF_SWG_ESCPROP:
							{
								// prozentuale Veraenderung speichern !
								BYTE nProp = BYTE( nTokenValue / 100 );
								short nEsc = 0;
								if( 1 == ( nTokenValue % 100 ))
									// Erkennung unseres AutoFlags!
									nEsc = DFLT_ESC_AUTO_SUPER;

								if( PLAINID->nEscapement )
									pSet->Put( SvxEscapementItem( nEsc, nProp,
											   		PLAINID->nEscapement ));
							}
							break;

						case RTF_HYPHEN:
							{
								SvxHyphenZoneItem aHypenZone(
											(nTokenValue & 1) ? TRUE : FALSE,
												PARDID->nHyphenzone );
								aHypenZone.SetPageEnd(
											(nTokenValue & 2) ? TRUE : FALSE );

								if( PARDID->nHyphenzone &&
									RTF_HYPHLEAD == GetNextToken() &&
									RTF_HYPHTRAIL == GetNextToken() &&
									RTF_HYPHMAX == GetNextToken() )
								{
									aHypenZone.GetMinLead() =
										BYTE(GetStackPtr( -2 )->nTokenValue);
									aHypenZone.GetMinTrail() =
											BYTE(GetStackPtr( -1 )->nTokenValue);
									aHypenZone.GetMaxHyphens() =
											BYTE(nTokenValue);

									pSet->Put( aHypenZone );
								}
								else
									SkipGroup();		// ans Ende der Gruppe
							}
							break;

						case RTF_SHADOW:
							{
								int bSkip = TRUE;
								do {	// middle check loop
									SvxShadowLocation eSL = SvxShadowLocation( nTokenValue );
									if( RTF_SHDW_DIST != GetNextToken() )
										break;
									USHORT nDist = USHORT( nTokenValue );

									if( RTF_SHDW_STYLE != GetNextToken() )
										break;
									//! (pb) class Brush removed -> obsolete
									//! BrushStyle eStyle = BrushStyle( nTokenValue );

									if( RTF_SHDW_COL != GetNextToken() )
										break;
									USHORT nCol = USHORT( nTokenValue );

									if( RTF_SHDW_FCOL != GetNextToken() )
										break;
//									USHORT nFillCol = USHORT( nTokenValue );

									Color aColor = GetColor( nCol );

									if( PARDID->nShadow )
										pSet->Put( SvxShadowItem( PARDID->nShadow,
																  &aColor, nDist, eSL ) );

									bSkip = FALSE;
								} while( FALSE );

								if( bSkip )
									SkipGroup();		// ans Ende der Gruppe
							}
							break;

						default:
                            bHandled = false;
							if( (nToken & ~(0xff | RTF_SWGDEFS)) == RTF_TABSTOPDEF )
							{
								nToken = SkipToken( -2 );
								ReadTabAttr( nToken, *pSet );

                                /*
                                cmc: #i76140, he who consumed the { must consume the }
                                We rewound to a state of { being the current 
                                token so it is our responsibility to consume the } 
                                token if we consumed the {. We will not have consumed
                                the { if it belonged to our caller, i.e. if the { we
                                are handling is the "firsttoken" passed to us then
                                the *caller* must consume it, not us. Otherwise *we*
                                should consume it.
                                */
                                if (nToken == BRACELEFT && !bFirstToken)
                                {
                                    nToken = GetNextToken();
                                    DBG_ASSERT( nToken == BRACERIGHT, 
                                        "} did not follow { as expected\n");
                                }
							}
							else if( (nToken & ~(0xff| RTF_SWGDEFS)) == RTF_BRDRDEF)
							{
								nToken = SkipToken( -2 );
								ReadBorderAttr( nToken, *pSet );
							}
							else		// also kein Attribut mehr
								nSkip = -2;
							break;
						}

#if 1
                        /*
                        cmc: #i4727# / #i12713# Who owns this closing bracket?
                        If we read the opening one, we must read this one, if
                        other is counting the brackets so as to push/pop off
                        the correct environment then we will have pushed a new
                        environment for the start { of this, but will not see
                        the } and so is out of sync for the rest of the
                        document.
                        */
                        if (bHandled && !bFirstToken)
                            GetNextToken();
#endif
					}
					else
						nSkip = -2;

					if( nSkip )				// alles voellig unbekannt
					{
                        if (!bFirstToken)
						    --nSkip;	// BRACELEFT: ist das naechste Token
						SkipToken( nSkip );
						bWeiter = FALSE;
					}
				}
				break;
			default:
				if( (nToken & ~0xff ) == RTF_TABSTOPDEF )
					ReadTabAttr( nToken, *pSet );
				else if( (nToken & ~0xff ) == RTF_BRDRDEF )
					ReadBorderAttr( nToken, *pSet );
				else if( (nToken & ~0xff ) == RTF_SHADINGDEF )
					ReadBackgroundAttr( nToken, *pSet );
				else
				{
					// kenne das Token nicht also das Token "in den Parser zurueck"
					if( !bFirstToken )
						SkipToken( -1 );
					bWeiter = FALSE;
				}
			}
		}
		if( bWeiter )
		{
			nToken = GetNextToken();
		}
		bFirstToken = FALSE;
	}

/*
	// teste Attribute gegen ihre Styles
	if( IsChkStyleAttr() && pSet->Count() && !pInsPos->GetCntIdx() )
	{
		SvxRTFStyleType* pStyle = aStyleTbl.Get( nStyleNo );
		if( pStyle && pStyle->aAttrSet.Count() )
		{
			// alle Attribute, die schon vom Style definiert sind, aus dem
			// akt. Set entfernen
			const SfxPoolItem* pItem;
			SfxItemIter aIter( *pSet );
			USHORT nWhich = aIter.GetCurItem()->Which();
			while( TRUE )
			{
				if( SFX_ITEM_SET == pStyle->aAttrSet.GetItemState(
					nWhich, FALSE, &pItem ) && *pItem == *aIter.GetCurItem())
					pSet->ClearItem( nWhich );		// loeschen

				if( aIter.IsAtEnd() )
					break;
				nWhich = aIter.NextItem()->Which();
			}
		}
	}
*/
}

void SvxRTFParser::ReadTabAttr( int nToken, SfxItemSet& rSet )
{
	bool bMethodOwnsToken = false; // #i52542# patch from cmc.
// dann lese doch mal alle TabStops ein
	SvxTabStop aTabStop;
	SvxTabStopItem aAttr( 0, 0, SVX_TAB_ADJUST_DEFAULT, PARDID->nTabStop );
	int bWeiter = TRUE;
	do {
		switch( nToken )
		{
		case RTF_TB:		// BarTab ???
		case RTF_TX:
			{
				if( IsCalcValue() )
					CalcValue();
				aTabStop.GetTabPos() = nTokenValue;
				aAttr.Insert( aTabStop );
				aTabStop = SvxTabStop();	// alle Werte default
			}
			break;

		case RTF_TQL:
			aTabStop.GetAdjustment() = SVX_TAB_ADJUST_LEFT;
			break;
		case RTF_TQR:
			aTabStop.GetAdjustment() = SVX_TAB_ADJUST_RIGHT;
			break;
		case RTF_TQC:
			aTabStop.GetAdjustment() = SVX_TAB_ADJUST_CENTER;
			break;
		case RTF_TQDEC:
			aTabStop.GetAdjustment() = SVX_TAB_ADJUST_DECIMAL;
			break;

		case RTF_TLDOT:		aTabStop.GetFill() = '.';	break;
		case RTF_TLHYPH:	aTabStop.GetFill() = ' ';	break;
		case RTF_TLUL:		aTabStop.GetFill() = '_';	break;
		case RTF_TLTH:		aTabStop.GetFill() = '-';	break;
		case RTF_TLEQ:		aTabStop.GetFill() = '=';	break;

		case BRACELEFT:
			{
				// Swg - Kontrol BRACELEFT RTF_IGNOREFLAG RTF_TLSWG BRACERIGHT
				short nSkip = 0;
				if( RTF_IGNOREFLAG != GetNextToken() )
					nSkip = -1;
				else if( RTF_TLSWG != ( nToken = GetNextToken() ))
					nSkip = -2;
				else
				{
					aTabStop.GetDecimal() = BYTE(nTokenValue & 0xff);
					aTabStop.GetFill() = BYTE((nTokenValue >> 8) & 0xff);
					// ueberlese noch die schliessende Klammer
					if (bMethodOwnsToken)
						GetNextToken();
				}
				if( nSkip )
				{
					SkipToken( nSkip );		// Ignore wieder zurueck
					bWeiter = FALSE;
				}
			}
			break;

		default:
			bWeiter = FALSE;
		}
		if( bWeiter )
		{
			nToken = GetNextToken();
			bMethodOwnsToken = true;
		}
	} while( bWeiter );

	// mit Defaults aufuellen fehlt noch !!!
	rSet.Put( aAttr );
	SkipToken( -1 );
}

static void SetBorderLine( int nBorderTyp, SvxBoxItem& rItem,
							const SvxBorderLine& rBorder )
{
	switch( nBorderTyp )
	{
	case RTF_BOX:			// alle Stufen durchlaufen

	case RTF_BRDRT:
		rItem.SetLine( &rBorder, BOX_LINE_TOP );
		if( RTF_BOX != nBorderTyp )
			return;

	case RTF_BRDRB:
		rItem.SetLine( &rBorder, BOX_LINE_BOTTOM );
		if( RTF_BOX != nBorderTyp )
			return;

	case RTF_BRDRL:
		rItem.SetLine( &rBorder, BOX_LINE_LEFT );
		if( RTF_BOX != nBorderTyp )
			return;

	case RTF_BRDRR:
		rItem.SetLine( &rBorder, BOX_LINE_RIGHT );
		if( RTF_BOX != nBorderTyp )
			return;
	}
}

void SvxRTFParser::ReadBorderAttr( int nToken, SfxItemSet& rSet,
									int bTableDef )
{
	// dann lese doch mal das BoderAttribut ein
	SvxBoxItem aAttr( PARDID->nBox );
	const SfxPoolItem* pItem;
	if( SFX_ITEM_SET == rSet.GetItemState( PARDID->nBox, FALSE, &pItem ) )
		aAttr = *(SvxBoxItem*)pItem;

	SvxBorderLine aBrd( 0, DEF_LINE_WIDTH_0, 0, 0 );	// einfache Linien
	int bWeiter = TRUE, nBorderTyp = 0;

	do {
		switch( nToken )
		{
		case RTF_BOX:
		case RTF_BRDRT:
		case RTF_BRDRB:
		case RTF_BRDRL:
		case RTF_BRDRR:
			nBorderTyp = nToken;
			goto SETBORDER;

		case RTF_CLBRDRT:
			if( !bTableDef )
				break;
			nBorderTyp = RTF_BRDRT;
			goto SETBORDER;
		case RTF_CLBRDRB:
			if( !bTableDef )
				break;
			nBorderTyp = RTF_BRDRB;
			goto SETBORDER;
		case RTF_CLBRDRL:
			if( !bTableDef )
				break;
			nBorderTyp = RTF_BRDRL;
			goto SETBORDER;
		case RTF_CLBRDRR:
			if( !bTableDef )
				break;
			nBorderTyp = RTF_BRDRR;
			goto SETBORDER;

SETBORDER:
			{
				// auf defaults setzen
				aBrd.SetOutWidth( DEF_LINE_WIDTH_0 );
				aBrd.SetInWidth( 0 );
				aBrd.SetDistance( 0 );
				aBrd.SetColor( Color( COL_BLACK ) );
			}
			break;


// werden noch nicht ausgewertet
		case RTF_BRSP:
			{
				switch( nBorderTyp )
				{
				case RTF_BRDRB:
					aAttr.SetDistance( (USHORT)nTokenValue, BOX_LINE_BOTTOM );
					break;

				case RTF_BRDRT:
					aAttr.SetDistance( (USHORT)nTokenValue, BOX_LINE_TOP );
					break;

				case RTF_BRDRL:
					aAttr.SetDistance( (USHORT)nTokenValue, BOX_LINE_LEFT );
					break;

				case RTF_BRDRR:
					aAttr.SetDistance( (USHORT)nTokenValue, BOX_LINE_RIGHT );
					break;

				case RTF_BOX:
					aAttr.SetDistance( (USHORT)nTokenValue );
					break;
				}
			}
			break;

case RTF_BRDRBTW:
case RTF_BRDRBAR:			break;


		case RTF_BRDRCF:
			{
				aBrd.SetColor( GetColor( USHORT(nTokenValue) ) );
				SetBorderLine( nBorderTyp, aAttr, aBrd );
			}
			break;

		case RTF_BRDRTH:
			aBrd.SetOutWidth( DEF_LINE_WIDTH_1 );
			aBrd.SetInWidth( 0 );
			aBrd.SetDistance( 0 );
			goto SETBORDERLINE;

		case RTF_BRDRDB:
			aBrd.SetOutWidth( DEF_DOUBLE_LINE0_OUT );
			aBrd.SetInWidth( DEF_DOUBLE_LINE0_IN );
			aBrd.SetDistance( DEF_DOUBLE_LINE0_DIST );
			goto SETBORDERLINE;

		case RTF_BRDRSH:
			// schattierte Box
			{
				rSet.Put( SvxShadowItem( PARDID->nShadow, (Color*) 0, 60 /*3pt*/,
										SVX_SHADOW_BOTTOMRIGHT ) );
			}
			break;

		case RTF_BRDRW:
			if( -1 != nTokenValue )
			{
				// sollte es eine "dicke" Linie sein ?
				if( DEF_LINE_WIDTH_0 != aBrd.GetOutWidth() )
					nTokenValue *= 2;

				// eine Doppelline?
				if( aBrd.GetInWidth() )
				{
					// WinWord - Werte an StarOffice anpassen
					if( nTokenValue < DEF_LINE_WIDTH_1 - (DEF_LINE_WIDTH_1/10))
					{
						aBrd.SetOutWidth( DEF_DOUBLE_LINE0_OUT );
						aBrd.SetInWidth( DEF_DOUBLE_LINE0_IN );
						aBrd.SetDistance( DEF_DOUBLE_LINE0_DIST );
					}
					else
					if( nTokenValue < DEF_LINE_WIDTH_2 - (DEF_LINE_WIDTH_2/10))
					{
						aBrd.SetOutWidth( DEF_DOUBLE_LINE1_OUT );
						aBrd.SetInWidth( DEF_DOUBLE_LINE1_IN );
						aBrd.SetDistance( DEF_DOUBLE_LINE1_DIST );
					}
					else
					{
						aBrd.SetOutWidth( DEF_DOUBLE_LINE2_OUT );
						aBrd.SetInWidth( DEF_DOUBLE_LINE2_IN );
						aBrd.SetDistance( DEF_DOUBLE_LINE2_DIST );
					}
				}
				else
				{
					// WinWord - Werte an StarOffice anpassen
					if( nTokenValue < DEF_LINE_WIDTH_1 - (DEF_LINE_WIDTH_1/10))
						aBrd.SetOutWidth( DEF_LINE_WIDTH_0 );
					else
					if( nTokenValue < DEF_LINE_WIDTH_2 - (DEF_LINE_WIDTH_2/10))
						aBrd.SetOutWidth( DEF_LINE_WIDTH_1 );
					else
					if( nTokenValue < DEF_LINE_WIDTH_3 - (DEF_LINE_WIDTH_3/10))
						aBrd.SetOutWidth( DEF_LINE_WIDTH_2 );
					else
					if( nTokenValue < DEF_LINE_WIDTH_4 )
						aBrd.SetOutWidth( DEF_LINE_WIDTH_3 );
					else
						aBrd.SetOutWidth( DEF_LINE_WIDTH_4 );
				}
			}
			goto SETBORDERLINE;

		case RTF_BRDRS:
		case RTF_BRDRDOT:
		case RTF_BRDRHAIR:
		case RTF_BRDRDASH:
SETBORDERLINE:
			SetBorderLine( nBorderTyp, aAttr, aBrd );
			break;

		case BRACELEFT:
			{
				short nSkip = 0;
				if( RTF_IGNOREFLAG != GetNextToken() )
					nSkip = -1;
				else
				{
					int bSwgControl = TRUE, bFirstToken = TRUE;
					nToken = GetNextToken();
					do {
						switch( nToken )
						{
						case RTF_BRDBOX:
							aAttr.SetDistance( USHORT(nTokenValue) );
							break;

						case RTF_BRDRT:
						case RTF_BRDRB:
						case RTF_BRDRR:
						case RTF_BRDRL:
							nBorderTyp = nToken;
							bFirstToken = FALSE;
							if( RTF_BRDLINE_COL != GetNextToken() )
							{
								bSwgControl = FALSE;
								break;
							}
							aBrd.SetColor( GetColor( USHORT(nTokenValue) ));

							if( RTF_BRDLINE_IN != GetNextToken() )
							{
								bSwgControl = FALSE;
								break;
							}
							aBrd.SetInWidth( USHORT(nTokenValue));

							if( RTF_BRDLINE_OUT != GetNextToken() )
							{
								bSwgControl = FALSE;
								break;
							}
							aBrd.SetOutWidth( USHORT(nTokenValue));

							if( RTF_BRDLINE_DIST != GetNextToken() )
							{
								bSwgControl = FALSE;
								break;
							}
							aBrd.SetDistance( USHORT(nTokenValue));
							SetBorderLine( nBorderTyp, aAttr, aBrd );
							break;

						default:
							bSwgControl = FALSE;
							break;
						}

						if( bSwgControl )
						{
							nToken = GetNextToken();
							bFirstToken = FALSE;
						}
					} while( bSwgControl );

					// Ende der Swg-Gruppe
					// -> lese noch die schliessende Klammer
					if( BRACERIGHT == nToken )
						;
					else if( !bFirstToken )
					{
						// es ist ein Parser-Fehler, springe zum
						// Ende der Gruppe
						SkipGroup();
						// schliessende BRACERIGHT ueberspringen
						GetNextToken();
					}
					else
						nSkip = -2;
				}

				if( nSkip )
				{
					SkipToken( nSkip );		// Ignore wieder zurueck
					bWeiter = FALSE;
				}
			}
			break;

		default:
			bWeiter = (nToken & ~(0xff| RTF_SWGDEFS)) == RTF_BRDRDEF;
		}
		if( bWeiter )
			nToken = GetNextToken();
	} while( bWeiter );
	rSet.Put( aAttr );
	SkipToken( -1 );
}

inline ULONG CalcShading( ULONG nColor, ULONG nFillColor, BYTE nShading )
{
	nColor = (nColor * nShading) / 100;
	nFillColor = (nFillColor * ( 100 - nShading )) / 100;
	return nColor + nFillColor;
}

void SvxRTFParser::ReadBackgroundAttr( int nToken, SfxItemSet& rSet,
										int bTableDef )
{
	// dann lese doch mal das BoderAttribut ein
	int bWeiter = TRUE;
	USHORT nColor = USHRT_MAX, nFillColor = USHRT_MAX;
	BYTE nFillValue = 0;

	USHORT nWh = ( nToken & ~0xff ) == RTF_CHRFMT
					? PLAINID->nBgColor
					: PARDID->nBrush;

	do {
		switch( nToken )
		{
		case RTF_CLCBPAT:
		case RTF_CHCBPAT:
		case RTF_CBPAT:
			nFillColor = USHORT( nTokenValue );
			break;

		case RTF_CLCFPAT:
		case RTF_CHCFPAT:
		case RTF_CFPAT:
			nColor = USHORT( nTokenValue );
			break;

		case RTF_CLSHDNG:
		case RTF_CHSHDNG:
		case RTF_SHADING:
			nFillValue = (BYTE)( nTokenValue / 100 );
			break;

		case RTF_CLBGDKHOR:
		case RTF_CHBGDKHORIZ:
		case RTF_BGDKHORIZ:
		case RTF_CLBGDKVERT:
		case RTF_CHBGDKVERT:
		case RTF_BGDKVERT:
		case RTF_CLBGDKBDIAG:
		case RTF_CHBGDKBDIAG:
		case RTF_BGDKBDIAG:
		case RTF_CLBGDKFDIAG:
		case RTF_CHBGDKFDIAG:
		case RTF_BGDKFDIAG:
		case RTF_CLBGDKCROSS:
		case RTF_CHBGDKCROSS:
		case RTF_BGDKCROSS:
		case RTF_CLBGDKDCROSS:
		case RTF_CHBGDKDCROSS:
		case RTF_BGDKDCROSS:
			// dark -> 60%
			nFillValue = 60;
			break;

		case RTF_CLBGHORIZ:
		case RTF_CHBGHORIZ:
		case RTF_BGHORIZ:
		case RTF_CLBGVERT:
		case RTF_CHBGVERT:
		case RTF_BGVERT:
		case RTF_CLBGBDIAG:
		case RTF_CHBGBDIAG:
		case RTF_BGBDIAG:
		case RTF_CLBGFDIAG:
		case RTF_CHBGFDIAG:
		case RTF_BGFDIAG:
		case RTF_CLBGCROSS:
		case RTF_CHBGCROSS:
		case RTF_BGCROSS:
		case RTF_CLBGDCROSS:
		case RTF_CHBGDCROSS:
		case RTF_BGDCROSS:
			// light -> 20%
			nFillValue = 20;
			break;

		default:
			if( bTableDef )
				bWeiter = (nToken & ~(0xff | RTF_TABLEDEF) ) == RTF_SHADINGDEF;
			else
				bWeiter = (nToken & ~0xff) == RTF_SHADINGDEF;
		}
		if( bWeiter )
			nToken = GetNextToken();
	} while( bWeiter );

	Color aCol( COL_WHITE ), aFCol;
	if( !nFillValue )
	{
		// es wurde nur eine von beiden Farben angegeben oder kein BrushTyp
		if( USHRT_MAX != nFillColor )
		{
			nFillValue = 100;
			aCol = GetColor( nFillColor );
		}
		else if( USHRT_MAX != nColor )
			aFCol = GetColor( nColor );
	}
	else
	{
		if( USHRT_MAX != nColor )
			aCol = GetColor( nColor );
		else
			aCol = Color( COL_BLACK );

		if( USHRT_MAX != nFillColor )
			aFCol = GetColor( nFillColor );
		else
			aFCol = Color( COL_WHITE );
	}

	Color aColor;
	if( 0 == nFillValue || 100 == nFillValue )
		aColor = aCol;
	else
		aColor = Color(
			(BYTE)CalcShading( aCol.GetRed(), aFCol.GetRed(), nFillValue ),
			(BYTE)CalcShading( aCol.GetGreen(), aFCol.GetGreen(), nFillValue ),
			(BYTE)CalcShading( aCol.GetBlue(), aFCol.GetBlue(), nFillValue ) );

	rSet.Put( SvxBrushItem( aColor, nWh ) );
	SkipToken( -1 );
}


// pard / plain abarbeiten
void SvxRTFParser::RTFPardPlain( int bPard, SfxItemSet** ppSet )
{
	if( !bNewGroup && aAttrStack.Top() )	// nicht am Anfang einer neuen Gruppe
	{
		SvxRTFItemStackType* pAkt = aAttrStack.Top();

		int nLastToken = GetStackPtr(-1)->nTokenId;
		int bNewStkEntry = TRUE;
		if( RTF_PARD != nLastToken &&
			RTF_PLAIN != nLastToken &&
			BRACELEFT != nLastToken )
		{
			if( pAkt->aAttrSet.Count() || pAkt->pChildList || pAkt->nStyleNo )
			{
				// eine neue Gruppe aufmachen
				SvxRTFItemStackType* pNew = new SvxRTFItemStackType( *pAkt, *pInsPos, TRUE );
				pNew->SetRTFDefaults( GetRTFDefaults() );

				// alle bis hierher gueltigen Attribute "setzen"
				AttrGroupEnd();
				pAkt = aAttrStack.Top();  // can be changed after AttrGroupEnd!
				pNew->aAttrSet.SetParent( pAkt ? &pAkt->aAttrSet : 0 );
				aAttrStack.Push( pNew );
				pAkt = pNew;
			}
			else
			{
				// diesen Eintrag als neuen weiterbenutzen
				pAkt->SetStartPos( *pInsPos );
				bNewStkEntry = FALSE;
			}
		}

		// jetzt noch alle auf default zuruecksetzen
		if( bNewStkEntry &&
			( pAkt->aAttrSet.GetParent() || pAkt->aAttrSet.Count() ))
		{
			const SfxPoolItem *pItem, *pDef;
			const USHORT* pPtr;
			USHORT nCnt;
			const SfxItemSet* pDfltSet = &GetRTFDefaults();
			if( bPard )
			{
				pAkt->nStyleNo = 0;
				pPtr = aPardMap.GetData();
				nCnt = aPardMap.Count();
			}
			else
			{
				pPtr = aPlainMap.GetData();
				nCnt = aPlainMap.Count();
			}

			for( USHORT n = 0; n < nCnt; ++n, ++pPtr )
			{
				// Item gesetzt und unterschiedlich -> das Pooldefault setzen
				//JP 06.04.98: bei Items die nur SlotItems sind, darf nicht
				//				auf das Default zugefriffen werden. Diese
				//				werden gecleart
				if( !*pPtr )
					;
				else if( SFX_WHICH_MAX < *pPtr )
					pAkt->aAttrSet.ClearItem( *pPtr );
				else if( IsChkStyleAttr() )
					pAkt->aAttrSet.Put( pDfltSet->Get( *pPtr ) );
				else if( !pAkt->aAttrSet.GetParent() )
				{
					if( SFX_ITEM_SET ==
						pDfltSet->GetItemState( *pPtr, FALSE, &pDef ))
						pAkt->aAttrSet.Put( *pDef );
					else
						pAkt->aAttrSet.ClearItem( *pPtr );
				}
				else if( SFX_ITEM_SET == pAkt->aAttrSet.GetParent()->
							GetItemState( *pPtr, TRUE, &pItem ) &&
						*( pDef = &pDfltSet->Get( *pPtr )) != *pItem )
					pAkt->aAttrSet.Put( *pDef );
				else
				{
					if( SFX_ITEM_SET ==
						pDfltSet->GetItemState( *pPtr, FALSE, &pDef ))
						pAkt->aAttrSet.Put( *pDef );
					else
						pAkt->aAttrSet.ClearItem( *pPtr );
				}
			}
		}
		else if( bPard )
			pAkt->nStyleNo = 0;		// Style-Nummer zuruecksetzen

		*ppSet = &pAkt->aAttrSet;

		if (!bPard)
        {
            //Once we have a default font, then any text without a font specifier is
            //in the default font, and thus has the default font charset, otherwise
            //we can fall back to the ansicpg set codeset
            if (nDfltFont != -1)
            {
                const Font& rSVFont = GetFont(USHORT(nDfltFont));
                SetEncoding(rSVFont.GetCharSet());
            }
            else
			    SetEncoding(GetCodeSet());
        }
	}
}

void SvxRTFParser::SetDefault( int nToken, int nValue )
{
	if( !bNewDoc )
		return;

	SfxItemSet aTmp( *pAttrPool, aWhichMap.GetData() );
	BOOL bOldFlag = bIsLeftToRightDef;
	bIsLeftToRightDef = TRUE;
	switch( nToken )
	{
	case RTF_ADEFF:	bIsLeftToRightDef = FALSE;  // no break!
	case RTF_DEFF:
		{
			if( -1 == nValue )
				nValue = 0;
			const Font& rSVFont = GetFont( USHORT(nValue) );
			SvxFontItem aTmpItem(
								rSVFont.GetFamily(), rSVFont.GetName(),
								rSVFont.GetStyleName(),	rSVFont.GetPitch(),
								rSVFont.GetCharSet(), SID_ATTR_CHAR_FONT );
			SetScriptAttr( NOTDEF_CHARTYPE, aTmp, aTmpItem );
		}
		break;

	case RTF_ADEFLANG:	bIsLeftToRightDef = FALSE;  // no break!
	case RTF_DEFLANG:
		// default Language merken
		if( -1 != nValue )
		{
			SvxLanguageItem aTmpItem( (const LanguageType)nValue,
									    SID_ATTR_CHAR_LANGUAGE );
			SetScriptAttr( NOTDEF_CHARTYPE, aTmp, aTmpItem );
		}
		break;

	case RTF_DEFTAB:
		if( PARDID->nTabStop )
		{
			// RTF definiert 720 twips als default
			bIsSetDfltTab = TRUE;
			if( -1 == nValue || !nValue )
				nValue = 720;

			// wer keine Twips haben moechte ...
			if( IsCalcValue() )
			{
				nTokenValue = nValue;
				CalcValue();
				nValue = nTokenValue;
			}
#if 1
            /*
            cmc:
             This stuff looks a little hairy indeed, this should be totally
             unnecessary where default tabstops are understood. Just make one
             tabstop and stick the value in there, the first one is all that
             matters.

             e.g.

            SvxTabStopItem aNewTab(1, USHORT(nValue), SVX_TAB_ADJUST_DEFAULT,
                PARDID->nTabStop);
            ((SvxTabStop&)aNewTab[0]).GetAdjustment() = SVX_TAB_ADJUST_DEFAULT;


             It must exist as a foul hack to support somebody that does not
             have a true concept of default tabstops by making a tabsetting
             result from the default tabstop, creating a lot of them all at
             the default locations to give the effect of the first real
             default tabstop being in use just in case the receiving
             application doesn't do that for itself.
             */
#endif

			// Verhaeltnis der def. TabWidth / Tabs errechnen und
			// enstsprechend die neue Anzahl errechnen.
/*-----------------14.12.94 19:32-------------------
 ?? wie kommt man auf die 13 ??
--------------------------------------------------*/
			USHORT nAnzTabs = (SVX_TAB_DEFDIST * 13 ) / USHORT(nValue);
            /*
             cmc, make sure we have at least one, or all hell breaks loose in
             everybodies exporters, #i8247#
            */
            if (nAnzTabs < 1)
                nAnzTabs = 1;

			// wir wollen Defaulttabs
			SvxTabStopItem aNewTab( nAnzTabs, USHORT(nValue),
								SVX_TAB_ADJUST_DEFAULT, PARDID->nTabStop );
			while( nAnzTabs )
				((SvxTabStop&)aNewTab[ --nAnzTabs ]).GetAdjustment() = SVX_TAB_ADJUST_DEFAULT;

			pAttrPool->SetPoolDefaultItem( aNewTab );
		}
		break;
	}
	bIsLeftToRightDef = bOldFlag;

	if( aTmp.Count() )
	{
		SfxItemIter aIter( aTmp );
		const SfxPoolItem* pItem = aIter.GetCurItem();
		while( TRUE )
		{
			pAttrPool->SetPoolDefaultItem( *pItem );
			if( aIter.IsAtEnd() )
				break;
			pItem = aIter.NextItem();
		}
	}
}

// default: keine Umrechnung, alles bei Twips lassen.
void SvxRTFParser::CalcValue()
{
}

	// fuer Tokens, die im ReadAttr nicht ausgewertet werden
void SvxRTFParser::UnknownAttrToken( int, SfxItemSet* )
{
}

/* vi:set tabstop=4 shiftwidth=4 expandtab: */
