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
#include "precompiled_sfx2.hxx"
#ifndef GCC
#endif

#include <sfx2/topfrm.hxx>
#include <sfx2/signaturestate.hxx>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/frame/XFrame.hpp>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif
#include <com/sun/star/frame/XFramesSupplier.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/util/CloseVetoException.hpp>
#ifndef _TOOLKIT_UNOHLP_HXX
#include <toolkit/helper/vclunohelper.hxx>
#endif
#ifndef _UNO_COM_SUN_STAR_AWT_POSSIZE_HPP_
#include <com/sun/star/awt/PosSize.hpp>
#endif
#include <com/sun/star/container/XIndexAccess.hpp>
#ifndef _COM_SUN_STAR_CONTAINER_XPROPERTYSET_HPP_
#include <com/sun/star/beans/XPropertySet.hpp>
#endif
#include <com/sun/star/frame/XLayoutManager.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XMaterialHolder.hpp>
#include <com/sun/star/awt/XWindow2.hpp>
#include <vcl/menu.hxx>
#include <svtools/rectitem.hxx>
#include <svtools/intitem.hxx>
#include <svtools/eitem.hxx>
#include <svtools/stritem.hxx>
#include <svtools/asynclink.hxx>
#include <svtools/sfxecode.hxx>
#include <vcl/dialog.hxx>
#include <svtools/urihelper.hxx>
#include <svtools/moduleoptions.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/bootstrap.hxx>

#include <sfxresid.hxx>

// wg. pTopFrames
#include "appdata.hxx"
#include <sfx2/app.hxx>
#include <sfx2/sfx.hrc>
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/request.hxx>
#include <sfx2/objitem.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/msg.hxx>
#include "objshimp.hxx"
#include "workwin.hxx"
#include "sfxtypes.hxx"
#include "splitwin.hxx"
#include "arrdecl.hxx"
#include "sfxhelp.hxx"
#include <sfx2/fcontnr.hxx>
#include <sfx2/docfac.hxx>
#include "statcach.hxx"
#include <sfx2/event.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::beans;

//------------------------------------------------------------------------

#define SfxTopViewFrame
#include "sfxslots.hxx"

DBG_NAME(SfxTopViewFrame)

#include <comphelper/sequenceashashmap.hxx>
static ::rtl::OUString GetModuleName_Impl( const ::rtl::OUString& sDocService )
{
    uno::Reference< container::XNameAccess > xMM( ::comphelper::getProcessServiceFactory()->createInstance(::rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager")), uno::UNO_QUERY );
    ::rtl::OUString sVar;
    if ( !xMM.is() )
        return sVar;

    try
    {
        ::comphelper::SequenceAsHashMap aAnalyzer( xMM->getByName(sDocService) );
        sVar = aAnalyzer.getUnpackedValueOrDefault( ::rtl::OUString::createFromAscii("ooSetupFactoryUIName"), ::rtl::OUString() );
    }
    catch( uno::Exception& )
    {
        sVar = ::rtl::OUString();
    }

    return sVar;
}

class SfxTopFrame_Impl
{
public:
    Window*             pWindow;        // maybe external
    BOOL                bHidden;
    BOOL                bLockResize;
    BOOL                bMenuBarOn;
};

class SfxTopWindow_Impl : public Window
{
public:
	SfxTopFrame*   		pFrame;

    SfxTopWindow_Impl( SfxTopFrame* pF );
//        : Window( pF->pImp->pWindow, WB_CLIPCHILDREN | WB_NODIALOGCONTROL | WB_3DLOOK )
//        , pFrame( pF )
//    { SetBackground(); }
    ~SfxTopWindow_Impl( );

    virtual void        DataChanged( const DataChangedEvent& rDCEvt );
	virtual void		StateChanged( StateChangedType nStateChange );
	virtual long 		PreNotify( NotifyEvent& rNEvt );
	virtual long		Notify( NotifyEvent& rEvt );
	virtual void        Resize();
    virtual void        GetFocus();
	void				DoResize();
	DECL_LINK(			CloserHdl, void* );
};

SfxTopWindow_Impl::SfxTopWindow_Impl( SfxTopFrame* pF )
        : Window( pF->pImp->pWindow, WB_BORDER | WB_CLIPCHILDREN | WB_NODIALOGCONTROL | WB_3DLOOK )
        , pFrame( pF )
{
}

SfxTopWindow_Impl::~SfxTopWindow_Impl( )
{
}

void SfxTopWindow_Impl::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );
    SfxWorkWindow *pWorkWin = pFrame->GetWorkWindow_Impl();
    if ( pWorkWin )
        pWorkWin->DataChanged_Impl( rDCEvt );
}

long SfxTopWindow_Impl::Notify( NotifyEvent& rNEvt )
{
	if ( pFrame->IsClosing_Impl() || !pFrame->GetFrameInterface().is() )
		return sal_False;

    SfxViewFrame* pView = pFrame->GetCurrentViewFrame();
    if ( !pView || !pView->GetObjectShell() )
        return Window::Notify( rNEvt );

    if ( rNEvt.GetType() == EVENT_GETFOCUS )
    {
        if ( pView->GetViewShell() && !pView->GetViewShell()->GetUIActiveIPClient_Impl() && !pFrame->IsInPlace() )
        {
            DBG_TRACE("SfxTopFrame: GotFocus");
            pView->MakeActive_Impl( FALSE );
        }

        // TODO/LATER: do we still need this code?
        Window* pWindow = rNEvt.GetWindow();
        ULONG nHelpId  = 0;
        while ( !nHelpId && pWindow )
        {
            nHelpId = pWindow->GetHelpId();
            pWindow = pWindow->GetParent();
        }

        if ( nHelpId )
            SfxHelp::OpenHelpAgent( pFrame, nHelpId );

		// if focus was on an external window, the clipboard content might have been changed
		pView->GetBindings().Invalidate( SID_PASTE );
		pView->GetBindings().Invalidate( SID_PASTE_SPECIAL );
		return sal_True;
	}
    else if( rNEvt.GetType() == EVENT_KEYINPUT )
	{
        if ( pView->GetViewShell()->KeyInput( *rNEvt.GetKeyEvent() ) )
            return TRUE;
	}
    else if ( rNEvt.GetType() == EVENT_EXECUTEDIALOG /*|| rNEvt.GetType() == EVENT_INPUTDISABLE*/ )
	{
        pView->SetModalMode( sal_True );
		return sal_True;
	}
	else if ( rNEvt.GetType() == EVENT_ENDEXECUTEDIALOG /*|| rNEvt.GetType() == EVENT_INPUTENABLE*/ )
	{
		//EnableInput( sal_True, sal_True );
        pView->SetModalMode( sal_False );
		return sal_True;
	}

    return Window::Notify( rNEvt );
}

