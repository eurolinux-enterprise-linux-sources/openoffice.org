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
#include <svtools/style.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/module.hxx>
#include <vcl/mnemonic.hxx>














#define _SVX_PARAGRPH_CXX   0




#include <svtools/languageoptions.hxx>
#include <pgrditem.hxx>
#include <svx/dialogs.hrc>
#include "paragrph.hrc"
#include "paragrph.hxx"
#include <svx/frmdiritem.hxx>

#include <svx/lspcitem.hxx>
#include <svx/adjitem.hxx>
#include <svx/orphitem.hxx>
#include <svx/widwitem.hxx>
#include <svx/tstpitem.hxx>
#include <svx/pmdlitem.hxx>
#include <svx/spltitem.hxx>
#include <svx/hyznitem.hxx>
#include <svx/ulspitem.hxx>
#include <svx/lrspitem.hxx>
#include <svx/brkitem.hxx>
#include <svx/keepitem.hxx>
#include "dlgutil.hxx"
#include <svx/dialmgr.hxx>
#include "htmlmode.hxx"
#include <svx/paravertalignitem.hxx>
#include <svtools/eitem.hxx> //add CHINA001
#include <sfx2/request.hxx> //add CHINA001
#include <svtools/intitem.hxx> //add CHINA001

// static ----------------------------------------------------------------

static USHORT pStdRanges[] =
{
	SID_ATTR_PARA_LINESPACE,		// 10033
	SID_ATTR_PARA_LINESPACE,
	SID_ATTR_LRSPACE,				// 10048 -
	SID_ATTR_ULSPACE,				// 10049
	SID_ATTR_PARA_REGISTER,			// 10413
	SID_ATTR_PARA_REGISTER,
	0
};

static USHORT pAlignRanges[] =
{
	SID_ATTR_PARA_ADJUST,			// 10027
	SID_ATTR_PARA_ADJUST,
	0
};

static USHORT pExtRanges[] =
{
	SID_ATTR_PARA_PAGEBREAK,		// 10037 -
	SID_ATTR_PARA_WIDOWS,			// 10041
	SID_ATTR_PARA_MODEL,			// 10065 -
	SID_ATTR_PARA_KEEP,				// 10066
	0
};

// define ----------------------------------------------------------------

#define MAX_DURCH 5670      // 10 cm ist sinnvoll als maximaler Durchschuss
							// laut BP
#define FIX_DIST_DEF 283    // Standard-Fix-Abstand 0,5cm

// enum ------------------------------------------------------------------

enum LineSpaceList
{
	LLINESPACE_1    = 0,
	LLINESPACE_15   = 1,
	LLINESPACE_2    = 2,
	LLINESPACE_PROP = 3,
	LLINESPACE_MIN  = 4,
	LLINESPACE_DURCH= 5,
	LLINESPACE_FIX 	= 6,
	LLINESPACE_END
};

// C-Funktion ------------------------------------------------------------

void SetLineSpace_Impl( SvxLineSpacingItem&, int, long lValue = 0 );

void SetLineSpace_Impl( SvxLineSpacingItem& rLineSpace,
						int eSpace, long lValue )
{
	switch ( eSpace )
	{
		case LLINESPACE_1:
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;
			rLineSpace.GetInterLineSpaceRule() = SVX_INTER_LINE_SPACE_OFF;
			break;

		case LLINESPACE_15:
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;
			rLineSpace.SetPropLineSpace( 150 );
			break;

		case LLINESPACE_2:
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;
			rLineSpace.SetPropLineSpace( 200 );
			break;

		case LLINESPACE_PROP:
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;
			rLineSpace.SetPropLineSpace( (BYTE)lValue );
			break;

		case LLINESPACE_MIN:
			rLineSpace.SetLineHeight( (USHORT)lValue );
			rLineSpace.GetInterLineSpaceRule() = SVX_INTER_LINE_SPACE_OFF;
			break;

		case LLINESPACE_DURCH:
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_AUTO;
			rLineSpace.SetInterLineSpace( (USHORT)lValue );
			break;

		case LLINESPACE_FIX:
			rLineSpace.SetLineHeight((USHORT)lValue);
			rLineSpace.GetLineSpaceRule() = SVX_LINE_SPACE_FIX;
			rLineSpace.GetInterLineSpaceRule() = SVX_INTER_LINE_SPACE_OFF;
		break;
	}
}


USHORT GetHtmlMode_Impl(const SfxItemSet& rSet)
{
	USHORT nHtmlMode = 0;
	const SfxPoolItem* pItem = 0;
	SfxObjectShell* pShell;
	if(SFX_ITEM_SET == rSet.GetItemState(SID_HTML_MODE, FALSE, &pItem) ||
		( 0 != (pShell = SfxObjectShell::Current()) &&
					0 != (pItem = pShell->GetItem(SID_HTML_MODE))))
	{
		nHtmlMode = ((SfxUInt16Item*)pItem)->GetValue();
	}
	return nHtmlMode;

}

// class SvxStdParagraphTabPage ------------------------------------------

IMPL_LINK( SvxStdParagraphTabPage, ELRLoseFocusHdl, Edit *, EMPTYARG )
{
//! if ( aLeftIndent.IsRelativeMode() )
//! 	return 0; //!!!

	SfxItemPool* pPool = GetItemSet().GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool" );
	FieldUnit eUnit =
		MapToFieldUnit( pPool->GetMetric( GetWhich( SID_ATTR_LRSPACE ) ) );

	sal_Int64 nL = aLeftIndent.Denormalize( aLeftIndent.GetValue( eUnit ) );
	sal_Int64 nR = aRightIndent.Denormalize( aRightIndent.GetValue( eUnit ) );
	String aTmp = aFLineIndent.GetText();

	// Erstzeilen Einzug
    if( aLeftIndent.GetMin() < 0 )
		aFLineIndent.SetMin( -99999, FUNIT_MM );
	else
		aFLineIndent.SetMin( aFLineIndent.Normalize( -nL ), eUnit );

	// Check nur fuer konkrete Breite (Shell)
	sal_Int64 nTmp = nWidth - nL - nR - MM50;
	aFLineIndent.SetMax( aFLineIndent.Normalize( nTmp ), eUnit );

	if ( !aTmp.Len() )
		aFLineIndent.SetEmptyFieldValue();
	// Maximum Links Rechts
	aTmp = aLeftIndent.GetText();
	nTmp = nWidth - nR - MM50;
	aLeftIndent.SetMax( aLeftIndent.Normalize( nTmp ), eUnit );

	if ( !aTmp.Len() )
		aLeftIndent.SetEmptyFieldValue();
	aTmp = aRightIndent.GetText();
	nTmp = nWidth - nL - MM50;
	aRightIndent.SetMax( aRightIndent.Normalize( nTmp ), eUnit );

	if ( !aTmp.Len() )
		aRightIndent.SetEmptyFieldValue();
	return 0;
}

// -----------------------------------------------------------------------

SfxTabPage* SvxStdParagraphTabPage::Create( Window* pParent,
											const SfxItemSet& rSet)
{
	return new SvxStdParagraphTabPage( pParent, rSet );
}

// -----------------------------------------------------------------------

