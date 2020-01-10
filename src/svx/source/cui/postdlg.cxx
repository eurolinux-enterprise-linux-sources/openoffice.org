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
#include <tools/date.hxx>
#include <tools/time.hxx>
#include <vcl/svapp.hxx>
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <svtools/itempool.hxx>
#include <svtools/itemset.hxx>
#include <svtools/useroptions.hxx>
#include <unotools/localedatawrapper.hxx>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif

#define _SVX_POSTDLG_CXX

#include <svx/dialogs.hrc>
#include "postdlg.hrc"





#include "postattr.hxx"
#include "postdlg.hxx"
#include <svx/dialmgr.hxx>

#include "helpid.hrc"

// static ----------------------------------------------------------------

static USHORT pRanges[] =
{
	SID_ATTR_POSTIT_AUTHOR,
	SID_ATTR_POSTIT_TEXT,
	0
};

// class SvxPostItDialog -------------------------------------------------

SvxPostItDialog::SvxPostItDialog( Window* pParent,
								  const SfxItemSet& rCoreSet,
								  BOOL bPrevNext,
								  BOOL bRedline ) :

	SfxModalDialog( pParent, SVX_RES( RID_SVXDLG_POSTIT ) ),

    aPostItFL       ( this, SVX_RES( FL_POSTIT ) ),
    aLastEditLabelFT( this, SVX_RES( FT_LASTEDITLABEL ) ),
	aLastEditFT 	( this, SVX_RES( FT_LASTEDIT ) ),
	aEditFT 		( this, SVX_RES( FT_EDIT ) ),
	aEditED 		( this, SVX_RES( ED_EDIT ) ),
    aAuthorFT       ( this, SVX_RES( FT_AUTHOR) ),
    aAuthorBtn      ( this, SVX_RES( BTN_AUTHOR ) ),
    aOKBtn          ( this, SVX_RES( BTN_POST_OK ) ),
	aCancelBtn		( this, SVX_RES( BTN_POST_CANCEL ) ),
    aHelpBtn        ( this, SVX_RES( BTN_POST_HELP ) ),
    aPrevBtn        ( this, SVX_RES( BTN_PREV ) ),
	aNextBtn		( this, SVX_RES( BTN_NEXT ) ),

	rSet		( rCoreSet ),
	pOutSet 	( 0 )

{
	if (bRedline)	// HelpIDs fuer Redlining
	{
		SetHelpId(HID_REDLINING_DLG);
		aEditED.SetHelpId(HID_REDLINING_EDIT);
		aPrevBtn.SetHelpId(HID_REDLINING_PREV);
		aNextBtn.SetHelpId(HID_REDLINING_NEXT);
	}

	aPrevBtn.SetClickHdl( LINK( this, SvxPostItDialog, PrevHdl ) );
	aNextBtn.SetClickHdl( LINK( this, SvxPostItDialog, NextHdl ) );
	aAuthorBtn.SetClickHdl( LINK( this, SvxPostItDialog, Stamp ) );
	aOKBtn.SetClickHdl( LINK( this, SvxPostItDialog, OKHdl ) );

	Font aFont( aEditED.GetFont() );
	aFont.SetWeight( WEIGHT_LIGHT );
	aEditED.SetFont( aFont );

	BOOL bNew = TRUE;
	USHORT nWhich			 = 0;

	if ( !bPrevNext )
	{
		aPrevBtn.Hide();
		aNextBtn.Hide();
	}

	nWhich = rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_AUTHOR );
	String aAuthorStr, aDateStr, aTextStr;

	if ( rSet.GetItemState( nWhich, TRUE ) >= SFX_ITEM_AVAILABLE )
	{
		bNew = FALSE;
		const SvxPostItAuthorItem& rAuthor =
			(const SvxPostItAuthorItem&)rSet.Get( nWhich );
		aAuthorStr = rAuthor.GetValue();
	}
	else
		aAuthorStr = SvtUserOptions().GetID();

	nWhich = rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_DATE );

	if ( rSet.GetItemState( nWhich, TRUE ) >= SFX_ITEM_AVAILABLE )
	{
		const SvxPostItDateItem& rDate =
			(const SvxPostItDateItem&)rSet.Get( nWhich );
		aDateStr = rDate.GetValue();
	}
	else
	{
		LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
		aDateStr = aLocaleWrapper.getDate( Date() );
	}

	nWhich = rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_TEXT );

	if ( rSet.GetItemState( nWhich, TRUE ) >= SFX_ITEM_AVAILABLE )
	{
		const SvxPostItTextItem& rText =
			(const SvxPostItTextItem&)rSet.Get( nWhich );
		aTextStr = rText.GetValue();
	}

	ShowLastAuthor(aAuthorStr, aDateStr);
	aEditED.SetText( aTextStr.ConvertLineEnd() );

	if ( !bNew )
		SetText( SVX_RESSTR( STR_NOTIZ_EDIT ) );
	else
		// neu anlegen
		SetText( SVX_RESSTR( STR_NOTIZ_INSERT ) );

	FreeResource();
}

