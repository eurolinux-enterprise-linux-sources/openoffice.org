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

#include "ViewTabBar.hxx"

#define USE_TAB_CONTROL

#include "ViewShell.hxx"
#include "ViewShellBase.hxx"
#include "DrawViewShell.hxx"
#include "FrameView.hxx"
#include "EventMultiplexer.hxx"
#include "framework/FrameworkHelper.hxx"
#include "framework/Pane.hxx"
#include "DrawController.hxx"

#include "sdresid.hxx"
#include "strings.hrc"
#include "helpids.h"
#include "Client.hxx"
#include <vcl/svapp.hxx>
#include <vcl/tabpage.hxx>
#include <vos/mutex.hxx>
#include <sfx2/viewfrm.hxx>
#include <com/sun/star/drawing/framework/ResourceId.hpp>
#include <com/sun/star/drawing/framework/XControllerManager.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <comphelper/processfactory.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;
using ::sd::framework::FrameworkHelper;
using ::sd::tools::EventMultiplexerEvent;
using ::rtl::OUString;

namespace sd {

namespace {
bool IsEqual (const TabBarButton& rButton1, const TabBarButton& rButton2)
{
    return (
        (rButton1.ResourceId.is()
            && rButton2.ResourceId.is()
            && rButton1.ResourceId->compareTo(rButton2.ResourceId)==0)
        || rButton1.ButtonLabel == rButton2.ButtonLabel);
}

class TabBarControl : public ::TabControl
{
public:
    TabBarControl (
        ::Window* pParentWindow,
        const ::rtl::Reference<ViewTabBar>& rpViewTabBar);
    virtual void Paint (const Rectangle& rRect);
    virtual void ActivatePage (void);
private:
    ::rtl::Reference<ViewTabBar> mpViewTabBar;
};

} // end of anonymous namespace





class ViewTabPage : public TabPage
{
public:
    ViewTabPage (Window* pParent) : TabPage(pParent) {}
    virtual void Resize (void) 
    { SetPosSizePixel(Point(0,0),GetParent()->GetOutputSizePixel()); }
};




//===== ViewTabBar ============================================================

ViewTabBar::ViewTabBar (
    const Reference<XResourceId>& rxViewTabBarId,
    const Reference<frame::XController>& rxController)
    : ViewTabBarInterfaceBase(maMutex),
      mpTabControl(new TabBarControl(GetAnchorWindow(rxViewTabBarId,rxController), this)),
      mxController(rxController),
      maTabBarButtons(),
      mpTabPage(NULL),
      mxViewTabBarId(rxViewTabBarId),
      mpViewShellBase(NULL)
{
    // Set one new tab page for all tab entries.  We need it only to
    // determine the height of the tab bar.
    mpTabPage.reset(new TabPage (mpTabControl.get()));
    mpTabPage->Hide();

    // add some space before the tabitems
    mpTabControl->SetItemsOffset(Point(5, 3));

    // Tunnel through the controller and use the ViewShellBase to obtain the
    // view frame.
    try
    {
        Reference<lang::XUnoTunnel> xTunnel (mxController, UNO_QUERY_THROW);
        DrawController* pController = reinterpret_cast<DrawController*>(
            xTunnel->getSomething(DrawController::getUnoTunnelId()));
        mpViewShellBase = pController->GetViewShellBase();
    }
    catch(RuntimeException&)
    {}

    // Register as listener at XConfigurationController.
    Reference<XControllerManager> xControllerManager (mxController, UNO_QUERY);
    if (xControllerManager.is())
    {
        mxConfigurationController = xControllerManager->getConfigurationController();
        if (mxConfigurationController.is())
        {
            mxConfigurationController->addConfigurationChangeListener(
                this,
                    FrameworkHelper::msResourceActivationEvent,
                Any());
        }
    }

    mpTabControl->Show();
    
    if (mpViewShellBase != NULL
        && rxViewTabBarId->isBoundToURL(
            FrameworkHelper::msCenterPaneURL, AnchorBindingMode_DIRECT))
    {
        mpViewShellBase->SetViewTabBar(this);
    }
}




ViewTabBar::~ViewTabBar (void)
{
}




void ViewTabBar::disposing (void)
{
    if (mpViewShellBase != NULL
        && mxViewTabBarId->isBoundToURL(
            FrameworkHelper::msCenterPaneURL, AnchorBindingMode_DIRECT))
    {
        mpViewShellBase->SetViewTabBar(NULL);
    }

    if (mxConfigurationController.is())
    {
        // Unregister listener from XConfigurationController.
        try
        {
            mxConfigurationController->removeConfigurationChangeListener(this);
        }
        catch (lang::DisposedException e)
        {
            // Receiving a disposed exception is the normal case.  Is there
            // a way to avoid it?
        }
        mxConfigurationController = NULL;
    }

    {
        const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
        // Set all references to the one tab page to NULL and delete the page.
        for (USHORT nIndex=0; nIndex<mpTabControl->GetPageCount(); ++nIndex)
            mpTabControl->SetTabPage(nIndex, NULL);
        mpTabPage.reset();
        mpTabControl.reset();
    }
    
    mxController = NULL;
}




::boost::shared_ptr< ::TabControl> ViewTabBar::GetTabControl (void) const
{
    return mpTabControl;
}




::Window* ViewTabBar::GetAnchorWindow(
    const Reference<XResourceId>& rxViewTabBarId,
    const Reference<frame::XController>& rxController)
{
    ::Window* pWindow = NULL;
    ViewShellBase* pBase = NULL;
    
    // Tunnel through the controller and use the ViewShellBase to obtain the
    // view frame.
    try
    {
        Reference<lang::XUnoTunnel> xTunnel (rxController, UNO_QUERY_THROW);
        DrawController* pController = reinterpret_cast<DrawController*>(
            xTunnel->getSomething(DrawController::getUnoTunnelId()));
        pBase = pController->GetViewShellBase();
    }
    catch(RuntimeException&)
    {}

    // The ViewTabBar supports at the moment only the center pane.
    if (rxViewTabBarId.is()
        && rxViewTabBarId->isBoundToURL(
            FrameworkHelper::msCenterPaneURL, AnchorBindingMode_DIRECT))
    {
        if (pBase != NULL && pBase->GetViewFrame() != NULL)
            pWindow = &pBase->GetViewFrame()->GetWindow();
    }

    // The rest is (at the moment) just for the emergency case.
    if (pWindow == NULL)
    {
        Reference<XPane> xPane;
        try
        {
            Reference<XControllerManager> xControllerManager (rxController, UNO_QUERY_THROW);
            Reference<XConfigurationController> xCC (
                xControllerManager->getConfigurationController());
            if (xCC.is())
                xPane = Reference<XPane>(xCC->getResource(rxViewTabBarId->getAnchor()), UNO_QUERY);
        }
        catch (RuntimeException&)
        {}

        // Tunnel through the XWindow to the VCL side.
        try
        {
            Reference<lang::XUnoTunnel> xTunnel (xPane, UNO_QUERY_THROW);
            framework::Pane* pPane = reinterpret_cast<framework::Pane*>(
                xTunnel->getSomething(framework::Pane::getUnoTunnelId()));
            if (pPane != NULL)
                pWindow = pPane->GetWindow()->GetParent();
        }
        catch (RuntimeException&)
        {}
    }
    
    return pWindow;
}




//----- XConfigurationChangeListener ------------------------------------------

void SAL_CALL  ViewTabBar::notifyConfigurationChange (
    const ConfigurationChangeEvent& rEvent)
    throw (RuntimeException)
{
    if (rEvent.Type.equals(FrameworkHelper::msResourceActivationEvent)
        && rEvent.ResourceId->getResourceURL().match(FrameworkHelper::msViewURLPrefix)
        && rEvent.ResourceId->isBoundTo(mxViewTabBarId->getAnchor(), AnchorBindingMode_DIRECT))
    {
        UpdateActiveButton();
    }
}




//----- XEventListener --------------------------------------------------------

void SAL_CALL ViewTabBar::disposing(
    const lang::EventObject& rEvent)
    throw (RuntimeException)
{
    if (rEvent.Source == mxConfigurationController)
    {
        mxConfigurationController = NULL;
        mxController = NULL;
    }
}




//----- XTabBar ---------------------------------------------------------------

void SAL_CALL ViewTabBar::addTabBarButtonAfter (
    const TabBarButton& rButton,
    const TabBarButton& rAnchor)
    throw (::com::sun::star::uno::RuntimeException)
{
    const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
    AddTabBarButton(rButton, rAnchor);
}




void SAL_CALL ViewTabBar::appendTabBarButton (const TabBarButton& rButton)
    throw (::com::sun::star::uno::RuntimeException)
{
    const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
    AddTabBarButton(rButton);
}



void SAL_CALL ViewTabBar::removeTabBarButton (const TabBarButton& rButton)
    throw (::com::sun::star::uno::RuntimeException)
{
    const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
    RemoveTabBarButton(rButton);
}




sal_Bool SAL_CALL ViewTabBar::hasTabBarButton (const TabBarButton& rButton)
    throw (::com::sun::star::uno::RuntimeException)
{
    const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
    return HasTabBarButton(rButton);
}




Sequence<TabBarButton> SAL_CALL ViewTabBar::getTabBarButtons (void)
    throw (::com::sun::star::uno::RuntimeException)
{
    const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
    return GetTabBarButtons();
}




//----- XResource -------------------------------------------------------------

Reference<XResourceId> SAL_CALL ViewTabBar::getResourceId (void)
    throw (RuntimeException)
{
    return mxViewTabBarId;
}




sal_Bool SAL_CALL ViewTabBar::isAnchorOnly (void)
    throw (RuntimeException)
{
    return false;
}




//----- XUnoTunnel ------------------------------------------------------------

const Sequence<sal_Int8>& ViewTabBar::getUnoTunnelId (void)
{
	static Sequence<sal_Int8>* pSequence = NULL;
	if (pSequence == NULL)
	{
        const ::vos::OGuard aSolarGuard (Application::GetSolarMutex());
		if (pSequence == NULL)
		{
			static ::com::sun::star::uno::Sequence<sal_Int8> aSequence (16);
			rtl_createUuid((sal_uInt8*)aSequence.getArray(), 0, sal_True);
			pSequence = &aSequence;
		}
	}
	return *pSequence;
}




sal_Int64 SAL_CALL ViewTabBar::getSomething (const Sequence<sal_Int8>& rId)
    throw (RuntimeException)
{    
    sal_Int64 nResult = 0;

    if (rId.getLength() == 16
        && rtl_compareMemory(getUnoTunnelId().getConstArray(), rId.getConstArray(), 16) == 0)
	{
		nResult = reinterpret_cast<sal_Int64>(this);
	}

    return nResult;
}




//-----------------------------------------------------------------------------

bool ViewTabBar::ActivatePage (void)
{
    try
    {
        Reference<XControllerManager> xControllerManager (mxController,UNO_QUERY_THROW);
        Reference<XConfigurationController> xConfigurationController (
            xControllerManager->getConfigurationController());
        if ( ! xConfigurationController.is())
            throw RuntimeException();
        Reference<XView> xView;
        try
        {
            xView = Reference<XView>(xConfigurationController->getResource(
                ResourceId::create(
                    ::comphelper::getProcessComponentContext(),
                    FrameworkHelper::msCenterPaneURL)),
                UNO_QUERY);
        }
        catch (DeploymentException)
        {
        }
        
        Client* pIPClient = NULL;
        if (mpViewShellBase != NULL)
            pIPClient = dynamic_cast<Client*>(mpViewShellBase->GetIPClient());
        if (pIPClient==NULL || ! pIPClient->IsObjectInPlaceActive())
        {
            USHORT nIndex (mpTabControl->GetCurPageId() - 1);
            if (nIndex < maTabBarButtons.size())
            {
                xConfigurationController->requestResourceActivation(
                    maTabBarButtons[nIndex].ResourceId,
                    ResourceActivationMode_REPLACE);
            }

            return true;
        }
        else
        {
            // When we run into this else branch then we have an active OLE
            // object.  We ignore the request to switch views.  Additionally
            // we put the active tab back to the one for the current view.
            UpdateActiveButton();
        }
    }
    catch (RuntimeException&)
    {
        DBG_ASSERT(false,"ViewTabBar::ActivatePage(): caught exception");
    }
    
    return false;
}




int ViewTabBar::GetHeight (void)
{
    int nHeight (0);

    if (maTabBarButtons.size() > 0)
    {
        TabPage* pActivePage (mpTabControl->GetTabPage(
            mpTabControl->GetCurPageId()));
        if (pActivePage!=NULL && mpTabControl->IsReallyVisible())
            nHeight = pActivePage->GetPosPixel().Y();

        if (nHeight <= 0)
            // Using a default when the real height can not be determined.
            // To get correct height this method should be called when the
            // control is visible.
            nHeight = 21;
    }

    return nHeight;
}




void ViewTabBar::AddTabBarButton (
    const ::com::sun::star::drawing::framework::TabBarButton& rButton,
    const ::com::sun::star::drawing::framework::TabBarButton& rAnchor)
{
    sal_uInt32 nIndex;

    if ( ! rAnchor.ResourceId.is()
        || (rAnchor.ResourceId->getResourceURL().getLength() == 0
            && rAnchor.ButtonLabel.getLength() == 0))
    {
        nIndex = 0;
    }
    else
    {
        for (nIndex=0; nIndex<maTabBarButtons.size(); ++nIndex)
        {
            if (IsEqual(maTabBarButtons[nIndex], rAnchor))
            {
                ++nIndex;
                break;
            }
        }
    }
    
    AddTabBarButton(rButton,nIndex);
}




void ViewTabBar::AddTabBarButton (
    const ::com::sun::star::drawing::framework::TabBarButton& rButton)
{
    AddTabBarButton(rButton, maTabBarButtons.size());
}




void ViewTabBar::AddTabBarButton (
    const ::com::sun::star::drawing::framework::TabBarButton& rButton,
    sal_Int32 nPosition)
{
    if (nPosition>=0
        && nPosition<=mpTabControl->GetPageCount())
    {
        USHORT nIndex ((USHORT)nPosition);
        
        // Insert the button into our local array.
        maTabBarButtons.insert(maTabBarButtons.begin()+nIndex, rButton);
        UpdateTabBarButtons();
        UpdateActiveButton();
    }
}




void ViewTabBar::RemoveTabBarButton (
    const ::com::sun::star::drawing::framework::TabBarButton& rButton)
{
    USHORT nIndex;
    for (nIndex=0; nIndex<maTabBarButtons.size(); ++nIndex)
    {
        if (IsEqual(maTabBarButtons[nIndex], rButton))
        {
            maTabBarButtons.erase(maTabBarButtons.begin()+nIndex);
            UpdateTabBarButtons();
            UpdateActiveButton();
            break;
        }
    }
}




bool ViewTabBar::HasTabBarButton (
    const ::com::sun::star::drawing::framework::TabBarButton& rButton)
{
    bool bResult (false);
    
    for (sal_uInt32 nIndex=0; nIndex<maTabBarButtons.size(); ++nIndex)
    {
        if (IsEqual(maTabBarButtons[nIndex], rButton))
        {
            bResult = true;
            break;
        }
    }

    return bResult;
}




::com::sun::star::uno::Sequence<com::sun::star::drawing::framework::TabBarButton>
    ViewTabBar::GetTabBarButtons (void)
{
    sal_uInt32 nCount (maTabBarButtons.size());
    ::com::sun::star::uno::Sequence<com::sun::star::drawing::framework::TabBarButton>
          aList (nCount);

    for (sal_uInt32 nIndex=0; nIndex<nCount; ++nIndex)
        aList[nIndex] = maTabBarButtons[nIndex];

    return aList;
}




void ViewTabBar::UpdateActiveButton (void)
{
    Reference<XView> xView;
    if (mpViewShellBase != NULL)
        xView = FrameworkHelper::Instance(*mpViewShellBase)->GetView(
            mxViewTabBarId->getAnchor());
    if (xView.is())
    {
        Reference<XResourceId> xViewId (xView->getResourceId());
        for (USHORT nIndex=0; nIndex<maTabBarButtons.size(); ++nIndex)
        {
            if (maTabBarButtons[nIndex].ResourceId->compareTo(xViewId) == 0)
            {
                mpTabControl->SetCurPageId(nIndex+1);
                mpTabControl->::TabControl::ActivatePage();
                break;
            }
        }
    }
}




void ViewTabBar::UpdateTabBarButtons (void)
{
    TabBarButtonList::const_iterator iTab;
    USHORT nPageCount (mpTabControl->GetPageCount());
    USHORT nIndex;
    for (iTab=maTabBarButtons.begin(),nIndex=1; iTab!=maTabBarButtons.end(); ++iTab,++nIndex)
    {
        // Create a new tab when there are not enough.
        if (nPageCount < nIndex)
            mpTabControl->InsertPage(nIndex, iTab->ButtonLabel);

        // Update the tab.
        mpTabControl->SetPageText(nIndex, iTab->ButtonLabel);
        mpTabControl->SetHelpText(nIndex, iTab->HelpText);
        mpTabControl->SetTabPage(nIndex, mpTabPage.get());
    }

    // Delete tabs that are no longer used.
    for (; nIndex<=nPageCount; ++nIndex)
        mpTabControl->RemovePage(nIndex);

    mpTabPage->Hide();
}




//===== TabBarControl =========================================================

TabBarControl::TabBarControl (
    ::Window* pParentWindow,
    const ::rtl::Reference<ViewTabBar>& rpViewTabBar)
    : ::TabControl(pParentWindow),
      mpViewTabBar(rpViewTabBar)
{
}




void TabBarControl::Paint (const Rectangle& rRect)
{
    Color aOriginalFillColor (GetFillColor());
    Color aOriginalLineColor (GetLineColor());

    // Because the actual window background is transparent--to avoid
    // flickering due to multiple background paintings by this and by child
    // windows--we have to paint the background for this control explicitly:
    // the actual control is not painted over its whole bounding box.
    SetFillColor (GetSettings().GetStyleSettings().GetDialogColor());
    SetLineColor ();
    DrawRect (rRect);
    ::TabControl::Paint (rRect);

    SetFillColor (aOriginalFillColor);
    SetLineColor (aOriginalLineColor);
}




void TabBarControl::ActivatePage (void)
{
    if (mpViewTabBar->ActivatePage())
    {        
        // Call the parent so that the correct tab is highlighted.
        this->::TabControl::ActivatePage();
    }
}

} // end of namespace sd