BOOL SvxStdParagraphTabPage::FillItemSet( SfxItemSet& rOutSet )
{
	SfxItemState eState = SFX_ITEM_UNKNOWN;
	const SfxPoolItem* pOld = 0;
	SfxItemPool* pPool = rOutSet.GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool" );

	BOOL bModified = FALSE;
	USHORT nWhich;
	USHORT nPos = aLineDist.GetSelectEntryPos();

	if ( LISTBOX_ENTRY_NOTFOUND != nPos &&
		 ( nPos != aLineDist.GetSavedValue() ||
		   aLineDistAtPercentBox.IsValueModified() ||
		   aLineDistAtMetricBox.IsValueModified() ) )
	{
		nWhich = GetWhich( SID_ATTR_PARA_LINESPACE );
		SfxMapUnit eUnit = pPool->GetMetric( nWhich );
		SvxLineSpacingItem aSpacing(
			(const SvxLineSpacingItem&)GetItemSet().Get( nWhich ) );

		switch ( nPos )
		{
			case LLINESPACE_1:
			case LLINESPACE_15:
			case LLINESPACE_2:
				SetLineSpace_Impl( aSpacing, nPos );
				break;

			case LLINESPACE_PROP:
				SetLineSpace_Impl( aSpacing, nPos,
								   static_cast<long>(aLineDistAtPercentBox.Denormalize(
								   aLineDistAtPercentBox.GetValue() )) );
				break;

			case LLINESPACE_MIN:
			case LLINESPACE_DURCH:
			case LLINESPACE_FIX:
				SetLineSpace_Impl( aSpacing, nPos,
					GetCoreValue( aLineDistAtMetricBox, eUnit ) );
			break;

			default:
				DBG_ERROR( "unbekannter Type fuer Zeilenabstand." );
				break;
		}
		eState = GetItemSet().GetItemState( nWhich );
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_LINESPACE );

		if ( !pOld || !( *(const SvxLineSpacingItem*)pOld == aSpacing ) ||
			 SFX_ITEM_DONTCARE == eState )
		{
			rOutSet.Put( aSpacing );
			bModified |= TRUE;
		}
	}

	if ( aTopDist.IsValueModified() || aBottomDist.IsValueModified() )
	{
		nWhich = GetWhich( SID_ATTR_ULSPACE );
		SfxMapUnit eUnit = pPool->GetMetric( nWhich );
		pOld = GetOldItem( rOutSet, SID_ATTR_ULSPACE );
		SvxULSpaceItem aMargin( nWhich );

		if ( bRelativeMode )
		{
			DBG_ASSERT( GetItemSet().GetParent(), "No ParentSet" );

			const SvxULSpaceItem& rOldItem =
				(const SvxULSpaceItem&)GetItemSet().GetParent()->Get( nWhich );

			if ( aTopDist.IsRelative() )
				aMargin.SetUpper( rOldItem.GetUpper(),
								  (USHORT)aTopDist.GetValue() );
			else
				aMargin.SetUpper( (USHORT)GetCoreValue( aTopDist, eUnit ) );

			if ( aBottomDist.IsRelative() )
				aMargin.SetLower( rOldItem.GetLower(),
								  (USHORT)aBottomDist.GetValue() );
			else
				aMargin.SetLower( (USHORT)GetCoreValue( aBottomDist, eUnit ) );

		}
		else
		{
			aMargin.SetUpper( (USHORT)GetCoreValue( aTopDist, eUnit ) );
			aMargin.SetLower( (USHORT)GetCoreValue( aBottomDist, eUnit ) );
		}
		eState = GetItemSet().GetItemState( nWhich );

		if ( !pOld || !( *(const SvxULSpaceItem*)pOld == aMargin ) ||
			 SFX_ITEM_DONTCARE == eState )
		{
			rOutSet.Put( aMargin );
			bModified |= TRUE;
		}
	}
	FASTBOOL bNullTab = FALSE;

	if ( aLeftIndent.IsValueModified() ||
		 aFLineIndent.IsValueModified() ||
		 aRightIndent.IsValueModified()
		 ||	 aAutoCB.GetSavedValue() != aAutoCB.IsChecked() )
	{
		nWhich = GetWhich( SID_ATTR_LRSPACE );
		SfxMapUnit eUnit = pPool->GetMetric( nWhich );
		SvxLRSpaceItem aMargin( nWhich );
		pOld = GetOldItem( rOutSet, SID_ATTR_LRSPACE );

		if ( bRelativeMode )
		{
			DBG_ASSERT( GetItemSet().GetParent(), "No ParentSet" );

			const SvxLRSpaceItem& rOldItem =
				(const SvxLRSpaceItem&)GetItemSet().GetParent()->Get( nWhich );

			if ( aLeftIndent.IsRelative() )
				aMargin.SetTxtLeft( rOldItem.GetTxtLeft(),
									(USHORT)aLeftIndent.GetValue() );
			else
				aMargin.SetTxtLeft( GetCoreValue( aLeftIndent, eUnit ) );

			if ( aRightIndent.IsRelative() )
				aMargin.SetRight( rOldItem.GetRight(),
								  (USHORT)aRightIndent.GetValue() );
			else
				aMargin.SetRight( GetCoreValue( aRightIndent, eUnit ) );

			if ( aFLineIndent.IsRelative() )
				aMargin.SetTxtFirstLineOfst( rOldItem.GetTxtFirstLineOfst(),
											 (USHORT)aFLineIndent.GetValue() );
			else
				aMargin.SetTxtFirstLineOfst(
					(USHORT)GetCoreValue( aFLineIndent, eUnit ) );
		}
		else
		{
			aMargin.SetTxtLeft( GetCoreValue( aLeftIndent, eUnit ) );
			aMargin.SetRight( GetCoreValue( aRightIndent, eUnit ) );
			aMargin.SetTxtFirstLineOfst(
				(USHORT)GetCoreValue( aFLineIndent, eUnit ) );
		}
		aMargin.SetAutoFirst(aAutoCB.IsChecked());
		if ( aMargin.GetTxtFirstLineOfst() < 0 )
			bNullTab = TRUE;
		eState = GetItemSet().GetItemState( nWhich );

		if ( !pOld || !( *(const SvxLRSpaceItem*)pOld == aMargin ) ||
			 SFX_ITEM_DONTCARE == eState )
		{
			rOutSet.Put( aMargin );
			bModified |= TRUE;
		}
	}

	if ( bNullTab )
	{
		MapUnit eUnit = (MapUnit)pPool->GetMetric( GetWhich( SID_ATTR_TABSTOP ) );
		if ( MAP_100TH_MM != eUnit )
		{

			// negativer Erstzeileneinzug -> ggf. Null Default-Tabstop setzen
            USHORT _nWhich = GetWhich( SID_ATTR_TABSTOP );
			const SfxItemSet& rInSet = GetItemSet();

            if ( rInSet.GetItemState( _nWhich ) >= SFX_ITEM_AVAILABLE )
			{
				const SvxTabStopItem& rTabItem =
                    (const SvxTabStopItem&)rInSet.Get( _nWhich );
				SvxTabStopItem aNullTab( rTabItem );
				SvxTabStop aNull( 0, SVX_TAB_ADJUST_DEFAULT );
				aNullTab.Insert( aNull );
				rOutSet.Put( aNullTab );
			}
		}
	}
	if( aRegisterCB.IsVisible())
	{
		const SfxBoolItem* pBoolItem = (SfxBoolItem*)GetOldItem(
							rOutSet, SID_ATTR_PARA_REGISTER);
		SfxBoolItem* pRegItem = (SfxBoolItem*)pBoolItem->Clone();
        USHORT _nWhich = GetWhich( SID_ATTR_PARA_REGISTER );
		BOOL bSet = pRegItem->GetValue();

		if(aRegisterCB.IsChecked() != bSet )
		{
			pRegItem->SetValue(!bSet);
			rOutSet.Put(*pRegItem);
			bModified |= TRUE;
		}
        else if ( SFX_ITEM_DEFAULT == GetItemSet().GetItemState( _nWhich, FALSE ) )
            rOutSet.ClearItem(_nWhich);
		delete pRegItem;
	}

	return bModified;
}

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::Reset( const SfxItemSet& rSet )
{
	SfxItemPool* pPool = rSet.GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool?" );
	String aEmpty;

	// Metrik einstellen
	FieldUnit eFUnit = GetModuleFieldUnit( &rSet );
	SetFieldUnit( aLeftIndent, eFUnit );
	SetFieldUnit( aRightIndent, eFUnit );
	SetFieldUnit( aFLineIndent, eFUnit );
	SetFieldUnit( aTopDist, eFUnit );
	SetFieldUnit( aBottomDist, eFUnit );
	SetFieldUnit( aLineDistAtMetricBox, eFUnit );

    USHORT _nWhich = GetWhich( SID_ATTR_LRSPACE );
    SfxItemState eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
	{
        SfxMapUnit eUnit = pPool->GetMetric( _nWhich );

		if ( bRelativeMode )
		{
			const SvxLRSpaceItem& rOldItem =
                (const SvxLRSpaceItem&)rSet.Get( _nWhich );

			if ( rOldItem.GetPropLeft() != 100 )
			{
				aLeftIndent.SetRelative( TRUE );
				aLeftIndent.SetValue( rOldItem.GetPropLeft() );
			}
			else
			{
				aLeftIndent.SetRelative();
				SetFieldUnit( aLeftIndent, eFUnit );
				SetMetricValue( aLeftIndent, rOldItem.GetTxtLeft(), eUnit );
            }

			if ( rOldItem.GetPropRight() != 100 )
			{
				aRightIndent.SetRelative( TRUE );
				aRightIndent.SetValue( rOldItem.GetPropRight() );
			}
			else
			{
				aRightIndent.SetRelative();
				SetFieldUnit( aRightIndent, eFUnit );
				SetMetricValue( aRightIndent, rOldItem.GetRight(), eUnit );
			}

			if ( rOldItem.GetPropTxtFirstLineOfst() != 100 )
			{
				aFLineIndent.SetRelative( TRUE );
				aFLineIndent.SetValue( rOldItem.GetPropTxtFirstLineOfst() );
			}
			else
			{
				aFLineIndent.SetRelative();
                aFLineIndent.SetMin(-9999);
				SetFieldUnit( aFLineIndent, eFUnit );
				SetMetricValue( aFLineIndent, rOldItem.GetTxtFirstLineOfst(),
								eUnit );
			}
			aAutoCB.Check(rOldItem.IsAutoFirst());
		}
		else
		{
			const SvxLRSpaceItem& rSpace =
                (const SvxLRSpaceItem&)rSet.Get( _nWhich );

			SetMetricValue( aLeftIndent, rSpace.GetTxtLeft(), eUnit );
			SetMetricValue( aRightIndent, rSpace.GetRight(), eUnit );
			SetMetricValue( aFLineIndent, rSpace.GetTxtFirstLineOfst(), eUnit );
			aAutoCB.Check(rSpace.IsAutoFirst());
		}
		AutoHdl_Impl(&aAutoCB);
	}
	else
	{
		aLeftIndent.SetEmptyFieldValue();
		aRightIndent.SetEmptyFieldValue();
		aFLineIndent.SetEmptyFieldValue();
	}

    _nWhich = GetWhich( SID_ATTR_ULSPACE );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
	{
        SfxMapUnit eUnit = pPool->GetMetric( _nWhich );

		if ( bRelativeMode )
		{
			const SvxULSpaceItem& rOldItem =
                (const SvxULSpaceItem&)rSet.Get( _nWhich );

			if ( rOldItem.GetPropUpper() != 100 )
			{
				aTopDist.SetRelative( TRUE );
				aTopDist.SetValue( rOldItem.GetPropUpper() );
			}
			else
			{
				aTopDist.SetRelative();
				SetFieldUnit( aTopDist, eFUnit );
				SetMetricValue( aTopDist, rOldItem.GetUpper(), eUnit );
			}

			if ( rOldItem.GetPropLower() != 100 )
			{
				aBottomDist.SetRelative( TRUE );
				aBottomDist.SetValue( rOldItem.GetPropLower() );
			}
			else
			{
				aBottomDist.SetRelative();
				SetFieldUnit( aBottomDist, eFUnit );
				SetMetricValue( aBottomDist, rOldItem.GetLower(), eUnit );
			}
		}
		else
		{
			const SvxULSpaceItem& rTopMargin =
                (const SvxULSpaceItem&)rSet.Get( _nWhich );
			SetMetricValue( aTopDist, rTopMargin.GetUpper(), eUnit );
			SetMetricValue( aBottomDist, rTopMargin.GetLower(), eUnit );
		}
	}
	else
	{
		aTopDist.SetEmptyFieldValue();
		aBottomDist.SetEmptyFieldValue();
	}

    _nWhich = GetWhich( SID_ATTR_PARA_LINESPACE );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
        SetLineSpacing_Impl( (const SvxLineSpacingItem &)rSet.Get( _nWhich ) );
	else
		aLineDist.SetNoSelection();


    _nWhich = GetWhich( SID_ATTR_PARA_REGISTER );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
        aRegisterCB.Check( ((const SfxBoolItem &)rSet.Get( _nWhich )).GetValue());
	aRegisterCB.SaveValue();
	USHORT nHtmlMode = GetHtmlMode_Impl(rSet);
	if(nHtmlMode & HTMLMODE_ON)
	{
		aRegisterCB.Hide();
        aRegisterFL.Hide();
		aAutoCB.Hide();
		if(!(nHtmlMode & HTMLMODE_SOME_STYLES)) // IE oder SW
		{
			aRightLabel.Disable();
			aRightIndent.Disable();
			aTopDist.Disable();  //HTML3.2 und NS 3.0
			aBottomDist.Disable();
			if(!(nHtmlMode & HTMLMODE_FIRSTLINE)) //NS 3.0
			{
				aFLineIndent.Disable();
				aFLineLabel.Disable();
			}
		}
	}

	ELRLoseFocusHdl( NULL );
	aAutoCB.SaveValue();
	aLineDist.SaveValue();
}

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::EnableRelativeMode()
{
	DBG_ASSERT( GetItemSet().GetParent(), "RelativeMode, but no parent-set!" );

	aLeftIndent.EnableRelativeMode( 0, 999 );
	aFLineIndent.EnableRelativeMode( 0, 999 );
	aRightIndent.EnableRelativeMode( 0, 999 );
	aTopDist.EnableRelativeMode( 0, 999 );
	aBottomDist.EnableRelativeMode( 0, 999 );
	bRelativeMode = TRUE;
}

// -----------------------------------------------------------------------

int SvxStdParagraphTabPage::DeactivatePage( SfxItemSet* _pSet )
{
	ELRLoseFocusHdl( NULL );

    if ( _pSet )
        FillItemSet( *_pSet );
	return LEAVE_PAGE;
}

// -----------------------------------------------------------------------

