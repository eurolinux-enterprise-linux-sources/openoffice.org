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
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------
#include <tools/list.hxx>
#include "scitems.hxx"
#include <sfx2/bindings.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/dispatch.hxx>
#include <svx/charmap.hxx>
#include <svx/fontitem.hxx>
#include <svx/langitem.hxx>
#include <svx/scripttypeitem.hxx>
#include <svtools/itempool.hxx>
#include <svtools/itemset.hxx>
#include <svtools/cjkoptions.hxx>
#include <svtools/ctloptions.hxx>
#include <vcl/svapp.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/wrkwin.hxx>
#include <sfx2/request.hxx>
#include <sfx2/objsh.hxx>
#include <svtools/stritem.hxx>
#include <svtools/eitem.hxx>

#include <com/sun/star/i18n/TransliterationModules.hpp>

#include "viewutil.hxx"
#include "global.hxx"
#include "chgtrack.hxx"
#include "chgviset.hxx"
#include "markdata.hxx"

#include <svx/svxdlg.hxx> //CHINA001
#include <svx/dialogs.hrc> //CHINA001
// STATIC DATA -----------------------------------------------------------

//==================================================================

//	static
void ScViewUtil::PutItemScript( SfxItemSet& rShellSet, const SfxItemSet& rCoreSet,
								USHORT nWhichId, USHORT nScript )
{
	//	take the effective item from rCoreSet according to nScript
	//	and put in rShellSet under the (base) nWhichId

	SfxItemPool& rPool = *rShellSet.GetPool();
	SvxScriptSetItem aSetItem( rPool.GetSlotId(nWhichId), rPool );
	//	use PutExtended with eDefaultAs = SFX_ITEM_SET, so defaults from rCoreSet
	//	(document pool) are read and put into rShellSet (MessagePool)
	aSetItem.GetItemSet().PutExtended( rCoreSet, SFX_ITEM_DONTCARE, SFX_ITEM_SET );
	const SfxPoolItem* pI = aSetItem.GetItemOfScript( nScript );
	if (pI)
		rShellSet.Put( *pI, nWhichId );
	else
		rShellSet.InvalidateItem( nWhichId );
}

//	static
USHORT ScViewUtil::GetEffLanguage( ScDocument* pDoc, const ScAddress& rPos )
{
	//	used for thesaurus

	BYTE nScript = pDoc->GetScriptType( rPos.Col(), rPos.Row(), rPos.Tab() );
	USHORT nWhich = ( nScript == SCRIPTTYPE_ASIAN ) ? ATTR_CJK_FONT_LANGUAGE :
					( ( nScript == SCRIPTTYPE_COMPLEX ) ? ATTR_CTL_FONT_LANGUAGE : ATTR_FONT_LANGUAGE );
	const SfxPoolItem* pItem = pDoc->GetAttr( rPos.Col(), rPos.Row(), rPos.Tab(), nWhich);
	SvxLanguageItem* pLangIt = PTR_CAST( SvxLanguageItem, pItem );
	LanguageType eLnge;
	if (pLangIt)
	{
		eLnge = (LanguageType) pLangIt->GetValue();
		if (eLnge == LANGUAGE_DONTKNOW)					//! can this happen?
		{
			LanguageType eLatin, eCjk, eCtl;
			pDoc->GetLanguage( eLatin, eCjk, eCtl );
			eLnge = ( nScript == SCRIPTTYPE_ASIAN ) ? eCjk :
					( ( nScript == SCRIPTTYPE_COMPLEX ) ? eCtl : eLatin );
		}
	}
	else
		eLnge = LANGUAGE_ENGLISH_US;
	if ( eLnge == LANGUAGE_SYSTEM )
        eLnge = Application::GetSettings().GetLanguage();   // never use SYSTEM for spelling

	return eLnge;
}

