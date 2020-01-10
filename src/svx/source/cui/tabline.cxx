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
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <svtools/pathoptions.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>

#define _SVX_TABLINE_CXX
#include <svx/dialogs.hrc>
#include "tabline.hrc"
#include "dlgname.hrc"

#include "cuitabarea.hxx"
#include "cuitabline.hxx"
#include "dlgname.hxx"
#include <svx/dialmgr.hxx>
#include <svx/svdmodel.hxx>
#include <svx/xtable.hxx>
#include "drawitem.hxx"

#define DLGWIN this->GetParent()->GetParent()

#define BITMAP_WIDTH   32
#define BITMAP_HEIGHT  12
#define XOUT_WIDTH    150

/*************************************************************************
|*
|* Konstruktor des Tab-Dialogs: Fuegt die Seiten zum Dialog hinzu
|*
\************************************************************************/

SvxLineTabDialog::SvxLineTabDialog
(
	Window* pParent,
	const SfxItemSet* pAttr,
	SdrModel* pModel,
	const SdrObject* pSdrObj,
	BOOL bHasObj
) :

	SfxTabDialog	( pParent, SVX_RES( RID_SVXDLG_LINE ), pAttr ),
	pDrawModel		( pModel ),
	pObj			( pSdrObj ),
    rOutAttrs       ( *pAttr ),
    pColorTab       ( pModel->GetColorTable() ),
    mpNewColorTab   ( pModel->GetColorTable() ),
	pDashList       ( pModel->GetDashList() ),
    pNewDashList    ( pModel->GetDashList() ),
    pLineEndList    ( pModel->GetLineEndList() ),
    pNewLineEndList ( pModel->GetLineEndList() ),
    bObjSelected    ( bHasObj ),
    nLineEndListState( CT_NONE ),
    nDashListState( CT_NONE ),
	mnColorTableState( CT_NONE ),
    nPageType( 0 ), // wird hier in erster Linie benutzt, um mit FillItemSet
                   // die richtigen Attribute zu erhalten ( noch Fragen? )
    nDlgType( 0 ),
    nPosDashLb( 0 ),
    nPosLineEndLb( 0 ),
    mnPos( 0 ),
    mbAreaTP( sal_False ),
    mbDeleteColorTable( TRUE )
{
	FreeResource();

	bool bLineOnly = false;
	if( pObj && pObj->GetObjInventor() == SdrInventor )
	{
		switch( pObj->GetObjIdentifier() )
		{
		case OBJ_LINE:
		case OBJ_PLIN:
		case OBJ_PATHLINE:
		case OBJ_FREELINE:
		case OBJ_MEASURE:
		case OBJ_EDGE:
			bLineOnly = true;

		default:
			break;
		}

	}

	AddTabPage( RID_SVXPAGE_LINE, SvxLineTabPage::Create, 0);
	if( bLineOnly )
		AddTabPage( RID_SVXPAGE_SHADOW, SvxShadowTabPage::Create, 0 );
	else
		RemoveTabPage( RID_SVXPAGE_SHADOW );

	AddTabPage( RID_SVXPAGE_LINE_DEF, SvxLineDefTabPage::Create, 0);
	AddTabPage( RID_SVXPAGE_LINEEND_DEF, SvxLineEndDefTabPage::Create, 0);
//	AddTabPage( RID_SVXPAGE_COLOR, SvxColorTabPage::Create, 0 );

	SetCurPageId( RID_SVXPAGE_LINE );

	CancelButton& rBtnCancel = GetCancelButton();
	rBtnCancel.SetClickHdl( LINK( this, SvxLineTabDialog, CancelHdlImpl ) );
//! rBtnCancel.SetText( SVX_RESSTR( RID_SVXSTR_CLOSE ) );
}

// -----------------------------------------------------------------------

SvxLineTabDialog::~SvxLineTabDialog()
{
}

// -----------------------------------------------------------------------

void SvxLineTabDialog::SavePalettes()
{
    SfxObjectShell* pShell = SfxObjectShell::Current();
	if( mpNewColorTab != pDrawModel->GetColorTable() )
	{
		if(mbDeleteColorTable)
			delete pDrawModel->GetColorTable();
		pDrawModel->SetColorTable( mpNewColorTab );
        if ( pShell )
            pShell->PutItem( SvxColorTableItem( mpNewColorTab, SID_COLOR_TABLE ) );
		pColorTab = pDrawModel->GetColorTable();
	}
	if( pNewDashList != pDrawModel->GetDashList() )
	{
		delete pDrawModel->GetDashList();
		pDrawModel->SetDashList( pNewDashList );
        if ( pShell )
            pShell->PutItem( SvxDashListItem( pNewDashList, SID_DASH_LIST ) );
		pDashList = pDrawModel->GetDashList();
	}
	if( pNewLineEndList != pDrawModel->GetLineEndList() )
	{
		delete pDrawModel->GetLineEndList();
		pDrawModel->SetLineEndList( pNewLineEndList );
        if ( pShell )
            pShell->PutItem( SvxLineEndListItem( pNewLineEndList, SID_LINEEND_LIST ) );
		pLineEndList = pDrawModel->GetLineEndList();
	}

	// Speichern der Tabellen, wenn sie geaendert wurden.

	const String aPath( SvtPathOptions().GetPalettePath() );

	if( nDashListState & CT_MODIFIED )
	{
		pDashList->SetPath( aPath );
		pDashList->Save();

		// ToolBoxControls werden benachrichtigt:
        if ( pShell )
            pShell->PutItem( SvxDashListItem( pDashList, SID_DASH_LIST ) );
	}

	if( nLineEndListState & CT_MODIFIED )
	{
		pLineEndList->SetPath( aPath );
		pLineEndList->Save();

		// ToolBoxControls werden benachrichtigt:
        if ( pShell )
            pShell->PutItem( SvxLineEndListItem( pLineEndList, SID_LINEEND_LIST ) );
	}

	if( mnColorTableState & CT_MODIFIED )
	{
		pColorTab->SetPath( aPath );
		pColorTab->Save();

		// ToolBoxControls werden benachrichtigt:
        if ( pShell )
            pShell->PutItem( SvxColorTableItem( pColorTab, SID_COLOR_TABLE ) );
	}	
}

