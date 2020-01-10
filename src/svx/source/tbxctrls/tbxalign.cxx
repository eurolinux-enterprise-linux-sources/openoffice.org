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
#include <svtools/aeitem.hxx>

#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>

#include "tbxalign.hxx"
#include "tbxdraw.hxx"
#include "tbxdraw.hrc"
#include <tools/shl.hxx>
#ifndef	_SFX_IMAGEMGR_HXX
#include <sfx2/imagemgr.hxx>
#endif
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>

#include <sfx2/app.hxx>
#include <vcl/toolbox.hxx>

SFX_IMPL_TOOLBOX_CONTROL(SvxTbxCtlAlign, SfxAllEnumItem);

/*************************************************************************
|*
|* Klasse fuer SwToolbox
|*
\************************************************************************/

SvxTbxCtlAlign::SvxTbxCtlAlign( USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :
	SfxToolBoxControl( nSlotId, nId, rTbx )
    ,   m_aSubTbName( RTL_CONSTASCII_USTRINGPARAM( "alignmentbar" ))
    ,   m_aSubTbResName( RTL_CONSTASCII_USTRINGPARAM( "private:resource/toolbar/alignmentbar" ))
{
	rTbx.SetItemBits( nId, TIB_DROPDOWN | rTbx.GetItemBits( nId ) );
	rTbx.Invalidate();

    m_aCommand = m_aCommandURL;
}

/*************************************************************************
|*
|* Wenn man ein PopupWindow erzeugen will
|*
\************************************************************************/

SfxPopupWindowType SvxTbxCtlAlign::GetPopupWindowType() const
{
	return(SFX_POPUPWINDOW_ONCLICK);
}

/*************************************************************************
|*
|* Hier wird das Fenster erzeugt
|* Lage der Toolbox mit GetToolBox() abfragbar
|* rItemRect sind die Screen-Koordinaten
|*
\************************************************************************/

SfxPopupWindow*	SvxTbxCtlAlign::CreatePopupWindow()
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
	if ( GetSlotId() == SID_OBJECT_ALIGN )
        createAndPositionSubToolBar( m_aSubTbResName );
	return NULL;
}

//========================================================================
// XSubToolbarController
//========================================================================

::sal_Bool SAL_CALL SvxTbxCtlAlign::opensSubToolbar() throw (::com::sun::star::uno::RuntimeException)
{
    // We control a sub-toolbar therefor, we have to return true.
    return sal_True;
}

::rtl::OUString SAL_CALL SvxTbxCtlAlign::getSubToolbarName() throw (::com::sun::star::uno::RuntimeException)
{
    // Provide the controlled sub-toolbar name, so we are notified whenever 
    // this toolbar executes a function.
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    return m_aSubTbName;
}

void SAL_CALL SvxTbxCtlAlign::functionSelected( const ::rtl::OUString& aCommand ) throw (::com::sun::star::uno::RuntimeException)
{
    // Our sub-toolbar wants to executes a function. We have to change
    // the image of our toolbar button to reflect the new function.
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if ( !m_bDisposed )
    {
        if ( aCommand.getLength() > 0 )
        {
            ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > xFrame( getFrameInterface());
            Image aImage = GetImage( xFrame, aCommand, hasBigImages(), isHighContrast() );
            if ( !!aImage )
                GetToolBox().SetItemImage( GetId(), aImage );
        }
    }
}

void SAL_CALL SvxTbxCtlAlign::updateImage() throw (::com::sun::star::uno::RuntimeException)
{
    // We should update the button image of our parent (toolbar). Use the stored
    // command to set the correct current image.
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if ( m_aCommand.getLength() > 0 )
    {
        ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > xFrame( getFrameInterface());
        Image aImage = GetImage( xFrame, m_aCommand, hasBigImages(), isHighContrast() );
        if ( !!aImage )
            GetToolBox().SetItemImage( GetId(), aImage );
    }
}
