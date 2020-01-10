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

#include <string> // HACK: prevent conflict between STLPORT and Workshop headers

#include <tools/ref.hxx>
#include <tools/shl.hxx>
#include <svtools/aeitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/imagemgr.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/toolbox.hxx>

#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>

#include "tbxctl.hxx"
#include "tbxdraw.hxx"
#include "tbxcolor.hxx"
#include "tbxdraw.hrc"
#include <com/sun/star/frame/XLayoutManager.hpp>

SFX_IMPL_TOOLBOX_CONTROL(SvxTbxCtlDraw, SfxAllEnumItem);

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;

// -----------------------------------------------------------------------

SvxTbxCtlDraw::SvxTbxCtlDraw( USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :

    SfxToolBoxControl( nSlotId, nId, rTbx ),

    m_sToolboxName( RTL_CONSTASCII_USTRINGPARAM( "private:resource/toolbar/drawbar" ) )

{
    rTbx.SetItemBits( nId, TIB_CHECKABLE | rTbx.GetItemBits( nId ) );
	rTbx.Invalidate();
}

// -----------------------------------------------------------------------

void SvxTbxCtlDraw::StateChanged( USHORT nSID, SfxItemState eState,
								  const SfxPoolItem* pState )
{
    GetToolBox().EnableItem( GetId(), ( eState != SFX_ITEM_DISABLED ) );
    SfxToolBoxControl::StateChanged( nSID, eState, pState );

    Reference< XLayoutManager > xLayoutMgr = getLayoutManager();
    if ( xLayoutMgr.is() )
        GetToolBox().CheckItem(
            GetId(), xLayoutMgr->isElementVisible( m_sToolboxName ) != sal_False );
}

// -----------------------------------------------------------------------

SfxPopupWindowType SvxTbxCtlDraw::GetPopupWindowType() const
{
    return SFX_POPUPWINDOW_ONCLICK;
}

// -----------------------------------------------------------------------

void SvxTbxCtlDraw::toggleToolbox()
{
    Reference< XLayoutManager > xLayoutMgr = getLayoutManager();
    if ( xLayoutMgr.is() )
    {
        BOOL bCheck = FALSE;
        if ( xLayoutMgr->isElementVisible( m_sToolboxName ) )
        {
            xLayoutMgr->hideElement( m_sToolboxName );
            xLayoutMgr->destroyElement( m_sToolboxName );
        }
        else
        {
            bCheck = TRUE;
            xLayoutMgr->createElement( m_sToolboxName );
            xLayoutMgr->showElement( m_sToolboxName );
        }

        GetToolBox().CheckItem( GetId(), bCheck );
    }
}

// -----------------------------------------------------------------------

void SvxTbxCtlDraw::Select( BOOL )
{
    toggleToolbox();
}

