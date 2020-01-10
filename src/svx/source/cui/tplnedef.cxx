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
#include <tools/shl.hxx>
#include <tools/urlobj.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/pathoptions.hxx>
#include <sfx2/app.hxx>
#include <sfx2/module.hxx>
#include <sfx2/filedlghelper.hxx>
#include "com/sun/star/ui/dialogs/TemplateDescription.hpp"

#define _SVX_TPLNEDEF_CXX

#include <svx/dialogs.hrc>
#include "tabline.hrc"
#include "dlgname.hrc"
#include "helpid.hrc"




#include "xattr.hxx"
#include <svx/xpool.hxx>
#include <svx/xtable.hxx>

#include "drawitem.hxx"
#include "cuitabline.hxx"
#include "defdlgname.hxx" //CHINA001 #include "dlgname.hxx"
#include <svx/svxdlg.hxx> //CHINA001
#include <svx/dialmgr.hxx>
#include "dlgutil.hxx"

#define DLGWIN this->GetParent()->GetParent()

#define BITMAP_WIDTH   32
#define BITMAP_HEIGHT  12
#define XOUT_WIDTH    150

/*************************************************************************
|*
|*	Dialog zum Definieren von Linienstilen
|*
\************************************************************************/

SvxLineDefTabPage::SvxLineDefTabPage
(
	Window* pParent,
	const SfxItemSet& rInAttrs
) :

	SfxTabPage( pParent, SVX_RES( RID_SVXPAGE_LINE_DEF ), rInAttrs ),

    aFlDefinition   ( this, SVX_RES( FL_DEFINITION ) ),
    aFTLinestyle    ( this, SVX_RES( FT_LINESTYLE ) ),
    aLbLineStyles   ( this, SVX_RES( LB_LINESTYLES ) ),
    aFtType         ( this, SVX_RES( FT_TYPE ) ),
	aLbType1		( this, SVX_RES( LB_TYPE_1 ) ),
	aLbType2		( this, SVX_RES( LB_TYPE_2 ) ),
	aFtNumber		( this, SVX_RES( FT_NUMBER ) ),
	aNumFldNumber1	( this, SVX_RES( NUM_FLD_1 ) ),
	aNumFldNumber2	( this, SVX_RES( NUM_FLD_2 ) ),
    aFtLength       ( this, SVX_RES( FT_LENGTH ) ),
    aMtrLength1     ( this, SVX_RES( MTR_FLD_LENGTH_1 ) ),
    aMtrLength2     ( this, SVX_RES( MTR_FLD_LENGTH_2 ) ),
    aFtDistance     ( this, SVX_RES( FT_DISTANCE ) ),
	aMtrDistance	( this, SVX_RES( MTR_FLD_DISTANCE ) ),
    aCbxSynchronize ( this, SVX_RES( CBX_SYNCHRONIZE ) ),
    aBtnAdd         ( this, SVX_RES( BTN_ADD ) ),
	aBtnModify		( this, SVX_RES( BTN_MODIFY ) ),
	aBtnDelete		( this, SVX_RES( BTN_DELETE ) ),
    aBtnLoad        ( this, SVX_RES( BTN_LOAD ) ),
	aBtnSave		( this, SVX_RES( BTN_SAVE ) ),
    aCtlPreview     ( this, SVX_RES( CTL_PREVIEW ) ),
    
    rOutAttrs       ( rInAttrs ),

    pXPool              ( (XOutdevItemPool*) rInAttrs.GetPool() ),
    aXLStyle            ( XLINE_DASH ),
    aXWidth             ( XOUT_WIDTH ),
    aXDash              ( String(), XDash( XDASH_RECT, 3, 7, 2, 40, 15 ) ),
    aXColor             ( String(), COL_BLACK ),
    aXLineAttr          ( pXPool ),
    rXLSet              ( aXLineAttr.GetItemSet() )
{
	aBtnLoad.SetModeImage( Image( SVX_RES( RID_SVXIMG_LOAD_H ) ), BMP_COLOR_HIGHCONTRAST );
	aBtnSave.SetModeImage( Image( SVX_RES( RID_SVXIMG_SAVE_H ) ), BMP_COLOR_HIGHCONTRAST );

	FreeResource();

	// diese Page braucht ExchangeSupport
	SetExchangeSupport();

	// Metrik einstellen
	eFUnit = GetModuleFieldUnit( &rInAttrs );

	switch ( eFUnit )
	{
		case FUNIT_M:
		case FUNIT_KM:
			eFUnit = FUNIT_MM;
			break;
        default: ; //prevent warning
	}
	SetFieldUnit( aMtrDistance, eFUnit );
	SetFieldUnit( aMtrLength1, eFUnit );
	SetFieldUnit( aMtrLength2, eFUnit );

	// PoolUnit ermitteln
	SfxItemPool* pPool = rOutAttrs.GetPool();
	DBG_ASSERT( pPool, "Wo ist der Pool?" );
	ePoolUnit = pPool->GetMetric( SID_ATTR_LINE_WIDTH );

	rXLSet.Put( aXLStyle );
	rXLSet.Put( aXWidth );
	rXLSet.Put( aXDash );
	rXLSet.Put( aXColor );

	// #i34740#
	aCtlPreview.SetLineAttributes(aXLineAttr.GetItemSet());

	aBtnAdd.SetClickHdl( LINK( this, SvxLineDefTabPage, ClickAddHdl_Impl ) );
	aBtnModify.SetClickHdl(
		LINK( this, SvxLineDefTabPage, ClickModifyHdl_Impl ) );
	aBtnDelete.SetClickHdl(
		LINK( this, SvxLineDefTabPage, ClickDeleteHdl_Impl ) );
	aBtnLoad.SetClickHdl( LINK( this, SvxLineDefTabPage, ClickLoadHdl_Impl ) );
	aBtnSave.SetClickHdl( LINK( this, SvxLineDefTabPage, ClickSaveHdl_Impl ) );

	aNumFldNumber1.SetModifyHdl(
		LINK( this, SvxLineDefTabPage, ChangeNumber1Hdl_Impl ) );
	aNumFldNumber2.SetModifyHdl(
		LINK( this, SvxLineDefTabPage, ChangeNumber2Hdl_Impl ) );
	aLbLineStyles.SetSelectHdl(
		LINK( this, SvxLineDefTabPage, SelectLinestyleHdl_Impl ) );

	// Absolut (in mm) oder Relativ (in %)
	aCbxSynchronize.SetClickHdl(
		LINK( this, SvxLineDefTabPage, ChangeMetricHdl_Impl ) );

	// Wenn sich etwas aendert, muss Preview upgedatet werden werden
	Link aLink = LINK( this, SvxLineDefTabPage, SelectTypeHdl_Impl );
	aLbType1.SetSelectHdl( aLink );
	aLbType2.SetSelectHdl( aLink );
	aLink = LINK( this, SvxLineDefTabPage, ChangePreviewHdl_Impl );
	aMtrLength1.SetModifyHdl( aLink );
	aMtrLength2.SetModifyHdl( aLink );
	aMtrDistance.SetModifyHdl( aLink );

	pDashList = NULL;
}