SvxStdParagraphTabPage::SvxStdParagraphTabPage( Window* pParent,
												const SfxItemSet& rAttr ) :

	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_STD_PARAGRAPH ), rAttr ),

	aLeftLabel              ( this, SVX_RES( FT_LEFTINDENT ) ),
	aLeftIndent             ( this, SVX_RES( ED_LEFTINDENT ) ),
    aRightLabel             ( this, SVX_RES( FT_RIGHTINDENT ) ),
    aRightIndent            ( this, SVX_RES( ED_RIGHTINDENT ) ),
    
    aFLineLabel             ( this, SVX_RES( FT_FLINEINDENT ) ),
	aFLineIndent            ( this, SVX_RES( ED_FLINEINDENT ) ),
	aAutoCB                 ( this, SVX_RES( CB_AUTO ) ),
    aIndentFrm              ( this, SVX_RES( FL_INDENT ) ),
	aTopLabel               ( this, SVX_RES( FT_TOPDIST ) ),
	aTopDist                ( this, SVX_RES( ED_TOPDIST ) ),
	aBottomLabel            ( this, SVX_RES( FT_BOTTOMDIST ) ),
	aBottomDist             ( this, SVX_RES( ED_BOTTOMDIST ) ),
    aDistFrm                ( this, SVX_RES( FL_DIST ) ),
	aLineDist               ( this, SVX_RES( LB_LINEDIST ) ),
	aLineDistAtLabel        ( this, SVX_RES( FT_LINEDIST ) ),
	aLineDistAtPercentBox   ( this, SVX_RES( ED_LINEDISTPERCENT ) ),
	aLineDistAtMetricBox    ( this, SVX_RES( ED_LINEDISTMETRIC ) ),
    aLineDistFrm            ( this, SVX_RES( FL_LINEDIST ) ),
    sAbsDist                ( SVX_RES(ST_LINEDIST_ABS) ),
    aExampleWin             ( this, SVX_RES( WN_EXAMPLE ) ),
	aRegisterCB             ( this, SVX_RES( CB_REGISTER ) ),
    aRegisterFL             ( this, SVX_RES( FL_REGISTER ) ),
    pActLineDistFld ( &aLineDistAtPercentBox ),
    nAbst           ( MAX_DURCH ),
    nWidth          ( 11905 /*567 * 50*/ ),
    nMinFixDist(0L),

    bRelativeMode   ( FALSE ),
    bNegativeIndents(FALSE)

{
	// diese Page braucht ExchangeSupport
	SetExchangeSupport();

	aLineDistAtMetricBox.Hide();
    FreeResource();
	Init_Impl();
    aFLineIndent.SetMin(-9999);    // wird default auf 0 gesetzt
}


// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::EnableNegativeMode()
{
    aLeftIndent.SetMin(-9999);
    aRightIndent.SetMin(-9999);
    aRightIndent.EnableNegativeMode();
    aLeftIndent.EnableNegativeMode();
    bNegativeIndents = TRUE;
}

// -----------------------------------------------------------------------

USHORT* SvxStdParagraphTabPage::GetRanges()
{
	return pStdRanges;
}

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::SetLineSpacing_Impl
(
	const SvxLineSpacingItem &rAttr
)
{
	SfxMapUnit eUnit = GetItemSet().GetPool()->GetMetric( rAttr.Which() );

	switch( rAttr.GetLineSpaceRule() )
	{
		case SVX_LINE_SPACE_AUTO:
		{
			SvxInterLineSpace eInter = rAttr.GetInterLineSpaceRule();

			switch( eInter )
			{
				// Default einzeilig
				case SVX_INTER_LINE_SPACE_OFF:
					aLineDist.SelectEntryPos( LLINESPACE_1 );
					break;

				// Default einzeilig
				case SVX_INTER_LINE_SPACE_PROP:
					if ( 100 == rAttr.GetPropLineSpace() )
					{
						aLineDist.SelectEntryPos( LLINESPACE_1 );
						break;
					}
					// 1.5zeilig
					if ( 150 == rAttr.GetPropLineSpace() )
					{
						aLineDist.SelectEntryPos( LLINESPACE_15 );
						break;
					}
					// 2zeilig
					if ( 200 == rAttr.GetPropLineSpace() )
					{
						aLineDist.SelectEntryPos( LLINESPACE_2 );
						break;
					}
					// eingestellter Prozentwert
					aLineDistAtPercentBox.
						SetValue( aLineDistAtPercentBox.Normalize(
										rAttr.GetPropLineSpace() ) );
					aLineDist.SelectEntryPos( LLINESPACE_PROP );
					break;

				case SVX_INTER_LINE_SPACE_FIX:
					SetMetricValue( aLineDistAtMetricBox,
									rAttr.GetInterLineSpace(), eUnit );
					aLineDist.SelectEntryPos( LLINESPACE_DURCH );
					break;
                default: ;//prevent warning
			}
		}
		break;
		case SVX_LINE_SPACE_FIX:
			SetMetricValue(aLineDistAtMetricBox, rAttr.GetLineHeight(), eUnit);
			aLineDist.SelectEntryPos( LLINESPACE_FIX );
		break;

		case SVX_LINE_SPACE_MIN:
			SetMetricValue(aLineDistAtMetricBox, rAttr.GetLineHeight(), eUnit);
			aLineDist.SelectEntryPos( LLINESPACE_MIN );
			break;
        default: ;//prevent warning
	}
	LineDistHdl_Impl( &aLineDist );
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxStdParagraphTabPage, LineDistHdl_Impl, ListBox *, pBox )
{
	switch( pBox->GetSelectEntryPos() )
	{
		case LLINESPACE_1:
		case LLINESPACE_15:
		case LLINESPACE_2:
			aLineDistAtLabel.Enable(FALSE);
			pActLineDistFld->Enable(FALSE);
			pActLineDistFld->SetText( String() );
			break;

		case LLINESPACE_DURCH:
			// Setzen eines sinnvollen Defaults?
			// MS Begrenzen min(10, aPageSize)
			aLineDistAtPercentBox.Hide();
			pActLineDistFld = &aLineDistAtMetricBox;
            aLineDistAtMetricBox.SetMin(0);


			if ( !aLineDistAtMetricBox.GetText().Len() )
				aLineDistAtMetricBox.SetValue(
					aLineDistAtMetricBox.Normalize( 1 ) );
			aLineDistAtPercentBox.Hide();
			pActLineDistFld->Show();
			pActLineDistFld->Enable();
			aLineDistAtLabel.Enable();
			break;

		case LLINESPACE_MIN:
			aLineDistAtPercentBox.Hide();
			pActLineDistFld = &aLineDistAtMetricBox;
            aLineDistAtMetricBox.SetMin(0);

			if ( !aLineDistAtMetricBox.GetText().Len() )
				aLineDistAtMetricBox.SetValue(
					aLineDistAtMetricBox.Normalize( 10 ), FUNIT_TWIP );
			aLineDistAtPercentBox.Hide();
			pActLineDistFld->Show();
			pActLineDistFld->Enable();
			aLineDistAtLabel.Enable();
			break;

		case LLINESPACE_PROP:
			aLineDistAtMetricBox.Hide();
			pActLineDistFld = &aLineDistAtPercentBox;

			if ( !aLineDistAtPercentBox.GetText().Len() )
				aLineDistAtPercentBox.SetValue(
					aLineDistAtPercentBox.Normalize( 100 ), FUNIT_TWIP );
			aLineDistAtMetricBox.Hide();
			pActLineDistFld->Show();
			pActLineDistFld->Enable();
			aLineDistAtLabel.Enable();
			break;
		case LLINESPACE_FIX:
		{
			aLineDistAtPercentBox.Hide();
			pActLineDistFld = &aLineDistAtMetricBox;
			sal_Int64 nTemp = aLineDistAtMetricBox.GetValue();
			aLineDistAtMetricBox.SetMin(aLineDistAtMetricBox.Normalize(nMinFixDist), FUNIT_TWIP);

			// wurde der Wert beim SetMin veraendert, dann ist es Zeit
			// fuer den default
			if ( aLineDistAtMetricBox.GetValue() != nTemp )
				SetMetricValue( aLineDistAtMetricBox,
									FIX_DIST_DEF, SFX_MAPUNIT_TWIP ); // fix gibt's nur im Writer
			aLineDistAtPercentBox.Hide();
			pActLineDistFld->Show();
			pActLineDistFld->Enable();
			aLineDistAtLabel.Enable();
		}
		break;
	}
	UpdateExample_Impl( TRUE );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvxStdParagraphTabPage, ModifyHdl_Impl, SvxRelativeField *, EMPTYARG )
{
	UpdateExample_Impl();
	return 0;
}
IMPL_LINK_INLINE_END( SvxStdParagraphTabPage, ModifyHdl_Impl, SvxRelativeField *, EMPTYARG )

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::Init_Impl()
{
	aLineDist.SetSelectHdl(
		LINK( this, SvxStdParagraphTabPage, LineDistHdl_Impl ) );

	Link aLink = LINK( this, SvxStdParagraphTabPage, ELRLoseFocusHdl );
	aFLineIndent.SetLoseFocusHdl( aLink );
	aLeftIndent.SetLoseFocusHdl( aLink );
	aRightIndent.SetLoseFocusHdl( aLink );

	aLink = LINK( this, SvxStdParagraphTabPage, ModifyHdl_Impl );
	aFLineIndent.SetModifyHdl( aLink );
	aLeftIndent.SetModifyHdl( aLink );
	aRightIndent.SetModifyHdl( aLink );
	aTopDist.SetModifyHdl( aLink );
	aBottomDist.SetModifyHdl( aLink );

	aAutoCB.SetClickHdl( LINK( this, SvxStdParagraphTabPage, AutoHdl_Impl ));
	SfxItemPool* pPool = GetItemSet().GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool" );
	FieldUnit eUnit =
		MapToFieldUnit( pPool->GetMetric( GetWhich( SID_ATTR_LRSPACE ) ) );

	aTopDist.SetMax( aTopDist.Normalize( nAbst ), eUnit );
	aBottomDist.SetMax( aBottomDist.Normalize( nAbst ), eUnit );
	aLineDistAtMetricBox.SetMax(
		aLineDistAtMetricBox.Normalize( nAbst ), eUnit );
}

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::UpdateExample_Impl( BOOL bAll )
{
	aExampleWin.SetFirstLineOfst(
		(short)aFLineIndent.Denormalize( aFLineIndent.GetValue( FUNIT_TWIP ) ) );
	aExampleWin.SetLeftMargin(
		static_cast<long>(aLeftIndent.Denormalize( aLeftIndent.GetValue( FUNIT_TWIP ) ) ) );
	aExampleWin.SetRightMargin(
		static_cast<long>(aRightIndent.Denormalize( aRightIndent.GetValue( FUNIT_TWIP ) ) ) );
	aExampleWin.SetUpper(
		(USHORT)aTopDist.Denormalize( aTopDist.GetValue( FUNIT_TWIP ) ) );
	aExampleWin.SetLower(
		(USHORT)aBottomDist.Denormalize( aBottomDist.GetValue( FUNIT_TWIP ) ) );



	USHORT nPos = aLineDist.GetSelectEntryPos();

	switch ( nPos )
	{
		case LLINESPACE_1:
		case LLINESPACE_15:
		case LLINESPACE_2:
			aExampleWin.SetLineSpace( (SvxPrevLineSpace)nPos );
			break;

		case LLINESPACE_PROP:
			aExampleWin.SetLineSpace( (SvxPrevLineSpace)nPos,
				(USHORT)aLineDistAtPercentBox.Denormalize(
				aLineDistAtPercentBox.GetValue() ) );
			break;

		case LLINESPACE_MIN:
		case LLINESPACE_DURCH:
		case LLINESPACE_FIX:
			aExampleWin.SetLineSpace( (SvxPrevLineSpace)nPos,
				(USHORT)GetCoreValue( aLineDistAtMetricBox, SFX_MAPUNIT_TWIP ) );
			break;
	}
	aExampleWin.Draw( bAll );
}

// -----------------------------------------------------------------------

void SvxStdParagraphTabPage::EnableRegisterMode()
{
	aRegisterCB.Show();
    aRegisterFL.Show();
}

