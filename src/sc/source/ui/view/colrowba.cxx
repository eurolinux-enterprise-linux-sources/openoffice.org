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

#include <svx/svdtrans.hxx>
#include <unotools/localedatawrapper.hxx>

#include "colrowba.hxx"
#include "document.hxx"
#include "scmod.hxx"
#include "tabvwsh.hxx"
#include "docsh.hxx"
#include "appoptio.hxx"
#include "globstr.hrc"

// STATIC DATA -----------------------------------------------------------

//==================================================================

String lcl_MetricString( long nTwips, const String& rText )
{
	if ( nTwips <= 0 )
		return ScGlobal::GetRscString(STR_TIP_HIDE);
	else
	{
		FieldUnit eUserMet = SC_MOD()->GetAppOptions().GetAppMetric();

        sal_Int64 nUserVal = MetricField::ConvertValue( nTwips*100, 1, 2, FUNIT_TWIP, eUserMet );

		String aStr = rText;
		aStr += ' ';
        aStr += ScGlobal::pLocaleData->getNum( nUserVal, 2 );
		aStr += ' ';
		aStr += SdrFormatter::GetUnitStr(eUserMet);

		return aStr;
	}
}

//==================================================================

ScColBar::ScColBar( Window* pParent, ScViewData* pData, ScHSplitPos eWhichPos,
					ScHeaderFunctionSet* pFunc, ScHeaderSelectionEngine* pEng ) :
			ScHeaderControl( pParent, pEng, MAXCOL+1, HDR_HORIZONTAL ),
			pViewData( pData ),
			eWhich( eWhichPos ),
			pFuncSet( pFunc ),
			pSelEngine( pEng )
{
	Show();
}

ScColBar::~ScColBar()
{
}

inline BOOL ScColBar::UseNumericHeader() const
{
    return pViewData->GetDocument()->GetAddressConvention() == formula::FormulaGrammar::CONV_XL_R1C1;
}

SCCOLROW ScColBar::GetPos()
{
	return pViewData->GetPosX(eWhich);
}

USHORT ScColBar::GetEntrySize( SCCOLROW nEntryNo )
{
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	if ( pDoc->GetColFlags( static_cast<SCCOL>(nEntryNo), nTab ) & CR_HIDDEN )
		return 0;
	else
		return (USHORT) ScViewData::ToPixel( pDoc->GetColWidth( static_cast<SCCOL>(nEntryNo), nTab ), pViewData->GetPPTX() );
}

String ScColBar::GetEntryText( SCCOLROW nEntryNo )
{
	return UseNumericHeader()
	    ? String::CreateFromInt32( nEntryNo + 1 )
	    : ScColToAlpha( static_cast<SCCOL>(nEntryNo) );
}

void ScColBar::SetEntrySize( SCCOLROW nPos, USHORT nNewSize )
{
	USHORT nSizeTwips;
	ScSizeMode eMode = SC_SIZE_DIRECT;
	if (nNewSize>0 && nNewSize<10) nNewSize=10;				// (Pixel)

	if ( nNewSize == HDR_SIZE_OPTIMUM )
	{
		nSizeTwips = STD_EXTRA_WIDTH;
		eMode = SC_SIZE_OPTIMAL;
	}
	else
		nSizeTwips = (USHORT) ( nNewSize / pViewData->GetPPTX() );

	ScMarkData& rMark = pViewData->GetMarkData();
//	SCTAB nTab = pViewData->GetTabNo();

	SCCOLROW* pRanges = new SCCOLROW[MAXCOL+1];
	SCCOL nRangeCnt = 0;
	if ( rMark.IsColumnMarked( static_cast<SCCOL>(nPos) ) )
	{
		SCCOL nStart = 0;
		while (nStart<=MAXCOL)
		{
			while (nStart<MAXCOL && !rMark.IsColumnMarked(nStart))
				++nStart;
			if (rMark.IsColumnMarked(nStart))
			{
				SCCOL nEnd = nStart;
				while (nEnd<MAXCOL && rMark.IsColumnMarked(nEnd))
					++nEnd;
				if (!rMark.IsColumnMarked(nEnd))
					--nEnd;
				pRanges[static_cast<size_t>(2*nRangeCnt)  ] = nStart;
				pRanges[static_cast<size_t>(2*nRangeCnt+1)] = nEnd;
				++nRangeCnt;
				nStart = nEnd+1;
			}
			else
				nStart = MAXCOL+1;
		}
	}
	else
	{
		pRanges[0] = nPos;
		pRanges[1] = nPos;
		nRangeCnt = 1;
	}

	pViewData->GetView()->SetWidthOrHeight( TRUE, nRangeCnt, pRanges, eMode, nSizeTwips );
	delete[] pRanges;
}

