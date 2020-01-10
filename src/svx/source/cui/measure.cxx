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
#include <sfx2/app.hxx>
#include <sfx2/module.hxx>
#include <tools/shl.hxx>

#include <svx/dialogs.hrc>

#define _SVX_MEASURE_CXX

#include <svx/svdomeas.hxx>
#include <svx/svdattr.hxx>
#include <svx/svdattrx.hxx>
#include <svx/svdview.hxx>

#include "measctrl.hxx"
#include "measure.hxx"
#include "measure.hrc"
#include <svx/dialmgr.hxx>
#include "dlgutil.hxx"
#include <svx/strarray.hxx>
#include <sfx2/request.hxx> //add CHINA001 
#include "ofaitem.hxx" //add CHINA001 

static USHORT pRanges[] =
{
	SDRATTR_MEASURE_FIRST,
	SDRATTR_MEASURE_LAST,
	0
};

/*************************************************************************
|*
|* Dialog to change measure-attributes
|*
\************************************************************************/

SvxMeasureDialog::SvxMeasureDialog( Window* pParent, const SfxItemSet& rInAttrs,
								const SdrView* pSdrView ) :
		SfxSingleTabDialog( pParent, rInAttrs, RID_SVXPAGE_MEASURE )
{
    SvxMeasurePage* _pPage = new SvxMeasurePage( this, rInAttrs );

    _pPage->SetView( pSdrView );
    _pPage->Construct();

    SetTabPage( _pPage );
    SetText( _pPage->GetText() );
}

/*************************************************************************
|*
|* Dtor
|*
\************************************************************************/

SvxMeasureDialog::~SvxMeasureDialog()
{
}

/*************************************************************************
|*
|* Tabpage for changing measure-attributes
|*
\************************************************************************/

