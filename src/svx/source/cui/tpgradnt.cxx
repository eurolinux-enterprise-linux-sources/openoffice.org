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
#include <vcl/wrkwin.hxx>
#include <tools/shl.hxx>
#include <tools/urlobj.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/pathoptions.hxx>
#include <sfx2/app.hxx>
#include <sfx2/filedlghelper.hxx>
#include "com/sun/star/ui/dialogs/TemplateDescription.hpp"

#define _SVX_TPGRADNT_CXX

#include <svx/dialogs.hrc>
#include "helpid.hrc"

#include "xattr.hxx"
#include <svx/xtable.hxx>
#include <svx/xpool.hxx>
#include "drawitem.hxx"
#include "cuitabarea.hxx"
#include "tabarea.hrc"
#include "defdlgname.hxx" //CHINA001 #include "dlgname.hxx"
#include "dlgname.hrc"
#include <svx/svxdlg.hxx> //CHINA001
#include <svx/dialmgr.hxx>

#define DLGWIN this->GetParent()->GetParent()

#define BITMAP_WIDTH  32
#define BITMAP_HEIGHT 12

/*************************************************************************
|*
|*	Dialog zum Aendern und Definieren der Farbverlaeufe
|*
\************************************************************************/

SvxGradientTabPage::SvxGradientTabPage
(
	Window* pParent,
	const SfxItemSet& rInAttrs
) :
	SfxTabPage			( pParent, SVX_RES( RID_SVXPAGE_GRADIENT ), rInAttrs ),

    aFlProp             ( this, SVX_RES( FL_PROP ) ),
    aFtType             ( this, SVX_RES( FT_TYPE ) ),
	aLbGradientType		( this, SVX_RES( LB_GRADIENT_TYPES ) ),
	aFtCenterX			( this, SVX_RES( FT_CENTER_X ) ),
	aMtrCenterX			( this, SVX_RES( MTR_CENTER_X ) ),
	aFtCenterY			( this, SVX_RES( FT_CENTER_Y ) ),
	aMtrCenterY			( this, SVX_RES( MTR_CENTER_Y ) ),
	aFtAngle			( this, SVX_RES( FT_ANGLE ) ),
	aMtrAngle			( this, SVX_RES( MTR_ANGLE ) ),
	aFtBorder			( this, SVX_RES( FT_BORDER ) ),
	aMtrBorder			( this, SVX_RES( MTR_BORDER ) ),
	aFtColorFrom		( this, SVX_RES( FT_COLOR_FROM ) ),
    aLbColorFrom        ( this, SVX_RES( LB_COLOR_FROM ) ),
    aMtrColorFrom       ( this, SVX_RES( MTR_COLOR_FROM ) ),
    aFtColorTo          ( this, SVX_RES( FT_COLOR_TO ) ),
    aLbColorTo          ( this, SVX_RES( LB_COLOR_TO ) ),
    aMtrColorTo         ( this, SVX_RES( MTR_COLOR_TO ) ),
    aLbGradients        ( this, SVX_RES( LB_GRADIENTS ) ),
    aCtlPreview         ( this, SVX_RES( CTL_PREVIEW ) ),
	aBtnAdd				( this, SVX_RES( BTN_ADD ) ),
	aBtnModify			( this, SVX_RES( BTN_MODIFY ) ),
	aBtnDelete			( this, SVX_RES( BTN_DELETE ) ),
	aBtnLoad			( this, SVX_RES( BTN_LOAD ) ),
	aBtnSave			( this, SVX_RES( BTN_SAVE ) ),
    
    rOutAttrs           ( rInAttrs ),

    pColorTab( NULL ),
    pGradientList( NULL ),

    pXPool              ( (XOutdevItemPool*) rInAttrs.GetPool() ),
    aXFStyleItem        ( XFILL_GRADIENT ),
    aXGradientItem      ( String(), XGradient( COL_BLACK, COL_WHITE ) ),
    aXFillAttr          ( pXPool ),
    rXFSet              ( aXFillAttr.GetItemSet() )
{
	aBtnLoad.SetModeImage( Image( SVX_RES( RID_SVXIMG_LOAD_H ) ), BMP_COLOR_HIGHCONTRAST );
	aBtnSave.SetModeImage( Image( SVX_RES( RID_SVXIMG_SAVE_H ) ), BMP_COLOR_HIGHCONTRAST );

	FreeResource();

	// diese Page braucht ExchangeSupport
	SetExchangeSupport();

	// Solange NICHT vom Item unterstuetzt

	//aMtrColorTo.Disable();
	aMtrColorTo.SetValue( 100 );
	//aMtrColorFrom.Disable();
	aMtrColorFrom.SetValue( 100 );

	// Setzen des Output-Devices
	rXFSet.Put( aXFStyleItem );
	rXFSet.Put( aXGradientItem );
	aCtlPreview.SetAttributes( aXFillAttr.GetItemSet() );

    // Handler ueberladen
	aLbGradients.SetSelectHdl(
		LINK( this, SvxGradientTabPage, ChangeGradientHdl_Impl ) );
	aBtnAdd.SetClickHdl( LINK( this, SvxGradientTabPage, ClickAddHdl_Impl ) );
	aBtnModify.SetClickHdl(
		LINK( this, SvxGradientTabPage, ClickModifyHdl_Impl ) );
	aBtnDelete.SetClickHdl(
		LINK( this, SvxGradientTabPage, ClickDeleteHdl_Impl ) );

	Link aLink = LINK( this, SvxGradientTabPage, ModifiedHdl_Impl );
	aLbGradientType.SetSelectHdl( aLink );
	aMtrCenterX.SetModifyHdl( aLink );
	aMtrCenterY.SetModifyHdl( aLink );
	aMtrAngle.SetModifyHdl( aLink );
	aMtrBorder.SetModifyHdl( aLink );
	aMtrColorFrom.SetModifyHdl( aLink );
	aLbColorFrom.SetSelectHdl( aLink );
	aMtrColorTo.SetModifyHdl( aLink );
	aLbColorTo.SetSelectHdl( aLink );

	aBtnLoad.SetClickHdl(
		LINK( this, SvxGradientTabPage, ClickLoadHdl_Impl ) );
	aBtnSave.SetClickHdl(
		LINK( this, SvxGradientTabPage, ClickSaveHdl_Impl ) );
    
    // #i76307# always paint the preview in LTR, because this is what the document does
    aCtlPreview.EnableRTL( FALSE );
}