//	static
sal_Int32 ScViewUtil::GetTransliterationType( USHORT nSlotID )
{
	sal_Int32 nType = 0;
	switch ( nSlotID )
	{
		case SID_TRANSLITERATE_UPPER:
			nType = com::sun::star::i18n::TransliterationModules_LOWERCASE_UPPERCASE;
			break;
		case SID_TRANSLITERATE_LOWER:
			nType = com::sun::star::i18n::TransliterationModules_UPPERCASE_LOWERCASE;
			break;
		case SID_TRANSLITERATE_HALFWIDTH:
			nType = com::sun::star::i18n::TransliterationModules_FULLWIDTH_HALFWIDTH;
			break;
		case SID_TRANSLITERATE_FULLWIDTH:
			nType = com::sun::star::i18n::TransliterationModules_HALFWIDTH_FULLWIDTH;
			break;
		case SID_TRANSLITERATE_HIRAGANA:
			nType = com::sun::star::i18n::TransliterationModules_KATAKANA_HIRAGANA;
			break;
		case SID_TRANSLITERATE_KATAGANA:
			nType = com::sun::star::i18n::TransliterationModules_HIRAGANA_KATAKANA;
			break;
	}
	return nType;
}

//	static
BOOL ScViewUtil::IsActionShown( const ScChangeAction& rAction,
								const ScChangeViewSettings& rSettings,
								ScDocument& rDocument )
{
	// abgelehnte werden durch eine invertierende akzeptierte Action dargestellt,
	// die Reihenfolge von ShowRejected/ShowAccepted ist deswegen wichtig

	if ( !rSettings.IsShowRejected() && rAction.IsRejecting() )
		return FALSE;

	if ( !rSettings.IsShowAccepted() && rAction.IsAccepted() && !rAction.IsRejecting() )
		return FALSE;

	if ( rSettings.HasAuthor() )
	{
		if ( rSettings.IsEveryoneButMe() )
		{
			//	GetUser() am ChangeTrack ist der aktuelle Benutzer
			ScChangeTrack* pTrack = rDocument.GetChangeTrack();
			if ( !pTrack || rAction.GetUser() == pTrack->GetUser() )
				return FALSE;
		}
		else if ( rAction.GetUser() != rSettings.GetTheAuthorToShow() )
			return FALSE;
	}

	if ( rSettings.HasComment() )
	{
		String aComStr=rAction.GetComment();
		aComStr.AppendAscii(RTL_CONSTASCII_STRINGPARAM( " (" ));
		rAction.GetDescription( aComStr, &rDocument );
		aComStr+=')';

		if(!rSettings.IsValidComment(&aComStr))
			return FALSE;
	}

	if ( rSettings.HasRange() )
		if ( !rSettings.GetTheRangeList().Intersects( rAction.GetBigRange().MakeRange() ) )
			return FALSE;

	if ( rSettings.HasDate() && rSettings.GetTheDateMode() != SCDM_NO_DATEMODE )
	{
		DateTime aDateTime = rAction.GetDateTime();
		const DateTime& rFirst = rSettings.GetTheFirstDateTime();
		const DateTime& rLast  = rSettings.GetTheLastDateTime();
		switch ( rSettings.GetTheDateMode() )
		{	// korrespondiert mit ScHighlightChgDlg::OKBtnHdl
			case SCDM_DATE_BEFORE:
				if ( aDateTime > rFirst )
					return FALSE;
				break;

			case SCDM_DATE_SINCE:
				if ( aDateTime < rFirst )
					return FALSE;
				break;

			case SCDM_DATE_EQUAL:
			case SCDM_DATE_BETWEEN:
				if ( aDateTime < rFirst || aDateTime > rLast )
					return FALSE;
				break;

			case SCDM_DATE_NOTEQUAL:
				if ( aDateTime >= rFirst && aDateTime <= rLast )
					return FALSE;
				break;

			case SCDM_DATE_SAVE:
				{
				ScChangeTrack* pTrack = rDocument.GetChangeTrack();
				if ( !pTrack || pTrack->GetLastSavedActionNumber() >=
						rAction.GetActionNumber() )
					return FALSE;
				}
				break;

            default:
            {
                // added to avoid warnings
            }
		}
	}

    if ( rSettings.HasActionRange() )
    {
        ULONG nAction = rAction.GetActionNumber();
        ULONG nFirstAction;
        ULONG nLastAction;
        rSettings.GetTheActionRange( nFirstAction, nLastAction );
        if ( nAction < nFirstAction || nAction > nLastAction )
        {
            return FALSE;
        }
    }

	return TRUE;
}