SvxMeasurePage::SvxMeasurePage( Window* pWindow, const SfxItemSet& rInAttrs ) :
				SvxTabPage		( pWindow, SVX_RES( RID_SVXPAGE_MEASURE ),
								  rInAttrs ),

        aFlLine                 ( this, SVX_RES( FL_LINE ) ),
		aFtLineDist				( this, SVX_RES( FT_LINE_DIST ) ),
		aMtrFldLineDist			( this, SVX_RES( MTR_LINE_DIST ) ),
		aFtHelplineOverhang		( this, SVX_RES( FT_HELPLINE_OVERHANG ) ),
		aMtrFldHelplineOverhang	( this, SVX_RES( MTR_FLD_HELPLINE_OVERHANG ) ),
		aFtHelplineDist			( this, SVX_RES( FT_HELPLINE_DIST ) ),
		aMtrFldHelplineDist		( this, SVX_RES( MTR_FLD_HELPLINE_DIST ) ),
		aFtHelpline1Len			( this, SVX_RES( FT_HELPLINE1_LEN ) ),
		aMtrFldHelpline1Len		( this, SVX_RES( MTR_FLD_HELPLINE1_LEN ) ),
		aFtHelpline2Len			( this, SVX_RES( FT_HELPLINE2_LEN ) ),
		aMtrFldHelpline2Len		( this, SVX_RES( MTR_FLD_HELPLINE2_LEN ) ),
		aTsbBelowRefEdge		( this, SVX_RES( TSB_BELOW_REF_EDGE ) ),
		aFtDecimalPlaces		( this, SVX_RES( FT_DECIMALPLACES ) ),
		aMtrFldDecimalPlaces	( this, SVX_RES( MTR_FLD_DECIMALPLACES ) ),

		aFlLabel    			( this, SVX_RES( FL_LABEL ) ),
        aFtPosition             ( this, SVX_RES( FT_POSITION ) ),
        aCtlPosition            ( this, SVX_RES( CTL_POSITION ) ),
        aTsbAutoPosV            ( this, SVX_RES( TSB_AUTOPOSV ) ),
        aTsbAutoPosH            ( this, SVX_RES( TSB_AUTOPOSH ) ),
        aTsbShowUnit            ( this, SVX_RES( TSB_SHOW_UNIT ) ),
        aLbUnit                 ( this, SVX_RES( LB_UNIT ) ),
        aTsbParallel            ( this, SVX_RES( TSB_PARALLEL ) ),
        aCtlPreview             ( this, SVX_RES( CTL_PREVIEW ), rInAttrs ),
        
        aFlVert                 ( this, SVX_RES( FL_VERT ) ),
        rOutAttrs               ( rInAttrs ),
        aAttrSet                ( *rInAttrs.GetPool() ),
        pView( 0 ),
        
        bPositionModified       ( FALSE )
{
	FillUnitLB();

	FreeResource();

	const FieldUnit eFUnit = GetModuleFieldUnit( &rInAttrs );
	SetFieldUnit( aMtrFldLineDist, eFUnit );
	SetFieldUnit( aMtrFldHelplineOverhang, eFUnit );
	SetFieldUnit( aMtrFldHelplineDist, eFUnit );
	SetFieldUnit( aMtrFldHelpline1Len, eFUnit );
	SetFieldUnit( aMtrFldHelpline2Len, eFUnit );
	if( eFUnit == FUNIT_MM )
	{
		aMtrFldLineDist.SetSpinSize( 50 );
		aMtrFldHelplineOverhang.SetSpinSize( 50 );
		aMtrFldHelplineDist.SetSpinSize( 50 );
		aMtrFldHelpline1Len.SetSpinSize( 50 );
		aMtrFldHelpline2Len.SetSpinSize( 50 );
	}

	aTsbAutoPosV.SetClickHdl( LINK( this, SvxMeasurePage, ClickAutoPosHdl_Impl ) );
	aTsbAutoPosH.SetClickHdl( LINK( this, SvxMeasurePage, ClickAutoPosHdl_Impl ) );

	// set background and border of iconchoicectrl
	const StyleSettings& rStyles = Application::GetSettings().GetStyleSettings();
	aCtlPreview.SetBackground ( rStyles.GetWindowColor() );
	aCtlPreview.SetBorderStyle(WINDOW_BORDER_MONO);

	Link aLink( LINK( this, SvxMeasurePage, ChangeAttrHdl_Impl ) );
	aMtrFldLineDist.SetModifyHdl( aLink );
	aMtrFldHelplineOverhang.SetModifyHdl( aLink );
	aMtrFldHelplineDist.SetModifyHdl( aLink );
	aMtrFldHelpline1Len.SetModifyHdl( aLink );
	aMtrFldHelpline2Len.SetModifyHdl( aLink );
	aMtrFldDecimalPlaces.SetModifyHdl( aLink );
	aTsbBelowRefEdge.SetClickHdl( aLink );
	aTsbParallel.SetClickHdl( aLink );
	aTsbShowUnit.SetClickHdl( aLink );
	aLbUnit.SetSelectHdl( aLink );
}

/*************************************************************************
|*
|* Dtor
|*
\************************************************************************/

SvxMeasurePage::~SvxMeasurePage()
{
}

/*************************************************************************
|*
|* read the delivered Item-Set
|*
\************************************************************************/

