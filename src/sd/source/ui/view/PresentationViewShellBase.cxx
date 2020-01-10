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

#include "PresentationViewShellBase.hxx"
#include "sdresid.hxx"
#include "DrawDocShell.hxx"
#include "strings.hrc"
#include "UpdateLockManager.hxx"
#include "framework/FrameworkHelper.hxx"
#include "framework/PresentationModule.hxx"

#include <sfx2/viewfrm.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace sd {

class DrawDocShell;

TYPEINIT1(PresentationViewShellBase, ViewShellBase);

// We have to expand the SFX_IMPL_VIEWFACTORY macro to call LateInit() after a
// new PresentationViewShellBase object has been constructed.

/*
SFX_IMPL_VIEWFACTORY(PresentationViewShellBase, SdResId(STR_DEFAULTVIEW))
{
	SFX_VIEW_REGISTRATION(DrawDocShell);
}
*/
SfxViewFactory* PresentationViewShellBase::pFactory;
SfxViewShell* __EXPORT PresentationViewShellBase::CreateInstance (
    SfxViewFrame *_pFrame, SfxViewShell *pOldView)
{
    PresentationViewShellBase* pBase = 
        new PresentationViewShellBase(_pFrame, pOldView);
    pBase->LateInit(framework::FrameworkHelper::msPresentationViewURL);
    return pBase;
}
void PresentationViewShellBase::RegisterFactory( USHORT nPrio )
{ 
    pFactory = new SfxViewFactory(
        &CreateInstance,&InitFactory,nPrio,SdResId(STR_DEFAULTVIEW));
    InitFactory();
}
void PresentationViewShellBase::InitFactory()
{
	SFX_VIEW_REGISTRATION(DrawDocShell);
}




PresentationViewShellBase::PresentationViewShellBase (
    SfxViewFrame* _pFrame, 
    SfxViewShell* pOldShell)
    : ViewShellBase (_pFrame, pOldShell)
{
    GetUpdateLockManager()->Disable();

    // Hide the automatic (non-context sensitive) tool bars.
    if (_pFrame!=NULL && _pFrame->GetFrame()!=NULL)
    {
        Reference<beans::XPropertySet> xFrameSet (
            _pFrame->GetFrame()->GetFrameInterface(),
            UNO_QUERY);
        if (xFrameSet.is())
        {
            Reference<beans::XPropertySet> xLayouterSet (
                xFrameSet->getPropertyValue(::rtl::OUString::createFromAscii("LayoutManager")),
                UNO_QUERY);
            if (xLayouterSet.is())
            {
                xLayouterSet->setPropertyValue(
                    ::rtl::OUString::createFromAscii("AutomaticToolbars"),
                    makeAny(sal_False));
            }
        }
    }
}




PresentationViewShellBase::~PresentationViewShellBase (void)
{
}




void PresentationViewShellBase::InitializeFramework (void)
{
    com::sun::star::uno::Reference<com::sun::star::frame::XController>
        xController (GetController());
    sd::framework::PresentationModule::Initialize(xController);
}

} // end of namespace sd