// -----------------------------------------------------------------------

void SvxGradientTabPage::Construct()
{
	// Farbtabelle
	aLbColorFrom.Fill( pColorTab );
	aLbColorTo.CopyEntries( aLbColorFrom );

	// Farbverlauftabelle
	aLbGradients.Fill( pGradientList );
}

// -----------------------------------------------------------------------

void SvxGradientTabPage::ActivatePage( const SfxItemSet&  )
{
	USHORT nPos;
	USHORT nCount;

	if( *pDlgType == 0 ) // Flaechen-Dialog
	{
		*pbAreaTP = FALSE;

		if( pColorTab )
		{
			// ColorTable
			if( *pnColorTableState & CT_CHANGED ||
				*pnColorTableState & CT_MODIFIED )
			{
				if( *pnColorTableState & CT_CHANGED )
					pColorTab = ( (SvxAreaTabDialog*) DLGWIN )->GetNewColorTable();

				// LbColorFrom
				nPos = aLbColorFrom.GetSelectEntryPos();
				aLbColorFrom.Clear();
				aLbColorFrom.Fill( pColorTab );
				nCount = aLbColorFrom.GetEntryCount();
				if( nCount == 0 )
					; // Dieser Fall sollte nicht auftreten
				else if( nCount <= nPos )
					aLbColorFrom.SelectEntryPos( 0 );
				else
					aLbColorFrom.SelectEntryPos( nPos );

				// LbColorTo
				nPos = aLbColorTo.GetSelectEntryPos();
				aLbColorTo.Clear();
				aLbColorTo.CopyEntries( aLbColorFrom );
				nCount = aLbColorTo.GetEntryCount();
				if( nCount == 0 )
					; // Dieser Fall sollte nicht auftreten
				else if( nCount <= nPos )
					aLbColorTo.SelectEntryPos( 0 );
				else
					aLbColorTo.SelectEntryPos( nPos );

				ModifiedHdl_Impl( this );
			}

			// Ermitteln (evtl. abschneiden) des Namens und in
			// der GroupBox darstellen
			String			aString( SVX_RES( RID_SVXSTR_TABLE ) ); aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );
			INetURLObject	aURL( pGradientList->GetPath() );

			aURL.Append( pGradientList->GetName() );
			DBG_ASSERT( aURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

			if ( aURL.getBase().getLength() > 18 )
			{
				aString += String(aURL.getBase()).Copy( 0, 15 );
				aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
			}
			else
				aString += String(aURL.getBase());

			if ( *pPageType == PT_GRADIENT && *pPos != LISTBOX_ENTRY_NOTFOUND )
			{
				aLbGradients.SelectEntryPos( *pPos );
			}
			// Farben koennten geloescht worden sein
			ChangeGradientHdl_Impl( this );

			*pPageType = PT_GRADIENT;
			*pPos = LISTBOX_ENTRY_NOTFOUND;
		}
	}
}