// -----------------------------------------------------------------------

void SvxLineDefTabPage::Construct()
{
	// Linienstile
	aLbLineStyles.Fill( pDashList );
}

// -----------------------------------------------------------------------

void SvxLineDefTabPage::ActivatePage( const SfxItemSet& )
{
	if( *pDlgType == 0 ) // Flaechen-Dialog
	{
		// ActivatePage() wird aufgerufen bevor der Dialog PageCreated() erhaelt !!!
		if( pDashList )
		{
            if( *pPageType == 1 &&
				*pPosDashLb != LISTBOX_ENTRY_NOTFOUND )
			{
				aLbLineStyles.SelectEntryPos( *pPosDashLb );
			}
			// Damit evtl. vorhandener Linestyle verworfen wird
			SelectLinestyleHdl_Impl( this );

			// Ermitteln (evtl. abschneiden) des Namens und in
			// der GroupBox darstellen
			String			aString( SVX_RES( RID_SVXSTR_TABLE ) ); aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );
			INetURLObject	aURL( pDashList->GetPath() );

			aURL.Append( pDashList->GetName() );
			DBG_ASSERT( aURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

/*			if ( aURL.getBase().Len() > 18 )
			{
				aString += aURL.getBase().Copy( 0, 15 );
				aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
			}
			else
				aString += aURL.getBase();

			aFTLinestyle.SetText( aString );
*/
			*pPageType = 0; // 2
			*pPosDashLb = LISTBOX_ENTRY_NOTFOUND;
		}
	}
}

// -----------------------------------------------------------------------

int SvxLineDefTabPage::DeactivatePage( SfxItemSet* _pSet )
{
	CheckChanges_Impl();

    if( _pSet )
        FillItemSet( *_pSet );

	return( LEAVE_PAGE );
}

// -----------------------------------------------------------------------

