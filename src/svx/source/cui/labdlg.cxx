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
#include <sfx2/app.hxx>
#include <sfx2/module.hxx>
#include <swpossizetabpage.hxx>

#define _SVX_LABDLG_CXX

#include <svx/svdattrx.hxx>
#include <svx/dialogs.hrc>
#include <svx/dialmgr.hxx>
#include "dlgutil.hxx"
#include "transfrm.hxx"

#include "labdlg.hrc"
#include "labdlg.hxx"

// define ----------------------------------------------------------------

#define AZ_OPTIMAL		0
#define AZ_VON_OBEN		1
#define AZ_VON_LINKS	2
#define AZ_HORIZONTAL	3
#define AZ_VERTIKAL		4

#define AT_OBEN			0
#define AT_MITTE		1
#define AT_UNTEN		2

#define WK_OPTIMAL		0
#define WK_30			1
#define WK_45			2
#define WK_60			3
#define WK_90			4


//========================================================================


SvxCaptionTabDialog::SvxCaptionTabDialog(Window* pParent, const SdrView* pSdrView, USHORT nAnchorTypes)
 :	SfxTabDialog( pParent, SVX_RES( RID_SVXDLG_CAPTION ) ),
    pView       ( pSdrView ),
    nAnchorCtrls(nAnchorTypes)
{
	FreeResource();

	DBG_ASSERT( pView, "Keine gueltige View Uebergeben!" );

    //different positioning page in Writer
    if(nAnchorCtrls & 0x00ff )
    {        
        AddTabPage( RID_SVXPAGE_SWPOSSIZE, SvxSwPosSizeTabPage::Create, 
                                SvxSwPosSizeTabPage::GetRanges );
        RemoveTabPage( RID_SVXPAGE_POSITION_SIZE);
    }
    else
    {        
        AddTabPage( RID_SVXPAGE_POSITION_SIZE, SvxPositionSizeTabPage::Create,
                                SvxPositionSizeTabPage::GetRanges );
        RemoveTabPage( RID_SVXPAGE_SWPOSSIZE );
    }
}

// -----------------------------------------------------------------------

SvxCaptionTabDialog::~SvxCaptionTabDialog()
{
}

// -----------------------------------------------------------------------

void SvxCaptionTabDialog::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	switch( nId )
	{
		case RID_SVXPAGE_POSITION_SIZE:
			( (SvxPositionSizeTabPage&) rPage ).SetView( pView );
			( (SvxPositionSizeTabPage&) rPage ).Construct();
			if( nAnchorCtrls & SVX_OBJ_NORESIZE )
				( (SvxPositionSizeTabPage&) rPage ).DisableResize();

			if( nAnchorCtrls & SVX_OBJ_NOPROTECT )
				( (SvxPositionSizeTabPage&) rPage ).DisableProtect();
		break;
        case RID_SVXPAGE_SWPOSSIZE :
        {
            SvxSwPosSizeTabPage& rSwPage = static_cast<SvxSwPosSizeTabPage&>(rPage);
            rSwPage.EnableAnchorTypes(nAnchorCtrls);
            rSwPage.SetValidateFramePosLink( aValidateLink );
        }            
        break;
	}
}
/*-- 05.03.2004 13:54:26---------------------------------------------------

  -----------------------------------------------------------------------*/
void SvxCaptionTabDialog::SetValidateFramePosLink( const Link& rLink )
{
    aValidateLink = rLink;
}            