void __EXPORT SvxMeasurePage::Reset( const SfxItemSet& rAttrs )
{
	SfxItemPool* pPool = rAttrs.GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool" );
	eUnit = pPool->GetMetric( SDRATTR_MEASURELINEDIST );

	const SfxPoolItem* pItem = GetItem( rAttrs, SDRATTR_MEASURELINEDIST );

	// SdrMeasureLineDistItem
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASURELINEDIST );
	if( pItem )
	{
		long nValue = ( ( const SdrMeasureLineDistItem* )pItem )->GetValue();
		SetMetricValue( aMtrFldLineDist, nValue, eUnit );
	}
	else
	{
		aMtrFldLineDist.SetText( String() );
	}
	aMtrFldLineDist.SaveValue();

	// SdrMeasureHelplineOverhangItem
	pItem = GetItem( rAttrs, SDRATTR_MEASUREHELPLINEOVERHANG );
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASUREHELPLINEOVERHANG );
	if( pItem )
	{
		long nValue = ( ( const SdrMeasureHelplineOverhangItem* )pItem )->GetValue();
		SetMetricValue( aMtrFldHelplineOverhang, nValue, eUnit );
	}
	else
	{
		aMtrFldHelplineOverhang.SetText( String() );
	}
	aMtrFldHelplineOverhang.SaveValue();

	// SdrMeasureHelplineDistItem
	pItem = GetItem( rAttrs, SDRATTR_MEASUREHELPLINEDIST );
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASUREHELPLINEDIST );
	if( pItem )
	{
		long nValue = ( ( const SdrMeasureHelplineDistItem* )pItem )->GetValue();
		SetMetricValue( aMtrFldHelplineDist, nValue, eUnit );
	}
	else
	{
		aMtrFldHelplineDist.SetText( String() );
	}
	aMtrFldHelplineDist.SaveValue();

	// SdrMeasureHelpline1LenItem
	pItem = GetItem( rAttrs, SDRATTR_MEASUREHELPLINE1LEN );
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASUREHELPLINE1LEN );
	if( pItem )
	{
		long nValue = ( ( const SdrMeasureHelpline1LenItem* )pItem )->GetValue();
		SetMetricValue( aMtrFldHelpline1Len, nValue, eUnit );
	}
	else
	{
		aMtrFldHelpline1Len.SetText( String() );
	}
	aMtrFldHelpline1Len.SaveValue();

	// SdrMeasureHelpline2LenItem
	pItem = GetItem( rAttrs, SDRATTR_MEASUREHELPLINE2LEN );
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASUREHELPLINE2LEN );
	if( pItem )
	{
		long nValue = ( ( const SdrMeasureHelpline2LenItem* )pItem )->GetValue();
		SetMetricValue( aMtrFldHelpline2Len, nValue, eUnit );
	}
	else
	{
		aMtrFldHelpline2Len.SetText( String() );
	}
	aMtrFldHelpline2Len.SaveValue();

	// SdrMeasureBelowRefEdgeItem
	if( rAttrs.GetItemState( SDRATTR_MEASUREBELOWREFEDGE ) != SFX_ITEM_DONTCARE )
	{
		aTsbBelowRefEdge.SetState( ( ( const SdrMeasureBelowRefEdgeItem& )rAttrs.Get( SDRATTR_MEASUREBELOWREFEDGE ) ).
						GetValue() ? STATE_CHECK : STATE_NOCHECK );
		aTsbBelowRefEdge.EnableTriState( FALSE );
	}
	else
	{
		aTsbBelowRefEdge.SetState( STATE_DONTKNOW );
	}
	aTsbBelowRefEdge.SaveValue();

	// SdrMeasureDecimalPlacesItem
	pItem = GetItem( rAttrs, SDRATTR_MEASUREDECIMALPLACES );
    if( pItem == NULL )
        pItem = &pPool->GetDefaultItem( SDRATTR_MEASUREDECIMALPLACES );
	if( pItem )
	{
		INT16 nValue = ( ( const SdrMeasureDecimalPlacesItem* )pItem )->GetValue();
		aMtrFldDecimalPlaces.SetValue( nValue );
	}
	else
	{
		aMtrFldDecimalPlaces.SetText( String() );
	}
	aMtrFldDecimalPlaces.SaveValue();

	// SdrMeasureTextRota90Item
	// Attention: negate !
	if( rAttrs.GetItemState( SDRATTR_MEASURETEXTROTA90 ) != SFX_ITEM_DONTCARE )
	{
		aTsbParallel.SetState( ( ( const SdrMeasureTextRota90Item& )rAttrs.Get( SDRATTR_MEASURETEXTROTA90 ) ).
						GetValue() ? STATE_NOCHECK : STATE_CHECK );
		aTsbParallel.EnableTriState( FALSE );
	}
	else
	{
		aTsbParallel.SetState( STATE_DONTKNOW );
	}
	aTsbParallel.SaveValue();

	// SdrMeasureShowUnitItem
	if( rAttrs.GetItemState( SDRATTR_MEASURESHOWUNIT ) != SFX_ITEM_DONTCARE )
	{
		aTsbShowUnit.SetState( ( ( const SdrMeasureShowUnitItem& )rAttrs.Get( SDRATTR_MEASURESHOWUNIT ) ).
						GetValue() ? STATE_CHECK : STATE_NOCHECK );
		aTsbShowUnit.EnableTriState( FALSE );
	}
	else
	{
		aTsbShowUnit.SetState( STATE_DONTKNOW );
	}
	aTsbShowUnit.SaveValue();

	// SdrMeasureUnitItem
	if( rAttrs.GetItemState( SDRATTR_MEASUREUNIT ) != SFX_ITEM_DONTCARE )
	{
		long nFieldUnit = (long) ( ( const SdrMeasureUnitItem& )rAttrs.
									Get( SDRATTR_MEASUREUNIT ) ).GetValue();

		for( USHORT i = 0; i < aLbUnit.GetEntryCount(); ++i )
		{
			if ( (long)aLbUnit.GetEntryData( i ) == nFieldUnit )
			{
				aLbUnit.SelectEntryPos( i );
				break;
			}
		}
	}
	else
	{
		aLbUnit.SetNoSelection();
	}
	aLbUnit.SaveValue();

	// Position
	if ( rAttrs.GetItemState( SDRATTR_MEASURETEXTVPOS ) != SFX_ITEM_DONTCARE )
	{
		SdrMeasureTextVPos eVPos = (SdrMeasureTextVPos)
					( ( const SdrMeasureTextVPosItem& )rAttrs.Get( SDRATTR_MEASURETEXTVPOS ) ).GetValue();
		{
			if ( rAttrs.GetItemState( SDRATTR_MEASURETEXTHPOS ) != SFX_ITEM_DONTCARE )
			{
				aTsbAutoPosV.EnableTriState( FALSE );
				aTsbAutoPosH.EnableTriState( FALSE );

				SdrMeasureTextHPos eHPos = (SdrMeasureTextHPos)
							( ( const SdrMeasureTextHPosItem& )rAttrs.Get( SDRATTR_MEASURETEXTHPOS ) ).GetValue();
				RECT_POINT eRP = RP_MM;
				switch( eVPos )
				{
				case SDRMEASURE_ABOVE:
					switch( eHPos )
					{
					case SDRMEASURE_TEXTLEFTOUTSIDE:	eRP = RP_LT; break;
					case SDRMEASURE_TEXTINSIDE:			eRP = RP_MT; break;
					case SDRMEASURE_TEXTRIGHTOUTSIDE:	eRP = RP_RT; break;
					case SDRMEASURE_TEXTHAUTO:			eRP = RP_MT; break;
					}
					break;
				case SDRMEASURETEXT_VERTICALCENTERED:
					switch( eHPos )
					{
					case SDRMEASURE_TEXTLEFTOUTSIDE:	eRP = RP_LM; break;
					case SDRMEASURE_TEXTINSIDE:			eRP = RP_MM; break;
					case SDRMEASURE_TEXTRIGHTOUTSIDE:	eRP = RP_RM; break;
					case SDRMEASURE_TEXTHAUTO:			eRP = RP_MM; break;
					}
					break;
				case SDRMEASURE_BELOW:
					switch( eHPos )
					{
					case SDRMEASURE_TEXTLEFTOUTSIDE:	eRP = RP_LB; break;
					case SDRMEASURE_TEXTINSIDE:			eRP = RP_MB; break;
					case SDRMEASURE_TEXTRIGHTOUTSIDE:	eRP = RP_RB; break;
					case SDRMEASURE_TEXTHAUTO:			eRP = RP_MB; break;
					}
					break;
				case SDRMEASURE_TEXTVAUTO:
					switch( eHPos )
					{
					case SDRMEASURE_TEXTLEFTOUTSIDE:	eRP = RP_LM; break;
					case SDRMEASURE_TEXTINSIDE:			eRP = RP_MM; break;
					case SDRMEASURE_TEXTRIGHTOUTSIDE:	eRP = RP_RM; break;
					case SDRMEASURE_TEXTHAUTO:			eRP = RP_MM; break;
					}
					break;
                 default: ;//prevent warning
				}

				CTL_STATE nState = 0;

				if( eHPos == SDRMEASURE_TEXTHAUTO )
				{
					aTsbAutoPosH.SetState( STATE_CHECK );
					nState = CS_NOHORZ;
				}

				if( eVPos == SDRMEASURE_TEXTVAUTO )
				{
					aTsbAutoPosV.SetState( STATE_CHECK );
					nState |= CS_NOVERT;
				}

				aCtlPosition.SetState( nState );
				aCtlPosition.SetActualRP( eRP );
			}
		}
	}
	else
	{
		aCtlPosition.Reset();
		aTsbAutoPosV.SetState( STATE_DONTKNOW );
		aTsbAutoPosH.SetState( STATE_DONTKNOW );
	}

	// put the attributes to the preview-control,
	// otherwise the control don't know about
	// the settings of the dialog (#67930)
	ChangeAttrHdl_Impl( &aTsbShowUnit );
	aCtlPreview.SetAttributes( rAttrs );

	bPositionModified = FALSE;
}