long SfxTopWindow_Impl::PreNotify( NotifyEvent& rNEvt )
{
	USHORT nType = rNEvt.GetType();
	if ( nType == EVENT_KEYINPUT || nType == EVENT_KEYUP )
    {
		SfxViewFrame* pView = pFrame->GetCurrentViewFrame();
		SfxViewShell* pShell = pView ? pView->GetViewShell() : NULL;
		if ( pShell && pShell->HasKeyListeners_Impl() && pShell->HandleNotifyEvent_Impl( rNEvt ) )
			return sal_True;
	}
	else if ( nType == EVENT_MOUSEBUTTONUP || nType == EVENT_MOUSEBUTTONDOWN )
    {
		Window* pWindow = rNEvt.GetWindow();
		SfxViewFrame* pView = pFrame->GetCurrentViewFrame();
		SfxViewShell* pShell = pView ? pView->GetViewShell() : NULL;
		if ( pShell )
			if ( pWindow == pShell->GetWindow() || pShell->GetWindow()->IsChild( pWindow ) )
				if ( pShell->HasMouseClickListeners_Impl() && pShell->HandleNotifyEvent_Impl( rNEvt ) )
					return sal_True;
	}

	if ( nType == EVENT_MOUSEBUTTONDOWN )
    {
        Window* pWindow = rNEvt.GetWindow();
        const MouseEvent* pMEvent = rNEvt.GetMouseEvent();
        Point aPos = pWindow->OutputToScreenPixel( pMEvent->GetPosPixel() );
        SfxWorkWindow *pWorkWin = pFrame->GetWorkWindow_Impl();
        if ( pWorkWin )
            pWorkWin->EndAutoShow_Impl( aPos );
    }

	return Window::PreNotify( rNEvt );
}

void SfxTopWindow_Impl::GetFocus()
{
    if ( pFrame && !pFrame->IsClosing_Impl() && pFrame->GetCurrentViewFrame() && pFrame->GetFrameInterface().is() )
        pFrame->GetCurrentViewFrame()->MakeActive_Impl( TRUE );
}

void SfxTopWindow_Impl::Resize()
{
    if ( IsReallyVisible() || IsReallyShown() || GetOutputSizePixel().Width() )
		DoResize();
}

void SfxTopWindow_Impl::StateChanged( StateChangedType nStateChange )
{
	if ( nStateChange == STATE_CHANGE_INITSHOW )
	{
        pFrame->pImp->bHidden = FALSE;
        if ( pFrame->IsInPlace() )
            // TODO/MBA: workaround for bug in LayoutManager: the final resize does not get through because the
            // LayoutManager works asynchronously and between resize and time execution the DockingAcceptor was exchanged so that
            // the resize event never is sent to the component
            SetSizePixel( GetParent()->GetOutputSizePixel() );

		DoResize();
        SfxViewFrame* pView = pFrame->GetCurrentViewFrame();
        if ( pView )
            pView->GetBindings().GetWorkWindow_Impl()->ShowChilds_Impl();
	}

    Window::StateChanged( nStateChange );
}

void SfxTopWindow_Impl::DoResize()
{
    if ( !pFrame->pImp->bLockResize )
        pFrame->Resize();
}

class StopButtonTimer_Impl : public Timer
{
	BOOL bState;
    SfxViewFrame* pFrame;
protected:
	virtual void Timeout();
public:
    StopButtonTimer_Impl( SfxViewFrame*);
	void SetButtonState( BOOL bStateP );
	BOOL GetButtonState() const { return bState; }
};

StopButtonTimer_Impl::StopButtonTimer_Impl( SfxViewFrame*p)
	: bState( FALSE )
    , pFrame( p )
{
	SetTimeout( 200 );
}

void StopButtonTimer_Impl::SetButtonState( BOOL bStateP )
{
	if( bStateP )
	{
		bState = TRUE;
		Stop();
	}
	else if( bState )
		Start();
}

void StopButtonTimer_Impl::Timeout()
{
	bState = FALSE;
    pFrame->GetBindings().Invalidate( SID_BROWSE_STOP );
}

class SfxTopViewWin_Impl : public Window
{
friend class SfxInternalFrame;

	BOOL				bActive;
    SfxTopViewFrame*    pFrame;

public:
                        SfxTopViewWin_Impl( SfxTopViewFrame* p,
								Window *pParent, WinBits nBits=0 ) :
							Window( pParent, nBits | WB_BORDER | WB_CLIPCHILDREN ),
							bActive( FALSE ),
							pFrame( p )
						{
                            p->GetFrame()->GetWindow().SetBorderStyle( WINDOW_BORDER_NOBORDER );
						}

	virtual void		Resize();
	virtual void		StateChanged( StateChangedType nStateChange );
};

//--------------------------------------------------------------------
void SfxTopViewWin_Impl::StateChanged( StateChangedType nStateChange )
{
	if ( nStateChange == STATE_CHANGE_INITSHOW )
    {
        SfxObjectShell* pDoc = pFrame->GetObjectShell();
        if ( pDoc && !pFrame->IsVisible_Impl() )
            pFrame->Show();

        pFrame->Resize();
    }
	else
        Window::StateChanged( nStateChange );
}

void SfxTopViewWin_Impl::Resize()
{
    if ( IsReallyVisible() || IsReallyShown() || GetOutputSizePixel().Width() )
        pFrame->Resize();
}

class SfxTopViewFrame_Impl
{
public:
    sal_Bool            bActive;
    Window*             pWindow;
    String          	aFactoryName;
    StopButtonTimer_Impl* pStopButtonTimer;

						SfxTopViewFrame_Impl()
							: bActive( sal_False )
                            , pWindow( 0 )
                            , pStopButtonTimer( 0 )
                        {}
};

static svtools::AsynchronLink* pPendingCloser = 0;

static String _getTabString()
{
    String result;

    Reference < XMaterialHolder > xHolder(
        ::comphelper::getProcessServiceFactory()->createInstance(
        DEFINE_CONST_UNICODE("com.sun.star.tab.tabreg") ), UNO_QUERY );
    if (xHolder.is())
    {
        rtl::OUString aTabString;
        Sequence< NamedValue > sMaterial;
        if (xHolder->getMaterial() >>= sMaterial) {
            for (int i=0; i < sMaterial.getLength(); i++) {
                if ((sMaterial[i].Name.equalsAscii("title")) &&
                    (sMaterial[i].Value >>= aTabString))
                {
                    result += ' ';
                    result += String(aTabString);
                }
            }
        }
    }
    return result;
}