void SvxLineDefTabPage::CheckChanges_Impl()
{
	// wird hier benutzt, um Aenderungen NICHT zu verlieren
	//XDashStyle eXDS;

	if( aNumFldNumber1.GetText() 	 != aNumFldNumber1.GetSavedValue() ||
		aMtrLength1.GetText() 		 != aMtrLength1.GetSavedValue() ||
		aLbType1.GetSelectEntryPos() != aLbType1.GetSavedValue() ||
		aNumFldNumber2.GetText() 	 != aNumFldNumber2.GetSavedValue() ||
		aMtrLength2.GetText() 		 != aMtrLength2.GetSavedValue() ||
		aLbType2.GetSelectEntryPos() != aLbType2.GetSavedValue() ||
		aMtrDistance.GetText() 		 != aMtrDistance.GetSavedValue() )
	{
		ResMgr& rMgr = DIALOG_MGR();
        Image aWarningBoxImage = WarningBox::GetStandardImage();
		//CHINA001 SvxMessDialog aMessDlg( DLGWIN,
		//CHINA001 	String( ResId( RID_SVXSTR_LINESTYLE, rMgr ) ),
		//CHINA001 	String( ResId( RID_SVXSTR_ASK_CHANGE_LINESTYLE, rMgr ) ),
		//CHINA001 	&aWarningBoxImage );
		SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
		DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
		AbstractSvxMessDialog* aMessDlg = pFact->CreateSvxMessDialog( DLGWIN, RID_SVXDLG_MESSBOX,
													String( ResId( RID_SVXSTR_LINESTYLE, rMgr ) ),
													String( ResId( RID_SVXSTR_ASK_CHANGE_LINESTYLE, rMgr ) ),
													&aWarningBoxImage );
		DBG_ASSERT(aMessDlg, "Dialogdiet fail!");//CHINA001
		aMessDlg->SetButtonText( MESS_BTN_1, //CHINA001 aMessDlg.SetButtonText( MESS_BTN_1,
								String( ResId( RID_SVXSTR_CHANGE, rMgr ) ) );
		aMessDlg->SetButtonText( MESS_BTN_2, //CHINA001 aMessDlg.SetButtonText( MESS_BTN_2,
								String( ResId( RID_SVXSTR_ADD, rMgr ) ) );

		short nRet = aMessDlg->Execute(); //CHINA001 short nRet = aMessDlg.Execute();

		switch( nRet )
		{
			case RET_BTN_1: // Aendern
			{
				ClickModifyHdl_Impl( this );
				//aXDash = pDashList->Get( nPos )->GetDash();
			}
			break;

			case RET_BTN_2: // Hinzufuegen
			{
				ClickAddHdl_Impl( this );
				//nPos = aLbLineStyles.GetSelectEntryPos();
				//aXDash = pDashList->Get( nPos )->GetDash();
			}
			break;

			case RET_CANCEL:
			break;
			// return( TRUE ); // Abbruch
		}
		delete aMessDlg; //add by CHINA001
	}



	USHORT nPos = aLbLineStyles.GetSelectEntryPos();
	if( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		*pPosDashLb = nPos;
	}
}

// -----------------------------------------------------------------------

BOOL SvxLineDefTabPage::FillItemSet( SfxItemSet& rAttrs )
{
	if( *pDlgType == 0 ) // Linien-Dialog
	{
		if( *pPageType == 2 )
		{
			//CheckChanges_Impl();

			FillDash_Impl();

			String aString( aLbLineStyles.GetSelectEntry() );
			rAttrs.Put( XLineStyleItem( XLINE_DASH ) );
			rAttrs.Put( XLineDashItem( aString, aDash ) );
		}
	}
	return( TRUE );
}

// -----------------------------------------------------------------------

void SvxLineDefTabPage::Reset( const SfxItemSet& rAttrs )
{
	if( rAttrs.GetItemState( GetWhich( XATTR_LINESTYLE ) ) != SFX_ITEM_DONTCARE )
	{
		XLineStyle eXLS = (XLineStyle) ( ( const XLineStyleItem& ) rAttrs.Get( GetWhich( XATTR_LINESTYLE ) ) ).GetValue();

		switch( eXLS )
		{
			case XLINE_NONE:
			case XLINE_SOLID:
				aLbLineStyles.SelectEntryPos( 0 );
				break;

			case XLINE_DASH:
			{
				const XLineDashItem& rDashItem = ( const XLineDashItem& ) rAttrs.Get( XATTR_LINEDASH );
                aDash = rDashItem.GetDashValue();

				aLbLineStyles.SetNoSelection();
				aLbLineStyles.SelectEntry( rDashItem.GetName() );
			}
				break;

			default:
				break;
		}
	}
	SelectLinestyleHdl_Impl( NULL );

	// Status der Buttons ermitteln
	if( pDashList->Count() )
	{
		aBtnModify.Enable();
		aBtnDelete.Enable();
		aBtnSave.Enable();
	}
	else
	{
		aBtnModify.Disable();
		aBtnDelete.Disable();
		aBtnSave.Disable();
	}
}