// -----------------------------------------------------------------------

int SvxGradientTabPage::DeactivatePage( SfxItemSet* _pSet )
{
	if( CheckChanges_Impl() == -1L )
		return KEEP_PAGE;

    if( _pSet )
        FillItemSet( *_pSet );

	return LEAVE_PAGE;
}

// -----------------------------------------------------------------------

long SvxGradientTabPage::CheckChanges_Impl()
{
	// wird hier benutzt, um Aenderungen NICHT zu verlieren
	XGradient aTmpGradient( aLbColorFrom.GetSelectEntryColor(),
						  aLbColorTo.GetSelectEntryColor(),
						  (XGradientStyle) aLbGradientType.GetSelectEntryPos(),
						  static_cast<long>(aMtrAngle.GetValue() * 10), // sollte in Resource geaendert werden
						  (USHORT) aMtrCenterX.GetValue(),
						  (USHORT) aMtrCenterY.GetValue(),
						  (USHORT) aMtrBorder.GetValue(),
						  (USHORT) aMtrColorFrom.GetValue(),
						  (USHORT) aMtrColorTo.GetValue() );

	USHORT nPos = aLbGradients.GetSelectEntryPos();
	if( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
        XGradient aGradient = pGradientList->GetGradient( nPos )->GetGradient();
		String aString = aLbGradients.GetSelectEntry();

		if( !( aTmpGradient == aGradient ) )
		{
			ResMgr& rMgr = DIALOG_MGR();
            Image aWarningBoxImage = WarningBox::GetStandardImage();
			//CHINA001 SvxMessDialog aMessDlg( DLGWIN,
			//CHINA001 	String( ResId( RID_SVXSTR_GRADIENT, rMgr ) ),
			//CHINA001 	String( ResId( RID_SVXSTR_ASK_CHANGE_GRADIENT, rMgr ) ),
			//CHINA001 	&aWarningBoxImage );
			SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
			DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
			AbstractSvxMessDialog* aMessDlg = pFact->CreateSvxMessDialog( DLGWIN, RID_SVXDLG_MESSBOX,
														String( ResId( RID_SVXSTR_GRADIENT, rMgr ) ),
														String( ResId( RID_SVXSTR_ASK_CHANGE_GRADIENT, rMgr ) ),
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
                    aGradient = pGradientList->GetGradient( nPos )->GetGradient();
				}
				break;

				case RET_BTN_2: // Hinzufuegen
				{
					ClickAddHdl_Impl( this );
					nPos = aLbGradients.GetSelectEntryPos();
                    aGradient = pGradientList->GetGradient( nPos )->GetGradient();
				}
				break;

				case RET_CANCEL:
					// return( -1L ); <-- wuerde die Seite nicht verlassen
				break;
				// return( TRUE ); // Abbruch
			}
			delete aMessDlg; //add by CHINA001
		}
	}
	nPos = aLbGradients.GetSelectEntryPos();
	if( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		*pPos = nPos;
	}
	return 0L;
}

// -----------------------------------------------------------------------