void ScColBar::HideEntries( SCCOLROW nStart, SCCOLROW nEnd )
{
	SCCOLROW nRange[2];
	nRange[0] = nStart;
	nRange[1] = nEnd;
	pViewData->GetView()->SetWidthOrHeight( TRUE, 1, nRange, SC_SIZE_DIRECT, 0 );
}

void ScColBar::SetMarking( BOOL bSet )
{
	pViewData->GetMarkData().SetMarking( bSet );
	if (!bSet)
	{
		pViewData->GetView()->UpdateAutoFillMark();
	}
}

void ScColBar::SelectWindow()
{
	ScTabViewShell* pViewSh = pViewData->GetViewShell();

	pViewSh->SetActive();			// Appear und SetViewFrame
	pViewSh->DrawDeselectAll();

	ScSplitPos eActive = pViewData->GetActivePart();
	if (eWhich==SC_SPLIT_LEFT)
	{
		if (eActive==SC_SPLIT_TOPRIGHT)		eActive=SC_SPLIT_TOPLEFT;
		if (eActive==SC_SPLIT_BOTTOMRIGHT)	eActive=SC_SPLIT_BOTTOMLEFT;
	}
	else
	{
		if (eActive==SC_SPLIT_TOPLEFT)		eActive=SC_SPLIT_TOPRIGHT;
		if (eActive==SC_SPLIT_BOTTOMLEFT)	eActive=SC_SPLIT_BOTTOMRIGHT;
	}
	pViewSh->ActivatePart( eActive );

	pFuncSet->SetColumn( TRUE );
	pFuncSet->SetWhich( eActive );

	pViewSh->ActiveGrabFocus();
}

BOOL ScColBar::IsDisabled()
{
	ScModule* pScMod = SC_MOD();
	return pScMod->IsFormulaMode() || pScMod->IsModalMode();
}

BOOL ScColBar::ResizeAllowed()
{
	return !pViewData->HasEditView( pViewData->GetActivePart() ) &&
			!pViewData->GetDocShell()->IsReadOnly();
}

void ScColBar::DrawInvert( long nDragPosP )
{
	Rectangle aRect( nDragPosP,0, nDragPosP+HDR_SLIDERSIZE-1,GetOutputSizePixel().Width()-1 );
	Update();
	Invert(aRect);

	pViewData->GetView()->InvertVertical(eWhich,nDragPosP);
}

String ScColBar::GetDragHelp( long nVal )
{
	long nTwips = (long) ( nVal / pViewData->GetPPTX() );
	return lcl_MetricString( nTwips, ScGlobal::GetRscString(STR_TIP_WIDTH) );
}

BOOL ScColBar::IsLayoutRTL()		// overloaded only for columns
{
	return pViewData->GetDocument()->IsLayoutRTL( pViewData->GetTabNo() );
}

//==================================================================

ScRowBar::ScRowBar( Window* pParent, ScViewData* pData, ScVSplitPos eWhichPos,
					ScHeaderFunctionSet* pFunc, ScHeaderSelectionEngine* pEng ) :
			ScHeaderControl( pParent, pEng, MAXROW+1, HDR_VERTICAL ),
			pViewData( pData ),
			eWhich( eWhichPos ),
			pFuncSet( pFunc ),
			pSelEngine( pEng )
{
	Show();
}

ScRowBar::~ScRowBar()
{
}

SCCOLROW ScRowBar::GetPos()
{
	return pViewData->GetPosY(eWhich);
}

USHORT ScRowBar::GetEntrySize( SCCOLROW nEntryNo )
{
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	if ( pDoc->GetRowFlags( nEntryNo, nTab ) & CR_HIDDEN )
		return 0;
	else
        return (USHORT) ScViewData::ToPixel( pDoc->GetOriginalHeight( nEntryNo,
                    nTab ), pViewData->GetPPTY() );
}

String ScRowBar::GetEntryText( SCCOLROW nEntryNo )
{
	return String::CreateFromInt32( nEntryNo + 1 );
}