SfxTopFrame* SfxTopFrame::Create( SfxObjectShell* pDoc, USHORT nViewId, BOOL bHidden, const SfxItemSet* pSet )
{
    Reference < XFrame > xDesktop ( ::comphelper::getProcessServiceFactory()->createInstance( DEFINE_CONST_UNICODE("com.sun.star.frame.Desktop") ), UNO_QUERY );
	SfxTopFrame *pFrame = NULL;
	BOOL bNewView = FALSE;
	if ( pSet )
	{
    	SFX_ITEMSET_ARG( pSet, pItem, SfxBoolItem, SID_OPEN_NEW_VIEW, sal_False );
		bNewView = pItem && pItem->GetValue();
	}

	if ( pDoc && !bHidden && !bNewView )
	{
	    URL aTargetURL;
	    aTargetURL.Complete = pDoc->GetMedium()->GetURLObject().GetMainURL( INetURLObject::NO_DECODE );

        BOOL bIsBasic = FALSE;
		if ( !aTargetURL.Complete.getLength() )
		{
            String sFactory = String::CreateFromAscii(pDoc->GetFactory().GetShortName());
            bIsBasic = (sFactory.CompareIgnoreCaseToAscii("sbasic")==COMPARE_EQUAL);

            if (!bIsBasic)
            {
                String aURL = String::CreateFromAscii("private:factory/");
                aURL += sFactory;
                aTargetURL.Complete = aURL;
            }
		}

        if (bIsBasic)
        {
            Reference < XFramesSupplier > xSupplier( xDesktop, UNO_QUERY );
            if (xSupplier.is())
            {
                Reference < XIndexAccess > xContainer(xSupplier->getFrames(), UNO_QUERY);
                if (xContainer.is())
                {
                    Reference< ::com::sun::star::frame::XModuleManager > xCheck(::comphelper::getProcessServiceFactory()->createInstance( rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager" )), UNO_QUERY);
                    sal_Int32 nCount = xContainer->getCount();
                    for (sal_Int32 i=0; i<nCount; ++i)
                    {
                        try
                        {
                            Reference < XFrame > xFrame;
                            if (!(xContainer->getByIndex(i) >>= xFrame) || !xFrame.is())
                                continue;
                            ::rtl::OUString sModule = xCheck->identify(xFrame);
                            if (sModule.equalsAscii("com.sun.star.frame.StartModule"))
                            {
                                pFrame = Create(xFrame);
                                break;
                            }
                        }
                        catch(const Exception&) {}
                    }
                }
            }
        }
        else
        {
            Reference < XURLTransformer > xTrans( ::comphelper::getProcessServiceFactory()->createInstance( rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer" )), UNO_QUERY );
            xTrans->parseStrict( aTargetURL );

            Reference < ::com::sun::star::frame::XDispatchProvider > xProv( xDesktop, UNO_QUERY );
            Reference < ::com::sun::star::frame::XDispatch > xDisp;
            if ( xProv.is() )
            {
                Sequence < ::com::sun::star::beans::PropertyValue > aSeq(1);
                aSeq[0].Name = ::rtl::OUString::createFromAscii("Model");
                aSeq[0].Value <<= pDoc->GetModel();
                ::rtl::OUString aTargetFrame( ::rtl::OUString::createFromAscii("_default") );
                xDisp = xProv->queryDispatch( aTargetURL, aTargetFrame , 0 );
                if ( xDisp.is() )
                    xDisp->dispatch( aTargetURL, aSeq );
            }

            SfxFrameArr_Impl& rArr = *SFX_APP()->Get_Impl()->pTopFrames;
            for( USHORT nPos = rArr.Count(); nPos--; )
            {
                SfxTopFrame *pF = (SfxTopFrame*) rArr[ nPos ];
                if ( pF->GetCurrentDocument() == pDoc )
                {
                    pFrame = pF;
                    break;
                }
            }
        }
	}

	if ( !pFrame  )
	{
	    Reference < XFrame > xFrame = xDesktop->findFrame( DEFINE_CONST_UNICODE("_blank"), 0 );
	    pFrame = Create( xFrame );
	}

    pFrame->pImp->bHidden = bHidden;
    Window* pWindow = pFrame->GetTopWindow_Impl();
    if ( pWindow && pDoc )
    {
        ::rtl::OUString aDocServiceName( pDoc->GetFactory().GetDocumentServiceName() );
        ::rtl::OUString aProductName;
        ::utl::ConfigManager::GetDirectConfigProperty(::utl::ConfigManager::PRODUCTNAME) >>= aProductName;
        String aTitle = pDoc->GetTitle( SFX_TITLE_DETECT );
        aTitle += String::CreateFromAscii( " - " );
        aTitle += String(aProductName);
        aTitle += ' ';
        aTitle += String( GetModuleName_Impl( aDocServiceName ) );
#ifndef PRODUCT
		::rtl::OUString	aDefault;
		aTitle += DEFINE_CONST_UNICODE(" [");
		String aVerId( utl::Bootstrap::getBuildIdData( aDefault ));
		aTitle += aVerId;
		aTitle += ']';
#endif

        // append TAB string if available
        aTitle += _getTabString();

        /* AS_TITLE
        pWindow->SetText( aTitle );
        */
        
        /* AS_ICON
        if( pWindow->GetType() == WINDOW_WORKWINDOW )
        {
            SvtModuleOptions::EFactory eFactory;
            if( SvtModuleOptions::ClassifyFactoryByName( aDocServiceName, eFactory ) )
            {
                WorkWindow* pWorkWindow = (WorkWindow*)pWindow;
                pWorkWindow->SetIcon( (sal_uInt16) SvtModuleOptions().GetFactoryIcon( eFactory ) );
            }
        }
        */
    }

    pFrame->SetItemSet_Impl( pSet );
    if ( pDoc && pDoc != pFrame->GetCurrentDocument() )
    {
        if ( nViewId )
            pDoc->GetMedium()->GetItemSet()->Put( SfxUInt16Item( SID_VIEW_ID, nViewId ) );
        pFrame->InsertDocument( pDoc );
        if ( pWindow && !bHidden )
            pWindow->Show();
    }

    return pFrame;
}

SfxTopFrame* SfxTopFrame::Create( SfxObjectShell* pDoc, Window* pWindow, USHORT nViewId, BOOL bHidden, const SfxItemSet* pSet )
{
    Reference < ::com::sun::star::lang::XMultiServiceFactory > xFact( ::comphelper::getProcessServiceFactory() );
    Reference < XFramesSupplier > xDesktop ( xFact->createInstance( DEFINE_CONST_UNICODE("com.sun.star.frame.Desktop") ), UNO_QUERY );
    Reference < XFrame > xFrame( xFact->createInstance( DEFINE_CONST_UNICODE("com.sun.star.frame.Frame") ), UNO_QUERY );

    xFrame->initialize( VCLUnoHelper::GetInterface ( pWindow ) );
    if ( xDesktop.is() )
        xDesktop->getFrames()->append( xFrame );

    uno::Reference< awt::XWindow2 > xWin( VCLUnoHelper::GetInterface ( pWindow ), uno::UNO_QUERY );
	if ( xWin.is() && xWin->isActive() )
        xFrame->activate();

    SfxTopFrame* pFrame = new SfxTopFrame( pWindow );
    pFrame->SetFrameInterface_Impl( xFrame );
    pFrame->pImp->bHidden = bHidden;

    pFrame->SetItemSet_Impl( pSet );
    if ( pDoc )
    {
        if ( nViewId )
            pDoc->GetMedium()->GetItemSet()->Put( SfxUInt16Item( SID_VIEW_ID, nViewId ) );
        pFrame->InsertDocument( pDoc );
    }

    return pFrame;
}

SfxTopFrame* SfxTopFrame::Create( Reference < XFrame > xFrame )
{
    // create a new TopFrame to an external XFrame object ( wrap controller )
    DBG_ASSERT( xFrame.is(), "Wrong parameter!" );

    Window* pWindow = VCLUnoHelper::GetWindow( xFrame->getContainerWindow() );
    SfxTopFrame* pFrame = new SfxTopFrame( pWindow );
    pFrame->SetFrameInterface_Impl( xFrame );
    return pFrame;
}

SfxTopFrame::SfxTopFrame( Window* pExternal, sal_Bool bHidden )
	: SfxFrame( NULL )
	, pWindow( NULL )
{
    pImp = new SfxTopFrame_Impl;
    pImp->bHidden = bHidden;
    pImp->bLockResize = FALSE;
    pImp->bMenuBarOn = TRUE;
	InsertTopFrame_Impl( this );
    if ( pExternal )
    {
        pImp->pWindow = pExternal;
    }
    else
    {
        DBG_ERROR( "TopFrame without window created!" );
/*
        pImp->pWindow = new SfxTopFrameWindow_Impl( this );
        pImp->pWindow->SetActivateMode( ACTIVATE_MODE_GRABFOCUS );
        pImp->pWindow->SetPosSizePixel( Point( 20,20 ), Size( 800,600 ) );
        if ( GetFrameInterface().is() )
            GetFrameInterface()->initialize( VCLUnoHelper::GetInterface( pImp->pWindow ) );
        pImp->pWindow->Show();
 */
    }

    pWindow = new SfxTopWindow_Impl( this );
/** AS:
    Hide this window till the component was realy loaded. Otherwhise it overpaint e.g. the old component hardly
    and produce repaint errors.
    pWindow->Show();
  */
}

SfxTopFrame::~SfxTopFrame()
{
	RemoveTopFrame_Impl( this );
    DELETEZ( pWindow );
    delete pImp;
}

void SfxTopFrame::SetPresentationMode( BOOL bSet )
{
    if ( GetCurrentViewFrame() )
        GetCurrentViewFrame()->GetWindow().SetBorderStyle( bSet ? WINDOW_BORDER_NOBORDER : WINDOW_BORDER_NORMAL );

    Reference< com::sun::star::beans::XPropertySet > xPropSet( GetFrameInterface(), UNO_QUERY );
    Reference< ::com::sun::star::frame::XLayoutManager > xLayoutManager;

	if ( xPropSet.is() )
	{
		Any aValue = xPropSet->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "LayoutManager" )));
		aValue >>= xLayoutManager;
    }

    if ( xLayoutManager.is() )
        xLayoutManager->setVisible( !bSet ); // we don't want to have ui in presentation mode

    SetMenuBarOn_Impl( !bSet );
    if ( GetWorkWindow_Impl() )
        GetWorkWindow_Impl()->SetDockingAllowed( !bSet );
    if ( GetCurrentViewFrame() )
        GetCurrentViewFrame()->GetDispatcher()->Update_Impl( TRUE );
}

SystemWindow*
SfxTopFrame::GetSystemWindow() const
{
	return GetTopWindow_Impl();
}

SystemWindow* SfxTopFrame::GetTopWindow_Impl() const
{
    if ( pImp->pWindow->IsSystemWindow() )
        return (SystemWindow*) pImp->pWindow;
    else
        return NULL;
}

Window& SfxTopFrame::GetWindow() const
{
    return *pWindow;
}

sal_Bool SfxTopFrame::Close()
{
	delete this;
	return sal_True;
}

void SfxTopFrame::LockResize_Impl( BOOL bLock )
{
    pImp->bLockResize = bLock;
}

IMPL_LINK( SfxTopWindow_Impl, CloserHdl, void*, EMPTYARG )
{
	if ( pFrame && !pFrame->PrepareClose_Impl( TRUE ) )
		return 0L;

	if ( pFrame )
		pFrame->GetCurrentViewFrame()->GetBindings().Execute( SID_CLOSEWIN, 0, 0, SFX_CALLMODE_ASYNCHRON );
	return 0L;
}

void SfxTopFrame::SetMenuBarOn_Impl( BOOL bOn )
{
    pImp->bMenuBarOn = bOn;

    Reference< com::sun::star::beans::XPropertySet > xPropSet( GetFrameInterface(), UNO_QUERY );
    Reference< ::com::sun::star::frame::XLayoutManager > xLayoutManager;

	if ( xPropSet.is() )
	{
		Any aValue = xPropSet->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "LayoutManager" )));
		aValue >>= xLayoutManager;
    }

    if ( xLayoutManager.is() )
    {
        rtl::OUString aMenuBarURL( RTL_CONSTASCII_USTRINGPARAM( "private:resource/menubar/menubar" ));

        if ( bOn )
            xLayoutManager->showElement( aMenuBarURL );
        else
            xLayoutManager->hideElement( aMenuBarURL );
    }
}