// -----------------------------------------------------------------------

SfxTabPage* SvxLineDefTabPage::Create( Window* pWindow,
				const SfxItemSet& rOutAttrs )
{
	return( new SvxLineDefTabPage( pWindow, rOutAttrs ) );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, SelectLinestyleHdl_Impl, void *, p )
{
	if( pDashList->Count() > 0 )
	{
		int nTmp = aLbLineStyles.GetSelectEntryPos();
		if( nTmp == LISTBOX_ENTRY_NOTFOUND )
		{
		}
		else
            aDash = pDashList->GetDash( nTmp )->GetDash();

		FillDialog_Impl();

		rXLSet.Put( XLineDashItem( String(), aDash ) );

		// #i34740#
		aCtlPreview.SetLineAttributes(aXLineAttr.GetItemSet());

		aCtlPreview.Invalidate();

		// Wird erst hier gesetzt, um den Style nur dann zu uebernehmen,
		// wenn in der ListBox ein Eintrag ausgewaehlt wurde
		// Wenn ueber Reset() gerufen wurde ist p == NULL
		if( p )
			*pPageType = 2;
	}
	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvxLineDefTabPage, ChangePreviewHdl_Impl, void *, EMPTYARG )
{
	FillDash_Impl();
	aCtlPreview.Invalidate();

	return( 0L );
}
IMPL_LINK_INLINE_END( SvxLineDefTabPage, ChangePreviewHdl_Impl, void *, EMPTYARG )

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ChangeNumber1Hdl_Impl, void *, EMPTYARG )
{
	if( aNumFldNumber1.GetValue() == 0L )
	{
        aNumFldNumber2.SetMin( 1L );
        aNumFldNumber2.SetFirst( 1L );
	}
	else
	{
        aNumFldNumber2.SetMin( 0L );
        aNumFldNumber2.SetFirst( 0L );
	}

	ChangePreviewHdl_Impl( this );

	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ChangeNumber2Hdl_Impl, void *, EMPTYARG )
{
	if( aNumFldNumber2.GetValue() == 0L )
	{
        aNumFldNumber1.SetMin( 1L );
        aNumFldNumber1.SetFirst( 1L );
	}
	else
	{
        aNumFldNumber1.SetMin( 0L );
        aNumFldNumber1.SetFirst( 0L );
	}

	ChangePreviewHdl_Impl( this );

	return( 0L );
}