BOOL SvxGradientTabPage::FillItemSet( SfxItemSet& rSet )
{
	if( *pDlgType == 0 && *pPageType == PT_GRADIENT && *pbAreaTP == FALSE )
	{
		// CheckChanges(); <-- doppelte Abfrage ?

		XGradient*	pXGradient = NULL;
		String	  	aString;
		USHORT	  	nPos = aLbGradients.GetSelectEntryPos();
		if( nPos != LISTBOX_ENTRY_NOTFOUND )
		{
            pXGradient = new XGradient( pGradientList->GetGradient( nPos )->GetGradient() );
			aString = aLbGradients.GetSelectEntry();

		}
		else
		// Farbverlauf wurde (unbekannt) uebergeben
		{
			pXGradient = new XGradient( aLbColorFrom.GetSelectEntryColor(),
						aLbColorTo.GetSelectEntryColor(),
						(XGradientStyle) aLbGradientType.GetSelectEntryPos(),
						static_cast<long>(aMtrAngle.GetValue() * 10), // sollte in Resource geaendert werden
						(USHORT) aMtrCenterX.GetValue(),
						(USHORT) aMtrCenterY.GetValue(),
						(USHORT) aMtrBorder.GetValue(),
						(USHORT) aMtrColorFrom.GetValue(),
						(USHORT) aMtrColorTo.GetValue() );
		}
		DBG_ASSERT( pXGradient, "XGradient konnte nicht erzeugt werden" );
        rSet.Put( XFillStyleItem( XFILL_GRADIENT ) );
        rSet.Put( XFillGradientItem( aString, *pXGradient ) );

		delete pXGradient;
	}
	return TRUE;
}

// -----------------------------------------------------------------------