// -----------------------------------------------------------------------

SvxPostItDialog::~SvxPostItDialog()
{
	delete pOutSet;
	pOutSet = 0;
}

// -----------------------------------------------------------------------

void SvxPostItDialog::ShowLastAuthor(const String& rAuthor, const String& rDate)
{
	String sTxt( rAuthor );
	sTxt.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " ) );
	sTxt += rDate;
	aLastEditFT.SetText( sTxt );
}

// -----------------------------------------------------------------------

USHORT* SvxPostItDialog::GetRanges()
{
	return pRanges;
}

// -----------------------------------------------------------------------

void SvxPostItDialog::EnableTravel(BOOL bNext, BOOL bPrev)
{
	aPrevBtn.Enable(bPrev);
	aNextBtn.Enable(bNext);
}

// -----------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvxPostItDialog, PrevHdl, Button *, EMPTYARG )
{
	aPrevHdlLink.Call( this );
	return 0;
}
IMPL_LINK_INLINE_END( SvxPostItDialog, PrevHdl, Button *, EMPTYARG )

// -----------------------------------------------------------------------

IMPL_LINK_INLINE_START( SvxPostItDialog, NextHdl, Button *, EMPTYARG )
{
	aNextHdlLink.Call( this );
	return 0;
}
IMPL_LINK_INLINE_END( SvxPostItDialog, NextHdl, Button *, EMPTYARG )

// -----------------------------------------------------------------------

IMPL_LINK( SvxPostItDialog, Stamp, Button *, EMPTYARG )
{
	Date aDate;
	Time aTime;
	String aTmp( SvtUserOptions().GetID() );
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
	String aStr( aEditED.GetText() );
	aStr.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "\n---- " ) );

	if ( aTmp.Len() > 0 )
	{
		aStr += aTmp;
		aStr.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " ) );
	}
	aStr += aLocaleWrapper.getDate(aDate);
	aStr.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " ) );
	aStr += aLocaleWrapper.getTime(aTime, FALSE, FALSE);
	aStr.AppendAscii( RTL_CONSTASCII_STRINGPARAM( " ----\n" ) );


	aEditED.SetText( aStr.ConvertLineEnd() );
	xub_StrLen nLen = aStr.Len();
	aEditED.GrabFocus();
	aEditED.SetSelection( Selection( nLen, nLen ) );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxPostItDialog, OKHdl, Button *, EMPTYARG )
{
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
	pOutSet = new SfxItemSet( rSet );
	pOutSet->Put( SvxPostItAuthorItem( SvtUserOptions().GetID(),
				  					   rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_AUTHOR ) ) );
	pOutSet->Put( SvxPostItDateItem( aLocaleWrapper.getDate( Date() ),
									 rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_DATE ) ) );
	pOutSet->Put( SvxPostItTextItem( aEditED.GetText(),
									 rSet.GetPool()->GetWhich( SID_ATTR_POSTIT_TEXT ) ) );
	EndDialog( RET_OK );
	return 0;
}