/*-----------------16.01.97 19.54-------------------

--------------------------------------------------*/
IMPL_LINK( SvxStdParagraphTabPage, AutoHdl_Impl, CheckBox*, pBox )
{
	BOOL bEnable = !pBox->IsChecked();
	aFLineLabel .Enable(bEnable);
	aFLineIndent.Enable(bEnable);
	return 0;
}

/*-----------------16.01.97 18.00-------------------

--------------------------------------------------*/
void SvxStdParagraphTabPage::SetPageWidth( USHORT nPageWidth )
{
	nWidth = nPageWidth;
}
/*-----------------16.01.97 18.01-------------------

--------------------------------------------------*/
void SvxStdParagraphTabPage::SetMaxDistance( USHORT nMaxDist )
{
	nAbst = nMaxDist;
}

/*-----------------17.01.97 08.11-------------------

--------------------------------------------------*/
void SvxStdParagraphTabPage::EnableAutoFirstLine()
{
	aAutoCB.Show();
}

/*-----------------11.06.97 11.48-------------------
	absoluter Zeilenabstand
--------------------------------------------------*/
void	SvxStdParagraphTabPage::EnableAbsLineDist(long nMinTwip)
{
	aLineDist.InsertEntry(sAbsDist);
	nMinFixDist = nMinTwip;
}

//addd CHINA001 begin
void	SvxStdParagraphTabPage::PageCreated(SfxAllItemSet aSet)
{

/* CHINA001 different bit represent call to different method of SvxStdParagraphTabPage
						0x0001 --->EnableRelativeMode()
						0x0002 --->EnableRegisterMode()
						0x0004 --->EnableAutoFirstLine()
						0x0008 --->EnableNegativeMode()


			*/
	SFX_ITEMSET_ARG	(&aSet,pPageWidthItem,SfxUInt16Item,SID_SVXSTDPARAGRAPHTABPAGE_PAGEWIDTH,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pFlagSetItem,SfxUInt32Item,SID_SVXSTDPARAGRAPHTABPAGE_FLAGSET,sal_False);
	SFX_ITEMSET_ARG	(&aSet,pLineDistItem,SfxUInt32Item,SID_SVXSTDPARAGRAPHTABPAGE_ABSLINEDIST,sal_False);

	if (pPageWidthItem)
		SetPageWidth(pPageWidthItem->GetValue());

	if (pFlagSetItem )
		if (( 0x0001 & pFlagSetItem->GetValue())== 0x0001 )
			EnableRelativeMode();

	if (pFlagSetItem)
		if (( 0x0002 & pFlagSetItem->GetValue())== 0x0002 )
				EnableRegisterMode();

	if (pFlagSetItem)
		if ( ( 0x0004 & pFlagSetItem->GetValue())== 0x0004 )
			EnableAutoFirstLine();

	if(pLineDistItem)
		EnableAbsLineDist(pLineDistItem->GetValue());

	if (pFlagSetItem)
		if	(( 0x0008 & pFlagSetItem->GetValue()) == 0x0008 )
				EnableNegativeMode();

}
//end of CHINA001

#define LASTLINEPOS_DEFAULT		0
#define LASTLINEPOS_LEFT		1

#define LASTLINECOUNT_OLD		3
#define LASTLINECOUNT_NEW		4

// class SvxParaAlignTabPage ------------------------------------------------

/*-----------------16.01.97 19.34-------------------

--------------------------------------------------*/
SvxParaAlignTabPage::SvxParaAlignTabPage( Window* pParent, const SfxItemSet& rSet )
	: SfxTabPage(pParent, SVX_RES( RID_SVXPAGE_ALIGN_PARAGRAPH ),rSet),
    aAlignFrm           ( this, SVX_RES( FL_ALIGN ) ),
    aLeft               ( this, SVX_RES( BTN_LEFTALIGN ) ),
	aRight				( this, SVX_RES( BTN_RIGHTALIGN ) ),
	aCenter				( this, SVX_RES( BTN_CENTERALIGN ) ),
	aJustify			( this, SVX_RES( BTN_JUSTIFYALIGN ) ),
    aLastLineFT         ( this, SVX_RES( FT_LASTLINE ) ),
	aLastLineLB			( this, SVX_RES( LB_LASTLINE ) ),
	aExpandCB			( this, SVX_RES( CB_EXPAND ) ),
	aSnapToGridCB		( this, SVX_RES( CB_SNAP ) ),
	aExampleWin			( this, SVX_RES( WN_EXAMPLE ) ),

	aVertAlignFL		( this, SVX_RES( FL_VERTALIGN ) ),
	aVertAlignFT		( this, SVX_RES( FT_VERTALIGN ) ),
	aVertAlignLB		( this, SVX_RES( LB_VERTALIGN ) ),

	aPropertiesFL		( this, SVX_RES( FL_PROPERTIES    )),
	aTextDirectionFT	( this, SVX_RES( FT_TEXTDIRECTION )),
	aTextDirectionLB	( this, SVX_RES( LB_TEXTDIRECTION ))
{
	SvtLanguageOptions aLangOptions;
	USHORT nLastLinePos = LASTLINEPOS_DEFAULT;

    if ( aLangOptions.IsAsianTypographyEnabled() )
	{
		String sLeft(SVX_RES(ST_LEFTALIGN_ASIAN));
		aLeft.SetText(sLeft);
		aRight.SetText(String(SVX_RES(ST_RIGHTALIGN_ASIAN)));
		sLeft = MnemonicGenerator::EraseAllMnemonicChars( sLeft );

		if ( aLastLineLB.GetEntryCount() == LASTLINECOUNT_OLD )
		{
			aLastLineLB.RemoveEntry( 0 );
			aLastLineLB.InsertEntry( sLeft, 0 );
		}
		else
			nLastLinePos = LASTLINEPOS_LEFT;
	}
	// remove "Default" or "Left" entry, depends on CJKOptions
	if ( aLastLineLB.GetEntryCount() == LASTLINECOUNT_NEW )
		aLastLineLB.RemoveEntry( nLastLinePos );

	FreeResource();
	Link aLink = LINK( this, SvxParaAlignTabPage, AlignHdl_Impl );
	aLeft.SetClickHdl( aLink );
	aRight.SetClickHdl( aLink );
	aCenter.SetClickHdl( aLink );
	aJustify.SetClickHdl( aLink );
	aLastLineLB.SetSelectHdl( LINK( this, SvxParaAlignTabPage, LastLineHdl_Impl ) );
	aTextDirectionLB.SetSelectHdl( LINK( this, SvxParaAlignTabPage, TextDirectionHdl_Impl ) );

    USHORT nHtmlMode = GetHtmlMode_Impl(rSet);
    if(!(nHtmlMode & HTMLMODE_ON) || (0 != (nHtmlMode & HTMLMODE_SOME_STYLES)) )
    {
        if( aLangOptions.IsCTLFontEnabled() )
        {
            aTextDirectionLB.InsertEntryValue( SVX_RESSTR( RID_SVXSTR_FRAMEDIR_LTR ), FRMDIR_HORI_LEFT_TOP );
            aTextDirectionLB.InsertEntryValue( SVX_RESSTR( RID_SVXSTR_FRAMEDIR_RTL ), FRMDIR_HORI_RIGHT_TOP );
            aTextDirectionLB.InsertEntryValue( SVX_RESSTR( RID_SVXSTR_FRAMEDIR_SUPER ), FRMDIR_ENVIRONMENT );

            aPropertiesFL.Show();
            aTextDirectionFT.Show();
            aTextDirectionLB.Show();
        }
    }
}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
SvxParaAlignTabPage::~SvxParaAlignTabPage()
{
}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
int SvxParaAlignTabPage::DeactivatePage( SfxItemSet* _pSet )
{
    if ( _pSet )
        FillItemSet( *_pSet );
	return LEAVE_PAGE;
}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
SfxTabPage*	SvxParaAlignTabPage::Create( Window* pParent, const SfxItemSet& rSet )
{
	return new SvxParaAlignTabPage(pParent, rSet);
}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
USHORT*	SvxParaAlignTabPage::GetRanges()
{
	return pAlignRanges;

}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
BOOL SvxParaAlignTabPage::FillItemSet( SfxItemSet& rOutSet )
{
	BOOL bModified = FALSE;

	FASTBOOL bAdj = FALSE, bChecked = FALSE;
	SvxAdjust eAdjust = SVX_ADJUST_LEFT;

	if ( aLeft.IsChecked() )
	{
		eAdjust = SVX_ADJUST_LEFT;
		bAdj = !aLeft.GetSavedValue();
		bChecked = TRUE;
	}
	else if ( aRight.IsChecked() )
	{
		eAdjust = SVX_ADJUST_RIGHT;
		bAdj = !aRight.GetSavedValue();
		bChecked = TRUE;
	}
	else if ( aCenter.IsChecked() )
	{
		eAdjust = SVX_ADJUST_CENTER;
		bAdj = !aCenter.GetSavedValue();
		bChecked = TRUE;
	}
	else if ( aJustify.IsChecked() )
	{
		eAdjust = SVX_ADJUST_BLOCK;
		bAdj = !aJustify.GetSavedValue() ||
			aExpandCB.IsChecked() != aExpandCB.GetSavedValue() ||
			aLastLineLB.GetSelectEntryPos() != aLastLineLB.GetSavedValue();
		bChecked = TRUE;
	}
    USHORT _nWhich = GetWhich( SID_ATTR_PARA_ADJUST );

	if ( bAdj )
	{
		const SvxAdjustItem* pOld =
			(const SvxAdjustItem*)GetOldItem( rOutSet, SID_ATTR_PARA_ADJUST );
		SvxAdjust eOneWord = aExpandCB.IsChecked() ? SVX_ADJUST_BLOCK
												   : SVX_ADJUST_LEFT;
		USHORT nLBPos = aLastLineLB.GetSelectEntryPos();
		SvxAdjust eLastBlock = SVX_ADJUST_LEFT;

		if ( 1 == nLBPos )
			eLastBlock = SVX_ADJUST_CENTER;
		else if ( 2 == nLBPos )
			eLastBlock = SVX_ADJUST_BLOCK;

		FASTBOOL bNothingWasChecked =
			!aLeft.GetSavedValue() && !aRight.GetSavedValue() &&
			!aCenter.GetSavedValue() && !aJustify.GetSavedValue();

		if ( !pOld || pOld->GetAdjust() != eAdjust ||
			 pOld->GetOneWord() != eOneWord ||
			 pOld->GetLastBlock() != eLastBlock ||
			 ( bChecked && bNothingWasChecked ) )
		{
			bModified |= TRUE;
			SvxAdjustItem aAdj(
                (const SvxAdjustItem&)GetItemSet().Get( _nWhich ) );
			aAdj.SetAdjust( eAdjust );
			aAdj.SetOneWord( eOneWord );
			aAdj.SetLastBlock( eLastBlock );
			rOutSet.Put( aAdj );
		}
	}
    if(aSnapToGridCB.IsChecked() != aSnapToGridCB.GetSavedValue())
    {
        rOutSet.Put(SvxParaGridItem(aSnapToGridCB.IsChecked(), GetWhich( SID_ATTR_PARA_SNAPTOGRID )));
        bModified = TRUE;
    }
    if(aVertAlignLB.GetSavedValue() != aVertAlignLB.GetSelectEntryPos())
    {
        rOutSet.Put(SvxParaVertAlignItem(aVertAlignLB.GetSelectEntryPos(), GetWhich( SID_PARA_VERTALIGN )));
        bModified = TRUE;
    }

    if( aTextDirectionLB.IsVisible() )
	{
        SvxFrameDirection eDir = aTextDirectionLB.GetSelectEntryValue();
        if( eDir != aTextDirectionLB.GetSavedValue() )
		{
            rOutSet.Put( SvxFrameDirectionItem( eDir, GetWhich( SID_ATTR_FRAMEDIRECTION ) ) );
			bModified = TRUE;
		}
	}

    return bModified;
}

