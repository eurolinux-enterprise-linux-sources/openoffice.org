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
#include "precompiled_svtools.hxx"
#include <filedlg.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/logindlg.hxx>

#ifndef _SVTOOLS_LOGINDLG_HRC_
#include "logindlg.hrc"
#endif
#ifndef _SVTOOLS_HRC
#include <svtools/svtools.hrc>
#endif
#include <svtools/svtdata.hxx>

#ifdef UNX
#include <limits.h>
#define _MAX_PATH PATH_MAX
#endif

// LoginDialog -------------------------------------------------------

//............................................................................
namespace svt
{
//............................................................................

void LoginDialog::HideControls_Impl( USHORT nFlags )
{
	FASTBOOL bPathHide = FALSE;
	FASTBOOL bErrorHide = FALSE;
	FASTBOOL bAccountHide = FALSE;

	if ( ( nFlags & LF_NO_PATH ) == LF_NO_PATH )
	{
		aPathFT.Hide();
		aPathED.Hide();
		aPathBtn.Hide();
		bPathHide = TRUE;
	}
	else if ( ( nFlags & LF_PATH_READONLY ) == LF_PATH_READONLY )
	{
		aPathED.Hide();
		aPathInfo.Show();
		aPathBtn.Hide();
	}

	if ( ( nFlags & LF_NO_USERNAME ) == LF_NO_USERNAME )
	{
		aNameFT.Hide();
		aNameED.Hide();
	}
	else if ( ( nFlags & LF_USERNAME_READONLY ) == LF_USERNAME_READONLY )
	{
		aNameED.Hide();
		aNameInfo.Show();
	}

	if ( ( nFlags & LF_NO_PASSWORD ) == LF_NO_PASSWORD )
	{
		aPasswordFT.Hide();
		aPasswordED.Hide();
	}

	if ( ( nFlags & LF_NO_SAVEPASSWORD ) == LF_NO_SAVEPASSWORD )
		aSavePasswdBtn.Hide();

	if ( ( nFlags & LF_NO_ERRORTEXT ) == LF_NO_ERRORTEXT )
	{
		aErrorInfo.Hide();
		aErrorGB.Hide();
		bErrorHide = TRUE;
	}

	if ( ( nFlags & LF_NO_ACCOUNT ) == LF_NO_ACCOUNT )
	{
		aAccountFT.Hide();
		aAccountED.Hide();
		bAccountHide = TRUE;
	}

	if ( bErrorHide )
	{
		long nOffset = aLoginGB.GetPosPixel().Y() -
					   aErrorGB.GetPosPixel().Y();
		Point aNewPnt = aRequestInfo.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aRequestInfo.SetPosPixel( aNewPnt );
		aNewPnt = aPathFT.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPathFT.SetPosPixel( aNewPnt );
		aNewPnt = aPathED.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPathED.SetPosPixel( aNewPnt );
		aNewPnt = aPathInfo.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPathInfo.SetPosPixel( aNewPnt );
		aNewPnt = aPathBtn.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPathBtn.SetPosPixel( aNewPnt );
		aNewPnt = aNameFT.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aNameFT.SetPosPixel( aNewPnt );
		aNewPnt = aNameED.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aNameED.SetPosPixel( aNewPnt );
		aNewPnt = aNameInfo.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aNameInfo.SetPosPixel( aNewPnt );
		aNewPnt = aPasswordFT.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPasswordFT.SetPosPixel( aNewPnt );
		aNewPnt = aPasswordED.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aPasswordED.SetPosPixel( aNewPnt );
		aNewPnt = aAccountFT.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aAccountFT.SetPosPixel( aNewPnt );
		aNewPnt = aAccountED.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aAccountED.SetPosPixel( aNewPnt );
		aNewPnt = aSavePasswdBtn.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aSavePasswdBtn.SetPosPixel( aNewPnt );
		aNewPnt = aLoginGB.GetPosPixel();
		aNewPnt.Y() -= nOffset;
		aLoginGB.SetPosPixel( aNewPnt );
		Size aNewSiz = GetSizePixel();
		aNewSiz.Height() -= nOffset;
		SetSizePixel( aNewSiz );
	}

	if ( bPathHide )
	{
		long nOffset = aNameED.GetPosPixel().Y() -
					   aPathED.GetPosPixel().Y();

		Point aTmpPnt1 = aNameFT.GetPosPixel();
		Point aTmpPnt2 = aPasswordFT.GetPosPixel();
		aNameFT.SetPosPixel( aPathFT.GetPosPixel() );
		aPasswordFT.SetPosPixel( aTmpPnt1 );
		aAccountFT.SetPosPixel( aTmpPnt2 );
		aTmpPnt1 = aNameED.GetPosPixel();
		aTmpPnt2 = aPasswordED.GetPosPixel();
		aNameED.SetPosPixel( aPathED.GetPosPixel() );
		aPasswordED.SetPosPixel( aTmpPnt1 );
		aAccountED.SetPosPixel( aTmpPnt2 );
		aNameInfo.SetPosPixel( aPathInfo.GetPosPixel() );
		aTmpPnt1 = aSavePasswdBtn.GetPosPixel();
		aTmpPnt1.Y() -= nOffset;
		aSavePasswdBtn.SetPosPixel( aTmpPnt1 );
		Size aNewSz = GetSizePixel();
		aNewSz.Height() -= nOffset;
		SetSizePixel( aNewSz );
	}

	if ( bAccountHide )
	{
		long nOffset = aAccountED.GetPosPixel().Y() - aPasswordED.GetPosPixel().Y();

		Point aTmpPnt = aSavePasswdBtn.GetPosPixel();
		aTmpPnt.Y() -= nOffset;
		aSavePasswdBtn.SetPosPixel( aTmpPnt );
		Size aNewSz = GetSizePixel();
		aNewSz.Height() -= nOffset;
		SetSizePixel( aNewSz );
	}
};

// -----------------------------------------------------------------------

IMPL_LINK( LoginDialog, OKHdl_Impl, OKButton *, EMPTYARG )
{
	// trim the strings
	aNameED.SetText( aNameED.GetText().EraseLeadingChars().
		EraseTrailingChars() );
	aPasswordED.SetText( aPasswordED.GetText().EraseLeadingChars().
		EraseTrailingChars() );
	EndDialog( RET_OK );
	return 1;
}

// -----------------------------------------------------------------------

IMPL_LINK( LoginDialog, PathHdl_Impl, PushButton *, EMPTYARG )
{
	PathDialog* pDlg = new PathDialog( this, WB_3DLOOK );
//	DirEntry aEntry;
//	aEntry.ToAbs();
//	pDlg->SetPath( aEntry.GetFull() );

	if ( pDlg->Execute() == RET_OK )
		aPathED.SetText( pDlg->GetPath() );

	delete pDlg;
	return 1;
}

// -----------------------------------------------------------------------

LoginDialog::LoginDialog
(
	Window* pParent,
	USHORT nFlags,
	const String& rServer,
	const String* pRealm
) :