void ScRowBar::SetEntrySize( SCCOLROW nPos, USHORT nNewSize )
{
	USHORT nSizeTwips;
	ScSizeMode eMode = SC_SIZE_DIRECT;
	if (nNewSize>0 && nNewSize<10) nNewSize=10;				// (Pixel)

	if ( nNewSize == HDR_SIZE_OPTIMUM )
	{
		nSizeTwips = 0;
		eMode = SC_SIZE_OPTIMAL;
	}
	else
		nSizeTwips = (USHORT) ( nNewSize / pViewData->GetPPTY() );

	ScMarkData& rMark = pViewData->GetMarkData();
//	SCTAB nTab = pViewData->GetTabNo();

	SCCOLROW* pRanges = new SCCOLROW[MAXROW+1];
	SCROW nRangeCnt = 0;
	if ( rMark.IsRowMarked( nPos ) )
	{
		SCROW nStart = 0;
		while (nStart<=MAXROW)
		{
			while (nStart<MAXROW && !rMark.IsRowMarked(nStart))
				++nStart;
			if (rMark.IsRowMarked(nStart))
			{
				SCROW nEnd = nStart;
				while (nEnd<MAXROW && rMark.IsRowMarked(nEnd))
					++nEnd;
				if (!rMark.IsRowMarked(nEnd))
					--nEnd;
				pRanges[static_cast<size_t>(2*nRangeCnt)  ] = nStart;
				pRanges[static_cast<size_t>(2*nRangeCnt+1)] = nEnd;
				++nRangeCnt;
				nStart = nEnd+1;
			}
			else
				nStart = MAXROW+1;
		}
	}
	else
	{
		pRanges[0] = nPos;
		pRanges[1] = nPos;
		nRangeCnt = 1;
	}

	pViewData->GetView()->SetWidthOrHeight( FALSE, nRangeCnt, pRanges, eMode, nSizeTwips );
	delete[] pRanges;
}

void ScRowBar::HideEntries( SCCOLROW nStart, SCCOLROW nEnd )
{
	SCCOLROW nRange[2];
	nRange[0] = nStart;
	nRange[1] = nEnd;
	pViewData->GetView()->SetWidthOrHeight( FALSE, 1, nRange, SC_SIZE_DIRECT, 0 );
}

void ScRowBar::SetMarking( BOOL bSet )
{
	pViewData->GetMarkData().SetMarking( bSet );
	if (!bSet)
	{
		pViewData->GetView()->UpdateAutoFillMark();
	}
}

void ScRowBar::SelectWindow()
{
	ScTabViewShell* pViewSh = pViewData->GetViewShell();

	pViewSh->SetActive();			// Appear und SetViewFrame
	pViewSh->DrawDeselectAll();

	ScSplitPos eActive = pViewData->GetActivePart();
	if (eWhich==SC_SPLIT_TOP)
	{
		if (eActive==SC_SPLIT_BOTTOMLEFT)	eActive=SC_SPLIT_TOPLEFT;
		if (eActive==SC_SPLIT_BOTTOMRIGHT)	eActive=SC_SPLIT_TOPRIGHT;
	}
	else
	{
		if (eActive==SC_SPLIT_TOPLEFT)		eActive=SC_SPLIT_BOTTOMLEFT;
		if (eActive==SC_SPLIT_TOPRIGHT)		eActive=SC_SPLIT_BOTTOMRIGHT;
	}
	pViewSh->ActivatePart( eActive );

	pFuncSet->SetColumn( FALSE );
	pFuncSet->SetWhich( eActive );

	pViewSh->ActiveGrabFocus();
}

BOOL ScRowBar::IsDisabled()
{
	ScModule* pScMod = SC_MOD();
	return pScMod->IsFormulaMode() || pScMod->IsModalMode();
}

BOOL ScRowBar::ResizeAllowed()
{
	return !pViewData->HasEditView( pViewData->GetActivePart() ) &&
			!pViewData->GetDocShell()->IsReadOnly();
}

void ScRowBar::DrawInvert( long nDragPosP )
{
	Rectangle aRect( 0,nDragPosP, GetOutputSizePixel().Width()-1,nDragPosP+HDR_SLIDERSIZE-1 );
	Update();
	Invert(aRect);

	pViewData->GetView()->InvertHorizontal(eWhich,nDragPosP);
}

String ScRowBar::GetDragHelp( long nVal )
{
	long nTwips = (long) ( nVal / pViewData->GetPPTY() );
	return lcl_MetricString( nTwips, ScGlobal::GetRscString(STR_TIP_HEIGHT) );
}

//	GetHiddenCount ist nur fuer Zeilen ueberladen

SCROW ScRowBar::GetHiddenCount( SCROW nEntryNo )
{
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	return pDoc->GetHiddenRowCount( nEntryNo, nTab );
}

BOOL ScRowBar::IsMirrored()			// overloaded only for rows
{
	return pViewData->GetDocument()->IsLayoutRTL( pViewData->GetTabNo() );
}