/*-----------------16.01.97 19.33-------------------

--------------------------------------------------*/
void SvxParaAlignTabPage::Reset( const SfxItemSet& rSet )
{
    USHORT _nWhich = GetWhich( SID_ATTR_PARA_ADJUST );
    SfxItemState eItemState = rSet.GetItemState( _nWhich );

	USHORT nLBSelect = 0;
	if ( eItemState >= SFX_ITEM_AVAILABLE )
	{
        const SvxAdjustItem& rAdj = (const SvxAdjustItem&)rSet.Get( _nWhich );

		switch ( rAdj.GetAdjust() /*!!! VB fragen rAdj.GetLastBlock()*/ )
		{
			case SVX_ADJUST_LEFT: aLeft.Check(); break;

			case SVX_ADJUST_RIGHT: aRight.Check(); break;

			case SVX_ADJUST_CENTER: aCenter.Check(); break;

			case SVX_ADJUST_BLOCK: aJustify.Check(); break;
            default: ; //prevent warning
		}
		BOOL bEnable = aJustify.IsChecked();
		aLastLineFT.Enable(bEnable);
		aLastLineLB.Enable(bEnable);
		aExpandCB  .Enable(bEnable);

		aExpandCB.Check(SVX_ADJUST_BLOCK == rAdj.GetOneWord());
		switch(rAdj.GetLastBlock())
		{
			case SVX_ADJUST_LEFT:  nLBSelect = 0; break;

			case SVX_ADJUST_CENTER: nLBSelect = 1;  break;

			case SVX_ADJUST_BLOCK: nLBSelect = 2;  break;
            default: ; //prevent warning
		}
	}
	else
	{
		aLeft.Check( FALSE );
		aRight.Check( FALSE );
		aCenter.Check( FALSE );
		aJustify.Check( FALSE );
	}
	aLastLineLB.SelectEntryPos(nLBSelect);

	USHORT nHtmlMode = GetHtmlMode_Impl(rSet);
	if(nHtmlMode & HTMLMODE_ON)
	{
		aLastLineLB.Hide();
		aLastLineFT.Hide();
		aExpandCB.Hide();
		if(!(nHtmlMode & (HTMLMODE_FULL_STYLES|HTMLMODE_FIRSTLINE)) )
			aJustify.Disable();
        aSnapToGridCB.Show(FALSE);
	}
    _nWhich = GetWhich(SID_ATTR_PARA_SNAPTOGRID);
    eItemState = rSet.GetItemState( _nWhich );
    if ( eItemState >= SFX_ITEM_AVAILABLE )
    {
        const SvxParaGridItem& rSnap = (const SvxParaGridItem&)rSet.Get( _nWhich );
        aSnapToGridCB.Check(rSnap.GetValue());
    }

    _nWhich = GetWhich( SID_PARA_VERTALIGN );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
    {
        aVertAlignLB.Show();
        aVertAlignFL.Show();
        aVertAlignFT.Show();

        const SvxParaVertAlignItem& rAlign = (const SvxParaVertAlignItem&)rSet.Get( _nWhich );
        aVertAlignLB.SelectEntryPos(rAlign.GetValue());
    }

    _nWhich = GetWhich( SID_ATTR_FRAMEDIRECTION );
    //text direction
    if( SFX_ITEM_AVAILABLE <= rSet.GetItemState( _nWhich ) )
	{
        const SvxFrameDirectionItem& rFrameDirItem = ( const SvxFrameDirectionItem& ) rSet.Get( _nWhich );
        aTextDirectionLB.SelectEntryValue( (SvxFrameDirection)rFrameDirItem.GetValue() );
        aTextDirectionLB.SaveValue();
	}

    aSnapToGridCB.SaveValue();
    aVertAlignLB.SaveValue();
    aLeft.SaveValue();
	aRight.SaveValue();
	aCenter.SaveValue();
	aJustify.SaveValue();
	aLastLineLB.SaveValue();
	aExpandCB.SaveValue();

	UpdateExample_Impl(TRUE);
}

/*-----------------17.01.97 08.06-------------------

--------------------------------------------------*/
IMPL_LINK( SvxParaAlignTabPage, AlignHdl_Impl, RadioButton*, EMPTYARG )
{
	BOOL bJustify = aJustify.IsChecked();
	aLastLineFT.Enable(bJustify);
	aLastLineLB.Enable(bJustify);
	aExpandCB.Enable(bJustify);
	UpdateExample_Impl(FALSE);
	return 0;
}

IMPL_LINK( SvxParaAlignTabPage, LastLineHdl_Impl, ListBox*, EMPTYARG )
{
	UpdateExample_Impl(FALSE);
	return 0;
}

IMPL_LINK( SvxParaAlignTabPage, TextDirectionHdl_Impl, ListBox*, EMPTYARG )
{
	SvxFrameDirection eDir = aTextDirectionLB.GetSelectEntryValue();
	switch ( eDir )
	{
		// check the default alignment for this text direction
		case FRMDIR_HORI_LEFT_TOP :		aLeft.Check( TRUE ); break;
		case FRMDIR_HORI_RIGHT_TOP :	aRight.Check( TRUE ); break;
		case FRMDIR_ENVIRONMENT :		/* do nothing */ break;
		default:
		{
			DBG_ERRORFILE( "SvxParaAlignTabPage::TextDirectionHdl_Impl(): other directions not supported" );
		}
	}

	return 0;
}

/*-----------------16.01.97 19.34-------------------

--------------------------------------------------*/
void	SvxParaAlignTabPage::UpdateExample_Impl( BOOL bAll )
{
	if ( aLeft.IsChecked() )
		aExampleWin.SetAdjust( SVX_ADJUST_LEFT );
	else if ( aRight.IsChecked() )
		aExampleWin.SetAdjust( SVX_ADJUST_RIGHT );
	else if ( aCenter.IsChecked() )
		aExampleWin.SetAdjust( SVX_ADJUST_CENTER );
	else if ( aJustify.IsChecked() )
	{
		aExampleWin.SetAdjust( SVX_ADJUST_BLOCK );
		SvxAdjust eLastBlock = SVX_ADJUST_LEFT;
		USHORT nLBPos = aLastLineLB.GetSelectEntryPos();
		if(nLBPos == 1)
			eLastBlock = SVX_ADJUST_CENTER;
		else if(nLBPos == 2)
			eLastBlock = SVX_ADJUST_BLOCK;
		aExampleWin.SetLastLine( eLastBlock );
	}

	aExampleWin.Draw( bAll );
}
/*-----------------17.01.97 08.04-------------------
	Erweiterungen fuer den Blocksatz einschalten
--------------------------------------------------*/
void SvxParaAlignTabPage::EnableJustifyExt()
{
	aLastLineFT.Show();
	aLastLineLB.Show();
	aExpandCB  .Show();
    SvtLanguageOptions aCJKOptions;
    if(aCJKOptions.IsAsianTypographyEnabled())
        aSnapToGridCB.Show();

}
//add CHINA001 begin
void SvxParaAlignTabPage::PageCreated (SfxAllItemSet aSet)
{
	SFX_ITEMSET_ARG	(&aSet,pBoolItem,SfxBoolItem,SID_SVXPARAALIGNTABPAGE_ENABLEJUSTIFYEXT,sal_False);
	if (pBoolItem)
		if(pBoolItem->GetValue())
			EnableJustifyExt();
}
//end of CHINA001
// class SvxExtParagraphTabPage ------------------------------------------

SfxTabPage* SvxExtParagraphTabPage::Create( Window* pParent,
											const SfxItemSet& rSet )
{
	return new SvxExtParagraphTabPage( pParent, rSet );
}

// -----------------------------------------------------------------------

