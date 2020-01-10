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

#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>
#include <verttexttbxctrl.hxx>
#include <svtools/languageoptions.hxx>
#include <sfx2/app.hxx>
#include <svtools/eitem.hxx>
#include <vcl/toolbox.hxx>
#include <rtl/ustring.hxx>

SFX_IMPL_TOOLBOX_CONTROL(SvxCTLTextTbxCtrl, SfxBoolItem);
SFX_IMPL_TOOLBOX_CONTROL(SvxVertTextTbxCtrl, SfxBoolItem);

// -----------------------------27.04.01 15:50--------------------------------

SvxCTLTextTbxCtrl::SvxCTLTextTbxCtrl(USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :
    SvxVertCTLTextTbxCtrl( nSlotId, nId, rTbx )
{
    SetVert(FALSE);
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:CTLFontState" )));
}

SvxVertTextTbxCtrl::SvxVertTextTbxCtrl( USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :
    SvxVertCTLTextTbxCtrl( nSlotId, nId, rTbx )
{
    SetVert(TRUE);
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:VerticalTextState" )));
}

/* ---------------------------------------------------------------------------*/
SvxVertCTLTextTbxCtrl::SvxVertCTLTextTbxCtrl( USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :
    SfxToolBoxControl( nSlotId, nId, rTbx ),
    bCheckVertical(sal_True)
{
}

/* -----------------------------27.04.01 15:53--------------------------------

 ---------------------------------------------------------------------------*/
SvxVertCTLTextTbxCtrl::~SvxVertCTLTextTbxCtrl( )
{
}
/* -----------------------------27.04.01 15:50--------------------------------

 ---------------------------------------------------------------------------*/
void SvxVertCTLTextTbxCtrl::StateChanged( 
    USHORT nSID, 
    SfxItemState eState,
    const SfxPoolItem* pState )
{
	SvtLanguageOptions aLangOptions;
    BOOL bCalc = sal_False;
    BOOL bVisible = GetToolBox().IsItemVisible(GetId());
	sal_Bool bEnabled = sal_False;
	if ( nSID == SID_VERTICALTEXT_STATE )
		bEnabled = aLangOptions.IsVerticalTextEnabled();
	else if ( nSID == SID_CTLFONT_STATE )
		bEnabled = aLangOptions.IsCTLFontEnabled();
	else
	{
		SfxToolBoxControl::StateChanged(nSID, eState, pState);
		return;
	}

    if(bEnabled)
    {
        if(!bVisible)
        {
            GetToolBox().ShowItem( GetId(), TRUE );
            bCalc = sal_True;
        }
    }
    else if(bVisible)
    {
        GetToolBox().HideItem( GetId() );
        bCalc = sal_True;
    }
    if(bCalc)
    {
        ToolBox& rTbx = GetToolBox();
		Window* pParent = rTbx.GetParent();
		WindowType nWinType = pParent->GetType();
        if(WINDOW_FLOATINGWINDOW == nWinType)
		{
			Size aSize(rTbx.CalcWindowSizePixel());
			rTbx.SetPosSizePixel( Point(), aSize );
            pParent->SetOutputSizePixel( aSize );
		}
    }
}
/* -----------------------------27.04.01 15:50--------------------------------

 ---------------------------------------------------------------------------*/