BOOL SfxTopFrame::IsMenuBarOn_Impl() const
{
    return pImp->bMenuBarOn;
}

String SfxTopFrame::GetWindowData()
{
	String aActWinData;
	char cToken = ',';

	SfxViewFrame *pActFrame = SfxViewFrame::Current();
	SfxViewFrame *pFrame = GetCurrentViewFrame();
	const sal_Bool bActWin = ( pActFrame->GetTopViewFrame() == pFrame );

	// ::com::sun::star::sdbcx::User-Daten der ViewShell
	String aUserData;
	pFrame->GetViewShell()->WriteUserData(aUserData);

	// assemble ini-data
	String aWinData;
	aWinData += String::CreateFromInt32( pFrame->GetCurViewId() );
	aWinData += cToken;

    aWinData += '1';                    // former attribute "isfloating"
	aWinData += cToken;

	// aktives kennzeichnen
	aWinData += cToken;
	aWinData += bActWin ? '1' : '0';

	aWinData += cToken;
	aWinData += aUserData;

	return aWinData;
}

sal_Bool SfxTopFrame::InsertDocument( SfxObjectShell* pDoc )
/* [Beschreibung]
 */
{
	// Spezielle Bedingungen testen: nicht im ModalMode!
	if ( !SfxFrame::InsertDocument( pDoc ) )
		return sal_False;

	SfxObjectShell *pOld = GetCurrentDocument();

	// Position und Groesse testen
	// Wenn diese schon gesetzt sind, soll offensichtlich nicht noch
	// LoadWindows_Impl aufgerufen werden ( z.B. weil dieses ein CreateFrame()
	// an einer Task aufgerufen hat! )
	const SfxItemSet* pSet = GetItemSet_Impl();
	if ( !pSet )
		pSet = pDoc->GetMedium()->GetItemSet();
	SetItemSet_Impl(0);

    // Position und Gr"o\se
	SFX_ITEMSET_ARG(
		pSet, pAreaItem, SfxRectangleItem, SID_VIEW_POS_SIZE, sal_False );
    // View-Id
	SFX_ITEMSET_ARG(
		pSet, pViewIdItem, SfxUInt16Item, SID_VIEW_ID, sal_False );
	// Zoom
	SFX_ITEMSET_ARG(
		pSet, pModeItem, SfxUInt16Item, SID_VIEW_ZOOM_MODE, sal_False );
	// Hidden
	SFX_ITEMSET_ARG(
		pSet, pHidItem, SfxBoolItem,	SID_HIDDEN, sal_False);
	// ViewDaten
	SFX_ITEMSET_ARG(
		pSet, pViewDataItem, SfxStringItem, SID_USER_DATA, sal_False );
	// ViewOnly
	SFX_ITEMSET_ARG(
		pSet, pEditItem, SfxBoolItem, SID_VIEWONLY, sal_False);
    // InPlace (Hack)
    SFX_ITEMSET_ARG(
        pSet, pPluginItem, SfxUInt16Item, SID_PLUGIN_MODE, sal_False );

    // Plugin (external InPlace)
    SFX_ITEMSET_ARG(
        pSet, pPluginMode, SfxUInt16Item, SID_PLUGIN_MODE, sal_False);
    // Jump (GotoBookmark)
    SFX_ITEMSET_ARG(
        pSet, pJumpItem, SfxStringItem, SID_JUMPMARK, sal_False);

	if ( pEditItem  && pEditItem->GetValue() )
		SetMenuBarOn_Impl( FALSE );

    if ( pHidItem )
        pImp->bHidden = pHidItem->GetValue();

    if( !pImp->bHidden )
        pDoc->OwnerLock( sal_True );

    // Wenn z.B. eine Fenstergr"o\se gesetzt wurde, soll keine Fensterinformation
    // aus den Dokument geladen werden, z.B. weil InsertDocument seinerseits
    // aus LoadWindows_Impl aufgerufen wurde!
    if ( !pJumpItem && !pPluginMode && pDoc && !pAreaItem && !pViewIdItem && !pModeItem &&
            pDoc->LoadWindows_Impl( this ) )
    {
        if ( GetCurrentDocument() != pDoc )
            // something went wrong during insertion
            return sal_False;

        if( !pImp->bHidden )
            pDoc->OwnerLock( sal_False );

        return sal_True;
    }

	if ( pDoc )
	{
		UpdateHistory( pDoc );
		UpdateDescriptor( pDoc );
	}

	SetFrameType_Impl( GetFrameType() & ~SFXFRAME_FRAMESET );
	sal_Bool bBrowsing = sal_True;
	SfxViewFrame *pFrame = GetCurrentViewFrame();
    if ( pFrame )
	{
        sal_Bool bChildActivated = sal_False;
		if ( pFrame->GetActiveChildFrame_Impl() && pFrame->GetActiveChildFrame_Impl() == SfxViewFrame::Current() )
		{
//            ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFramesSupplier >  xFrames( GetFrameInterface(), ::com::sun::star::uno::UNO_QUERY );
//            if ( xFrames.is() )
//                xFrames->setActiveFrame( ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > () );
            pFrame->SetActiveChildFrame_Impl(0);
            SfxViewFrame::SetViewFrame( pFrame );
            bChildActivated = sal_True;
		}

		if ( pFrame->GetObjectShell() )
        {
			pFrame->ReleaseObjectShell_Impl( sal_False );
		}

		if ( pViewIdItem )
			pFrame->SetViewData_Impl( pViewIdItem->GetValue(), String() );
		if ( pDoc )
			pFrame->SetObjectShell_Impl( *pDoc );
	}
	else
	{
        // 1: internal embedded object
        // 2: external embedded object
        // 3: OLE server
        if ( pPluginItem && pPluginItem->GetValue() != 2 )
            SetInPlace_Impl( TRUE );

		bBrowsing = sal_False;
		pFrame = new SfxTopViewFrame( this, pDoc, pViewIdItem ? pViewIdItem->GetValue() : 0 );
        if ( !pFrame->GetViewShell() )
            return sal_False;

        if ( pPluginItem && pPluginItem->GetValue() == 1 )
        {
            pFrame->ForceOuterResize_Impl( FALSE );
            pFrame->GetBindings().HidePopups(TRUE);

            // MBA: layoutmanager of inplace frame starts locked and invisible
            GetWorkWindow_Impl()->MakeVisible_Impl( FALSE );
            GetWorkWindow_Impl()->Lock_Impl( TRUE );

			GetWindow().SetBorderStyle( WINDOW_BORDER_NOBORDER );
			if ( GetCurrentViewFrame() )
				GetCurrentViewFrame()->GetWindow().SetBorderStyle( WINDOW_BORDER_NOBORDER );
        }
	}

	String aMark;
    SFX_ITEMSET_ARG( pSet, pMarkItem, SfxStringItem, SID_JUMPMARK, FALSE );
    if ( pMarkItem )
		aMark = pMarkItem->GetValue();

	if ( pDoc->Get_Impl()->nLoadedFlags & SFX_LOADED_MAINDOCUMENT )
	{
    	if ( pViewDataItem )
			pFrame->GetViewShell()->ReadUserData( pViewDataItem->GetValue(), sal_True );
		else if( aMark.Len() )
			GetCurrentViewFrame()->GetViewShell()->JumpToMark( aMark );
	}
	else
	{
		// Daten setzen, die in FinishedLoading ausgewertet werden
		MarkData_Impl*& rpMark = pDoc->Get_Impl()->pMarkData;
		if (!rpMark)
			rpMark = new MarkData_Impl;
		rpMark->pFrame = GetCurrentViewFrame();
		if ( pViewDataItem )
			rpMark->aUserData = pViewDataItem->GetValue();
		else
			rpMark->aMark = aMark;
	}

	// Position und Groesse setzen
	//sal_uInt16 nWinMode = pModeItem ? pModeItem->GetValue() : 1;
	if ( pAreaItem && !pOld )
	{
        Window *pWin = pImp->pWindow;

		// Groesse setzen
		const Rectangle aWinRect( pAreaItem->GetValue() );
        const Size aAppWindow( pImp->pWindow->GetDesktopRectPixel().GetSize() );
		Point aPos( aWinRect.TopLeft() );
		Size aSz(aWinRect.GetSize());
		if ( aSz.Width() && aSz.Height() )
		{
			aPos.X() = Min(aPos.X(),
							long(aAppWindow.Width() - aSz.Width() + aSz.Width() / 2) );
			aPos.Y() = Min(aPos.Y(),
							long( aAppWindow.Height() - aSz.Height() + aSz.Height() / 2) );
			if ( aPos.X() + aSz.Width() <
					aAppWindow.Width() + aSz.Width() / 2 &&
					aPos.Y() + aSz.Height() <
					aAppWindow.Height() + aSz.Height() / 2 )
			{
				pWin->SetPosPixel( aPos );
				pWin->SetOutputSizePixel( aSz );
			}
		}
	}

    if ( !pImp->bHidden )
	{
        if ( pDoc->IsHelpDocument() || (pPluginItem && pPluginItem->GetValue() == 2) )
            pFrame->GetDispatcher()->HideUI( TRUE );
        else
            pFrame->GetDispatcher()->HideUI( FALSE );

        if ( IsInPlace() )
            pFrame->LockAdjustPosSizePixel();

        if ( pPluginMode && pPluginMode->GetValue() == 3)
            GetWorkWindow_Impl()->SetInternalDockingAllowed(FALSE);

        if ( !IsInPlace() )
			pFrame->GetDispatcher()->Update_Impl();
		pFrame->Show();
        GetWindow().Show();
        if ( !IsInPlace() || (pPluginItem && pPluginItem->GetValue() == 3) )
            pFrame->MakeActive_Impl( GetFrameInterface()->isActive() );
		pDoc->OwnerLock( sal_False );

        // Dont show container window! Its done by framework or directly
        // by SfxTopFrame::Create() or SfxViewFrame::ExecView_Impl() ...

        if ( IsInPlace() )
        {
            pFrame->UnlockAdjustPosSizePixel();
            // force resize for OLE server to fix layout problems of writer and math
            // see i53651
            if ( pPluginItem && pPluginItem->GetValue() == 3 )
                pFrame->Resize(TRUE);
        }
	}
    else
    {
        DBG_ASSERT( !IsInPlace() && !pPluginMode && !pPluginItem, "Special modes not compatible with hidden mode!" );
        GetWindow().Show();
    }

	// Jetzt UpdateTitle, hidden TopFrames haben sonst keinen Namen!
	pFrame->UpdateTitle();

    if ( !IsInPlace() )
    {
        if ( pFrame->GetViewShell()->UseObjectSize() )
        {
            GetCurrentViewFrame()->UnlockAdjustPosSizePixel();
            GetCurrentViewFrame()->Resize(TRUE);
            GetCurrentViewFrame()->ForceInnerResize_Impl( FALSE );
            GetCurrentViewFrame()->Resize(TRUE);
        }
        else
            GetCurrentViewFrame()->Resize(TRUE);
    }

    SFX_APP()->NotifyEvent( SfxEventHint(SFX_EVENT_VIEWCREATED, GlobalEventConfig::GetEventName( STR_EVENT_VIEWCREATED ), pDoc ) );
	return sal_True;
}