BOOL SvxExtParagraphTabPage::FillItemSet( SfxItemSet& rOutSet )
{
	BOOL bModified = FALSE;
    USHORT _nWhich = GetWhich( SID_ATTR_PARA_HYPHENZONE );
	const TriState eHyphenState = aHyphenBox.GetState();
	const SfxPoolItem* pOld = GetOldItem( rOutSet, SID_ATTR_PARA_HYPHENZONE );

	if ( eHyphenState != aHyphenBox.GetSavedValue() 	||
		 aExtHyphenBeforeBox.IsValueModified() 	   		||
		 aExtHyphenAfterBox.IsValueModified()			||
		 aMaxHyphenEdit.IsValueModified() )
	{
		SvxHyphenZoneItem aHyphen(
            (const SvxHyphenZoneItem&)GetItemSet().Get( _nWhich ) );
		aHyphen.SetHyphen( eHyphenState == STATE_CHECK );

		if ( eHyphenState == STATE_CHECK )
		{
			aHyphen.GetMinLead() = (BYTE)aExtHyphenBeforeBox.GetValue();
			aHyphen.GetMinTrail() = (BYTE)aExtHyphenAfterBox.GetValue();
		}
		aHyphen.GetMaxHyphens() = (BYTE)aMaxHyphenEdit.GetValue();

		if ( !pOld ||
			!( *(SvxHyphenZoneItem*)pOld == aHyphen ) ||
				eHyphenState != aHyphenBox.GetSavedValue())
		{
			rOutSet.Put( aHyphen );
			bModified |= TRUE;
		}
	}

	if (aPagenumEdit.IsEnabled() && aPagenumEdit.IsValueModified())
	{
		SfxUInt16Item aPageNum( SID_ATTR_PARA_PAGENUM,
								(USHORT)aPagenumEdit.GetValue() );

		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_PAGENUM );

		if ( !pOld || ( (const SfxUInt16Item*)pOld )->GetValue() != aPageNum.GetValue() )
		{
			rOutSet.Put( aPageNum );
			bModified |= TRUE;
		}
	}

	// Seitenumbruch

	TriState eState = aApplyCollBtn.GetState();
	FASTBOOL bIsPageModel = FALSE;

    _nWhich = GetWhich( SID_ATTR_PARA_MODEL );
	String sPage;
	if ( eState != aApplyCollBtn.GetSavedValue() ||
		 ( STATE_CHECK == eState &&
		   aApplyCollBox.GetSelectEntryPos() != aApplyCollBox.GetSavedValue() ) )
	{
		if ( eState == STATE_CHECK )
		{
			sPage = aApplyCollBox.GetSelectEntry();
			bIsPageModel = 0 != sPage.Len();
		}
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_MODEL );

		if ( !pOld || ( (const SvxPageModelItem*)pOld )->GetValue() != sPage )
		{
            rOutSet.Put( SvxPageModelItem( sPage, FALSE, _nWhich ) );
			bModified |= TRUE;
		}
		else
			bIsPageModel = FALSE;
	}
	else if(STATE_CHECK == eState && aApplyCollBtn.IsEnabled())
		bIsPageModel = TRUE;
	else
        rOutSet.Put( SvxPageModelItem( sPage, FALSE, _nWhich ) );

    _nWhich = GetWhich( SID_ATTR_PARA_PAGEBREAK );

	if ( bIsPageModel )
		// wird PageModel eingeschaltet, dann immer PageBreak ausschalten
        rOutSet.Put( SvxFmtBreakItem( SVX_BREAK_NONE, _nWhich ) );
	else
	{
		eState = aPageBreakBox.GetState();
		SfxItemState eModelState = GetItemSet().GetItemState(SID_ATTR_PARA_MODEL, FALSE);

		if ( (eModelState == SFX_ITEM_SET && STATE_CHECK == aPageBreakBox.GetState()) ||
			 eState != aPageBreakBox.GetSavedValue()				||
             aBreakTypeLB.GetSelectEntryPos() != aBreakTypeLB.GetSavedValue()   ||
             aBreakPositionLB.GetSelectEntryPos() != aBreakPositionLB.GetSavedValue() )
		{
			const SvxFmtBreakItem rOldBreak(
                    (const SvxFmtBreakItem&)GetItemSet().Get( _nWhich ));
			SvxFmtBreakItem aBreak(rOldBreak.GetBreak(), rOldBreak.Which());

			switch ( eState )
			{
				case STATE_CHECK:
				{
                    BOOL bBefore = aBreakPositionLB.GetSelectEntryPos() == 0;

                    if ( aBreakTypeLB.GetSelectEntryPos() == 0 )
					{
						if ( bBefore )
							aBreak.SetValue( SVX_BREAK_PAGE_BEFORE );
						else
							aBreak.SetValue( SVX_BREAK_PAGE_AFTER );
					}
					else
					{
						if ( bBefore )
							aBreak.SetValue( SVX_BREAK_COLUMN_BEFORE );
						else
							aBreak.SetValue( SVX_BREAK_COLUMN_AFTER );
					}
					break;
				}

				case STATE_NOCHECK:
					aBreak.SetValue( SVX_BREAK_NONE );
					break;
                default: ; //prevent warning
			}
			pOld = GetOldItem( rOutSet, SID_ATTR_PARA_PAGEBREAK );

			if ( eState != aPageBreakBox.GetSavedValue()				||
					!pOld || !( *(const SvxFmtBreakItem*)pOld == aBreak ) )
			{
				bModified |= TRUE;
				rOutSet.Put( aBreak );
			}
		}
	}


	// Absatztrennung
    _nWhich = GetWhich( SID_ATTR_PARA_SPLIT );
	eState = aKeepTogetherBox.GetState();

	if ( eState != aKeepTogetherBox.GetSavedValue() )
	{
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_SPLIT );

		if ( !pOld || ( (const SvxFmtSplitItem*)pOld )->GetValue() !=
					  ( eState == STATE_NOCHECK ) )
		{
            rOutSet.Put( SvxFmtSplitItem( eState == STATE_NOCHECK, _nWhich ) );
			bModified |= TRUE;
		}
	}

	// Absaetze zusammenhalten
    _nWhich = GetWhich( SID_ATTR_PARA_KEEP );
	eState = aKeepParaBox.GetState();

	if ( eState != aKeepParaBox.GetSavedValue() )
	{
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_KEEP );

		// hat sich der Status geaendert, muss immer geputtet werden
        rOutSet.Put( SvxFmtKeepItem( eState == STATE_CHECK, _nWhich ) );
		bModified |= TRUE;
	}

	// Witwen und Waisen
    _nWhich = GetWhich( SID_ATTR_PARA_WIDOWS );
	eState = aWidowBox.GetState();

	if ( eState != aWidowBox.GetSavedValue() ||
		 aWidowRowNo.IsValueModified() )
	{
		SvxWidowsItem rItem( eState == STATE_CHECK ?
                             (BYTE)aWidowRowNo.GetValue() : 0, _nWhich );
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_WIDOWS );

		if ( eState != aWidowBox.GetSavedValue() || !pOld || !( *(const SvxWidowsItem*)pOld == rItem ) )
		{
			rOutSet.Put( rItem );
			bModified |= TRUE;
		}
	}

    _nWhich = GetWhich( SID_ATTR_PARA_ORPHANS );
	eState = aOrphanBox.GetState();

	if ( eState != aOrphanBox.GetSavedValue() ||
		 aOrphanRowNo.IsValueModified() )
	{
		SvxOrphansItem rItem( eState == STATE_CHECK ?
                             (BYTE)aOrphanRowNo.GetValue() : 0, _nWhich );
		pOld = GetOldItem( rOutSet, SID_ATTR_PARA_ORPHANS );

		if ( eState != aOrphanBox.GetSavedValue() ||
				!pOld ||
					!( *(const SvxOrphansItem*)pOld == rItem ) )
		{
			rOutSet.Put( rItem );
			bModified |= TRUE;
		}
	}

    return bModified;
}

// -----------------------------------------------------------------------

void SvxExtParagraphTabPage::Reset( const SfxItemSet& rSet )
{
    USHORT _nWhich = GetWhich( SID_ATTR_PARA_HYPHENZONE );
    SfxItemState eItemState = rSet.GetItemState( _nWhich );

	BOOL bItemAvailable = eItemState >= SFX_ITEM_AVAILABLE;
	BOOL bIsHyphen = FALSE;
	if( !bHtmlMode && bItemAvailable )
	{
		const SvxHyphenZoneItem& rHyphen =
            (const SvxHyphenZoneItem&)rSet.Get( _nWhich );
		aHyphenBox.EnableTriState( FALSE );

		bIsHyphen = rHyphen.IsHyphen();
		aHyphenBox.SetState( bIsHyphen ? STATE_CHECK : STATE_NOCHECK );

		aExtHyphenBeforeBox.SetValue( rHyphen.GetMinLead() );
		aExtHyphenAfterBox.SetValue( rHyphen.GetMinTrail() );
		aMaxHyphenEdit.SetValue( rHyphen.GetMaxHyphens() );
	}
	else
	{
		aHyphenBox.SetState( STATE_DONTKNOW );
	}
	BOOL bEnable = bItemAvailable && bIsHyphen;
	aExtHyphenBeforeBox.Enable(bEnable);
	aExtHyphenAfterBox.Enable(bEnable);
	aBeforeText.Enable(bEnable);
	aAfterText.Enable(bEnable);
	aMaxHyphenLabel.Enable(bEnable);
	aMaxHyphenEdit.Enable(bEnable);

    _nWhich = GetWhich( SID_ATTR_PARA_PAGENUM );

    if ( rSet.GetItemState(_nWhich) >= SFX_ITEM_AVAILABLE )
	{
		const USHORT nPageNum =
            ( (const SfxUInt16Item&)rSet.Get( _nWhich ) ).GetValue();
		aPagenumEdit.SetValue( nPageNum );
	}

	if ( bPageBreak )
	{
		// zuerst PageModel behandeln
        _nWhich = GetWhich( SID_ATTR_PARA_MODEL );
		BOOL bIsPageModel = FALSE;
        eItemState = rSet.GetItemState( _nWhich );

		if ( eItemState >= SFX_ITEM_SET )
		{
			aApplyCollBtn.EnableTriState( FALSE );

			const SvxPageModelItem& rModel =
                (const SvxPageModelItem&)rSet.Get( _nWhich );
			String aStr( rModel.GetValue() );

			if ( aStr.Len() &&
				 aApplyCollBox.GetEntryPos( aStr ) != LISTBOX_ENTRY_NOTFOUND )
			{
				aApplyCollBox.SelectEntry( aStr );
				aApplyCollBtn.SetState( STATE_CHECK );
				bIsPageModel = TRUE;

				aPageBreakBox.Enable();
				aPageBreakBox.EnableTriState( FALSE );
                aBreakTypeFT.Enable();
                aBreakTypeLB.Enable();
                aBreakPositionFT.Enable();
                aBreakPositionLB.Enable();
                aApplyCollBtn.Enable();
				aPageBreakBox.SetState( STATE_CHECK );

                //select page break
                aBreakTypeLB.SelectEntryPos(0);
                //select break before
                aBreakPositionLB.SelectEntryPos(0);
			}
			else
			{
				aApplyCollBox.SetNoSelection();
				aApplyCollBtn.SetState( STATE_NOCHECK );
			}
		}
		else if ( SFX_ITEM_DONTCARE == eItemState )
		{
			aApplyCollBtn.EnableTriState( TRUE );
			aApplyCollBtn.SetState( STATE_DONTKNOW );
			aApplyCollBox.SetNoSelection();
		}
		else
		{
			aApplyCollBtn.Enable(FALSE);
			aApplyCollBox.Enable(FALSE);
			aPagenumEdit.Enable(FALSE);
			aPagenumText.Enable(FALSE);
		}
//!!!	ApplyCollClickHdl_Impl( &aApplyCollBtn );

		if ( !bIsPageModel )
		{
            _nWhich = GetWhich( SID_ATTR_PARA_PAGEBREAK );
            eItemState = rSet.GetItemState( _nWhich );

			if ( eItemState >= SFX_ITEM_AVAILABLE )
			{
				const SvxFmtBreakItem& rPageBreak =
                    (const SvxFmtBreakItem&)rSet.Get( _nWhich );

				SvxBreak eBreak = (SvxBreak)rPageBreak.GetValue();

				// PageBreak nicht ueber CTRL-RETURN,
				// dann kann CheckBox frei gegeben werden
				aPageBreakBox.Enable();
				aPageBreakBox.EnableTriState( FALSE );
                aBreakTypeFT.Enable();
                aBreakTypeLB.Enable();
                aBreakPositionFT.Enable();
                aBreakPositionLB.Enable();

				aPageBreakBox.SetState( STATE_CHECK );

                BOOL _bEnable =     eBreak != SVX_BREAK_NONE &&
								eBreak != SVX_BREAK_COLUMN_BEFORE &&
								eBreak != SVX_BREAK_COLUMN_AFTER;
                aApplyCollBtn.Enable(_bEnable);
                if(!_bEnable)
				{
                    aApplyCollBox.Enable(_bEnable);
                    aPagenumEdit.Enable(_bEnable);
				}

				if ( eBreak == SVX_BREAK_NONE )
					aPageBreakBox.SetState( STATE_NOCHECK );

                USHORT nType = 0; // selection position in break type ListBox : Page
                USHORT nPosition = 0; //  selection position in break position ListBox : Before
                switch ( eBreak )
				{
					case SVX_BREAK_PAGE_BEFORE:
						break;
					case SVX_BREAK_PAGE_AFTER:
                        nPosition = 1;
						break;
					case SVX_BREAK_COLUMN_BEFORE:
                        nType = 1;
						break;
					case SVX_BREAK_COLUMN_AFTER:
                        nType = 1;
                        nPosition = 1;
						break;
                    default: ;//prevent warning
				}
                aBreakTypeLB.SelectEntryPos(nType);
                aBreakPositionLB.SelectEntryPos(nPosition);
            }
			else if ( SFX_ITEM_DONTCARE == eItemState )
				aPageBreakBox.SetState( STATE_DONTKNOW );
			else
			{
                aPageBreakBox.Enable(FALSE);
                aBreakTypeFT.Enable(FALSE);
                aBreakTypeLB.Enable(FALSE);
                aBreakPositionFT.Enable(FALSE);
                aBreakPositionLB.Enable(FALSE);
			}
		}

        PageBreakPosHdl_Impl( &aBreakPositionLB );
		PageBreakHdl_Impl( &aPageBreakBox );
	}

    _nWhich = GetWhich( SID_ATTR_PARA_KEEP );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
	{
		aKeepParaBox.EnableTriState( FALSE );
		const SvxFmtKeepItem& rKeep =
            (const SvxFmtKeepItem&)rSet.Get( _nWhich );

		if ( rKeep.GetValue() )
			aKeepParaBox.SetState( STATE_CHECK );
		else
			aKeepParaBox.SetState( STATE_NOCHECK );
	}
	else if ( SFX_ITEM_DONTCARE == eItemState )
		aKeepParaBox.SetState( STATE_DONTKNOW );
	else
		aKeepParaBox.Enable(FALSE);

    _nWhich = GetWhich( SID_ATTR_PARA_SPLIT );
    eItemState = rSet.GetItemState( _nWhich );

	if ( eItemState >= SFX_ITEM_AVAILABLE )
	{
		const SvxFmtSplitItem& rSplit =
            (const SvxFmtSplitItem&)rSet.Get( _nWhich );
		aKeepTogetherBox.EnableTriState( FALSE );

		if ( !rSplit.GetValue() )
			aKeepTogetherBox.SetState( STATE_CHECK );
		else
		{
			aKeepTogetherBox.SetState( STATE_NOCHECK );

			// Witwen und Waisen
			aWidowBox.Enable();
            _nWhich = GetWhich( SID_ATTR_PARA_WIDOWS );
            SfxItemState eTmpState = rSet.GetItemState( _nWhich );

			if ( eTmpState >= SFX_ITEM_AVAILABLE )
			{
				const SvxWidowsItem& rWidow =
                    (const SvxWidowsItem&)rSet.Get( _nWhich );
				aWidowBox.EnableTriState( FALSE );
				const USHORT nLines = rWidow.GetValue();

                BOOL _bEnable = nLines > 0;
				aWidowRowNo.SetValue( aWidowRowNo.Normalize( nLines ) );
                aWidowBox.SetState( _bEnable ? STATE_CHECK : STATE_NOCHECK);
                aWidowRowNo.Enable(_bEnable);
                aWidowRowLabel.Enable(_bEnable);

			}
			else if ( SFX_ITEM_DONTCARE == eTmpState )
				aWidowBox.SetState( STATE_DONTKNOW );
			else
				aWidowBox.Enable(FALSE);

			aOrphanBox.Enable();
            _nWhich = GetWhich( SID_ATTR_PARA_ORPHANS );
            eTmpState = rSet.GetItemState( _nWhich );

			if ( eTmpState >= SFX_ITEM_AVAILABLE )
			{
				const SvxOrphansItem& rOrphan =
                    (const SvxOrphansItem&)rSet.Get( _nWhich );
				const USHORT nLines = rOrphan.GetValue();
				aOrphanBox.EnableTriState( FALSE );

                BOOL _bEnable = nLines > 0;
                aOrphanBox.SetState( _bEnable ? STATE_CHECK : STATE_NOCHECK);
				aOrphanRowNo.SetValue( aOrphanRowNo.Normalize( nLines ) );
                aOrphanRowNo.Enable(_bEnable);
                aOrphanRowLabel.Enable(_bEnable);

			}
			else if ( SFX_ITEM_DONTCARE == eTmpState )
				aOrphanBox.SetState( STATE_DONTKNOW );
			else
				aOrphanBox.Enable(FALSE);
		}
	}
	else if ( SFX_ITEM_DONTCARE == eItemState )
		aKeepTogetherBox.SetState( STATE_DONTKNOW );
	else
		aKeepTogetherBox.Enable(FALSE);

	// damit alles richt enabled wird
	KeepTogetherHdl_Impl( 0 );
	WidowHdl_Impl( 0 );
	OrphanHdl_Impl( 0 );

    aHyphenBox.SaveValue();
	aExtHyphenBeforeBox.SaveValue();
	aExtHyphenAfterBox.SaveValue();
	aMaxHyphenEdit.SaveValue();
	aPageBreakBox.SaveValue();
    aBreakPositionLB.SaveValue();
    aBreakTypeLB.SaveValue();
	aApplyCollBtn.SaveValue();
	aApplyCollBox.SaveValue();
	aPagenumEdit.SaveValue();
	aKeepTogetherBox.SaveValue();
	aKeepParaBox.SaveValue();
	aWidowBox.SaveValue();
	aOrphanBox.SaveValue();
}