//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ChangeMetricHdl_Impl, void *, p )
{
	if( !aCbxSynchronize.IsChecked() && aMtrLength1.GetUnit() != eFUnit )
	{
		long nTmp1, nTmp2, nTmp3;

		// Wurde ueber Control geaendert
		if( p )
		{
			nTmp1 = GetCoreValue( aMtrLength1, ePoolUnit ) * XOUT_WIDTH / 100;
			nTmp2 = GetCoreValue( aMtrLength2, ePoolUnit ) * XOUT_WIDTH / 100;
			nTmp3 = GetCoreValue( aMtrDistance, ePoolUnit ) * XOUT_WIDTH / 100;
		}
		else
		{
			nTmp1 = GetCoreValue( aMtrLength1, ePoolUnit );
			nTmp2 = GetCoreValue( aMtrLength2, ePoolUnit );
			nTmp3 = GetCoreValue( aMtrDistance, ePoolUnit );
		}
		aMtrLength1.SetDecimalDigits( 2 );
		aMtrLength2.SetDecimalDigits( 2 );
		aMtrDistance.SetDecimalDigits( 2 );

		// Metrik einstellen
		aMtrLength1.SetUnit( eFUnit );
		aMtrLength2.SetUnit( eFUnit );
		aMtrDistance.SetUnit( eFUnit );

		SetMetricValue( aMtrLength1, nTmp1, ePoolUnit );
		SetMetricValue( aMtrLength2, nTmp2, ePoolUnit );
		SetMetricValue( aMtrDistance, nTmp3, ePoolUnit );
	}
	else if( aCbxSynchronize.IsChecked() && aMtrLength1.GetUnit() != FUNIT_CUSTOM )
	{
		long nTmp1, nTmp2, nTmp3;

		// Wurde ueber Control geaendert
		if( p )
		{
			nTmp1 = GetCoreValue( aMtrLength1, ePoolUnit ) * 100 / XOUT_WIDTH;
			nTmp2 = GetCoreValue( aMtrLength2, ePoolUnit ) * 100 / XOUT_WIDTH;
			nTmp3 = GetCoreValue( aMtrDistance, ePoolUnit ) * 100 / XOUT_WIDTH;
		}
		else
		{
			nTmp1 = GetCoreValue( aMtrLength1, ePoolUnit );
			nTmp2 = GetCoreValue( aMtrLength2, ePoolUnit );
			nTmp3 = GetCoreValue( aMtrDistance, ePoolUnit );
		}

		aMtrLength1.SetDecimalDigits( 0 );
		aMtrLength2.SetDecimalDigits( 0 );
		aMtrDistance.SetDecimalDigits( 0 );

		aMtrLength1.SetUnit( FUNIT_CUSTOM );
		aMtrLength2.SetUnit( FUNIT_CUSTOM );
		aMtrDistance.SetUnit( FUNIT_CUSTOM );


		SetMetricValue( aMtrLength1, nTmp1, ePoolUnit );
		SetMetricValue( aMtrLength2, nTmp2, ePoolUnit );
		SetMetricValue( aMtrDistance, nTmp3, ePoolUnit );
	}
	SelectTypeHdl_Impl( NULL );

	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, SelectTypeHdl_Impl, void *, p )
{
	if ( p == &aLbType1 || !p )
	{
		if ( aLbType1.GetSelectEntryPos() == 0 )
		{
			aMtrLength1.Disable();
			aMtrLength1.SetText( String() );
		}
		else if ( !aMtrLength1.IsEnabled() )
		{
			aMtrLength1.Enable();
			aMtrLength1.Reformat();
		}
	}

	if ( p == &aLbType2 || !p )
	{
		if ( aLbType2.GetSelectEntryPos() == 0 )
		{
			aMtrLength2.Disable();
			aMtrLength2.SetText( String() );
		}
		else if ( !aMtrLength2.IsEnabled() )
		{
			aMtrLength2.Enable();
			aMtrLength2.Reformat();
		}
	}
	ChangePreviewHdl_Impl( p );
	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ClickAddHdl_Impl, void *, EMPTYARG )
{
	ResMgr& rMgr = DIALOG_MGR();
	String aNewName( ResId( RID_SVXSTR_LINESTYLE, rMgr ) );
	String aDesc( ResId( RID_SVXSTR_DESC_LINESTYLE, rMgr ) );
	String aName;
	XDashEntry* pEntry;

	long nCount = pDashList->Count();
	long j = 1;
	BOOL bDifferent = FALSE;

	while ( !bDifferent )
	{
		aName = aNewName;
		aName += sal_Unicode(' ');
		aName += UniString::CreateFromInt32( j++ );
		bDifferent = TRUE;

		for ( long i = 0; i < nCount && bDifferent; i++ )
            if ( aName == pDashList->GetDash( i )->GetName() )
				bDifferent = FALSE;
	}

	//CHINA001 SvxNameDialog* pDlg = new SvxNameDialog( DLGWIN, aName, aDesc );
	SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
	DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
	AbstractSvxNameDialog* pDlg = pFact->CreateSvxNameDialog( DLGWIN, aName, aDesc, RID_SVXDLG_NAME );
	DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001
	BOOL bLoop = TRUE;

	while ( bLoop && pDlg->Execute() == RET_OK )
	{
		pDlg->GetName( aName );
		bDifferent = TRUE;

		for( long i = 0; i < nCount && bDifferent; i++ )
		{
            if( aName == pDashList->GetDash( i )->GetName() )
				bDifferent = FALSE;
		}

		if( bDifferent )
		{
			bLoop = FALSE;
			FillDash_Impl();

			pEntry = new XDashEntry( aDash, aName );

            long nDashCount = pDashList->Count();
            pDashList->Insert( pEntry, nDashCount );
            Bitmap* pBitmap = pDashList->GetBitmap( nDashCount );
			aLbLineStyles.Append( pEntry, pBitmap );

			aLbLineStyles.SelectEntryPos( aLbLineStyles.GetEntryCount() - 1 );

			// Flag fuer modifiziert setzen
			*pnDashListState |= CT_MODIFIED;

			*pPageType = 2;

			// Werte sichern fuer Changes-Erkennung ( -> Methode )
			aNumFldNumber1.SaveValue();
			aMtrLength1.SaveValue();
			aLbType1.SaveValue();
			aNumFldNumber2.SaveValue();
			aMtrLength2.SaveValue();
			aLbType2.SaveValue();
			aMtrDistance.SaveValue();
		}
		else
		{
			WarningBox aBox( DLGWIN, WinBits( WB_OK ),String( ResId( RID_SVXSTR_WARN_NAME_DUPLICATE, rMgr ) ) );
			aBox.SetHelpId( HID_WARN_NAME_DUPLICATE );
			aBox.Execute();
		}
	}
	delete( pDlg );

	// Status der Buttons ermitteln
	if ( pDashList->Count() )
	{
		aBtnModify.Enable();
		aBtnDelete.Enable();
		aBtnSave.Enable();
	}
	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ClickModifyHdl_Impl, void *, EMPTYARG )
{
	USHORT nPos = aLbLineStyles.GetSelectEntryPos();

	if( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		ResMgr& rMgr = DIALOG_MGR();
		String aNewName( ResId( RID_SVXSTR_LINESTYLE, rMgr ) );
		String aDesc( ResId( RID_SVXSTR_DESC_LINESTYLE, rMgr ) );
        String aName( pDashList->GetDash( nPos )->GetName() );
		String aOldName = aName;

		//CHINA001 SvxNameDialog* pDlg = new SvxNameDialog( DLGWIN, aName, aDesc );
		SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
		DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
		AbstractSvxNameDialog* pDlg = pFact->CreateSvxNameDialog( DLGWIN, aName, aDesc, RID_SVXDLG_NAME );
		DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001

		long nCount = pDashList->Count();
		BOOL bDifferent = FALSE;
		BOOL bLoop = TRUE;

		while ( bLoop && pDlg->Execute() == RET_OK )
		{
			pDlg->GetName( aName );
			bDifferent = TRUE;

			for( long i = 0; i < nCount && bDifferent; i++ )
			{
                if( aName == pDashList->GetDash( i )->GetName() &&
					aName != aOldName )
					bDifferent = FALSE;
			}

			if ( bDifferent )
			{
				bLoop = FALSE;
				FillDash_Impl();

				XDashEntry* pEntry = new XDashEntry( aDash, aName );

				delete pDashList->Replace( pEntry, nPos );
				Bitmap* pBitmap = pDashList->GetBitmap( nPos );
				aLbLineStyles.Modify( pEntry, nPos, pBitmap );

				aLbLineStyles.SelectEntryPos( nPos );

				// Flag fuer modifiziert setzen
				*pnDashListState |= CT_MODIFIED;

				*pPageType = 2;

				// Werte sichern fuer Changes-Erkennung ( -> Methode )
				aNumFldNumber1.SaveValue();
				aMtrLength1.SaveValue();
				aLbType1.SaveValue();
				aNumFldNumber2.SaveValue();
				aMtrLength2.SaveValue();
				aLbType2.SaveValue();
				aMtrDistance.SaveValue();
			}
			else
			{
				WarningBox aBox( DLGWIN, WinBits( WB_OK ), String( ResId( RID_SVXSTR_WARN_NAME_DUPLICATE, rMgr ) ) );
				aBox.SetHelpId( HID_WARN_NAME_DUPLICATE );
				aBox.Execute();
			}
		}
		delete( pDlg );
	}
	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ClickDeleteHdl_Impl, void *, EMPTYARG )
{
	USHORT nPos = aLbLineStyles.GetSelectEntryPos();

	if ( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		QueryBox aQueryBox( DLGWIN, WinBits( WB_YES_NO | WB_DEF_NO ),
//!			SVX_RES( RID_SVXSTR_DEL_LINESTYLE ),
			String( SVX_RES( RID_SVXSTR_ASK_DEL_LINESTYLE ) ) );

		if ( aQueryBox.Execute() == RET_YES )
		{
			delete pDashList->Remove( nPos );
			aLbLineStyles.RemoveEntry( nPos );
			aLbLineStyles.SelectEntryPos( 0 );

			SelectLinestyleHdl_Impl( this );
			*pPageType = 0; // Style soll nicht uebernommen werden

			// Flag fuer modifiziert setzen
			*pnDashListState |= CT_MODIFIED;

			ChangePreviewHdl_Impl( this );
		}
	}

	// Status der Buttons ermitteln
	if ( !pDashList->Count() )
	{
		aBtnModify.Disable();
		aBtnDelete.Disable();
		aBtnSave.Disable();
	}
	return( 0L );
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ClickLoadHdl_Impl, void *, EMPTYARG )
{
	ResMgr& rMgr = DIALOG_MGR();
	USHORT nReturn = RET_YES;

	if ( *pnDashListState & CT_MODIFIED )
	{
		nReturn = WarningBox( DLGWIN, WinBits( WB_YES_NO_CANCEL ),
			String( ResId( RID_SVXSTR_WARN_TABLE_OVERWRITE, rMgr ) ) ).Execute();

		if ( nReturn == RET_YES )
			pDashList->Save();
	}

	if ( nReturn != RET_CANCEL )
	{
    	::sfx2::FileDialogHelper aDlg(
            com::sun::star::ui::dialogs::TemplateDescription::FILEOPEN_SIMPLE,
            0 );
		String aStrFilterType( RTL_CONSTASCII_USTRINGPARAM( "*.sod" ) );
		aDlg.AddFilter( aStrFilterType, aStrFilterType );
		INetURLObject aFile( SvtPathOptions().GetPalettePath() );
		aDlg.SetDisplayDirectory( aFile.GetMainURL( INetURLObject::NO_DECODE ) );

		if( aDlg.Execute() == ERRCODE_NONE )
		{
			INetURLObject aURL( aDlg.GetPath() );
			INetURLObject aPathURL( aURL );

			aPathURL.removeSegment();
			aPathURL.removeFinalSlash();

			// Liste speichern
			XDashList* pDshLst = new XDashList( aPathURL.GetMainURL( INetURLObject::NO_DECODE ), pXPool );
			pDshLst->SetName( aURL.getName() );

			if( pDshLst->Load() )
			{
				if( pDshLst )
				{
					// Pruefen, ob Tabelle geloescht werden darf:
					if( pDashList != ( (SvxLineTabDialog*) DLGWIN )->GetDashList() )
						delete pDashList;

					pDashList = pDshLst;
					( (SvxLineTabDialog*) DLGWIN )->SetNewDashList( pDashList );

					aLbLineStyles.Clear();
					aLbLineStyles.Fill( pDashList );
					Reset( rOutAttrs );

					pDashList->SetName( aURL.getName() );

/*					// Ermitteln (evtl. abschneiden) des Namens und in
					// der GroupBox darstellen
					String aString( ResId( RID_SVXSTR_TABLE, rMgr ) );
					aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );

					if ( aURL.getBase().Len() > 18 )
					{
						aString += aURL.getBase().Copy( 0, 15 );
						aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
					}
					else
						aString += aURL.getBase();

					aGrpLinestyles.SetText( aString );
*/
					// Flag fuer gewechselt setzen
					*pnDashListState |= CT_CHANGED;
					// Flag fuer modifiziert entfernen
					*pnDashListState &= ~CT_MODIFIED;
				}
			}
			else
				//aIStream.Close();
				ErrorBox( DLGWIN, WinBits( WB_OK ),
					String( ResId( RID_SVXSTR_READ_DATA_ERROR, rMgr ) ) ).Execute();
		}
	}

	// Status der Buttons ermitteln
	if ( pDashList->Count() )
	{
		aBtnModify.Enable();
		aBtnDelete.Enable();
		aBtnSave.Enable();
	}
	else
	{
		aBtnModify.Disable();
		aBtnDelete.Disable();
		aBtnSave.Disable();
	}
	return( 0L );
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxLineDefTabPage, ClickSaveHdl_Impl, void *, EMPTYARG )
{
   	::sfx2::FileDialogHelper aDlg(
        com::sun::star::ui::dialogs::TemplateDescription::FILESAVE_SIMPLE, 0 );
	String aStrFilterType( RTL_CONSTASCII_USTRINGPARAM( "*.sod" ) );
	aDlg.AddFilter( aStrFilterType, aStrFilterType );

	INetURLObject aFile( SvtPathOptions().GetPalettePath() );
	DBG_ASSERT( aFile.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

	if( pDashList->GetName().Len() )
	{
		aFile.Append( pDashList->GetName() );

		if( !aFile.getExtension().getLength() )
			aFile.SetExtension( UniString::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "sod" ) ) );
	}

	aDlg.SetDisplayDirectory( aFile.GetMainURL( INetURLObject::NO_DECODE ) );
	if ( aDlg.Execute() == ERRCODE_NONE )
	{
		INetURLObject aURL( aDlg.GetPath() );
		INetURLObject aPathURL( aURL );

		aPathURL.removeSegment();
		aPathURL.removeFinalSlash();

		pDashList->SetName( aURL.getName() );
		pDashList->SetPath( aPathURL.GetMainURL( INetURLObject::NO_DECODE ) );

		if( pDashList->Save() )
		{
/*			// Ermitteln (evtl. abschneiden) des Namens und in
			// der GroupBox darstellen
			String aString( SVX_RES( RID_SVXSTR_TABLE ) );
			aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );

			if ( aURL.getBase().Len() > 18 )
			{
				aString += aURL.getBase().Copy( 0, 15 );
				aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
			}
			else
				aString += aURL.getBase();

			aGrpLinestyles.SetText( aString );
*/
			// Flag fuer gespeichert setzen
			*pnDashListState |= CT_SAVED;
			// Flag fuer modifiziert entfernen
			*pnDashListState &= ~CT_MODIFIED;
		}
		else
		{
			ErrorBox( DLGWIN, WinBits( WB_OK ),
				String( SVX_RES( RID_SVXSTR_WRITE_DATA_ERROR ) ) ).Execute();
		}
	}

	return( 0L );
}