//========================================================================

long SfxViewFrameClose_Impl( void* /*pObj*/, void* pArg )
{
	((SfxViewFrame*)pArg)->GetFrame()->DoClose();
	return 0;
}

TYPEINIT1(SfxTopFrame, SfxFrame);
TYPEINIT1(SfxTopViewFrame, SfxViewFrame);

//--------------------------------------------------------------------
SFX_IMPL_INTERFACE(SfxTopViewFrame,SfxViewFrame,SfxResId(0))
{
}

//--------------------------------------------------------------------
String SfxTopViewFrame::UpdateTitle()

/*	[Beschreibung]

	Mit dieser Methode kann der SfxTopViewFrame gezwungen werden, sich sofort
	den neuen Titel vom der <SfxObjectShell> zu besorgen.

	[Anmerkung]

	Dies ist z.B. dann notwendig, wenn man der SfxObjectShell als SfxListener
	zuh"ort und dort auf den <SfxSimpleHint> SFX_HINT_TITLECHANGED reagieren
	m"ochte, um dann die Titel seiner Views abzufragen. Diese Views (SfxTopViewFrames)
	jedoch sind ebenfalls SfxListener und da die Reihenfolge der Benachrichtigung
	nicht feststeht, mu\s deren Titel-Update vorab erzwungen werden.


	[Beispiel]

	void SwDocShell::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
	{
		if ( rHint.IsA(TYPE(SfxSimpleHint)) )
		{
			switch( ( (SfxSimpleHint&) rHint ).GetId() )
			{
				case SFX_HINT_TITLECHANGED:
					for ( SfxTopViewFrame *pTop = (SfxTopViewFrame*)
								SfxViewFrame::GetFirst(this, TYPE(SfxTopViewFrame));
						  pTop;
						  pTop = (SfxTopViewFrame*)
								SfxViewFrame::GetNext(this, TYPE(SfxTopViewFrame));
					{
						pTop->UpdateTitle();
						... pTop->GetName() ...
					}
					break;
				...
			}
		}
	}
*/

