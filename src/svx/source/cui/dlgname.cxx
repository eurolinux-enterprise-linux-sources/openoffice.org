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

#include <tools/ref.hxx>
#include <tools/list.hxx>
#include <tools/shl.hxx>
#include <tools/debug.hxx>
#include <vcl/msgbox.hxx>

#include <svx/dialogs.hrc>

#include "dlgname.hxx"
#include "defdlgname.hxx" //CHINA001
#include "dlgname.hrc"
#include <svx/dialmgr.hxx>

#define MAX_DESCRIPTION_LINES   ((long)5)

/*************************************************************************
|*
|* Dialog zum Editieren eines Namens
|*
\************************************************************************/

SvxNameDialog::SvxNameDialog( Window* pWindow, const String& rName, const String& rDesc ) :
	ModalDialog		( pWindow, SVX_RES( RID_SVXDLG_NAME ) ),
	aFtDescription		( this, SVX_RES( FT_DESCRIPTION ) ),
	aEdtName			( this, SVX_RES( EDT_STRING ) ),
	aBtnOK				( this, SVX_RES( BTN_OK ) ),
	aBtnCancel			( this, SVX_RES( BTN_CANCEL ) ),
	aBtnHelp			( this, SVX_RES( BTN_HELP ) )
{
	FreeResource();

	aFtDescription.SetText( rDesc );
	aEdtName.SetText( rName );
    aEdtName.SetSelection(Selection(SELECTION_MIN, SELECTION_MAX));
    ModifyHdl(&aEdtName);
    aEdtName.SetModifyHdl(LINK(this, SvxNameDialog, ModifyHdl));

    // dynamic height of the description field
    Size aSize = aFtDescription.GetSizePixel();
    long nTxtWidth = aFtDescription.GetCtrlTextWidth( rDesc );
    if ( nTxtWidth > aSize.Width() )
    {
        long nLines = Min( ( nTxtWidth / aSize.Width() + 1 ), MAX_DESCRIPTION_LINES );
        long nHeight = aSize.Height();
        aSize.Height() = nHeight * nLines;
        aFtDescription.SetSizePixel( aSize );
        Point aPnt = aEdtName.GetPosPixel();
        aPnt.Y() += ( aSize.Height() - nHeight );
        aEdtName.SetPosPixel( aPnt );
    }
}

/* -----------------------------27.02.2002 15:22------------------------------

 ---------------------------------------------------------------------------*/
