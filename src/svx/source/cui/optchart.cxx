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
#include <svtools/pathoptions.hxx>

#include <svx/dialogs.hrc>

#include "optchart.hxx"
#include "optchart.hrc"
#include <svx/dialmgr.hxx>

// ====================
// class ChartColorLB
// ====================
void ChartColorLB::FillBox( const SvxChartColorTable & rTab )
{
    long nCount = rTab.size();
	SetUpdateMode( FALSE );

	for( long i = 0; i < nCount; i++ )
	{
        Append( const_cast< XColorEntry * >( & rTab[ i ] ));
	}
	SetUpdateMode( TRUE );
}


// ====================
// class SvxDefaultColorOptPage
// ====================
SvxDefaultColorOptPage::SvxDefaultColorOptPage( Window* pParent, const SfxItemSet& rInAttrs ) :

	SfxTabPage( pParent, SVX_RES( RID_OPTPAGE_CHART_DEFCOLORS ), rInAttrs ),

	aGbChartColors	( this, SVX_RES( FL_CHART_COLOR_LIST ) ),
	aLbChartColors	( this, SVX_RES( LB_CHART_COLOR_LIST ) ),
	aGbColorBox		( this, SVX_RES( FL_COLOR_BOX ) ),
	aValSetColorBox	( this, SVX_RES( CT_COLOR_BOX ) ),
	aPBDefault		( this, SVX_RES( PB_RESET_TO_DEFAULT ) )
{
	FreeResource();

	aPBDefault.SetClickHdl( LINK( this, SvxDefaultColorOptPage, ResetToDefaults ) );
	aLbChartColors.SetSelectHdl( LINK( this, SvxDefaultColorOptPage, ListClickedHdl ) );
	aValSetColorBox.SetSelectHdl( LINK( this, SvxDefaultColorOptPage, BoxClickedHdl ) );

	aValSetColorBox.SetStyle( aValSetColorBox.GetStyle()
									| WB_VSCROLL | WB_ITEMBORDER | WB_NAMEFIELD );
	aValSetColorBox.SetColCount( 8 );
	aValSetColorBox.SetLineCount( 12 );
	aValSetColorBox.SetExtraSpacing( 0 );
	aValSetColorBox.Show();

	pChartOptions = new SvxChartOptions;
	pColorTab = new XColorTable( SvtPathOptions().GetPalettePath() );

	const SfxPoolItem* pItem = NULL;
	if ( rInAttrs.GetItemState( SID_SCH_EDITOPTIONS, FALSE, &pItem ) == SFX_ITEM_SET )
	{
		pColorConfig = SAL_STATIC_CAST( SvxChartColorTableItem*, pItem->Clone() );
	}
	else
	{
		SvxChartColorTable aTable;
		aTable.useDefault();
		pColorConfig = new SvxChartColorTableItem( SID_SCH_EDITOPTIONS, aTable );
		pColorConfig->SetOptions( pChartOptions );
	}

	Construct();
}

SvxDefaultColorOptPage::~SvxDefaultColorOptPage()
{
	// save changes
	pChartOptions->SetDefaultColors( pColorConfig->GetColorTable() );
	pChartOptions->Commit();

	delete pColorConfig;
	delete pColorTab;
	delete pChartOptions;
}

void SvxDefaultColorOptPage::Construct()
{
	if( pColorConfig )
		aLbChartColors.FillBox( pColorConfig->GetColorTable() );

	FillColorBox();

	aLbChartColors.SelectEntryPos( 0 );
	ListClickedHdl( &aLbChartColors );
}


SfxTabPage* __EXPORT SvxDefaultColorOptPage::Create( Window* pParent, const SfxItemSet& rAttrs )
{
	return new SvxDefaultColorOptPage( pParent, rAttrs );
}

BOOL __EXPORT SvxDefaultColorOptPage::FillItemSet( SfxItemSet& rOutAttrs )
{
	if( pColorConfig )
		rOutAttrs.Put( *SAL_STATIC_CAST( SfxPoolItem*, pColorConfig ));

	return TRUE;
}

void __EXPORT SvxDefaultColorOptPage::Reset( const SfxItemSet& )
{
	aLbChartColors.SelectEntryPos( 0 );
	ListClickedHdl( &aLbChartColors );
}

void SvxDefaultColorOptPage::FillColorBox()
{
	if( !pColorTab ) return;

	long nCount = pColorTab->Count();
	XColorEntry* pColorEntry;

	for( long i = 0; i < nCount; i++ )
	{
		pColorEntry = pColorTab->GetColor( i );
		aValSetColorBox.InsertItem( (USHORT) i + 1, pColorEntry->GetColor(), pColorEntry->GetName() );
	}
}


long SvxDefaultColorOptPage::GetColorIndex( const Color& rCol )
{
	if( pColorTab )
	{
		long nCount = pColorTab->Count();
		XColorEntry* pColorEntry;

		for( long i = nCount - 1; i >= 0; i-- )			// default chart colors are at the end of the table
		{
			pColorEntry = pColorTab->GetColor( i );
			if( pColorEntry && pColorEntry->GetColor() == rCol )
				return SAL_STATIC_CAST( XPropertyTable*, pColorTab )->Get( pColorEntry->GetName() );
		}
	}
	return -1L;
}



// --------------------
// event handlers
// --------------------

// ResetToDefaults
// ---------------

IMPL_LINK( SvxDefaultColorOptPage, ResetToDefaults, void *, EMPTYARG )
{
    if( pColorConfig )
    {
        pColorConfig->GetColorTable().useDefault();

        aLbChartColors.Clear();
        aLbChartColors.FillBox( pColorConfig->GetColorTable() );

        aLbChartColors.GetFocus();
    }

	return 0L;
}

// ListClickedHdl
// --------------

IMPL_LINK( SvxDefaultColorOptPage, ListClickedHdl, ChartColorLB*,  pColorList )
{
	Color aCol = pColorList->GetSelectEntryColor();

	long nIndex = GetColorIndex( aCol );

	if( nIndex == -1 )		// not found
	{
		aValSetColorBox.SetNoSelection();
	}
	else
	{
		aValSetColorBox.SelectItem( (USHORT)nIndex + 1 );		// ValueSet is 1-based
	}

	return 0L;
}

// BoxClickedHdl
// -------------

IMPL_LINK( SvxDefaultColorOptPage, BoxClickedHdl, ValueSet*, EMPTYARG )
{
	USHORT nIdx = aLbChartColors.GetSelectEntryPos();
	if( nIdx != LISTBOX_ENTRY_NOTFOUND )
	{
		XColorEntry aEntry( aValSetColorBox.GetItemColor( aValSetColorBox.GetSelectItemId() ),
                            aLbChartColors.GetSelectEntry() );

		aLbChartColors.Modify( & aEntry, nIdx );
		pColorConfig->ReplaceColorByIndex( nIdx, aEntry );

        aLbChartColors.SelectEntryPos( nIdx );	// reselect entry
	}

    return 0L;
}