void SvxGradientTabPage::Reset( const SfxItemSet& )
{
	// aLbGradients.SelectEntryPos( 0 );
	ChangeGradientHdl_Impl( this );

	// Status der Buttons ermitteln
	if( pGradientList->Count() )
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

SfxTabPage* SvxGradientTabPage::Create( Window* pWindow,
				const SfxItemSet& rOutAttrs )
{
	return new SvxGradientTabPage( pWindow, rOutAttrs );
}

//------------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ModifiedHdl_Impl, void *, pControl )
{
	XGradientStyle eXGS = (XGradientStyle) aLbGradientType.GetSelectEntryPos();

	XGradient aXGradient( aLbColorFrom.GetSelectEntryColor(),
						  aLbColorTo.GetSelectEntryColor(),
						  eXGS,
						  static_cast<long>(aMtrAngle.GetValue() * 10), // sollte in Resource geaendert werden
						  (USHORT) aMtrCenterX.GetValue(),
						  (USHORT) aMtrCenterY.GetValue(),
						  (USHORT) aMtrBorder.GetValue(),
						  (USHORT) aMtrColorFrom.GetValue(),
						  (USHORT) aMtrColorTo.GetValue() );

	// Enablen/Disablen von Controls
	if( pControl == &aLbGradientType || pControl == this )
		SetControlState_Impl( eXGS );

	// Anzeigen im XOutDev
	rXFSet.Put( XFillGradientItem( String(), aXGradient ) );
	aCtlPreview.SetAttributes( aXFillAttr.GetItemSet() );

	aCtlPreview.Invalidate();

	return 0L;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ClickAddHdl_Impl, void *, EMPTYARG )
{
	ResMgr& rMgr = DIALOG_MGR();
	String aNewName( ResId( RID_SVXSTR_GRADIENT, rMgr ) );
	String aDesc( ResId( RID_SVXSTR_DESC_GRADIENT, rMgr ) );
	String aName;

	long nCount = pGradientList->Count();
	long j = 1;
	BOOL bDifferent = FALSE;

	while( !bDifferent )
	{
		aName  = aNewName;
		aName += sal_Unicode(' ');
		aName += UniString::CreateFromInt32( j++ );
		bDifferent = TRUE;

		for( long i = 0; i < nCount && bDifferent; i++ )
            if( aName == pGradientList->GetGradient( i )->GetName() )
				bDifferent = FALSE;
	}

	//CHINA001 SvxNameDialog* pDlg     = new SvxNameDialog( DLGWIN, aName, aDesc );
	SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
	DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
	AbstractSvxNameDialog* pDlg = pFact->CreateSvxNameDialog( DLGWIN, aName, aDesc, RID_SVXDLG_NAME );
	DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001
	WarningBox*    pWarnBox = NULL;
	USHORT         nError   = RID_SVXSTR_WARN_NAME_DUPLICATE;

	while( pDlg->Execute() == RET_OK )
	{
		pDlg->GetName( aName );

		bDifferent = TRUE;

		for( long i = 0; i < nCount && bDifferent; i++ )
            if( aName == pGradientList->GetGradient( i )->GetName() )
				bDifferent = FALSE;

		if( bDifferent )
		{
			nError = 0;
			break;
		}

		if( !pWarnBox )
		{
			pWarnBox = new WarningBox( DLGWIN,
									   WinBits( WB_OK_CANCEL ),
									   String( ResId( nError, rMgr ) ) );
			pWarnBox->SetHelpId( HID_WARN_NAME_DUPLICATE );
		}

		if( pWarnBox->Execute() != RET_OK )
			break;
	}
	//Rectangle aDlgRect( pDlg->GetPosPixel(), pDlg->GetSizePixel() );
	delete pDlg;
	delete pWarnBox;

	if( !nError )
	{
		XGradient aXGradient( aLbColorFrom.GetSelectEntryColor(),
							  aLbColorTo.GetSelectEntryColor(),
							  (XGradientStyle) aLbGradientType.GetSelectEntryPos(),
							  static_cast<long>(aMtrAngle.GetValue() * 10), // sollte in Resource geaendert werden
							  (USHORT) aMtrCenterX.GetValue(),
							  (USHORT) aMtrCenterY.GetValue(),
							  (USHORT) aMtrBorder.GetValue(),
							  (USHORT) aMtrColorFrom.GetValue(),
							  (USHORT) aMtrColorTo.GetValue() );
		XGradientEntry* pEntry = new XGradientEntry( aXGradient, aName );

		pGradientList->Insert( pEntry, nCount );

		aLbGradients.Append( pEntry );

		aLbGradients.SelectEntryPos( aLbGradients.GetEntryCount() - 1 );

#ifdef WNT
		// hack: #31355# W.P.
		Rectangle aRect( aLbGradients.GetPosPixel(), aLbGradients.GetSizePixel() );
		if( TRUE ) {				// ??? overlapped with pDlg
									// and srolling
			Invalidate( aRect );
			//aLbGradients.Invalidate();
		}
#endif

		// Flag fuer modifiziert setzen
		*pnGradientListState |= CT_MODIFIED;

		ChangeGradientHdl_Impl( this );
	}

	// Status der Buttons ermitteln
	if( pGradientList->Count() )
	{
		aBtnModify.Enable();
		aBtnDelete.Enable();
		aBtnSave.Enable();
	}
	return 0L;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ClickModifyHdl_Impl, void *, EMPTYARG )
{
	USHORT nPos = aLbGradients.GetSelectEntryPos();

	if ( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		ResMgr& rMgr = DIALOG_MGR();
		String aNewName( ResId( RID_SVXSTR_GRADIENT, rMgr ) );
		String aDesc( ResId( RID_SVXSTR_DESC_GRADIENT, rMgr ) );
        String aName( pGradientList->GetGradient( nPos )->GetName() );
		String aOldName = aName;

		//CHINA001 SvxNameDialog* pDlg = new SvxNameDialog( DLGWIN, aName, aDesc );
		SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
		DBG_ASSERT(pFact, "Dialogdiet fail!");//CHINA001
		AbstractSvxNameDialog* pDlg = pFact->CreateSvxNameDialog( DLGWIN, aName, aDesc, RID_SVXDLG_NAME );
		DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001

		long nCount = pGradientList->Count();
		BOOL bDifferent = FALSE;
		BOOL bLoop = TRUE;

		while( bLoop && pDlg->Execute() == RET_OK )
		{
			pDlg->GetName( aName );
			bDifferent = TRUE;

			for( long i = 0; i < nCount && bDifferent; i++ )
			{
                if( aName == pGradientList->GetGradient( i )->GetName() &&
					aName != aOldName )
					bDifferent = FALSE;
			}

			if( bDifferent )
			{
				bLoop = FALSE;
				XGradient aXGradient( aLbColorFrom.GetSelectEntryColor(),
									  aLbColorTo.GetSelectEntryColor(),
									  (XGradientStyle) aLbGradientType.GetSelectEntryPos(),
									  static_cast<long>(aMtrAngle.GetValue() * 10), // sollte in Resource geaendert werden
									  (USHORT) aMtrCenterX.GetValue(),
									  (USHORT) aMtrCenterY.GetValue(),
									  (USHORT) aMtrBorder.GetValue(),
									  (USHORT) aMtrColorFrom.GetValue(),
									  (USHORT) aMtrColorTo.GetValue() );

				XGradientEntry* pEntry = new XGradientEntry( aXGradient, aName );

				delete pGradientList->Replace( pEntry, nPos );

				aLbGradients.Modify( pEntry, nPos );

				aLbGradients.SelectEntryPos( nPos );

				// Flag fuer modifiziert setzen
				*pnGradientListState |= CT_MODIFIED;
			}
			else
			{
				WarningBox aBox( DLGWIN, WinBits( WB_OK ),String( ResId( RID_SVXSTR_WARN_NAME_DUPLICATE, rMgr ) ) );
				aBox.SetHelpId( HID_WARN_NAME_DUPLICATE );
				aBox.Execute();
			}

		}
		delete pDlg;
	}
	return 0L;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ClickDeleteHdl_Impl, void *, EMPTYARG )
{
	USHORT nPos = aLbGradients.GetSelectEntryPos();

	if( nPos != LISTBOX_ENTRY_NOTFOUND )
	{
		QueryBox aQueryBox( DLGWIN, WinBits( WB_YES_NO | WB_DEF_NO ),
//!			SVX_RES( RID_SVXSTR_DEL_GRADIENT ),
			String( SVX_RES( RID_SVXSTR_ASK_DEL_GRADIENT ) ) );

		if ( aQueryBox.Execute() == RET_YES )
		{
			delete pGradientList->Remove( nPos );
			aLbGradients.RemoveEntry( nPos );
			aLbGradients.SelectEntryPos( 0 );

			aCtlPreview.Invalidate();

			ChangeGradientHdl_Impl( this );

			// Flag fuer modifiziert setzen
			*pnGradientListState |= CT_MODIFIED;
		}
	}
	// Status der Buttons ermitteln
	if( !pGradientList->Count() )
	{
		aBtnModify.Disable();
		aBtnDelete.Disable();
		aBtnSave.Disable();
	}
	return 0L;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ClickLoadHdl_Impl, void *, EMPTYARG )
{
	ResMgr& rMgr = DIALOG_MGR();
	USHORT nReturn = RET_YES;

	if ( *pnGradientListState & CT_MODIFIED )
	{
		nReturn = WarningBox( DLGWIN, WinBits( WB_YES_NO_CANCEL ),
			String( ResId( RID_SVXSTR_WARN_TABLE_OVERWRITE, rMgr ) ) ).Execute();

		if ( nReturn == RET_YES )
			pGradientList->Save();
	}

	if ( nReturn != RET_CANCEL )
	{
    	::sfx2::FileDialogHelper aDlg(
            com::sun::star::ui::dialogs::TemplateDescription::FILEOPEN_SIMPLE,
            0 );
		String aStrFilterType( RTL_CONSTASCII_USTRINGPARAM( "*.sog" ) );
		aDlg.AddFilter( aStrFilterType, aStrFilterType );
		INetURLObject aFile( SvtPathOptions().GetPalettePath() );
		aDlg.SetDisplayDirectory( aFile.GetMainURL( INetURLObject::NO_DECODE ) );

		if( aDlg.Execute() == ERRCODE_NONE )
		{
			EnterWait();

			INetURLObject aURL( aDlg.GetPath() );
			INetURLObject aPathURL( aURL );

			aPathURL.removeSegment();
			aPathURL.removeFinalSlash();

			// Liste speichern
			XGradientList* pGrdList = new XGradientList( aPathURL.GetMainURL( INetURLObject::NO_DECODE ), pXPool );
			pGrdList->SetName( aURL.getName() );

			if ( pGrdList->Load() )
			{
				if ( pGrdList )
				{
					// Pruefen, ob Tabelle geloescht werden darf:
					if ( pGradientList !=
						 ( (SvxAreaTabDialog*) DLGWIN )->GetGradientList() )
						delete pGradientList;

					pGradientList = pGrdList;
					( (SvxAreaTabDialog*) DLGWIN )->
						SetNewGradientList( pGradientList );

					aLbGradients.Clear();
					aLbGradients.Fill( pGradientList );
					Reset( rOutAttrs );

					pGradientList->SetName( aURL.getName() );

					// Ermitteln (evtl. abschneiden) des Namens und in
					// der GroupBox darstellen
					String aString( ResId( RID_SVXSTR_TABLE, rMgr ) );
					aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );

					if ( aURL.getBase().getLength() > 18 )
					{
						aString += String(aURL.getBase()).Copy( 0, 15 );
						aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
					}
					else
						aString += String(aURL.getBase());

					// Flag fuer gewechselt setzen
					*pnGradientListState |= CT_CHANGED;
					// Flag fuer modifiziert entfernen
					*pnGradientListState &= ~CT_MODIFIED;
				}
				LeaveWait();
			}
			else
			{
				LeaveWait();
				//aIStream.Close();
				ErrorBox( DLGWIN, WinBits( WB_OK ),
					String( ResId( RID_SVXSTR_READ_DATA_ERROR, rMgr ) ) ).Execute();
			}
		}
	}

	// Status der Buttons ermitteln
	if( pGradientList->Count() )
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
	return 0L;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ClickSaveHdl_Impl, void *, EMPTYARG )
{
   	::sfx2::FileDialogHelper aDlg(
        com::sun::star::ui::dialogs::TemplateDescription::FILESAVE_SIMPLE, 0 );
	String aStrFilterType( RTL_CONSTASCII_USTRINGPARAM( "*.sog" ) );
	aDlg.AddFilter( aStrFilterType, aStrFilterType );

	INetURLObject aFile( SvtPathOptions().GetPalettePath() );
	DBG_ASSERT( aFile.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

	if( pGradientList->GetName().Len() )
	{
		aFile.Append( pGradientList->GetName() );

		if( !aFile.getExtension().getLength() )
			aFile.SetExtension( UniString::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "sog" ) ) );
	}

	aDlg.SetDisplayDirectory( aFile.GetMainURL( INetURLObject::NO_DECODE ) );
	if ( aDlg.Execute() == ERRCODE_NONE )
	{
		INetURLObject	aURL( aDlg.GetPath() );
		INetURLObject	aPathURL( aURL );

		aPathURL.removeSegment();
		aPathURL.removeFinalSlash();

		pGradientList->SetName( aURL.getName() );
		pGradientList->SetPath( aPathURL.GetMainURL( INetURLObject::NO_DECODE ) );

		if( pGradientList->Save() )
		{
			// Ermitteln (evtl. abschneiden) des Namens und in
			// der GroupBox darstellen
			String aString( SVX_RES( RID_SVXSTR_TABLE ) );
			aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ": " ) );

			if ( aURL.getBase().getLength() > 18 )
			{
				aString += String(aURL.getBase()).Copy( 0, 15 );
				aString.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "..." ) );
			}
			else
				aString += String(aURL.getBase());

			// Flag fuer gespeichert setzen
			*pnGradientListState |= CT_SAVED;
			// Flag fuer modifiziert entfernen
			*pnGradientListState &= ~CT_MODIFIED;
		}
		else
		{
			ErrorBox( DLGWIN, WinBits( WB_OK ),
				String( SVX_RES( RID_SVXSTR_WRITE_DATA_ERROR ) ) ).Execute();
		}
	}

	return 0L;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxGradientTabPage, ChangeGradientHdl_Impl, void *, EMPTYARG )
{
	XGradient* pGradient = NULL;
	int nPos = aLbGradients.GetSelectEntryPos();

	if( nPos != LISTBOX_ENTRY_NOTFOUND )
        pGradient = new XGradient( ( (XGradientEntry*) pGradientList->GetGradient( nPos ) )->GetGradient() );
	else
	{
		const SfxPoolItem* pPoolItem = NULL;
		if( SFX_ITEM_SET == rOutAttrs.GetItemState( GetWhich( XATTR_FILLSTYLE ), TRUE, &pPoolItem ) )
		{
			if( ( XFILL_GRADIENT == (XFillStyle) ( ( const XFillStyleItem* ) pPoolItem )->GetValue() ) &&
				( SFX_ITEM_SET == rOutAttrs.GetItemState( GetWhich( XATTR_FILLGRADIENT ), TRUE, &pPoolItem ) ) )
			{
                pGradient = new XGradient( ( ( const XFillGradientItem* ) pPoolItem )->GetGradientValue() );
			}
		}
		if( !pGradient )
		{
			aLbGradients.SelectEntryPos( 0 );
			nPos = aLbGradients.GetSelectEntryPos();
			if( nPos != LISTBOX_ENTRY_NOTFOUND )
                pGradient = new XGradient( ( (XGradientEntry*) pGradientList->GetGradient( nPos ) )->GetGradient() );
		}
	}

	if( pGradient )
	{
		XGradientStyle eXGS = pGradient->GetGradientStyle();

		aLbGradientType.SelectEntryPos(
            sal::static_int_cast< USHORT >( eXGS ) );
		// Wenn der EIntrag nicht in der Listbox ist, werden die Farben
		// temporaer hinzugenommen
		aLbColorFrom.SetNoSelection();
		aLbColorFrom.SelectEntry( pGradient->GetStartColor() );

		if ( aLbColorFrom.GetSelectEntryCount() == 0 )
		{
			aLbColorFrom.InsertEntry( pGradient->GetStartColor(),
									  String() );
			aLbColorFrom.SelectEntry( pGradient->GetStartColor() );
		}
		aLbColorTo.SetNoSelection();
		aLbColorTo.SelectEntry( pGradient->GetEndColor() );

		if ( aLbColorTo.GetSelectEntryCount() == 0 )
		{
			aLbColorTo.InsertEntry( pGradient->GetEndColor(), String() );
			aLbColorTo.SelectEntry( pGradient->GetEndColor() );
		}

		aMtrAngle.SetValue( pGradient->GetAngle() / 10 ); // sollte in Resource geaendert werden
		aMtrBorder.SetValue( pGradient->GetBorder() );
		aMtrCenterX.SetValue( pGradient->GetXOffset() );
		aMtrCenterY.SetValue( pGradient->GetYOffset() );
		aMtrColorFrom.SetValue( pGradient->GetStartIntens() );
		aMtrColorTo.SetValue( pGradient->GetEndIntens() );

		// Controls Disablen/Enablen
		SetControlState_Impl( eXGS );

		// ItemSet fuellen und an aCtlPreview weiterleiten
		rXFSet.Put( XFillGradientItem( String(), *pGradient ) );
		aCtlPreview.SetAttributes( aXFillAttr.GetItemSet() );

		aCtlPreview.Invalidate();
		delete pGradient;
	}
	return 0L;
}