//------------------------------------------------------------------------

void SvxLineDefTabPage::FillDash_Impl()
{
	XDashStyle eXDS;

/*	Alle Stile werden z.Z. nicht benutzt
	if( aRbtEnds1.IsChecked() )
		eXDS = XDASH_ROUND;
	else if( aRbtEnds2.IsChecked() )
		eXDS = XDASH_RECT;
	else
		eXDS = XDASH_RECT;
*/
	if( aCbxSynchronize.IsChecked() )
		eXDS = XDASH_RECTRELATIVE;
	else
		eXDS = XDASH_RECT;

	aDash.SetDashStyle( eXDS );
	aDash.SetDots( (BYTE) aNumFldNumber1.GetValue() );
	aDash.SetDotLen( aLbType1.GetSelectEntryPos() == 0 ? 0 :
								GetCoreValue( aMtrLength1, ePoolUnit ) );
	aDash.SetDashes( (BYTE) aNumFldNumber2.GetValue() );
	aDash.SetDashLen( aLbType2.GetSelectEntryPos() == 0 ? 0 :
								GetCoreValue( aMtrLength2, ePoolUnit ) );
	aDash.SetDistance( GetCoreValue( aMtrDistance, ePoolUnit ) );

	rXLSet.Put( XLineDashItem( String(), aDash ) );

	// #i34740#
	aCtlPreview.SetLineAttributes(aXLineAttr.GetItemSet());
}