// -----------------------------------------------------------------------

int SvxExtParagraphTabPage::DeactivatePage( SfxItemSet* _pSet )
{
    if ( _pSet )
        FillItemSet( *_pSet );
	return LEAVE_PAGE;
}

// -----------------------------------------------------------------------

void SvxExtParagraphTabPage::DisablePageBreak()
{
	bPageBreak = FALSE;
	aPageBreakBox.Enable(FALSE);
    aBreakTypeLB.RemoveEntry(0);
    aBreakPositionFT.Enable(FALSE);
    aBreakPositionLB.Enable(FALSE);
	aApplyCollBtn.Enable(FALSE);
	aApplyCollBox.Enable(FALSE);
	aPagenumEdit.Enable(FALSE);
}

// -----------------------------------------------------------------------

SvxExtParagraphTabPage::SvxExtParagraphTabPage( Window* pParent, const SfxItemSet& rAttr ) :

	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_EXT_PARAGRAPH ), rAttr ),

	aHyphenBox    		( this, SVX_RES( BTN_HYPHEN ) ),
	aBeforeText			( this, SVX_RES( FT_HYPHENBEFORE ) ),
	aExtHyphenBeforeBox ( this, SVX_RES( ED_HYPHENBEFORE ) ),
	aAfterText			( this, SVX_RES( FT_HYPHENAFTER ) ),
	aExtHyphenAfterBox  ( this, SVX_RES( ED_HYPHENAFTER ) ),
	aMaxHyphenLabel     ( this, SVX_RES( FT_MAXHYPH ) ),
	aMaxHyphenEdit		( this, SVX_RES( ED_MAXHYPH ) ),
    aExtFL              ( this, SVX_RES( FL_HYPHEN ) ),
    aBreaksFL           ( this, SVX_RES( FL_BREAKS ) ),
    aPageBreakBox       ( this, SVX_RES( BTN_PAGEBREAK ) ),
    aBreakTypeFT        ( this, SVX_RES( FT_BREAKTYPE     )),
    aBreakTypeLB        ( this, SVX_RES( LB_BREAKTYPE     )),
    aBreakPositionFT    ( this, SVX_RES( FT_BREAKPOSITION )),
    aBreakPositionLB    ( this, SVX_RES( LB_BREAKPOSITION )),
//    aPageBox            ( this, SVX_RES( BTN_BREAKPAGE ) ),
//    aColumnBox          ( this, SVX_RES( BTN_BREAKCOLUMN ) ),
//    aBeforeBox          ( this, SVX_RES( BTN_PAGEBREAKBEFORE ) ),
//    aAfterBox           ( this, SVX_RES( BTN_PAGEBREAKAFTER ) ),
	aApplyCollBtn       ( this, SVX_RES( BTN_PAGECOLL ) ),
	aApplyCollBox       ( this, SVX_RES( LB_PAGECOLL ) ),
	aPagenumText		( this, SVX_RES( FT_PAGENUM ) ),
	aPagenumEdit		( this, SVX_RES( ED_PAGENUM ) ),
    aExtendFL           ( this, SVX_RES( FL_OPTIONS ) ),
    aKeepTogetherBox    ( this, SVX_RES( BTN_KEEPTOGETHER ) ),
	aKeepParaBox		( this, SVX_RES( CB_KEEPTOGETHER ) ),
	aOrphanBox          ( this, SVX_RES( BTN_ORPHANS ) ),
	aOrphanRowNo        ( this, SVX_RES( ED_ORPHANS ) ),
	aOrphanRowLabel     ( this, SVX_RES( FT_ORPHANS ) ),
	aWidowBox           ( this, SVX_RES( BTN_WIDOWS ) ),
	aWidowRowNo         ( this, SVX_RES( ED_WIDOWS ) ),
	aWidowRowLabel      ( this, SVX_RES( FT_WIDOWS ) ),
    bPageBreak  ( TRUE ),
    bHtmlMode   ( FALSE ),
    nStdPos     ( 0 )
{
	FreeResource();

	// diese Page braucht ExchangeSupport
	SetExchangeSupport();

	aHyphenBox.SetClickHdl( 		LINK( this, SvxExtParagraphTabPage, HyphenClickHdl_Impl ) );
	aPageBreakBox.SetClickHdl( 		LINK( this, SvxExtParagraphTabPage, PageBreakHdl_Impl ) );
	aKeepTogetherBox.SetClickHdl( 	LINK( this, SvxExtParagraphTabPage, KeepTogetherHdl_Impl ) );
	aWidowBox.SetClickHdl( 			LINK( this, SvxExtParagraphTabPage, WidowHdl_Impl ) );
	aOrphanBox.SetClickHdl( 		LINK( this, SvxExtParagraphTabPage, OrphanHdl_Impl ) );
	aApplyCollBtn.SetClickHdl( 		LINK( this, SvxExtParagraphTabPage, ApplyCollClickHdl_Impl ) );
    aBreakTypeLB.SetSelectHdl(      LINK( this, SvxExtParagraphTabPage, PageBreakTypeHdl_Impl ) );
    aBreakPositionLB.SetSelectHdl(  LINK( this, SvxExtParagraphTabPage, PageBreakPosHdl_Impl ) );

	SfxObjectShell* pSh = SfxObjectShell::Current();
	if ( pSh )
	{
		SfxStyleSheetBasePool* pPool = pSh->GetStyleSheetPool();
		pPool->SetSearchMask( SFX_STYLE_FAMILY_PAGE );
		SfxStyleSheetBase* pStyle = pPool->First();
		String aStdName;

		while( pStyle )
		{
			if ( aStdName.Len() == 0 )
				// first style == standard style
				aStdName = pStyle->GetName();
			aApplyCollBox.InsertEntry( pStyle->GetName() );
			pStyle = pPool->Next();
		}
		nStdPos = aApplyCollBox.GetEntryPos( aStdName );
	}

	USHORT nHtmlMode = GetHtmlMode_Impl( rAttr );
	if ( nHtmlMode & HTMLMODE_ON )
	{
		bHtmlMode = TRUE;
		aHyphenBox    		 .Enable(FALSE);
		aBeforeText			 .Enable(FALSE);
		aExtHyphenBeforeBox  .Enable(FALSE);
		aAfterText			 .Enable(FALSE);
		aExtHyphenAfterBox   .Enable(FALSE);
		aMaxHyphenLabel      .Enable(FALSE);
		aMaxHyphenEdit		 .Enable(FALSE);
        aExtFL               .Enable(FALSE);
		aPagenumText         .Enable(FALSE);
		aPagenumEdit         .Enable(FALSE);
        // no column break in HTML
        aBreakTypeLB.RemoveEntry(1);
	}
}

// -----------------------------------------------------------------------

__EXPORT SvxExtParagraphTabPage::~SvxExtParagraphTabPage()
{
}

// -----------------------------------------------------------------------