{
	DBG_CHKTHIS(SfxTopViewFrame, 0);

    const SfxObjectFactory &rFact = GetObjectShell()->GetFactory();
    pImp->aFactoryName = String::CreateFromAscii( rFact.GetShortName() );

    String aTitle = SfxViewFrame::UpdateTitle();

    ::rtl::OUString aProductName;
    ::utl::ConfigManager::GetDirectConfigProperty(::utl::ConfigManager::PRODUCTNAME) >>= aProductName;

    aTitle += String::CreateFromAscii( " - " );
    aTitle += String(aProductName);
    aTitle += ' ';
    ::rtl::OUString aDocServiceName( GetObjectShell()->GetFactory().GetDocumentServiceName() );
    aTitle += String( GetModuleName_Impl( aDocServiceName ) );
#ifndef PRODUCT
	::rtl::OUString	aDefault;
	aTitle += DEFINE_CONST_UNICODE(" [");
	String aVerId( utl::Bootstrap::getBuildIdData( aDefault ));
	aTitle += aVerId;
	aTitle += ']';
#endif

    // append TAB string if available
    aTitle += _getTabString();

    GetBindings().Invalidate( SID_NEWDOCDIRECT );

    /* AS_TITLE
    Window* pWindow = GetTopFrame_Impl()->GetTopWindow_Impl();
    if ( pWindow && pWindow->GetText() != aTitle )
        pWindow->SetText( aTitle );
    */
	return aTitle;
}

//--------------------------------------------------------------------
void SfxTopViewFrame::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
{
    {DBG_CHKTHIS(SfxTopViewFrame, 0);}

    if( IsDowning_Impl())
        return;

    // we know only SimpleHints
    if ( rHint.IsA(TYPE(SfxSimpleHint)) )
    {
        switch( ( (SfxSimpleHint&) rHint ).GetId() )
        {
            case SFX_HINT_MODECHANGED:
            case SFX_HINT_TITLECHANGED:
                // when the document changes its title, change views too
                UpdateTitle();
                break;

            case SFX_HINT_DEINITIALIZING:
                // on all other changes force repaint
                GetFrame()->DoClose();
                return;
	}
    }

    SfxViewFrame::Notify( rBC, rHint );
}

//--------------------------------------------------------------------
sal_Bool SfxTopViewFrame::Close()
{
    {DBG_CHKTHIS(SfxTopViewFrame, 0);}

	// Modaler Dialog oben ??
//	if ( pImp->GetModalDialog() )
//		return sal_False;

	// eigentliches Schlie\sen
	if ( SfxViewFrame::Close() )
	{
        if (SfxViewFrame::Current() == this)
            SfxViewFrame::SetViewFrame(0);

		// Da der Dispatcher leer ger"aumt wird, kann man ihn auch nicht mehr
		// vern"unftig verwenden - also besser still legen
		GetDispatcher()->Lock(sal_True);
		delete this;

		return sal_True;
	}

	return sal_False;
}

SfxTopViewFrame::SfxTopViewFrame
(
	SfxFrame*			pFrame,
	SfxObjectShell* 	pObjShell,
	sal_uInt16				nViewId
)

/*	[Beschreibung]

	Ctor des SfxTopViewFrame f"ur eine <SfxObjectShell> aus der Ressource.
	Die 'nViewId' der zu erzeugenden <SfxViewShell> kann angegeben werden
	(default ist die zuerst registrierte SfxViewShell-Subklasse).
*/

    : SfxViewFrame( *(new SfxBindings), pFrame, pObjShell, SFXFRAME_HASTITLE )
{
	DBG_CTOR(SfxTopViewFrame, 0);

	pCloser = 0;
	pImp = new SfxTopViewFrame_Impl;
    pImp->pStopButtonTimer = new StopButtonTimer_Impl(this);

//(mba)/task    if ( !pFrame->GetTask() )
    {
        pImp->pWindow = new SfxTopViewWin_Impl( this, &pFrame->GetWindow() );
        pImp->pWindow->SetSizePixel( pFrame->GetWindow().GetOutputSizePixel() );
        SetWindow_Impl( pImp->pWindow );
		pFrame->SetOwnsBindings_Impl( sal_True );
        pFrame->CreateWorkWindow_Impl();
    }

	sal_uInt32 nType = SFXFRAME_OWNSDOCUMENT | SFXFRAME_HASTITLE;
	if ( pObjShell && pObjShell->GetCreateMode() == SFX_CREATE_MODE_EMBEDDED )
		nType |= SFXFRAME_EXTERNAL;
	GetFrame()->SetFrameType_Impl( GetFrame()->GetFrameType() | nType );

    if ( GetFrame()->IsInPlace() )
    {
        LockAdjustPosSizePixel();
    }

    try
    {
        if ( pObjShell )
            SwitchToViewShell_Impl( nViewId );
    }
    catch (com::sun::star::uno::Exception& )
    {
        // make sure that the ctor is left regularly
        ReleaseObjectShell_Impl();
        return;
    }

    if ( GetFrame()->IsInPlace() )
    {
        UnlockAdjustPosSizePixel();
    }
    else if ( GetViewShell() && GetViewShell()->UseObjectSize() )
	{
        // initiale Gr"o\se festlegen
		// Zuerst die logischen Koordinaten von IP-Objekt und EditWindow
		// ber"ucksichtigen
		LockAdjustPosSizePixel();
		ForceInnerResize_Impl( TRUE );

		Window *pWindow = GetViewShell()->GetWindow();

		// Da in den Applikationen bei der R"ucktransformation immer die
		// Eckpunkte tranformiert werden und nicht die Size (um die Ecken
		// alignen zu k"onnen), transformieren wir hier auch die Punkte, um
		// m"oglichst wenig Rundungsfehler zu erhalten.
/*
        Rectangle aRect = pWindow->LogicToLogic( GetObjectShell()->GetVisArea(),
                                        GetObjectShell()->GetMapUnit(),
										pWindow->GetMapMode() );
*/
        Rectangle aRect = pWindow->LogicToPixel( GetObjectShell()->GetVisArea() );
		Size aSize = aRect.GetSize();
		GetViewShell()->GetWindow()->SetSizePixel( aSize );
		DoAdjustPosSizePixel(GetViewShell(), Point(), aSize );
	}
}