// -----------------------------------------------------------------------

short SvxLineTabDialog::Ok()
{
	SavePalettes();

	// Es wird RET_OK zurueckgeliefert, wenn wenigstens eine
	// TabPage in FillItemSet() TRUE zurueckliefert. Dieses
	// geschieht z.Z. standardmaessig.
	return( SfxTabDialog::Ok() );
}

// -----------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvxLineTabDialog, CancelHdlImpl, void *, EMPTYARG )
{
	SavePalettes();

	EndDialog( RET_CANCEL );
	return 0;
}
IMPL_LINK_INLINE_END( SvxLineTabDialog, CancelHdlImpl, void *, EMPTYARG )

// -----------------------------------------------------------------------

void SvxLineTabDialog::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	switch( nId )
	{
		case RID_SVXPAGE_LINE:
			( (SvxLineTabPage&) rPage ).SetColorTable( pColorTab );
			( (SvxLineTabPage&) rPage ).SetDashList( pDashList );
			( (SvxLineTabPage&) rPage ).SetLineEndList( pLineEndList );
			( (SvxLineTabPage&) rPage ).SetDlgType( nDlgType );//CHINA001 ( (SvxLineTabPage&) rPage ).SetDlgType( &nDlgType );
			( (SvxLineTabPage&) rPage ).SetPageType( nPageType );//CHINA001 ( (SvxLineTabPage&) rPage ).SetPageType( &nPageType );
			( (SvxLineTabPage&) rPage ).SetPosDashLb( &nPosDashLb );
			( (SvxLineTabPage&) rPage ).SetPosLineEndLb( &nPosLineEndLb );
			( (SvxLineTabPage&) rPage ).SetDashChgd( &nDashListState );
			( (SvxLineTabPage&) rPage ).SetLineEndChgd( &nLineEndListState );
			( (SvxLineTabPage&) rPage ).SetObjSelected( bObjSelected );
			( (SvxLineTabPage&) rPage ).Construct();
			( (SvxLineTabPage&) rPage ).SetColorChgd( &mnColorTableState );
			// ActivatePage() wird das erste mal nicht gerufen
			( (SvxLineTabPage&) rPage ).ActivatePage( rOutAttrs );
		break;

		case RID_SVXPAGE_LINE_DEF:
			( (SvxLineDefTabPage&) rPage ).SetDashList( pDashList );
			( (SvxLineDefTabPage&) rPage ).SetDlgType( &nDlgType );
			( (SvxLineDefTabPage&) rPage ).SetPageType( &nPageType );
			( (SvxLineDefTabPage&) rPage ).SetPosDashLb( &nPosDashLb );
			( (SvxLineDefTabPage&) rPage ).SetDashChgd( &nDashListState );
			( (SvxLineDefTabPage&) rPage ).SetObjSelected( bObjSelected );
			( (SvxLineDefTabPage&) rPage ).Construct();
		break;

		case RID_SVXPAGE_LINEEND_DEF:
			( (SvxLineEndDefTabPage&) rPage ).SetLineEndList( pLineEndList );
			( (SvxLineEndDefTabPage&) rPage ).SetPolyObj( pObj );
			( (SvxLineEndDefTabPage&) rPage ).SetDlgType( &nDlgType );
			( (SvxLineEndDefTabPage&) rPage ).SetPageType( &nPageType );
			( (SvxLineEndDefTabPage&) rPage ).SetPosLineEndLb( &nPosLineEndLb );
			( (SvxLineEndDefTabPage&) rPage ).SetLineEndChgd( &nLineEndListState );
			( (SvxLineEndDefTabPage&) rPage ).SetObjSelected( bObjSelected );
			( (SvxLineEndDefTabPage&) rPage ).Construct();
		break;

		case RID_SVXPAGE_SHADOW:
		{
			( (SvxShadowTabPage&) rPage ).SetColorTable( pColorTab );
			( (SvxShadowTabPage&) rPage ).SetPageType( nPageType );
			( (SvxShadowTabPage&) rPage ).SetDlgType( nDlgType );
			( (SvxShadowTabPage&) rPage ).SetAreaTP( &mbAreaTP );
			( (SvxShadowTabPage&) rPage ).SetColorChgd( &mnColorTableState );
			( (SvxShadowTabPage&) rPage ).Construct();
		}
		break;
/*
		case RID_SVXPAGE_COLOR:
			( (SvxColorTabPage&) rPage ).SetColorTable( pColorTab );
			( (SvxColorTabPage&) rPage ).SetPageType( &nPageType );
			( (SvxColorTabPage&) rPage ).SetDlgType( &nDlgType );
			( (SvxColorTabPage&) rPage ).SetPos( &mnPos );
			( (SvxColorTabPage&) rPage ).SetAreaTP( &mbAreaTP );
			( (SvxColorTabPage&) rPage ).SetColorChgd( &mnColorTableState );
			( (SvxColorTabPage&) rPage ).SetDeleteColorTable( mbDeleteColorTable );
			( (SvxColorTabPage&) rPage ).Construct();
		break;
*/
	}
}


