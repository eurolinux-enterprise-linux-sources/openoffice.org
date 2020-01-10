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

#include "precompiled_sd.hxx"

#include "BasicPaneFactory.hxx"

#include "ChildWindowPane.hxx"
#include "FrameWindowPane.hxx"
#include "FullScreenPane.hxx"

#include "framework/FrameworkHelper.hxx"
#include "ViewShellBase.hxx"
#include "PaneChildWindows.hxx"
#include "DrawController.hxx"
#include "DrawDocShell.hxx"
#include <com/sun/star/drawing/framework/XControllerManager.hpp>
#include <boost/bind.hpp>


using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;

using ::rtl::OUString;
using ::sd::framework::FrameworkHelper;

namespace {
    enum PaneId {
        CenterPaneId,
        FullScreenPaneId,
        LeftImpressPaneId,
        LeftDrawPaneId,
        RightPaneId
    };

    static const sal_Int32 gnConfigurationUpdateStartEvent(0);
    static const sal_Int32 gnConfigurationUpdateEndEvent(1);
}

namespace sd { namespace framework {


/** Store URL, XPane reference and (local) PaneId for every pane factory
    that is registered at the PaneController.
*/
class BasicPaneFactory::PaneDescriptor
{
public:
    OUString msPaneURL;
    Reference<XResource> mxPane;
    PaneId mePaneId;
    /** The mbReleased flag is set when the pane has been released.  Some
        panes are just hidden and destroyed.  When the pane is reused this
        flag is reset.
    */
    bool mbIsReleased;
    bool mbIsChildWindow;