// static
void ScViewUtil::UnmarkFiltered( ScMarkData& rMark, ScDocument* pDoc )
{
    rMark.MarkToMulti();

    ScRange aMultiArea;
    rMark.GetMultiMarkArea( aMultiArea );
    SCCOL nStartCol = aMultiArea.aStart.Col();
    SCROW nStartRow = aMultiArea.aStart.Row();
    SCCOL nEndCol = aMultiArea.aEnd.Col();
    SCROW nEndRow = aMultiArea.aEnd.Row();

    bool bChanged = false;
    SCTAB nTabCount = pDoc->GetTableCount();
    for (SCTAB nTab=0; nTab<nTabCount; nTab++)
        if ( rMark.GetTableSelect(nTab ) )
        {
            ScCompressedArrayIterator<SCROW, BYTE> aIter(pDoc->GetRowFlagsArray(nTab), nStartRow, nEndRow);
            do
            {
    			if (*aIter & CR_FILTERED)
    			{
                    // use nStartCol/nEndCol, so the multi mark area isn't extended to all columns
                    // (visible in repaint for indentation)

                    rMark.SetMultiMarkArea( ScRange( nStartCol, aIter.GetRangeStart(), nTab,
                                                     nEndCol, aIter.GetRangeEnd(), nTab ), FALSE );
                    bChanged = true;
    			}
            }
            while (aIter.NextRange());
        }

    if ( bChanged && !rMark.HasAnyMultiMarks() )
        rMark.ResetMark();

    rMark.MarkToSimple();
}


// static
bool ScViewUtil::FitToUnfilteredRows( ScRange & rRange, const ScDocument * pDoc, size_t nRows )
{
    SCTAB nTab = rRange.aStart.Tab();
    bool bOneTabOnly = (nTab == rRange.aEnd.Tab());
    // Always fit the range on its first sheet.
    DBG_ASSERT( bOneTabOnly, "ScViewUtil::ExtendToUnfilteredRows: works only on one sheet");
    SCROW nStartRow = rRange.aStart.Row();
    // FillArrayForCondition() usually is the fastest to determine such a set
    // in one pass, even if the array isn't used but the last element.
    SCROW* pArr = new SCROW[nRows];
    size_t nCount = pDoc->GetRowFlagsArray( nTab).FillArrayForCondition(
            nStartRow, MAXROW, CR_FILTERED, 0, pArr, nRows);
    if (nCount)
        rRange.aEnd.SetRow( pArr[nCount-1]);
    delete [] pArr;
    return nCount == nRows && bOneTabOnly;
}


// static
bool ScViewUtil::HasFiltered( const ScRange& rRange, const ScDocument* pDoc )
{
    SCROW nStartRow = rRange.aStart.Row();
    SCROW nEndRow = rRange.aEnd.Row();
    for (SCTAB nTab=rRange.aStart.Tab(); nTab<=rRange.aEnd.Tab(); nTab++)
    {
        if ( pDoc->GetRowFlagsArray( nTab).HasCondition( nStartRow, nEndRow,
                CR_FILTERED, CR_FILTERED ) )
            return true;
    }

    return false;
}