/*************************************************************************
|*
|* Fill the delivered Item-Set with dialogbox-attributes
|*
\************************************************************************/

BOOL SvxMeasurePage::FillItemSet( SfxItemSet& rAttrs)
{
	BOOL	 bModified = FALSE;
	INT32	 nValue;
	TriState eState;

	if( aMtrFldLineDist.GetText() != aMtrFldLineDist.GetSavedValue() )
	{
		nValue = GetCoreValue( aMtrFldLineDist, eUnit );
		rAttrs.Put( SdrMeasureLineDistItem( nValue ) );
		bModified = TRUE;
	}

	if( aMtrFldHelplineOverhang.GetText() != aMtrFldHelplineOverhang.GetSavedValue() )
	{
		nValue = GetCoreValue( aMtrFldHelplineOverhang, eUnit );
		rAttrs.Put( SdrMeasureHelplineOverhangItem( nValue ) );
		bModified = TRUE;
	}

	if( aMtrFldHelplineDist.GetText() != aMtrFldHelplineDist.GetSavedValue() )
	{
		nValue = GetCoreValue( aMtrFldHelplineDist, eUnit );
		rAttrs.Put( SdrMeasureHelplineDistItem( nValue ) );
		bModified = TRUE;
	}

	if( aMtrFldHelpline1Len.GetText() != aMtrFldHelpline1Len.GetSavedValue() )
	{
		nValue = GetCoreValue( aMtrFldHelpline1Len, eUnit );
		rAttrs.Put( SdrMeasureHelpline1LenItem( nValue ) );
		bModified = TRUE;
	}

	if( aMtrFldHelpline2Len.GetText() != aMtrFldHelpline2Len.GetSavedValue() )
	{
		nValue = GetCoreValue( aMtrFldHelpline2Len, eUnit );
		rAttrs.Put( SdrMeasureHelpline2LenItem( nValue ) );
		bModified = TRUE;
	}

	eState = aTsbBelowRefEdge.GetState();
	if( eState != aTsbBelowRefEdge.GetSavedValue() )
	{
		rAttrs.Put( SdrMeasureBelowRefEdgeItem( (BOOL) STATE_CHECK == eState ) );
		bModified = TRUE;
	}

	if( aMtrFldDecimalPlaces.GetText() != aMtrFldDecimalPlaces.GetSavedValue() )
	{
		nValue = static_cast<INT32>(aMtrFldDecimalPlaces.GetValue());
		rAttrs.Put(
            SdrMeasureDecimalPlacesItem(
                sal::static_int_cast< INT16 >( nValue ) ) );
		bModified = TRUE;
	}

	eState = aTsbParallel.GetState();
	if( eState != aTsbParallel.GetSavedValue() )
	{
		rAttrs.Put( SdrMeasureTextRota90Item( (BOOL) STATE_NOCHECK == eState ) );
		bModified = TRUE;
	}

	eState = aTsbShowUnit.GetState();
	if( eState != aTsbShowUnit.GetSavedValue() )
	{
		rAttrs.Put( SdrMeasureShowUnitItem( (BOOL) STATE_CHECK == eState ) );
		bModified = TRUE;
	}

	USHORT nPos = aLbUnit.GetSelectEntryPos();
	if( nPos != aLbUnit.GetSavedValue() )
	{
		if( nPos != LISTBOX_ENTRY_NOTFOUND )
		{
			USHORT nFieldUnit = (USHORT)(long)aLbUnit.GetEntryData( nPos );
            FieldUnit _eUnit = (FieldUnit) nFieldUnit;
            rAttrs.Put( SdrMeasureUnitItem( _eUnit ) );
			bModified = TRUE;
		}
	}

//enum SdrMeasureTextHPos {SDRMEASURE_TEXTHAUTO,SDRMEASURE_TEXTLEFTOUTSIDE,SDRMEASURE_TEXTINSIDE,SDRMEASURE_TEXTRIGHTOUTSIDE};
//enum SdrMeasureTextVPos {SDRMEASURE_TEXTVAUTO,SDRMEASURE_ABOVE,SDRMEASURETEXT_VERTICALCENTERED,SDRMEASURE_BELOW};

	if( bPositionModified )
	{
		// Position
		SdrMeasureTextVPos eVPos, eOldVPos;
		SdrMeasureTextHPos eHPos, eOldHPos;

		RECT_POINT eRP = aCtlPosition.GetActualRP();
		switch( eRP )
		{
			default:
			case RP_LT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_LM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_LB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_MT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_MM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_MB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_RT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
			case RP_RM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
			case RP_RB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
		}
		if( aTsbAutoPosH.GetState() == STATE_CHECK )
			eHPos = SDRMEASURE_TEXTHAUTO;

		if( aTsbAutoPosV.GetState() == STATE_CHECK )
			eVPos = SDRMEASURE_TEXTVAUTO;

		if ( rAttrs.GetItemState( SDRATTR_MEASURETEXTVPOS ) != SFX_ITEM_DONTCARE )
		{
			eOldVPos = (SdrMeasureTextVPos)
						( ( const SdrMeasureTextVPosItem& )rOutAttrs.Get( SDRATTR_MEASURETEXTVPOS ) ).GetValue();
			if( eOldVPos != eVPos )
			{
				rAttrs.Put( SdrMeasureTextVPosItem( eVPos ) );
				bModified = TRUE;
			}
		}
		else
		{
			rAttrs.Put( SdrMeasureTextVPosItem( eVPos ) );
			bModified = TRUE;
		}

		if ( rAttrs.GetItemState( SDRATTR_MEASURETEXTHPOS ) != SFX_ITEM_DONTCARE )
		{
			eOldHPos = (SdrMeasureTextHPos)
						( ( const SdrMeasureTextHPosItem& )rOutAttrs.Get( SDRATTR_MEASURETEXTHPOS ) ).GetValue();
			if( eOldHPos != eHPos )
			{
				rAttrs.Put( SdrMeasureTextHPosItem( eHPos ) );
				bModified = TRUE;
			}
		}
		else
		{
			rAttrs.Put( SdrMeasureTextHPosItem( eHPos ) );
			bModified = TRUE;
		}
	}

	return( bModified );
}

