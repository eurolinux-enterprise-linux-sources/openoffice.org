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


#include <svx/dialogs.hrc>
#include <svx/flstitem.hxx>
#include <svx/flagsdef.hxx>
#include <sfx2/objsh.hxx>

#include "sdresid.hxx"
#include "dlg_char.hxx"
#include <svx/svxids.hrc>
#include <svtools/intitem.hxx>

/*************************************************************************
|*
|* Konstruktor des Tab-Dialogs: Fuegt die Seiten zum Dialog hinzu
|*
\************************************************************************/

SdCharDlg::SdCharDlg( Window* pParent, const SfxItemSet* pAttr,
					const SfxObjectShell* pDocShell ) :
		SfxTabDialog        ( pParent, SdResId( TAB_CHAR ), pAttr ),
		rOutAttrs			( *pAttr ),
		rDocShell			( *pDocShell )
{
	FreeResource();

	AddTabPage( RID_SVXPAGE_CHAR_NAME );
	AddTabPage( RID_SVXPAGE_CHAR_EFFECTS );
	AddTabPage( RID_SVXPAGE_CHAR_POSITION );
}

// -----------------------------------------------------------------------

void SdCharDlg::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
	switch( nId )
	{
		case RID_SVXPAGE_CHAR_NAME:
		{
			SvxFontListItem aItem(*( (const SvxFontListItem*)
				( rDocShell.GetItem( SID_ATTR_CHAR_FONTLIST) ) ) );

			aSet.Put (SvxFontListItem( aItem.GetFontList(), SID_ATTR_CHAR_FONTLIST));
			rPage.PageCreated(aSet);
		}
		break;

		case RID_SVXPAGE_CHAR_EFFECTS:
			aSet.Put (SfxUInt16Item(SID_DISABLE_CTL,DISABLE_CASEMAP));
			rPage.PageCreated(aSet);
			break;

		default:
		break;
	}
}