// static
void ScViewUtil::HideDisabledSlot( SfxItemSet& rSet, SfxBindings& rBindings, USHORT nSlotId )
{
    SvtCJKOptions aCJKOptions;
    SvtCTLOptions aCTLOptions;
    bool bEnabled = true;

    switch( nSlotId )
    {
        case SID_CHINESE_CONVERSION:
        case SID_HANGUL_HANJA_CONVERSION:
            bEnabled = aCJKOptions.IsAnyEnabled();
        break;

        case SID_TRANSLITERATE_HALFWIDTH:
        case SID_TRANSLITERATE_FULLWIDTH:
        case SID_TRANSLITERATE_HIRAGANA:
        case SID_TRANSLITERATE_KATAGANA:
            bEnabled = aCJKOptions.IsChangeCaseMapEnabled();
        break;

        case SID_INSERT_RLM:
        case SID_INSERT_LRM:
        case SID_INSERT_ZWNBSP:
        case SID_INSERT_ZWSP:
            bEnabled = aCTLOptions.IsCTLFontEnabled();
        break;

        default:
            DBG_ERRORFILE( "ScViewUtil::HideDisabledSlot - unknown slot ID" );
            return;
    }

    rBindings.SetVisibleState( nSlotId, bEnabled );
    if( !bEnabled )
        rSet.DisableItem( nSlotId );
}

//==================================================================

BOOL ScViewUtil::ExecuteCharMap( const SvxFontItem& rOldFont,
								 SfxViewFrame& rFrame,
								 SvxFontItem& 		rNewFont,
								 String&			rString )
{
	BOOL bRet = FALSE;
	SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
	if(pFact)
	{
		SfxAllItemSet aSet( rFrame.GetObjectShell()->GetPool() );
		aSet.Put( SfxBoolItem( FN_PARAM_1, FALSE ) );
		aSet.Put( SvxFontItem( rOldFont.GetFamily(), rOldFont.GetFamilyName(), rOldFont.GetStyleName(), rOldFont.GetPitch(), rOldFont.GetCharSet(), aSet.GetPool()->GetWhich( SID_ATTR_CHAR_FONT ) ) );
		SfxAbstractDialog* pDlg = pFact->CreateSfxDialog( &rFrame.GetWindow(), aSet, rFrame.GetFrame()->GetFrameInterface(), RID_SVXDLG_CHARMAP );
		if ( pDlg->Execute() == RET_OK )
		{
			SFX_ITEMSET_ARG( pDlg->GetOutputItemSet(), pItem, SfxStringItem, SID_CHARMAP, FALSE );
			SFX_ITEMSET_ARG( pDlg->GetOutputItemSet(), pFontItem, SvxFontItem, SID_ATTR_CHAR_FONT, FALSE );
			if ( pItem )
				rString  = pItem->GetValue();
			if ( pFontItem )
				rNewFont = SvxFontItem( pFontItem->GetFamily(), pFontItem->GetFamilyName(), pFontItem->GetStyleName(), pFontItem->GetPitch(), pFontItem->GetCharSet(), rNewFont.Which() );
			bRet = TRUE;
		}
		delete pDlg;
	}
	return bRet;
}

bool ScViewUtil::IsFullScreen( SfxViewShell& rViewShell )
{
    SfxBindings&    rBindings       = rViewShell.GetViewFrame()->GetBindings();
    SfxPoolItem*    pItem           = 0;
    bool            bIsFullScreen   = false;

    if (rBindings.QueryState( SID_WIN_FULLSCREEN, pItem ) >= SFX_ITEM_DEFAULT)
        bIsFullScreen = static_cast< SfxBoolItem* >( pItem )->GetValue();
    return bIsFullScreen;
}

void ScViewUtil::SetFullScreen( SfxViewShell& rViewShell, bool bSet )
{
    if( IsFullScreen( rViewShell ) != bSet )
    {
        SfxBoolItem aItem( SID_WIN_FULLSCREEN, bSet );
        rViewShell.GetDispatcher()->Execute( SID_WIN_FULLSCREEN, SFX_CALLMODE_RECORD, &aItem, 0L );
    }
}

//------------------------------------------------------------------

ScUpdateRect::ScUpdateRect( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2 )
{
	PutInOrder( nX1, nX2 );
	PutInOrder( nY1, nY2 );

	nOldStartX = nX1;
	nOldStartY = nY1;
	nOldEndX = nX2;
	nOldEndY = nY2;
}