	ModalDialog( pParent, SvtResId( DLG_LOGIN ) ),

    aErrorInfo      ( this, SvtResId( INFO_LOGIN_ERROR ) ),
    aErrorGB        ( this, SvtResId( GB_LOGIN_ERROR ) ),
    aRequestInfo    ( this, SvtResId( INFO_LOGIN_REQUEST ) ),
    aPathFT         ( this, SvtResId( FT_LOGIN_PATH ) ),
    aPathED         ( this, SvtResId( ED_LOGIN_PATH ) ),
    aPathInfo       ( this, SvtResId( INFO_LOGIN_PATH ) ),
    aPathBtn        ( this, SvtResId( BTN_LOGIN_PATH ) ),
    aNameFT         ( this, SvtResId( FT_LOGIN_USERNAME ) ),
    aNameED         ( this, SvtResId( ED_LOGIN_USERNAME ) ),
    aNameInfo       ( this, SvtResId( INFO_LOGIN_USERNAME ) ),
    aPasswordFT     ( this, SvtResId( FT_LOGIN_PASSWORD ) ),
    aPasswordED     ( this, SvtResId( ED_LOGIN_PASSWORD ) ),
    aAccountFT      ( this, SvtResId( FT_LOGIN_ACCOUNT ) ),
    aAccountED      ( this, SvtResId( ED_LOGIN_ACCOUNT ) ),
    aSavePasswdBtn  ( this, SvtResId( CB_LOGIN_SAVEPASSWORD ) ),
    aLoginGB        ( this, SvtResId( GB_LOGIN_LOGIN ) ),
    aOKBtn          ( this, SvtResId( BTN_LOGIN_OK ) ),
    aCancelBtn      ( this, SvtResId( BTN_LOGIN_CANCEL ) ),
    aHelpBtn        ( this, SvtResId( BTN_LOGIN_HELP ) )

{
	// Einlog-Ort eintragen
	String aServer;

	if ( ( ( nFlags & LF_NO_ACCOUNT ) == LF_NO_ACCOUNT ) && pRealm && pRealm->Len() )
	{
		aServer = *pRealm;
        ( ( aServer += ' ' ) += String( SvtResId( STR_LOGIN_AT ) ) ) += ' ';
	}
	aServer += rServer;
	String aTxt = aRequestInfo.GetText();
	aTxt.SearchAndReplaceAscii( "%1", aServer );
	aRequestInfo.SetText( aTxt );

	FreeResource();

	aPathED.SetMaxTextLen( _MAX_PATH );
	aNameED.SetMaxTextLen( _MAX_PATH );

	aOKBtn.SetClickHdl( LINK( this, LoginDialog, OKHdl_Impl ) );
	aPathBtn.SetClickHdl( LINK( this, LoginDialog, PathHdl_Impl ) );

	HideControls_Impl( nFlags );
};

// -----------------------------------------------------------------------

void LoginDialog::SetName( const String& rNewName )
{
	aNameED.SetText( rNewName );
	aNameInfo.SetText( rNewName );
}

// -----------------------------------------------------------------------

void LoginDialog::ClearPassword()
{
	aPasswordED.SetText( String() );

	if ( 0 == aNameED.GetText().Len() )
		aNameED.GrabFocus();
	else
		aPasswordED.GrabFocus();
};

// -----------------------------------------------------------------------

void LoginDialog::ClearAccount()
{
	aAccountED.SetText( String() );
	aAccountED.GrabFocus();
};

//............................................................................
}	// namespace svt
//............................................................................
