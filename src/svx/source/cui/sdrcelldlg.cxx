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
#include <svtools/cjkoptions.hxx>
#include "flagsdef.hxx"
#include "svx/dialogs.hrc"
#include "sdrcelldlg.hxx"
#include "svx/dialmgr.hxx"
#include "cuitabarea.hxx"
#include "svx/svdmodel.hxx"
#include "border.hxx"

SvxFormatCellsDialog::SvxFormatCellsDialog( Window* pParent, const SfxItemSet* pAttr, SdrModel* pModel )
: SfxTabDialog        ( pParent, SVX_RES( RID_SVX_FORMAT_CELLS_DLG ), pAttr )
, mrOutAttrs			( *pAttr )
, mpColorTab           ( pModel->GetColorTable() )
, mpGradientList       ( pModel->GetGradientList() )
, mpHatchingList       ( pModel->GetHatchList() )
, mpBitmapList         ( pModel->GetBitmapList() )

{
	FreeResource();

	AddTabPage( RID_SVXPAGE_CHAR_NAME );
	AddTabPage( RID_SVXPAGE_CHAR_EFFECTS );
	AddTabPage( RID_SVXPAGE_BORDER );
	AddTabPage( RID_SVXPAGE_AREA );
	
/*
    SvtCJKOptions aCJKOptions;
    if( aCJKOptions.IsAsianTypographyEnabled() )
		AddTabPage( RID_SVXPAGE_PARA_ASIAN);
	else
		RemoveTabPage( RID_SVXPAGE_PARA_ASIAN );
*/
}

SvxFormatCellsDialog::~SvxFormatCellsDialog()
{
}

void SvxFormatCellsDialog::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	switch( nId )
	{
		case RID_SVXPAGE_AREA:
			( (SvxAreaTabPage&) rPage ).SetColorTable( mpColorTab );
			( (SvxAreaTabPage&) rPage ).SetGradientList( mpGradientList );
			( (SvxAreaTabPage&) rPage ).SetHatchingList( mpHatchingList );
			( (SvxAreaTabPage&) rPage ).SetBitmapList( mpBitmapList );
			( (SvxAreaTabPage&) rPage ).SetPageType( PT_AREA );
			( (SvxAreaTabPage&) rPage ).SetDlgType( 1 );
			( (SvxAreaTabPage&) rPage ).SetPos( 0 );
//			( (SvxAreaTabPage&) rPage ).SetAreaTP( &mbAreaTP );
//			( (SvxAreaTabPage&) rPage ).SetGrdChgd( &mnGradientListState );
//			( (SvxAreaTabPage&) rPage ).SetHtchChgd( &mnHatchingListState );
//			( (SvxAreaTabPage&) rPage ).SetBmpChgd( &mnBitmapListState );
//			( (SvxAreaTabPage&) rPage ).SetColorChgd( &mnColorTableState );
			( (SvxAreaTabPage&) rPage ).Construct();
			// ActivatePage() wird das erste mal nicht gerufen
			( (SvxAreaTabPage&) rPage ).ActivatePage( mrOutAttrs );

		break;

		default:
			SfxTabDialog::PageCreated( nId, rPage );
			break;
	}
}

void SvxFormatCellsDialog::Apply()
{
}