USHORT* SvxExtParagraphTabPage::GetRanges()
{
	return pExtRanges;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, PageBreakHdl_Impl, TriStateBox *, EMPTYARG )
{
	switch ( aPageBreakBox.GetState() )
	{
		case STATE_CHECK:
            aBreakTypeFT.Enable();
            aBreakTypeLB.Enable();
            aBreakPositionFT.Enable();
            aBreakPositionLB.Enable();

            if ( 0 == aBreakTypeLB.GetSelectEntryPos()&&
                0 == aBreakPositionLB.GetSelectEntryPos() )
			{
				aApplyCollBtn.Enable();

				BOOL bEnable = STATE_CHECK == aApplyCollBtn.GetState() &&
											aApplyCollBox.GetEntryCount();
				aApplyCollBox.Enable(bEnable);
				if(!bHtmlMode)
				{
					aPagenumText.Enable(bEnable);
					aPagenumEdit.Enable(bEnable);
				}
			}
			break;

		case STATE_NOCHECK:
		case STATE_DONTKNOW:
			aApplyCollBtn.SetState( STATE_NOCHECK );
			aApplyCollBtn.Enable(FALSE);
			aApplyCollBox.Enable(FALSE);
			aPagenumText.Enable(FALSE);
			aPagenumEdit.Enable(FALSE);
            aBreakTypeFT.Enable(FALSE);
            aBreakTypeLB.Enable(FALSE);
            aBreakPositionFT.Enable(FALSE);
            aBreakPositionLB.Enable(FALSE);
			break;
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, KeepTogetherHdl_Impl, TriStateBox *, EMPTYARG )
{
	BOOL bEnable = aKeepTogetherBox.GetState() == STATE_NOCHECK;
	aWidowBox.Enable(bEnable);
	aOrphanBox.Enable(bEnable);

	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, WidowHdl_Impl, TriStateBox *, EMPTYARG )
{
	switch ( aWidowBox.GetState() )
	{
		case STATE_CHECK:
			aWidowRowNo.Enable();
			aWidowRowLabel.Enable();
			aKeepTogetherBox.Enable(FALSE);
			break;

		case STATE_NOCHECK:
			if ( aOrphanBox.GetState() == STATE_NOCHECK )
				aKeepTogetherBox.Enable();

		// kein break
		case STATE_DONTKNOW:
			aWidowRowNo.Enable(FALSE);
			aWidowRowLabel.Enable(FALSE);
			break;
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, OrphanHdl_Impl, TriStateBox *, EMPTYARG )
{
	switch( aOrphanBox.GetState() )
	{
		case STATE_CHECK:
			aOrphanRowNo.Enable();
			aOrphanRowLabel.Enable();
			aKeepTogetherBox.Enable(FALSE);
			break;

		case STATE_NOCHECK:
			if ( aWidowBox.GetState() == STATE_NOCHECK )
				aKeepTogetherBox.Enable();

		// kein break
		case STATE_DONTKNOW:
			aOrphanRowNo.Enable(FALSE);
			aOrphanRowLabel.Enable(FALSE);
			break;
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, HyphenClickHdl_Impl, TriStateBox *, EMPTYARG )
{

	BOOL bEnable = aHyphenBox.GetState() == STATE_CHECK;
	aBeforeText.Enable(bEnable);
	aExtHyphenBeforeBox.Enable(bEnable);
	aAfterText.Enable(bEnable);
	aExtHyphenAfterBox.Enable(bEnable);
	aMaxHyphenLabel.Enable(bEnable);
	aMaxHyphenEdit.Enable(bEnable);
	aHyphenBox.SetState( bEnable ? STATE_CHECK : STATE_NOCHECK);

	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, ApplyCollClickHdl_Impl, TriStateBox *, EMPTYARG )
{
	BOOL bEnable = FALSE;
	if ( aApplyCollBtn.GetState() == STATE_CHECK &&
		 aApplyCollBox.GetEntryCount() )
	{
		bEnable = TRUE;
		aApplyCollBox.SelectEntryPos( nStdPos );
	}
	else
	{
		aApplyCollBox.SetNoSelection();
	}
	aApplyCollBox.Enable(bEnable);
	if(!bHtmlMode)
	{
		aPagenumText.Enable(bEnable);
		aPagenumEdit.Enable(bEnable);
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, PageBreakPosHdl_Impl, ListBox *, pListBox )
{
    if ( 0 == pListBox->GetSelectEntryPos() )
    {
        aApplyCollBtn.Enable();

        BOOL bEnable = aApplyCollBtn.GetState() == STATE_CHECK &&
                                    aApplyCollBox.GetEntryCount();

        aApplyCollBox.Enable(bEnable);
        if(!bHtmlMode)
        {
            aPagenumText.Enable(bEnable);
            aPagenumEdit.Enable(bEnable);
        }
    }
    else if ( 1 == pListBox->GetSelectEntryPos() )
    {
        aApplyCollBtn.SetState( STATE_NOCHECK );
        aApplyCollBtn.Enable(FALSE);
        aApplyCollBox.Enable(FALSE);
        aPagenumText.Enable(FALSE);
        aPagenumEdit.Enable(FALSE);
    }
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxExtParagraphTabPage, PageBreakTypeHdl_Impl, ListBox *, pListBox )
{
    //column break or break break after
    USHORT nBreakPos = aBreakPositionLB.GetSelectEntryPos();
    if ( pListBox->GetSelectEntryPos() == 1 || 1 == nBreakPos)
	{
		aApplyCollBtn.SetState( STATE_NOCHECK );
		aApplyCollBtn.Enable(FALSE);
		aApplyCollBox.Enable(FALSE);
		aPagenumText.Enable(FALSE);
		aPagenumEdit.Enable(FALSE);
	}
    else
        PageBreakPosHdl_Impl( &aBreakPositionLB );
	return 0;
}
//Add CHINA001 begin
void SvxExtParagraphTabPage::PageCreated(SfxAllItemSet aSet)
{


	SFX_ITEMSET_ARG	(&aSet,pDisablePageBreakItem,SfxBoolItem,SID_DISABLE_SVXEXTPARAGRAPHTABPAGE_PAGEBREAK,sal_False);

	if (pDisablePageBreakItem)
		if ( pDisablePageBreakItem->GetValue())
					DisablePageBreak();


}
//end of Add CHINA001
/*-- 29.11.00 11:36:24---------------------------------------------------

  -----------------------------------------------------------------------*/
SvxAsianTabPage::SvxAsianTabPage( Window* pParent, const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES( RID_SVXPAGE_PARA_ASIAN ), rSet),
    aOptionsFL(         this, SVX_RES(FL_AS_OPTIONS       )),
    aForbiddenRulesCB(  this, SVX_RES(CB_AS_FORBIDDEN     )),
    aHangingPunctCB(    this, SVX_RES(CB_AS_HANG_PUNC     )),
	aScriptSpaceCB(     this, SVX_RES(CB_AS_SCRIPT_SPACE	))//,

{
	FreeResource();

	Link aLink = LINK( this, SvxAsianTabPage, ClickHdl_Impl );
	aHangingPunctCB.SetClickHdl( aLink );
	aScriptSpaceCB.SetClickHdl( aLink );
	aForbiddenRulesCB.SetClickHdl( aLink );

}
/*-- 29.11.00 11:36:24---------------------------------------------------

  -----------------------------------------------------------------------*/
SvxAsianTabPage::~SvxAsianTabPage()
{
}
/*-- 29.11.00 11:36:24---------------------------------------------------

  -----------------------------------------------------------------------*/
SfxTabPage*	SvxAsianTabPage::Create(	Window* pParent, const SfxItemSet& rSet )
{
	return new SvxAsianTabPage(pParent, rSet);
}
/*-- 29.11.00 11:36:24---------------------------------------------------

  -----------------------------------------------------------------------*/
USHORT*		SvxAsianTabPage::GetRanges()
{
	static USHORT pRanges[] =
	{
		SID_ATTR_PARA_SCRIPTSPACE, SID_ATTR_PARA_FORBIDDEN_RULES,
		0
	};
	return pRanges;
}
/*-- 29.11.00 11:36:24---------------------------------------------------

  -----------------------------------------------------------------------*/
BOOL		SvxAsianTabPage::FillItemSet( SfxItemSet& rSet )
{
	BOOL bRet = FALSE;
	SfxItemPool* pPool = rSet.GetPool();
	if(aScriptSpaceCB.IsChecked() != aScriptSpaceCB.GetSavedValue())
	{
		SfxBoolItem* pNewItem = (SfxBoolItem*)rSet.Get(
			pPool->GetWhich(SID_ATTR_PARA_SCRIPTSPACE)).Clone();
		pNewItem->SetValue(aScriptSpaceCB.IsChecked());
		rSet.Put(*pNewItem);
		delete pNewItem;
		bRet = TRUE;
	}
	if(aHangingPunctCB.IsChecked() != aHangingPunctCB.GetSavedValue())
	{
		SfxBoolItem* pNewItem = (SfxBoolItem*)rSet.Get(
			pPool->GetWhich(SID_ATTR_PARA_HANGPUNCTUATION)).Clone();
		pNewItem->SetValue(aHangingPunctCB.IsChecked());
		rSet.Put(*pNewItem);
		delete pNewItem;
		bRet = TRUE;
	}
	if(aForbiddenRulesCB.IsChecked() != aForbiddenRulesCB.GetSavedValue())
	{
		SfxBoolItem* pNewItem = (SfxBoolItem*)rSet.Get(
			pPool->GetWhich(SID_ATTR_PARA_FORBIDDEN_RULES)).Clone();
		pNewItem->SetValue(aForbiddenRulesCB.IsChecked());
		rSet.Put(*pNewItem);
		delete pNewItem;
		bRet = TRUE;
	}
	return bRet;
}
/*-- 29.11.00 11:36:25---------------------------------------------------

  -----------------------------------------------------------------------*/
void lcl_SetBox(const SfxItemSet& rSet, USHORT nSlotId, TriStateBox& rBox)
{
    USHORT _nWhich = rSet.GetPool()->GetWhich(nSlotId);
    SfxItemState eState = rSet.GetItemState(_nWhich, TRUE);
	if(!eState || eState == SFX_ITEM_DISABLED )
		rBox.Enable(FALSE);
	else if(eState >= SFX_ITEM_AVAILABLE)
	{
		rBox.EnableTriState( FALSE );
        rBox.Check(((const SfxBoolItem&)rSet.Get(_nWhich)).GetValue());
	}
	else
		rBox.SetState( STATE_DONTKNOW );
	rBox.SaveValue();
}


void SvxAsianTabPage::Reset( const SfxItemSet& rSet )
{
	lcl_SetBox(rSet, SID_ATTR_PARA_FORBIDDEN_RULES, aForbiddenRulesCB );
//	lcl_SetBox(rSet, , aAllowWordBreakCB );
	lcl_SetBox(rSet, SID_ATTR_PARA_HANGPUNCTUATION, aHangingPunctCB );


	//character distance not yet available
//	lcl_SetBox(rSet, , aPuntuationCB    );
	lcl_SetBox(rSet, SID_ATTR_PARA_SCRIPTSPACE, aScriptSpaceCB );
//	lcl_SetBox(rSet, , aAdjustNumbersCB );
//	aAllowWordBreakCB	.Enable(FALSE);
//	aPuntuationCB		.Enable(FALSE);
//	aAdjustNumbersCB	.Enable(FALSE);
}
/* -----------------------------19.12.00 12:59--------------------------------

 ---------------------------------------------------------------------------*/
IMPL_LINK( SvxAsianTabPage, ClickHdl_Impl, TriStateBox*, pBox )
{
	pBox->EnableTriState( FALSE );
	return 0;
}