IMPL_LINK(SvxNameDialog, ModifyHdl, Edit*, EMPTYARG)
{
    if(aCheckNameHdl.IsSet())
        aBtnOK.Enable(aCheckNameHdl.Call(this) > 0);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// #i68101#
// Dialog for editing Object Name
// plus uniqueness-callback-linkHandler

SvxObjectNameDialog::SvxObjectNameDialog( 
	Window* pWindow, 
	const String& rName)
:	ModalDialog(pWindow, SVX_RES(RID_SVXDLG_OBJECT_NAME)),
	aFtName(this, SVX_RES(NTD_FT_NAME)),
	aEdtName(this, SVX_RES(NTD_EDT_NAME)),
	aFlSeparator(this, SVX_RES(FL_SEPARATOR_A)),
	aBtnHelp(this, SVX_RES(BTN_HELP)),
	aBtnOK(this, SVX_RES(BTN_OK)),
	aBtnCancel(this, SVX_RES(BTN_CANCEL))
{
	FreeResource();

	// set name
	aEdtName.SetText(rName);

	// activate name
    aEdtName.SetSelection(Selection(SELECTION_MIN, SELECTION_MAX));
    ModifyHdl(&aEdtName);
    aEdtName.SetModifyHdl(LINK(this, SvxObjectNameDialog, ModifyHdl));
}

IMPL_LINK(SvxObjectNameDialog, ModifyHdl, Edit*, EMPTYARG)
{
    if(aCheckNameHdl.IsSet())
	{
        aBtnOK.Enable(aCheckNameHdl.Call(this) > 0);
	}

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// #i68101#
// Dialog for editing Object Title and Description

SvxObjectTitleDescDialog::SvxObjectTitleDescDialog( 
	Window* pWindow, 
	const String& rTitle, 
	const String& rDescription)
:	ModalDialog(pWindow, SVX_RES(RID_SVXDLG_OBJECT_TITLE_DESC)),
	aFtTitle(this, SVX_RES(NTD_FT_TITLE)),
	aEdtTitle(this, SVX_RES(NTD_EDT_TITLE)),
	aFtDescription(this, SVX_RES(NTD_FT_DESC)),
	aEdtDescription(this, SVX_RES(NTD_EDT_DESC)),
	aFlSeparator(this, SVX_RES(FL_SEPARATOR_B)),
	aBtnHelp(this, SVX_RES(BTN_HELP)),
	aBtnOK(this, SVX_RES(BTN_OK)),
	aBtnCancel(this, SVX_RES(BTN_CANCEL))
{
	FreeResource();

	// set title & desc
	aEdtTitle.SetText(rTitle);
	aEdtDescription.SetText(rDescription);

	// activate title
    aEdtTitle.SetSelection(Selection(SELECTION_MIN, SELECTION_MAX));
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*************************************************************************
|*
|* Dialog zum Abbrechen, Speichern oder Hinzufuegen
|*
\************************************************************************/

SvxMessDialog::SvxMessDialog( Window* pWindow, const String& rText, const String& rDesc, Image* pImg ) :
	ModalDialog		( pWindow, SVX_RES( RID_SVXDLG_MESSBOX ) ),
	aFtDescription		( this, SVX_RES( FT_DESCRIPTION ) ),
	aBtn1				( this, SVX_RES( BTN_1 ) ),
	aBtn2				( this, SVX_RES( BTN_2 ) ),
	aBtnCancel			( this, SVX_RES( BTN_CANCEL ) ),
	aFtImage			( this )
{
	FreeResource();

	if( pImg )
	{
		pImage = new Image( *pImg );
		aFtImage.SetImage( *pImage );
		aFtImage.SetStyle( ( aFtImage.GetStyle()/* | WB_NOTABSTOP */) & ~WB_3DLOOK );
		aFtImage.SetPosSizePixel( LogicToPixel( Point( 3, 6 ), MAP_APPFONT ),
			                      aFtImage.GetImage().GetSizePixel() );
		aFtImage.Show();
	}

	SetText( rText );
	aFtDescription.SetText( rDesc );

	aBtn1.SetClickHdl( LINK( this, SvxMessDialog, Button1Hdl ) );
	aBtn2.SetClickHdl( LINK( this, SvxMessDialog, Button2Hdl ) );
}

SvxMessDialog::~SvxMessDialog()
{
	if( pImage )
		delete pImage;
}

/*************************************************************************/

IMPL_LINK_INLINE_START( SvxMessDialog, Button1Hdl, Button *, EMPTYARG )
{
	EndDialog( RET_BTN_1 );
	return 0;
}
IMPL_LINK_INLINE_END( SvxMessDialog, Button1Hdl, Button *, EMPTYARG )

/*************************************************************************/

IMPL_LINK_INLINE_START( SvxMessDialog, Button2Hdl, Button *, EMPTYARG )
{
	EndDialog( RET_BTN_2 );
	return 0;
}
IMPL_LINK_INLINE_END( SvxMessDialog, Button2Hdl, Button *, EMPTYARG )

/*************************************************************************/

void SvxMessDialog::SetButtonText( USHORT nBtnId, const String& rNewTxt )
{
	switch ( nBtnId )
	{
		case MESS_BTN_1:
			aBtn1.SetText( rNewTxt );
			break;

		case MESS_BTN_2:
			aBtn2.SetText( rNewTxt );
			break;

		default:
			DBG_ERROR( "Falsche Button-Nummer!!!" );
	}
}