void ScUpdateRect::SetNew( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2 )
{
	PutInOrder( nX1, nX2 );
	PutInOrder( nY1, nY2 );

	nNewStartX = nX1;
	nNewStartY = nY1;
	nNewEndX = nX2;
	nNewEndY = nY2;
}

BOOL ScUpdateRect::GetDiff( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 )
{
	if ( nNewStartX == nOldStartX && nNewEndX == nOldEndX &&
		 nNewStartY == nOldStartY && nNewEndY == nOldEndY )
	{
		rX1 = nNewStartX;
		rY1 = nNewStartY;
		rX2 = nNewStartX;
		rY2 = nNewStartY;
		return FALSE;
	}

	rX1 = Min(nNewStartX,nOldStartX);
	rY1 = Min(nNewStartY,nOldStartY);
	rX2 = Max(nNewEndX,nOldEndX);
	rY2 = Max(nNewEndY,nOldEndY);

	if ( nNewStartX == nOldStartX && nNewEndX == nOldEndX )
	{
		if ( nNewStartY == nOldStartY )
		{
			rY1 = Min( nNewEndY, nOldEndY );
			rY2 = Max( nNewEndY, nOldEndY );
		}
		else if ( nNewEndY == nOldEndY )
		{
			rY1 = Min( nNewStartY, nOldStartY );
			rY2 = Max( nNewStartY, nOldStartY );
		}
	}
	else if ( nNewStartY == nOldStartY && nNewEndY == nOldEndY )
	{
		if ( nNewStartX == nOldStartX )
		{
			rX1 = Min( nNewEndX, nOldEndX );
			rX2 = Max( nNewEndX, nOldEndX );
		}
		else if ( nNewEndX == nOldEndX )
		{
			rX1 = Min( nNewStartX, nOldStartX );
			rX2 = Max( nNewStartX, nOldStartX );
		}
	}

	return TRUE;
}