//------------------------------------------------------------------------

void SvxGradientTabPage::SetControlState_Impl( XGradientStyle eXGS )
{
	switch( eXGS )
	{
		case XGRAD_LINEAR:
		case XGRAD_AXIAL:
			aFtCenterX.Disable();
			aMtrCenterX.Disable();
			aFtCenterY.Disable();
			aMtrCenterY.Disable();
			aFtAngle.Enable();
			aMtrAngle.Enable();
			break;

		case XGRAD_RADIAL:
			aFtCenterX.Enable();
			aMtrCenterX.Enable();
			aFtCenterY.Enable();
			aMtrCenterY.Enable();
			aFtAngle.Disable();
			aMtrAngle.Disable();
			break;

		case XGRAD_ELLIPTICAL:
			aFtCenterX.Enable();
			aMtrCenterX.Enable();
			aFtCenterY.Enable();
			aMtrCenterY.Enable();
			aFtAngle.Enable();
			aMtrAngle.Enable();
			break;

		case XGRAD_SQUARE:
		case XGRAD_RECT:
			aFtCenterX.Enable();
			aMtrCenterX.Enable();
			aFtCenterY.Enable();
			aMtrCenterY.Enable();
			aFtAngle.Enable();
			aMtrAngle.Enable();
			break;
	}
}