//------------------------------------------------------------------------
SfxTopViewFrame::~SfxTopViewFrame()
{
	DBG_DTOR(SfxTopViewFrame, 0);

	SetDowning_Impl();

    if ( SfxViewFrame::Current() == this )
        SfxViewFrame::SetViewFrame(NULL);

	ReleaseObjectShell_Impl();
	if ( pPendingCloser == pCloser )
		pPendingCloser = 0;
	delete pCloser;
	if ( GetFrame()->OwnsBindings_Impl() )
		// Die Bindings l"oscht der Frame!
		KillDispatcher_Impl();

    delete pImp->pWindow;
    delete pImp->pStopButtonTimer;
    delete pImp;
}

//------------------------------------------------------------------------
sal_Bool SfxTopViewFrame::SetBorderPixelImpl( const SfxViewShell *pVSh, const SvBorder &rBorder )
{
	if( SfxViewFrame::SetBorderPixelImpl( GetViewShell(), rBorder ) )
	{
        if ( IsResizeInToOut_Impl() && !GetFrame()->IsInPlace() )
		{
			Size aSize = pVSh->GetWindow()->GetOutputSizePixel();
			if ( aSize.Width() && aSize.Height() )
			{
				aSize.Width() += rBorder.Left() + rBorder.Right();
				aSize.Height() += rBorder.Top() + rBorder.Bottom();

				Size aOldSize = GetWindow().GetOutputSizePixel();
				GetWindow().SetOutputSizePixel( aSize );
				Window* pParent = &GetWindow();
				while ( pParent->GetParent() )
					pParent = pParent->GetParent();
				Size aOuterSize = pParent->GetOutputSizePixel();
				aOuterSize.Width() += ( aSize.Width() - aOldSize.Width() );
				aOuterSize.Height() += ( aSize.Height() - aOldSize.Height() );
				pParent->SetOutputSizePixel( aOuterSize );
			}
		}
		else
		{
			Point aPoint;
			Rectangle aEditArea( aPoint, GetWindow().GetOutputSizePixel() );
			aEditArea.Left() += rBorder.Left();
			aEditArea.Right() -= rBorder.Right();
			aEditArea.Top() += rBorder.Top();
			aEditArea.Bottom() -= rBorder.Bottom();
			pVSh->GetWindow()->SetPosSizePixel( aEditArea.TopLeft(), aEditArea.GetSize() );
		}
		return sal_True;

	}
	return sal_False;
}

void SfxTopViewFrame::Exec_Impl(SfxRequest &rReq )
{
	// Wenn gerade die Shells ausgetauscht werden...
	if ( !GetObjectShell() || !GetViewShell() )
		return;

	switch ( rReq.GetSlot() )
	{
        case SID_SHOWPOPUPS :
        {
			SFX_REQUEST_ARG(rReq, pShowItem, SfxBoolItem, SID_SHOWPOPUPS, FALSE);
			BOOL bShow = pShowItem ? pShowItem->GetValue() : TRUE;
			SFX_REQUEST_ARG(rReq, pIdItem, SfxUInt16Item, SID_CONFIGITEMID, FALSE);
			USHORT nId = pIdItem ? pIdItem->GetValue() : 0;

			// ausfuehren
            SfxWorkWindow *pWorkWin = GetFrame()->GetWorkWindow_Impl();
			if ( bShow )
			{
				// Zuerst die Floats auch anzeigbar machen
				pWorkWin->MakeChildsVisible_Impl( bShow );
                GetDispatcher()->Update_Impl( TRUE );

				// Dann anzeigen
                GetBindings().HidePopups( !bShow );
			}
            else
			{
				// Alles hiden
				SfxBindings *pBind = &GetBindings();
				while ( pBind )
				{
					pBind->HidePopupCtrls_Impl( !bShow );
					pBind = pBind->GetSubBindings_Impl();
				}

				pWorkWin->HidePopups_Impl( !bShow, TRUE, nId );
				pWorkWin->MakeChildsVisible_Impl( bShow );
			}

            Invalidate( rReq.GetSlot() );
            rReq.Done();
			break;
        }

		case SID_ACTIVATE:
		{
            MakeActive_Impl( TRUE );
            rReq.SetReturnValue( SfxObjectItem( 0, this ) );
			break;
		}

		case SID_WIN_POSSIZE:
			break;

        case SID_NEWDOCDIRECT :
        {
            SFX_REQUEST_ARG( rReq, pFactoryItem, SfxStringItem, SID_NEWDOCDIRECT, FALSE);
            String aFactName;
            if ( pFactoryItem )
                aFactName = pFactoryItem->GetValue();
            else if ( pImp->aFactoryName.Len() )
				aFactName = pImp->aFactoryName;
			else
            {
                DBG_ERROR("Missing argument!");
                break;
            }

            SfxRequest aReq( SID_OPENDOC, SFX_CALLMODE_SYNCHRON, GetPool() );
            String aFact = String::CreateFromAscii("private:factory/");
            aFact += aFactName;
            aReq.AppendItem( SfxStringItem( SID_FILE_NAME, aFact ) );
            aReq.AppendItem( SfxFrameItem( SID_DOCFRAME, GetFrame() ) );
            aReq.AppendItem( SfxStringItem( SID_TARGETNAME, String::CreateFromAscii( "_blank" ) ) );
            SFX_APP()->ExecuteSlot( aReq );
            const SfxViewFrameItem* pItem = PTR_CAST( SfxViewFrameItem, aReq.GetReturnValue() );
            if ( pItem )
                rReq.SetReturnValue( SfxFrameItem( 0, pItem->GetFrame() ) );
            break;
        }

        case SID_CLOSEWIN:
		{
			// disable CloseWin, if frame is not a task
            Reference < XCloseable > xTask( GetFrame()->GetFrameInterface(),  UNO_QUERY );
			if ( !xTask.is() )
				break;

			if ( GetViewShell()->PrepareClose() )
			{
                // weitere Views auf dasselbe Doc?
				SfxObjectShell *pDocSh = GetObjectShell();
				int bOther = sal_False;
				for ( const SfxTopViewFrame *pFrame =
						  (SfxTopViewFrame *)SfxViewFrame::GetFirst( pDocSh, TYPE(SfxTopViewFrame) );
					  !bOther && pFrame;
					  pFrame = (SfxTopViewFrame *)SfxViewFrame::GetNext( *pFrame, pDocSh, TYPE(SfxTopViewFrame) ) )
					bOther = (pFrame != this);

                // Doc braucht nur gefragt zu werden, wenn keine weitere View
				sal_Bool bClosed = sal_False;
                sal_Bool bUI = TRUE;
                if ( ( bOther || pDocSh->PrepareClose( bUI ) ) )
				{
					if ( !bOther )
                    	pDocSh->SetModified( FALSE );
					rReq.Done(); // unbedingt vor Close() rufen!
                    bClosed = sal_False;
	                try
	                {
	                    xTask->close(sal_True);
	                    bClosed = sal_True;
	                }
	                catch( CloseVetoException& )
	                {
	                    bClosed = sal_False;
	                }
				}

				rReq.SetReturnValue( SfxBoolItem( rReq.GetSlot(), bClosed ));
			}
			return;
		}
	}

	rReq.Done();
}

