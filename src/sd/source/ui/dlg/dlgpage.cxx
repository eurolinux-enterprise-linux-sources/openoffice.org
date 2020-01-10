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
#include "precompiled_sd.hxx"

#ifdef SD_DLLIMPLEMENTATION
#undef SD_DLLIMPLEMENTATION
#endif

#include <svtools/intitem.hxx>
#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#include <svx/tabarea.hxx>
#include <svx/drawitem.hxx>

#ifndef _SD_SDRESID_HXX
#include "sdresid.hxx"
#endif
#include "dlgpage.hxx"

#include "DrawDocShell.hxx"
#include <svtools/aeitem.hxx>
#include <svx/flagsdef.hxx>
#include <svx/svxenum.hxx>

/*************************************************************************
|*
|* Konstruktor des Tab-Dialogs: Fuegt die Seiten zum Dialog hinzu
|*
\************************************************************************/

SdPageDlg::SdPageDlg( SfxObjectShell* pDocSh, Window* pParent, const SfxItemSet* pAttr, BOOL bAreaPage ) :
		SfxTabDialog ( pParent, SdResId( TAB_PAGE ), pAttr ),
		mrOutAttrs			( *pAttr ),
		mpDocShell			( pDocSh )
{
	SvxColorTableItem aColorTableItem(*( (const SvxColorTableItem*)
		( mpDocShell->GetItem( SID_COLOR_TABLE ) ) ) );
	SvxGradientListItem aGradientListItem(*( (const SvxGradientListItem*)
		( mpDocShell->GetItem( SID_GRADIENT_LIST ) ) ) );
	SvxBitmapListItem aBitmapListItem(*( (const SvxBitmapListItem*)
		( mpDocShell->GetItem( SID_BITMAP_LIST ) ) ) );
	SvxHatchListItem aHatchListItem(*( (const SvxHatchListItem*)
		( mpDocShell->GetItem( SID_HATCH_LIST ) ) ) );

	mpColorTab = aColorTableItem.GetColorTable();
	mpGradientList = aGradientListItem.GetGradientList();
	mpHatchingList = aHatchListItem.GetHatchList();
	mpBitmapList = aBitmapListItem.GetBitmapList();

	FreeResource();

	AddTabPage( RID_SVXPAGE_PAGE);
	AddTabPage( RID_SVXPAGE_AREA);

	if(!bAreaPage)  // I have to add the page before I remove it !
		RemoveTabPage( RID_SVXPAGE_AREA );
}


/*************************************************************************
|*
|* Seite wird erzeugt
|*
\************************************************************************/

void SdPageDlg::PageCreated(USHORT nId, SfxTabPage& rPage)
{
	SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
	switch(nId)
	{
	case RID_SVXPAGE_PAGE:
		aSet.Put (SfxAllEnumItem((const USHORT)SID_ENUM_PAGE_MODE, SVX_PAGE_MODE_PRESENTATION));
		aSet.Put (SfxAllEnumItem((const USHORT)SID_PAPER_START, PAPER_A0));
		aSet.Put (SfxAllEnumItem((const USHORT)SID_PAPER_END, PAPER_E));
		rPage.PageCreated(aSet);
		break;
	case RID_SVXPAGE_AREA:
			aSet.Put (SvxColorTableItem(mpColorTab,SID_COLOR_TABLE));
			aSet.Put (SvxGradientListItem(mpGradientList,SID_GRADIENT_LIST));
			aSet.Put (SvxHatchListItem(mpHatchingList,SID_HATCH_LIST));
			aSet.Put (SvxBitmapListItem(mpBitmapList,SID_BITMAP_LIST));
			aSet.Put (SfxUInt16Item(SID_PAGE_TYPE,0));
			aSet.Put (SfxUInt16Item(SID_DLG_TYPE,1));
			aSet.Put (SfxUInt16Item(SID_TABPAGE_POS,0));
			rPage.PageCreated(aSet);
		break;
	}
}