    bool CompareURL (const OUString& rsPaneURL) { return msPaneURL.equals(rsPaneURL); }
    bool ComparePane (const Reference<XResource>& rxPane) { return mxPane==rxPane; }
};


class BasicPaneFactory::PaneContainer
    : public ::std::vector<PaneDescriptor>
{
public:
    PaneContainer (void) {}
};



Reference<XInterface> SAL_CALL BasicPaneFactory_createInstance (
    const Reference<XComponentContext>& rxContext)
{
    return Reference<XInterface>(static_cast<XWeak*>(new BasicPaneFactory(rxContext)));
}




::rtl::OUString BasicPaneFactory_getImplementationName (void) throw(RuntimeException)
{
    return ::rtl::OUString(
        RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.Draw.framework.BasicPaneFactory"));
}




Sequence<rtl::OUString> SAL_CALL BasicPaneFactory_getSupportedServiceNames (void)
    throw (RuntimeException)
{
	static const ::rtl::OUString sServiceName(
        ::rtl::OUString::createFromAscii("com.sun.star.drawing.framework.BasicPaneFactory"));
	return Sequence<rtl::OUString>(&sServiceName, 1);
}




//===== PaneFactory ===========================================================

BasicPaneFactory::BasicPaneFactory (
    const Reference<XComponentContext>& rxContext)
    : BasicPaneFactoryInterfaceBase(m_aMutex),
      mxComponentContext(rxContext),
      mxConfigurationControllerWeak(),
      mpViewShellBase(NULL),
      mpPaneContainer(new PaneContainer),
      mbFirstUpdateSeen(false),
      mpUpdateLockManager()
{
}





BasicPaneFactory::~BasicPaneFactory (void)
{
}
    



void SAL_CALL BasicPaneFactory::disposing (void)
{
    Reference<XConfigurationController> xCC (mxConfigurationControllerWeak);
    if (xCC.is())
    {
        xCC->removeResourceFactoryForReference(this);
        xCC->removeConfigurationChangeListener(this);
        mxConfigurationControllerWeak = Reference<XConfigurationController>();
    }

    for (PaneContainer::const_iterator iDescriptor = mpPaneContainer->begin();
         iDescriptor != mpPaneContainer->end();
         ++iDescriptor)
    {
        if (iDescriptor->mbIsReleased)
        {
            Reference<XComponent> xComponent (iDescriptor->mxPane, UNO_QUERY);
            if (xComponent.is())
            {
                xComponent->removeEventListener(this);
                xComponent->dispose();
            }
        }
    }
}




void SAL_CALL BasicPaneFactory::initialize (const Sequence<Any>& aArguments)
    throw (Exception, RuntimeException)
{
    if (aArguments.getLength() > 0)
    {
        try
        {
            // Get the XController from the first argument.
            Reference<frame::XController> xController (aArguments[0], UNO_QUERY_THROW);
            mxControllerWeak = xController;

            // Tunnel through the controller to obtain access to the ViewShellBase.
            try
            {
                Reference<lang::XUnoTunnel> xTunnel (xController, UNO_QUERY_THROW);
                DrawController* pController
                    = reinterpret_cast<DrawController*>(
                        (sal::static_int_cast<sal_uIntPtr>(
                            xTunnel->getSomething(DrawController::getUnoTunnelId()))));
                mpViewShellBase = pController->GetViewShellBase();
                mpUpdateLockManager = mpViewShellBase->GetUpdateLockManager();
            }
            catch(RuntimeException&)
            {}

            Reference<XControllerManager> xCM (xController, UNO_QUERY_THROW);
            Reference<XConfigurationController> xCC (xCM->getConfigurationController());
            mxConfigurationControllerWeak = xCC;

            // Add pane factories for the two left panes (one for Impress and one for
            // Draw), the center pane, and the right pane.
            if (xController.is() && xCC.is())
            {
                PaneDescriptor aDescriptor;
                aDescriptor.msPaneURL = FrameworkHelper::msCenterPaneURL;
                aDescriptor.mePaneId = CenterPaneId;
                aDescriptor.mbIsReleased = false;
                aDescriptor.mbIsChildWindow = false;
                mpPaneContainer->push_back(aDescriptor);
                xCC->addResourceFactory(aDescriptor.msPaneURL, this);

                aDescriptor.msPaneURL = FrameworkHelper::msFullScreenPaneURL;
                aDescriptor.mePaneId = FullScreenPaneId;
                mpPaneContainer->push_back(aDescriptor);
                xCC->addResourceFactory(aDescriptor.msPaneURL, this);

                aDescriptor.msPaneURL = FrameworkHelper::msLeftImpressPaneURL;
                aDescriptor.mePaneId = LeftImpressPaneId;
                aDescriptor.mbIsChildWindow = true;
                mpPaneContainer->push_back(aDescriptor);
                xCC->addResourceFactory(aDescriptor.msPaneURL, this);

                aDescriptor.msPaneURL = FrameworkHelper::msLeftDrawPaneURL;
                aDescriptor.mePaneId = LeftDrawPaneId;
                mpPaneContainer->push_back(aDescriptor);
                xCC->addResourceFactory(aDescriptor.msPaneURL, this);

                aDescriptor.msPaneURL = FrameworkHelper::msRightPaneURL;
                aDescriptor.mePaneId = RightPaneId;
                mpPaneContainer->push_back(aDescriptor);
                xCC->addResourceFactory(aDescriptor.msPaneURL, this);
            }

            // Register as configuration change listener.
            if (xCC.is())
            {
                xCC->addConfigurationChangeListener(
                    this,
                    FrameworkHelper::msConfigurationUpdateStartEvent,
                    makeAny(gnConfigurationUpdateStartEvent));
                xCC->addConfigurationChangeListener(
                    this,
                    FrameworkHelper::msConfigurationUpdateEndEvent,
                    makeAny(gnConfigurationUpdateEndEvent));
            }
        }
        catch (RuntimeException&)
        {
            Reference<XConfigurationController> xCC (mxConfigurationControllerWeak);
            if (xCC.is())
                xCC->removeResourceFactoryForReference(this);
        }
    }
}




//===== XPaneFactory ==========================================================

Reference<XResource> SAL_CALL BasicPaneFactory::createResource (
    const Reference<XResourceId>& rxPaneId)
    throw (RuntimeException)
{
    ThrowIfDisposed();

    Reference<XResource> xPane;

    // Based on the ResourceURL of the given ResourceId look up the
    // corresponding factory descriptor.
    PaneContainer::iterator iDescriptor (
        ::std::find_if (
            mpPaneContainer->begin(),
            mpPaneContainer->end(),
            ::boost::bind(&PaneDescriptor::CompareURL, _1, rxPaneId->getResourceURL())));

    if (iDescriptor != mpPaneContainer->end())
    {
        if (iDescriptor->mxPane.is())
        {
            // The pane has already been created and is still active (has
            // not yet been released).  This should not happen.
            xPane = iDescriptor->mxPane;
        }
        else
        {
            // Create a new pane.
            switch (iDescriptor->mePaneId)
            {
                case CenterPaneId:
                    xPane = CreateFrameWindowPane(rxPaneId);
                    break;

                case FullScreenPaneId:
                    xPane = CreateFullScreenPane(mxComponentContext, rxPaneId);
                    break;

                case LeftImpressPaneId:
                case LeftDrawPaneId:
                case RightPaneId:
                    xPane = CreateChildWindowPane(
                        rxPaneId,
                        *iDescriptor);
                    break;
            }
            iDescriptor->mxPane = xPane;

            // Listen for the pane being disposed.
            Reference<lang::XComponent> xComponent (xPane, UNO_QUERY);
            if (xComponent.is())
                xComponent->addEventListener(this);
        }
        iDescriptor->mbIsReleased = false;
    }
    else
    {
        // The requested pane can not be created by any of the factories
        // managed by the called BasicPaneFactory object.
        throw lang::IllegalArgumentException(
            ::rtl::OUString::createFromAscii(
                "BasicPaneFactory::createPane() called for unknown resource id"),
            NULL,
            0);
    }

    return xPane;
}





void SAL_CALL BasicPaneFactory::releaseResource (
    const Reference<XResource>& rxPane)
    throw (RuntimeException)
{
    ThrowIfDisposed();

    // Based on the given XPane reference look up the corresponding factory
    // descriptor.
    PaneContainer::iterator iDescriptor (
        ::std::find_if(
            mpPaneContainer->begin(),
            mpPaneContainer->end(),
            ::boost::bind(&PaneDescriptor::ComparePane, _1, rxPane)));

    if (iDescriptor != mpPaneContainer->end())
    {
        // The given pane was created by one of the factories.  Child
        // windows are just hidden and will be reused when requested later.
        // Other windows are disposed and their reference is reset so that
        // on the next createPane() call for the same pane type the pane is
        // created anew.
        ChildWindowPane* pChildWindowPane = dynamic_cast<ChildWindowPane*>(rxPane.get());
        if (pChildWindowPane != NULL)
        {
            iDescriptor->mbIsReleased = true;
            pChildWindowPane->Hide();
        }
        else
        {
            iDescriptor->mxPane = NULL;
            Reference<XComponent> xComponent (rxPane, UNO_QUERY);
            if (xComponent.is())
            {
                // We are disposing the pane and do not have to be informed of
                // that.
                xComponent->removeEventListener(this);
                xComponent->dispose();
            }
        }
    }
    else
    {
        // The given XPane reference is either empty or the pane was not
        // created by any of the factories managed by the called
        // BasicPaneFactory object.
        throw lang::IllegalArgumentException(
            ::rtl::OUString::createFromAscii(
                "BasicPaneFactory::releasePane() called for pane that that was not created by same factory."),
            NULL,
            0);
    }
}




//===== XConfigurationChangeListener ==========================================

void SAL_CALL BasicPaneFactory::notifyConfigurationChange (
    const ConfigurationChangeEvent& rEvent)
    throw (RuntimeException)
{
    sal_Int32 nEventType = 0;
    rEvent.UserData >>= nEventType;
    switch (nEventType)
    {
        case gnConfigurationUpdateStartEvent:
            // Lock UI updates while we are switching the views except for
            // the first time after creation.  Outherwise this leads to
            // problems after reload (missing resizes for the side panes).
            if (mbFirstUpdateSeen)
            {
                if (mpUpdateLockManager.get()!=NULL)
                {
                    //                    ::osl::Guard< ::osl::Mutex > aGuard (::osl::Mutex::getGlobalMutex());
                    //                    mpUpdateLockManager->Lock();
                }
            }
            else
                mbFirstUpdateSeen = true;
            break;

        case gnConfigurationUpdateEndEvent:
            // Unlock the update lock here when only the visibility of
            // windows but not the view shells displayed in them have
            // changed.  Otherwise the UpdateLockManager takes care of
            // unlocking at the right time.
            if (mpUpdateLockManager.get() != NULL)
            {
                ::osl::Guard< ::osl::Mutex > aGuard (::osl::Mutex::getGlobalMutex());
                //                if (mpUpdateLockManager->IsLocked())
                //                    mpUpdateLockManager->Unlock();
            }
            break;
    }
}



                
//===== lang::XEventListener ==================================================

void SAL_CALL BasicPaneFactory::disposing (
    const lang::EventObject& rEventObject)
    throw (RuntimeException)
{
    if (mxConfigurationControllerWeak == rEventObject.Source)
    {
        mxConfigurationControllerWeak = Reference<XConfigurationController>();
    }
    else
    {
        // Has one of the panes been disposed?  If so, then release the
        // reference to that pane, but not the pane descriptor.
        Reference<XResource> xPane (rEventObject.Source, UNO_QUERY);
        PaneContainer::iterator iDescriptor (
            ::std::find_if (
                mpPaneContainer->begin(),
                mpPaneContainer->end(),
                ::boost::bind(&PaneDescriptor::ComparePane, _1, xPane)));
        if (iDescriptor != mpPaneContainer->end())
        {
            iDescriptor->mxPane = NULL;
        }
    }
}




//-----------------------------------------------------------------------------

Reference<XResource> BasicPaneFactory::CreateFrameWindowPane (
    const Reference<XResourceId>& rxPaneId)
{
    Reference<XResource> xPane;
    
    if (mpViewShellBase != NULL)
    {
        xPane = new FrameWindowPane(rxPaneId, mpViewShellBase->GetViewWindow());
    }

    return xPane;
}




Reference<XResource> BasicPaneFactory::CreateFullScreenPane (
    const Reference<XComponentContext>& rxComponentContext,
    const Reference<XResourceId>& rxPaneId)
{
    Reference<XResource> xPane (
        new FullScreenPane(
            rxComponentContext,
            rxPaneId,
            mpViewShellBase->GetViewWindow()));

    return xPane;
}




Reference<XResource> BasicPaneFactory::CreateChildWindowPane (
    const Reference<XResourceId>& rxPaneId,
    const PaneDescriptor& rDescriptor)
{
    Reference<XResource> xPane;

    if (mpViewShellBase != NULL)
    {
        // Create the corresponding shell and determine the id of the child window.
        USHORT nChildWindowId = 0;
        ::std::auto_ptr<SfxShell> pShell;
        switch (rDescriptor.mePaneId)
        {
            case LeftImpressPaneId:
                pShell.reset(new LeftImpressPaneShell());
                nChildWindowId = ::sd::LeftPaneImpressChildWindow::GetChildWindowId();
                break;
                
            case LeftDrawPaneId:
                pShell.reset(new LeftDrawPaneShell());
                nChildWindowId = ::sd::LeftPaneDrawChildWindow::GetChildWindowId();
                break;
            
            case RightPaneId:
                pShell.reset(new RightPaneShell());
                nChildWindowId = ::sd::RightPaneChildWindow::GetChildWindowId();
                break;

            default:
                break;
        }

        // With shell and child window id create the ChildWindowPane
        // wrapper.
        if (pShell.get() != NULL)
        {
            xPane = new ChildWindowPane(
                rxPaneId,
                nChildWindowId,
                *mpViewShellBase,
                pShell);
        }
    }

    return xPane;
}




bool BasicPaneFactory::IsBoundToChildWindow (const Reference<XResourceId>& rxResourceId) const
{
    if ( ! rxResourceId.is())
        return false;

    Reference<XResourceId> xAnchorId (rxResourceId->getAnchor());
    if ( ! xAnchorId.is())
        return false;

    const OUString sAnchorURL (xAnchorId->getResourceURL());
    if (sAnchorURL == FrameworkHelper::msLeftImpressPaneURL)
        return true;
    else if (sAnchorURL == FrameworkHelper::msLeftDrawPaneURL)
        return true;
    else if (sAnchorURL == FrameworkHelper::msRightPaneURL)
        return true;
    else
        return false;
}




void BasicPaneFactory::ThrowIfDisposed (void) const
    throw (lang::DisposedException)
{
	if (rBHelper.bDisposed || rBHelper.bInDispose)
	{
        throw lang::DisposedException (
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                "BasicPaneFactory object has already been disposed")),
            const_cast<uno::XWeak*>(static_cast<const uno::XWeak*>(this)));
    }
}


} } // end of namespace sd::framework