/*************************************************************************
|*
|* The View have to set at the measure-object to be able to notify
|* unit and floatingpoint-values
|*
\************************************************************************/

void SvxMeasurePage::Construct()
{
	DBG_ASSERT( pView, "Keine gueltige View Uebergeben!" );

	aCtlPreview.pMeasureObj->SetModel( pView->GetModel() );
	aCtlPreview.Invalidate();
}

/*************************************************************************
|*
|* create the tabpage
|*
\************************************************************************/

SfxTabPage* SvxMeasurePage::Create( Window* pWindow,
				const SfxItemSet& rAttrs )
{
	return( new SvxMeasurePage( pWindow, rAttrs ) );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

USHORT*	SvxMeasurePage::GetRanges()
{
	return( pRanges );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxMeasurePage::PointChanged( Window* pWindow, RECT_POINT /*eRP*/ )
{
	ChangeAttrHdl_Impl( pWindow );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxMeasurePage, ClickAutoPosHdl_Impl, void *, p )
{
	if( aTsbAutoPosH.GetState() == STATE_CHECK )
	{
		switch( aCtlPosition.GetActualRP() )
		{
			case RP_LT:
			case RP_RT:
				aCtlPosition.SetActualRP( RP_MT );
			break;

			case RP_LM:
			case RP_RM:
				aCtlPosition.SetActualRP( RP_MM );
			break;

			case RP_LB:
			case RP_RB:
				aCtlPosition.SetActualRP( RP_MB );
			break;
            default: ;//prevent warning
		}
	}
	if( aTsbAutoPosV.GetState() == STATE_CHECK )
	{
		switch( aCtlPosition.GetActualRP() )
		{
			case RP_LT:
			case RP_LB:
				aCtlPosition.SetActualRP( RP_LM );
			break;

			case RP_MT:
			case RP_MB:
				aCtlPosition.SetActualRP( RP_MM );
			break;

			case RP_RT:
			case RP_RB:
				aCtlPosition.SetActualRP( RP_RM );
			break;
            default: ;//prevent warning
		}
	}
	ChangeAttrHdl_Impl( p );

	return( 0L );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxMeasurePage, ChangeAttrHdl_Impl, void *, p )
{

	if( p == &aMtrFldLineDist )
	{
		INT32 nValue = GetCoreValue( aMtrFldLineDist, eUnit );
		aAttrSet.Put( SdrMeasureLineDistItem( nValue ) );
	}

	if( p == &aMtrFldHelplineOverhang )
	{
		INT32 nValue = GetCoreValue( aMtrFldHelplineOverhang, eUnit );
		aAttrSet.Put( SdrMeasureHelplineOverhangItem( nValue) );
	}

	if( p == &aMtrFldHelplineDist )
	{
		INT32 nValue = GetCoreValue( aMtrFldHelplineDist, eUnit );
		aAttrSet.Put( SdrMeasureHelplineDistItem( nValue) );
	}

	if( p == &aMtrFldHelpline1Len )
	{
		INT32 nValue = GetCoreValue( aMtrFldHelpline1Len, eUnit );
		aAttrSet.Put( SdrMeasureHelpline1LenItem( nValue ) );
	}

	if( p == &aMtrFldHelpline2Len )
	{
		INT32 nValue = GetCoreValue( aMtrFldHelpline2Len, eUnit );
		aAttrSet.Put( SdrMeasureHelpline2LenItem( nValue ) );
	}

	if( p == &aTsbBelowRefEdge )
	{
		TriState eState = aTsbBelowRefEdge.GetState();
		if( eState != STATE_DONTKNOW )
			aAttrSet.Put( SdrMeasureBelowRefEdgeItem( (BOOL) STATE_CHECK == eState ) );
	}

	if( p == &aMtrFldDecimalPlaces )
	{
		INT16 nValue = sal::static_int_cast< INT16 >(
            aMtrFldDecimalPlaces.GetValue() );
		aAttrSet.Put( SdrMeasureDecimalPlacesItem( nValue ) );
	}

	if( p == &aTsbParallel )
	{
		TriState eState = aTsbParallel.GetState();
		if( eState != STATE_DONTKNOW )
			aAttrSet.Put( SdrMeasureTextRota90Item( (BOOL) !STATE_CHECK == eState ) );
	}

	if( p == &aTsbShowUnit )
	{
		TriState eState = aTsbShowUnit.GetState();
		if( eState != STATE_DONTKNOW )
			aAttrSet.Put( SdrMeasureShowUnitItem( (BOOL) STATE_CHECK == eState ) );
	}

	if( p == &aLbUnit )
	{
		USHORT nPos = aLbUnit.GetSelectEntryPos();
		if( nPos != LISTBOX_ENTRY_NOTFOUND )
		{
			USHORT nFieldUnit = (USHORT)(long)aLbUnit.GetEntryData( nPos );
            FieldUnit _eUnit = (FieldUnit) nFieldUnit;
            aAttrSet.Put( SdrMeasureUnitItem( _eUnit ) );
		}
	}

	if( p == &aTsbAutoPosV || p == &aTsbAutoPosH || p == &aCtlPosition )
	{
		bPositionModified = TRUE;

		// Position
		RECT_POINT eRP = aCtlPosition.GetActualRP();
		SdrMeasureTextVPos eVPos;
		SdrMeasureTextHPos eHPos;

		switch( eRP )
		{
			default:
			case RP_LT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_LM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_LB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTLEFTOUTSIDE; break;
			case RP_MT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_MM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_MB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTINSIDE; break;
			case RP_RT: eVPos = SDRMEASURE_ABOVE;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
			case RP_RM: eVPos = SDRMEASURETEXT_VERTICALCENTERED;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
			case RP_RB: eVPos = SDRMEASURE_BELOW;
						eHPos = SDRMEASURE_TEXTRIGHTOUTSIDE; break;
		}

		CTL_STATE nState = 0;

		if( aTsbAutoPosH.GetState() == STATE_CHECK )
		{
			eHPos = SDRMEASURE_TEXTHAUTO;
			nState = CS_NOHORZ;
		}

		if( aTsbAutoPosV.GetState() == STATE_CHECK )
		{
			eVPos = SDRMEASURE_TEXTVAUTO;
			nState |= CS_NOVERT;
		}

		if( p == &aTsbAutoPosV || p == &aTsbAutoPosH )
			aCtlPosition.SetState( nState );

		aAttrSet.Put( SdrMeasureTextVPosItem( eVPos ) );
		aAttrSet.Put( SdrMeasureTextHPosItem( eHPos ) );
	}

	aCtlPreview.SetAttributes( aAttrSet );
	aCtlPreview.Invalidate();

	return( 0L );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxMeasurePage::FillUnitLB()
{
	// fill ListBox with metrics
	SvxStringArray aMetricArr( RID_SVXSTR_FIELDUNIT_TABLE );

	long nUnit = FUNIT_NONE;
	String aStrMetric( SVX_RES( STR_MEASURE_AUTOMATIC ) );
	USHORT nPos = aLbUnit.InsertEntry( aStrMetric );
	aLbUnit.SetEntryData( nPos, (void*)nUnit );

	for( USHORT i = 0; i < aMetricArr.Count(); ++i )
	{
		aStrMetric = aMetricArr.GetStringByPos( i );
		nUnit = aMetricArr.GetValue( i );
		nPos = aLbUnit.InsertEntry( aStrMetric );
		aLbUnit.SetEntryData( nPos, (void*)nUnit );
	}
}
void SvxMeasurePage::PageCreated (SfxAllItemSet aSet) //add CHINA001 
{
	SFX_ITEMSET_ARG (&aSet,pOfaPtrItem,OfaPtrItem,SID_OBJECT_LIST,sal_False);
	
	if (pOfaPtrItem)
		SetView( static_cast<SdrView *>(pOfaPtrItem->GetValue()));
	
	Construct();
}