void SfxTopViewFrame::GetState_Impl( SfxItemSet &rSet )
{
	SfxObjectShell *pDocSh = GetObjectShell();

	if ( !pDocSh )
		return;

	const sal_uInt16 *pRanges = rSet.GetRanges();
	DBG_ASSERT(pRanges, "Set ohne Bereich");
	while ( *pRanges )
	{
		for ( sal_uInt16 nWhich = *pRanges++; nWhich <= *pRanges; ++nWhich )
		{
			switch(nWhich)
			{
            case SID_NEWDOCDIRECT :
            {
				if ( pImp->aFactoryName.Len() )
				{
	                String aFact = String::CreateFromAscii("private:factory/");
		            aFact += pImp->aFactoryName;
	                rSet.Put( SfxStringItem( nWhich, aFact ) );
				}
                break;
            }

			case SID_NEWWINDOW:
				rSet.DisableItem(nWhich);
				break;

			case SID_CLOSEWIN:
			{
				// disable CloseWin, if frame is not a task
                Reference < XCloseable > xTask( GetFrame()->GetFrameInterface(),  UNO_QUERY );
				if ( !xTask.is() )
					rSet.DisableItem(nWhich);
				break;
			}

            case SID_SHOWPOPUPS :
				break;

			case SID_WIN_POSSIZE:
			{
				rSet.Put( SfxRectangleItem( nWhich, Rectangle(
						GetWindow().GetPosPixel(), GetWindow().GetSizePixel() ) ) );
				break;
			}

			default:
				DBG_ERROR( "invalid message-id" );
			}
		}
		++pRanges;
	}
}

void SfxTopViewFrame::INetExecute_Impl( SfxRequest &rRequest )
{
	sal_uInt16 nSlotId = rRequest.GetSlot();
	switch( nSlotId )
	{
		case SID_BROWSE_FORWARD:
		case SID_BROWSE_BACKWARD:
		{
			// Anzeige der n"achsten oder vorherigen Seite aus der History
			SFX_REQUEST_ARG( rRequest, pSteps, SfxUInt16Item, nSlotId, sal_False );
			GetFrame()->Browse( nSlotId == SID_BROWSE_FORWARD, pSteps ? pSteps->GetValue() : 1,
				(rRequest.GetModifier() & KEY_MOD1) != 0 );
			break;
		}
		case SID_CREATELINK:
		{
/*! (pb) we need new implementation to create a link
*/
			break;
		}
		case SID_BROWSE_STOP:
		{
            if ( GetCancelManager() )
                GetCancelManager()->Cancel( TRUE );

            // cancel jobs in hidden tasks
            SfxFrameArr_Impl& rArr = *SFX_APP()->Get_Impl()->pTopFrames;
            for( USHORT nPos = rArr.Count(); nPos--; )
            {
                SfxFrame *pFrame = rArr[ nPos ];
                if ( !pFrame->GetCurrentViewFrame() )
                    pFrame->GetCancelManager()->Cancel( TRUE );
            }

			break;
		}
		case SID_FOCUSURLBOX:
		{
            SfxStateCache *pCache = GetBindings().GetAnyStateCache_Impl( SID_OPENURL );
			if( pCache )
			{
				SfxControllerItem* pCtrl = pCache->GetItemLink();
				while( pCtrl )
				{
                    pCtrl->StateChanged( SID_FOCUSURLBOX, SFX_ITEM_UNKNOWN, 0 );
					pCtrl = pCtrl->GetItemLink();
				}
			}
		}
	}

	// Recording
	rRequest.Done();
}

void SfxTopViewFrame::INetState_Impl( SfxItemSet &rItemSet )
{
	if ( !GetFrame()->CanBrowseForward() )
		rItemSet.DisableItem( SID_BROWSE_FORWARD );

	if ( !GetFrame()->CanBrowseBackward() )
		rItemSet.DisableItem( SID_BROWSE_BACKWARD );

    // Add/SaveToBookmark bei BASIC-IDE, QUERY-EDITOR etc. disablen
	SfxObjectShell *pDocSh = GetObjectShell();
    sal_Bool bPseudo = pDocSh && !( pDocSh->GetFactory().GetFlags() & SFXOBJECTSHELL_HASOPENDOC );
    sal_Bool bEmbedded = pDocSh && pDocSh->GetCreateMode() == SFX_CREATE_MODE_EMBEDDED;
	if ( !pDocSh || bPseudo || bEmbedded || !pDocSh->HasName() )
		rItemSet.DisableItem( SID_CREATELINK );

    pImp->pStopButtonTimer->SetButtonState( GetCancelManager()->CanCancel() );
    if ( !pImp->pStopButtonTimer->GetButtonState() )
		rItemSet.DisableItem( SID_BROWSE_STOP );
}

void SfxTopViewFrame::SetZoomFactor( const Fraction &rZoomX, const Fraction &rZoomY )
{
	GetViewShell()->SetZoomFactor( rZoomX, rZoomY );
}

void SfxTopViewFrame::Activate( sal_Bool bMDI )
{
	DBG_ASSERT(GetViewShell(), "Keine Shell");
    if ( bMDI )
        pImp->bActive = sal_True;
//(mba): hier evtl. wie in Beanframe NotifyEvent ?!
}

void SfxTopViewFrame::Deactivate( sal_Bool bMDI )
{
	DBG_ASSERT(GetViewShell(), "Keine Shell");
    if ( bMDI )
        pImp->bActive = sal_False;
//(mba): hier evtl. wie in Beanframe NotifyEvent ?!
}

void SfxTopFrame::CheckMenuCloser_Impl( MenuBar* pMenuBar )
{
	Reference < ::com::sun::star::frame::XFrame > xFrame = GetFrameInterface();

	// checks if there is more than one "real" (not help) task window
	// in this case a close button is inserted into the menubar

    DBG_ASSERT( xFrame.is(), "Attention: this bug is very hard to reproduce. Please try to remember how you triggered it!");
	if ( !xFrame.is() || !xFrame->getController().is() )
		// dummy component
		return;

    Reference < ::com::sun::star::frame::XFramesSupplier > xDesktop( xFrame->getCreator(), UNO_QUERY );
	if ( !xDesktop.is() )
		// test only for task windows
		return;

	sal_Bool bLastTask = sal_False;
    Reference < ::com::sun::star::container::XIndexAccess >
			xList ( xDesktop->getFrames(), ::com::sun::star::uno::UNO_QUERY );
    sal_Int32 nCount = xList->getCount();
	if ( nCount<=1 )
		// only one task
		bLastTask = sal_True;
	else if ( nCount==2 )
	{
		// if we have to tasks, one can be the help task, that should be ignored
    	for( sal_Int32 i=0; i<nCount; ++i )
    	{
			Reference < ::com::sun::star::frame::XFrame > xTask;
        	::com::sun::star::uno::Any aVal = xList->getByIndex(i);
        	if ( (aVal>>=xTask) && xTask.is() && xTask->getName().compareToAscii("OFFICE_HELP_TASK") == COMPARE_EQUAL )
			{
				// one of the two open tasks was the help task -> ignored
				bLastTask = sal_True;
				break;
			}
		}
	}

	pMenuBar->ShowCloser(bLastTask);
}