//------------------------------------------------------------------------

void SvxLineDefTabPage::FillDialog_Impl()
{
	XDashStyle eXDS = aDash.GetDashStyle(); // XDASH_RECT, XDASH_ROUND
	if( eXDS == XDASH_RECTRELATIVE )
		aCbxSynchronize.Check();
	else
		aCbxSynchronize.Check( FALSE );

	aNumFldNumber1.SetValue( aDash.GetDots() );
	//aMtrLength1.SetValue( aDash.GetDotLen() );
	SetMetricValue( aMtrLength1, aDash.GetDotLen(), ePoolUnit );
	aLbType1.SelectEntryPos( aDash.GetDotLen() == 0 ? 0 : 1 );
	aNumFldNumber2.SetValue( aDash.GetDashes() );
	//aMtrLength2.SetValue( aDash.GetDashLen() );
	SetMetricValue( aMtrLength2, aDash.GetDashLen(), ePoolUnit );
	aLbType2.SelectEntryPos( aDash.GetDashLen() == 0 ? 0 : 1 );
	//aMtrDistance.SetValue( aDash.GetDistance() );
	SetMetricValue( aMtrDistance, aDash.GetDistance(), ePoolUnit );

	ChangeMetricHdl_Impl( NULL );

	// Werte sichern fuer Changes-Erkennung ( -> Methode )
	aNumFldNumber1.SaveValue();
	aMtrLength1.SaveValue();
	aLbType1.SaveValue();
	aNumFldNumber2.SaveValue();
	aMtrLength2.SaveValue();
	aLbType2.SaveValue();
	aMtrDistance.SaveValue();
}


void SvxLineDefTabPage::DataChanged( const DataChangedEvent& rDCEvt )
{
	SfxTabPage::DataChanged( rDCEvt );

	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_STYLE) )
	{
		USHORT nOldSelect = aLbLineStyles.GetSelectEntryPos();
		aLbLineStyles.Clear();
		aLbLineStyles.Fill( pDashList );
		aLbLineStyles.SelectEntryPos( nOldSelect );
	}
}