#ifdef OLD_SELECTION_PAINT
BOOL ScUpdateRect::GetXorDiff( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2, BOOL& rCont )
{
    rCont = FALSE;

    if (nNewStartX == nOldStartX && nNewEndX == nOldEndX &&
        nNewStartY == nOldStartY && nNewEndY == nOldEndY)
    {
        rX1 = nNewStartX;
        rY1 = nNewStartY;
        rX2 = nNewStartX;
        rY2 = nNewStartY;
        return FALSE;
    }

    rX1 = Min(nNewStartX,nOldStartX);
    rY1 = Min(nNewStartY,nOldStartY);
    rX2 = Max(nNewEndX,nOldEndX);
    rY2 = Max(nNewEndY,nOldEndY);

    if (nNewStartX == nOldStartX && nNewEndX == nOldEndX)             // nur vertikal
    {
        if (nNewStartY == nOldStartY)
        {
            rY1 = Min( nNewEndY, nOldEndY ) + 1;
            rY2 = Max( nNewEndY, nOldEndY );
        }
        else if (nNewEndY == nOldEndY)
        {
            rY1 = Min( nNewStartY, nOldStartY );
            rY2 = Max( nNewStartY, nOldStartY ) - 1;
        }
        else
        {
            rY1 = Min( nNewStartY, nOldStartY );
            rY2 = Max( nNewStartY, nOldStartY ) - 1;
            rCont = TRUE;
            nContY1 = Min( nNewEndY, nOldEndY ) + 1;
            nContY2 = Max( nNewEndY, nOldEndY );
            nContX1 = rX1;
            nContX2 = rX2;
        }
    }
    else if (nNewStartY == nOldStartY && nNewEndY == nOldEndY)        // nur horizontal
    {
        if (nNewStartX == nOldStartX)
        {
            rX1 = Min( nNewEndX, nOldEndX ) + 1;
            rX2 = Max( nNewEndX, nOldEndX );
        }
        else if (nNewEndX == nOldEndX)
        {
            rX1 = Min( nNewStartX, nOldStartX );
            rX2 = Max( nNewStartX, nOldStartX ) - 1;
        }
        else
        {
            rX1 = Min( nNewStartX, nOldStartX );
            rX2 = Max( nNewStartX, nOldStartX ) - 1;
            rCont = TRUE;
            nContX1 = Min( nNewEndX, nOldEndX ) + 1;
            nContX2 = Max( nNewEndX, nOldEndX );
            nContY1 = rY1;
            nContY2 = rY2;
        }
    }
    else if (nNewEndX == nOldEndX && nNewEndY == nOldEndY)            // links oben
    {
        if ((nNewStartX<nOldStartX) == (nNewStartY<nOldStartY))
            rX1 = Min( nNewStartX, nOldStartX );
        else
            rX1 = Max( nNewStartX, nOldStartX );            // Ecke weglassen
        rX2 = nOldEndX;
        rY1 = Min( nNewStartY, nOldStartY );                // oben
        rY2 = Max( nNewStartY, nOldStartY ) - 1;
        rCont = TRUE;
        nContY1 = rY2+1;
        nContY2 = nOldEndY;
        nContX1 = Min( nNewStartX, nOldStartX );            // links
        nContX2 = Max( nNewStartX, nOldStartX ) - 1;
    }
    else if (nNewStartX == nOldStartX && nNewEndY == nOldEndY)        // rechts oben
    {
        if ((nNewEndX<nOldEndX) != (nNewStartY<nOldStartY))
            rX2 = Max( nNewEndX, nOldEndX );
        else
            rX2 = Min( nNewEndX, nOldEndX );                // Ecke weglassen
        rX1 = nOldStartX;
        rY1 = Min( nNewStartY, nOldStartY );                // oben
        rY2 = Max( nNewStartY, nOldStartY ) - 1;
        rCont = TRUE;
        nContY1 = rY2+1;
        nContY2 = nOldEndY;
        nContX1 = Min( nNewEndX, nOldEndX ) + 1;            // rechts
        nContX2 = Max( nNewEndX, nOldEndX );
    }
    else if (nNewEndX == nOldEndX && nNewStartY == nOldStartY)        // links unten
    {
        if ((nNewStartX<nOldStartX) != (nNewEndY<nOldEndY))
            rX1 = Min( nNewStartX, nOldStartX );
        else
            rX1 = Max( nNewStartX, nOldStartX );            // Ecke weglassen
        rX2 = nOldEndX;
        rY1 = Min( nNewEndY, nOldEndY ) + 1;                // unten
        rY2 = Max( nNewEndY, nOldEndY );
        rCont = TRUE;
        nContY1 = nOldStartY;
        nContY2 = rY1-1;
        nContX1 = Min( nNewStartX, nOldStartX );            // links
        nContX2 = Max( nNewStartX, nOldStartX ) - 1;
    }
    else if (nNewStartX == nOldStartX && nNewStartY == nOldStartY)    // rechts unten
    {
        if ((nNewEndX<nOldEndX) == (nNewEndY<nOldEndY))
            rX2 = Max( nNewEndX, nOldEndX );
        else
            rX2 = Min( nNewEndX, nOldEndX );                // Ecke weglassen
        rX1 = nOldStartX;
        rY1 = Min( nNewEndY, nOldEndY ) + 1;                // unten
        rY2 = Max( nNewEndY, nOldEndY );
        rCont = TRUE;
        nContY1 = nOldStartY;
        nContY2 = rY1-1;
        nContX1 = Min( nNewEndX, nOldEndX ) + 1;            // rechts
        nContX2 = Max( nNewEndX, nOldEndX );
    }
    else                                                                // Ueberschlag
    {
        rX1 = nOldStartX;
        rY1 = nOldStartY;
        rX2 = nOldEndX;
        rY2 = nOldEndY;
        rCont = TRUE;
        nContX1 = nNewStartX;
        nContY1 = nNewStartY;
        nContX2 = nNewEndX;
        nContY2 = nNewEndY;
    }

    return TRUE;
}

void ScUpdateRect::GetContDiff( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 )
{
    rX1 = nContX1;
    rY1 = nContY1;
    rX2 = nContX2;
    rY2 = nContY2;
}
#endif






