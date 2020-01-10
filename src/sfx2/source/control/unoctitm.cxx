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

#include <tools/debug.hxx>
#include <svtools/eitem.hxx>
#include <svtools/stritem.hxx>
#include <svtools/intitem.hxx>
#include <svtools/itemset.hxx>
#include <svtools/visitem.hxx>
#include <svtools/javacontext.hxx>
#include <svtools/itempool.hxx>
#include <tools/urlobj.hxx>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XFrameActionListener.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/FrameActionEvent.hpp>
#include <com/sun/star/frame/FrameAction.hpp>
#include <com/sun/star/frame/status/ItemStatus.hpp>
#include <com/sun/star/frame/status/ItemState.hpp>
#include <com/sun/star/frame/DispatchResultState.hpp>
#include <com/sun/star/frame/status/Visibility.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <vos/mutex.hxx>
#include <uno/current_context.hxx>
#include <vcl/svapp.hxx>

#include <sfx2/app.hxx>
#include <sfx2/unoctitm.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/ctrlitem.hxx>
#include <sfx2/sfxuno.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/request.hxx>
#include "statcach.hxx"
#include <sfx2/msgpool.hxx>
#include <sfx2/objsh.hxx>

namespace css = ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
//long nOfficeDispatchCount = 0;

enum URLTypeId
{
    URLType_BOOL,
    URLType_BYTE,
    URLType_SHORT,
    URLType_LONG,
    URLType_HYPER,
    URLType_STRING,
    URLType_FLOAT,
    URLType_DOUBLE,
    URLType_COUNT
};

const char* URLTypeNames[URLType_COUNT] =
{
    "bool",
    "byte",
    "short",
    "long",
    "hyper",
    "string",
    "float",
    "double"
};

SFX_IMPL_XINTERFACE_2( SfxUnoControllerItem, OWeakObject, ::com::sun::star::frame::XStatusListener, ::com::sun::star::lang::XEventListener )
SFX_IMPL_XTYPEPROVIDER_2( SfxUnoControllerItem, ::com::sun::star::frame::XStatusListener, ::com::sun::star::lang::XEventListener )

SfxUnoControllerItem::SfxUnoControllerItem( SfxControllerItem *pItem, SfxBindings& rBind, const String& rCmd )
	: pCtrlItem( pItem )
    , pBindings( &rBind )
{
	DBG_ASSERT( !pCtrlItem || !pCtrlItem->IsBound(), "ControllerItem fehlerhaft!" );

	aCommand.Complete = rCmd;
    Reference < XURLTransformer > xTrans( ::comphelper::getProcessServiceFactory()->createInstance( rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer" )), UNO_QUERY );
    xTrans->parseStrict( aCommand );
	pBindings->RegisterUnoController_Impl( this );
}

SfxUnoControllerItem::~SfxUnoControllerItem()
{
	// tell bindings to forget this controller ( if still connected )
	if ( pBindings )
		pBindings->ReleaseUnoController_Impl( this );
}

void SfxUnoControllerItem::UnBind()
{
	// connection to SfxControllerItem is lost
	pCtrlItem = NULL;
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener >  aRef( (::cppu::OWeakObject*)this, ::com::sun::star::uno::UNO_QUERY );
	ReleaseDispatch();
}

void SAL_CALL SfxUnoControllerItem::statusChanged(const ::com::sun::star::frame::FeatureStateEvent& rEvent) throw ( ::com::sun::star::uno::RuntimeException )
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
	DBG_ASSERT( pCtrlItem, "Dispatch hat den StatusListener nicht entfern!" );

	if ( rEvent.Requery )
	{
		// Fehler kann nur passieren, wenn das alte Dispatch fehlerhaft implementiert
		// ist, also removeStatusListener nicht gefunzt hat. Aber sowas soll
		// ja vorkommen ...
		// Also besser vor ReleaseDispatch gegen Abflug sch"utzen!
		::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener >  aRef( (::cppu::OWeakObject*)this, ::com::sun::star::uno::UNO_QUERY  );
		ReleaseDispatch();
		if ( pCtrlItem )
			GetNewDispatch(); 		// asynchron ??
	}
	else if ( pCtrlItem )
	{
		SfxItemState eState = SFX_ITEM_DISABLED;
		SfxPoolItem* pItem = NULL;
		if ( rEvent.IsEnabled )
		{
			eState = SFX_ITEM_AVAILABLE;
			::com::sun::star::uno::Type pType =	rEvent.State.getValueType();

			if ( pType == ::getBooleanCppuType() )
			{
				sal_Bool bTemp = false;
				rEvent.State >>= bTemp ;
				pItem = new SfxBoolItem( pCtrlItem->GetId(), bTemp );
			}
			else if ( pType == ::getCppuType((const sal_uInt16*)0) )
			{
				sal_uInt16 nTemp = 0;
				rEvent.State >>= nTemp ;
				pItem = new SfxUInt16Item( pCtrlItem->GetId(), nTemp );
			}
			else if ( pType == ::getCppuType((const sal_uInt32*)0) )
			{
				sal_uInt32 nTemp = 0;
				rEvent.State >>= nTemp ;
				pItem = new SfxUInt32Item( pCtrlItem->GetId(), nTemp );
			}
			else if ( pType == ::getCppuType((const ::rtl::OUString*)0) )
			{
				::rtl::OUString sTemp ;
				rEvent.State >>= sTemp ;
				pItem = new SfxStringItem( pCtrlItem->GetId(), sTemp );
			}
			else
				pItem = new SfxVoidItem( pCtrlItem->GetId() );
		}

		pCtrlItem->StateChanged( pCtrlItem->GetId(), eState, pItem );
		delete pItem;
	}
}

void  SAL_CALL SfxUnoControllerItem::disposing( const ::com::sun::star::lang::EventObject& ) throw ( ::com::sun::star::uno::RuntimeException )
{
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener >  aRef( (::cppu::OWeakObject*)this, ::com::sun::star::uno::UNO_QUERY );
	ReleaseDispatch();
}

void SfxUnoControllerItem::ReleaseDispatch()
{
	if ( xDispatch.is() )
	{
		xDispatch->removeStatusListener( (::com::sun::star::frame::XStatusListener*) this, aCommand );
		xDispatch = ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > ();
	}
}

void SfxUnoControllerItem::GetNewDispatch()
{
	if ( !pBindings )
	{
		// Bindings released
		DBG_ERROR( "Tried to get dispatch, but no Bindings!" );
		return;
	}

	// forget old dispatch
	xDispatch = ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > ();

	// no arms, no cookies !
	if ( !pBindings->GetDispatcher_Impl() || !pBindings->GetDispatcher_Impl()->GetFrame() )
		return;

	SfxFrame *pFrame = pBindings->GetDispatcher_Impl()->GetFrame()->GetFrame();
	SfxFrame *pParent = pFrame->GetParentFrame();
	if ( pParent )
		// parent may intercept
		xDispatch = TryGetDispatch( pParent );

	if ( !xDispatch.is() )
	{
		// no interception
		::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >  xFrame = pFrame->GetFrameInterface();
		::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchProvider >  xProv( xFrame, ::com::sun::star::uno::UNO_QUERY );
		if ( xProv.is() )
			xDispatch = xProv->queryDispatch( aCommand, ::rtl::OUString(), 0 );
	}

	if ( xDispatch.is() )
		xDispatch->addStatusListener( (::com::sun::star::frame::XStatusListener*) this, aCommand );
	else if ( pCtrlItem )
		pCtrlItem->StateChanged( pCtrlItem->GetId(), SFX_ITEM_DISABLED, NULL );
}

::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch >  SfxUnoControllerItem::TryGetDispatch( SfxFrame *pFrame )
{
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch >  xDisp;
	SfxFrame *pParent = pFrame->GetParentFrame();
	if ( pParent )
		// parent may intercept
		xDisp = TryGetDispatch( pParent );

	// only components may intercept
	if ( !xDisp.is() && pFrame->HasComponent() )
	{
		// no interception
		::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >  xFrame = pFrame->GetFrameInterface();
		::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchProvider >  xProv( xFrame, ::com::sun::star::uno::UNO_QUERY );
		if ( xProv.is() )
			xDisp = xProv->queryDispatch( aCommand, ::rtl::OUString(), 0 );
	}

	return xDisp;
}

void SfxUnoControllerItem::Execute()
{
	// dispatch the resource
    ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > aSeq(1);
    aSeq[0].Name = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("Referer") );
    aSeq[0].Value <<= ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("private:select") );
	if ( xDispatch.is() )
        xDispatch->dispatch( aCommand, aSeq );
}

void SfxUnoControllerItem::ReleaseBindings()
{
	// connection to binding is lost; so forget the binding and the dispatch
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener >  aRef( (::cppu::OWeakObject*)this, ::com::sun::star::uno::UNO_QUERY );
	ReleaseDispatch();
	if ( pBindings )
		pBindings->ReleaseUnoController_Impl( this );
	pBindings = NULL;
}

void SfxStatusDispatcher::ReleaseAll()
{
	::com::sun::star::lang::EventObject aObject;
	aObject.Source = (::cppu::OWeakObject*) this;
	aListeners.disposeAndClear( aObject );
}

void SAL_CALL SfxStatusDispatcher::dispatch( const ::com::sun::star::util::URL&, const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& ) throw ( ::com::sun::star::uno::RuntimeException )
{
}

void SAL_CALL SfxStatusDispatcher::dispatchWithNotification(
    const ::com::sun::star::util::URL&,
    const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >&,
    const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchResultListener >& ) throw( ::com::sun::star::uno::RuntimeException )
{
}

SFX_IMPL_XINTERFACE_2( SfxStatusDispatcher, OWeakObject, ::com::sun::star::frame::XNotifyingDispatch, ::com::sun::star::frame::XDispatch )
SFX_IMPL_XTYPEPROVIDER_2( SfxStatusDispatcher, ::com::sun::star::frame::XNotifyingDispatch, ::com::sun::star::frame::XDispatch )
//IMPLNAME "com.sun.star.comp.sfx2.StatusDispatcher",

SfxStatusDispatcher::SfxStatusDispatcher()
	: aListeners( aMutex )
{
}

void SAL_CALL SfxStatusDispatcher::addStatusListener(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener > & aListener, const ::com::sun::star::util::URL& aURL) throw ( ::com::sun::star::uno::RuntimeException )
{
	aListeners.addInterface( aURL.Complete, aListener );
	if ( aURL.Complete.compareToAscii(".uno:LifeTime")==0 )
	{
		::com::sun::star::frame::FeatureStateEvent aEvent;
		aEvent.FeatureURL = aURL;
		aEvent.Source = (::com::sun::star::frame::XDispatch*) this;
		aEvent.IsEnabled = sal_True;
		aEvent.Requery = sal_False;
		aListener->statusChanged( aEvent );
	}
}

void SAL_CALL SfxStatusDispatcher::removeStatusListener( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener > & aListener, const ::com::sun::star::util::URL& aURL ) throw ( ::com::sun::star::uno::RuntimeException )
{
	aListeners.removeInterface( aURL.Complete, aListener );
}

SFX_IMPL_XINTERFACE_1( SfxOfficeDispatch, SfxStatusDispatcher, ::com::sun::star::lang::XUnoTunnel )
SFX_IMPL_XTYPEPROVIDER_2( SfxOfficeDispatch, ::com::sun::star::frame::XNotifyingDispatch, ::com::sun::star::lang::XUnoTunnel )


//-------------------------------------------------------------------------
// XUnoTunnel
sal_Int64 SAL_CALL SfxOfficeDispatch::getSomething( const ::com::sun::star::uno::Sequence< sal_Int8 >& aIdentifier ) throw(::com::sun::star::uno::RuntimeException)
{
    if ( aIdentifier == impl_getStaticIdentifier() )
        return sal::static_int_cast< sal_Int64 >( reinterpret_cast< sal_IntPtr >( this ));
    else
        return 0;
}

/* ASDBG
void* SfxOfficeDispatch::getImplementation(Reflection *p)
{
	if( p == ::getCppuType((const SfxOfficeDispatch*)0) )
		return this;
	else
		return ::cppu::OWeakObject::getImplementation(p);

}

Reflection* ::getCppuType((const SfxOfficeDispatch*)0)
{
	static StandardClassReflection aRefl(
		0,
		createStandardClass(
			"SfxOfficeDispatch", ::cppu::OWeakObject::get::cppu::OWeakObjectIdlClass(),
			1,
			::getCppuType((const ::com::sun::star::frame::XDispatch*)0) ) );
	return &aRefl;
}
*/

SfxOfficeDispatch::SfxOfficeDispatch( SfxBindings& rBindings, SfxDispatcher* pDispat, const SfxSlot* pSlot, const ::com::sun::star::util::URL& rURL )
{
//    nOfficeDispatchCount++;

    // this object is an adapter that shows a ::com::sun::star::frame::XDispatch-Interface to the outside and uses a SfxControllerItem to monitor a state
    pControllerItem = new SfxDispatchController_Impl( this, &rBindings, pDispat, pSlot, rURL );
}

SfxOfficeDispatch::SfxOfficeDispatch( SfxDispatcher* pDispat, const SfxSlot* pSlot, const ::com::sun::star::util::URL& rURL )
{
//    nOfficeDispatchCount++;

    // this object is an adapter that shows a ::com::sun::star::frame::XDispatch-Interface to the outside and uses a SfxControllerItem to monitor a state
    pControllerItem = new SfxDispatchController_Impl( this, NULL, pDispat, pSlot, rURL );
}

SfxOfficeDispatch::~SfxOfficeDispatch()
{
//    --nOfficeDispatchCount;

    if ( pControllerItem )
    {
        // when dispatch object is released, destroy its connection to this object and destroy it
        pControllerItem->UnBindController();
        delete pControllerItem;
    }
}

const ::com::sun::star::uno::Sequence< sal_Int8 >& SfxOfficeDispatch::impl_getStaticIdentifier()
{
    // {38 57 CA 80 09 36 11 d4 83 FE 00 50 04 52 6B 21}
    static sal_uInt8 pGUID[16] = { 0x38, 0x57, 0xCA, 0x80, 0x09, 0x36, 0x11, 0xd4, 0x83, 0xFE, 0x00, 0x50, 0x04, 0x52, 0x6B, 0x21 };
    static ::com::sun::star::uno::Sequence< sal_Int8 > seqID((sal_Int8*)pGUID,16) ;
    return seqID ;
}


void SAL_CALL SfxOfficeDispatch::dispatch( const ::com::sun::star::util::URL& aURL, const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& aArgs ) throw ( ::com::sun::star::uno::RuntimeException )
{
    // ControllerItem is the Impl class
    if ( pControllerItem )
    {
        // The JavaContext contains an interaction handler which is used when
        // the creation of a Java Virtual Machine fails. The second parameter
        // indicates, that there shall only be one user notification (message box)
        // even if the same error (interaction) reoccurs. The effect is, that if a
        // user selects a menu entry than they may get only one notification that
        // a JRE is not selected.
        com::sun::star::uno::ContextLayer layer(
            new svt::JavaContext( com::sun::star::uno::getCurrentContext(),
                                  true) );

        pControllerItem->dispatch( aURL, aArgs, ::com::sun::star::uno::Reference < ::com::sun::star::frame::XDispatchResultListener >() );
    }
}

void SAL_CALL SfxOfficeDispatch::dispatchWithNotification( const ::com::sun::star::util::URL& aURL,
        const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& aArgs,
        const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchResultListener >& rListener ) throw( ::com::sun::star::uno::RuntimeException )
{
    // ControllerItem is the Impl class
    if ( pControllerItem )
    {
        // see comment for SfxOfficeDispatch::dispatch
        com::sun::star::uno::ContextLayer layer(
            new svt::JavaContext( com::sun::star::uno::getCurrentContext(),
                                  true) );

        pControllerItem->dispatch( aURL, aArgs, rListener );
    }
}

void SAL_CALL SfxOfficeDispatch::addStatusListener(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener > & aListener, const ::com::sun::star::util::URL& aURL) throw ( ::com::sun::star::uno::RuntimeException )
{
    GetListeners().addInterface( aURL.Complete, aListener );
    if ( pControllerItem )
    {
        // ControllerItem is the Impl class
        pControllerItem->addStatusListener( aListener, aURL );
    }
}

SfxDispatcher* SfxOfficeDispatch::GetDispatcher_Impl()
{
    return pControllerItem->GetDispatcher();
}

void SfxOfficeDispatch::SetFrame(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& xFrame)
{
    if ( pControllerItem )
        pControllerItem->SetFrame( xFrame );
}

void SfxOfficeDispatch::SetMasterUnoCommand( sal_Bool bSet )
{
    if ( pControllerItem )
        pControllerItem->setMasterSlaveCommand( bSet );
}

sal_Bool SfxOfficeDispatch::IsMasterUnoCommand() const
{
    if ( pControllerItem )
        pControllerItem->isMasterSlaveCommand();
    return sal_False;
}

// Determine if URL contains a master/slave command which must be handled a little bit different
sal_Bool SfxOfficeDispatch::IsMasterUnoCommand( const ::com::sun::star::util::URL& aURL )
{
    if ( aURL.Protocol.equalsAscii( ".uno:" ) &&
         ( aURL.Path.indexOf( '.' ) > 0 ))
        return sal_True;

    return sal_False;
}

rtl::OUString SfxOfficeDispatch::GetMasterUnoCommand( const ::com::sun::star::util::URL& aURL )
{
    rtl::OUString aMasterCommand;
    if ( IsMasterUnoCommand( aURL ))
    {
        sal_Int32 nIndex = aURL.Path.indexOf( '.' );
        if ( nIndex > 0 )
            aMasterCommand = aURL.Path.copy( 0, nIndex );
    }

    return aMasterCommand;
}

SfxDispatchController_Impl::SfxDispatchController_Impl(
    SfxOfficeDispatch*                 pDisp,
    SfxBindings*                       pBind,
    SfxDispatcher*                     pDispat,
    const SfxSlot*                     pSlot,
    const ::com::sun::star::util::URL& rURL )
    : aDispatchURL( rURL )
    , pDispatcher( pDispat )
    , pBindings( pBind )
    , pLastState( 0 )
    , nSlot( pSlot->GetSlotId() )
    , pDispatch( pDisp )
    , bMasterSlave( sal_False )
    , bVisible( sal_True )
    , pUnoName( pSlot->pUnoName )
{
    if ( aDispatchURL.Protocol.equalsAscii("slot:") && pUnoName )
    {
        ByteString aTmp(".uno:");
        aTmp += pUnoName;
        aDispatchURL.Complete = ::rtl::OUString::createFromAscii( aTmp.GetBuffer() );
        Reference < ::com::sun::star::util::XURLTransformer > xTrans( ::comphelper::getProcessServiceFactory()->createInstance( rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer" )), UNO_QUERY );
        xTrans->parseStrict( aDispatchURL );
    }

    SetId( nSlot );
    if ( pBindings )
    {
        // Bind immediately to enable the cache to recycle dispatches when asked for the same command
        // a command in "slot" or in ".uno" notation must be treated as identical commands!
        pBindings->ENTERREGISTRATIONS();
        BindInternal_Impl( nSlot, pBindings );
        pBindings->LEAVEREGISTRATIONS();
    }
}

SfxDispatchController_Impl::~SfxDispatchController_Impl()
{
	if ( pLastState && !IsInvalidItem( pLastState ) )
		delete pLastState;

    if ( pDispatch )
    {
        // disconnect
        pDispatch->pControllerItem = NULL;

        // force all listeners to release the dispatch object
        ::com::sun::star::lang::EventObject aObject;
        aObject.Source = (::cppu::OWeakObject*) pDispatch;
        pDispatch->GetListeners().disposeAndClear( aObject );
    }
}

void SfxDispatchController_Impl::SetFrame(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& _xFrame)
{
    xFrame = _xFrame;
}

void SfxDispatchController_Impl::setMasterSlaveCommand( sal_Bool bSet )
{
    bMasterSlave = bSet;
}

sal_Bool SfxDispatchController_Impl::isMasterSlaveCommand() const
{
    return bMasterSlave;
}

void SfxDispatchController_Impl::UnBindController()
{
    pDispatch = NULL;
    if ( IsBound() )
    {
        GetBindings().ENTERREGISTRATIONS();
        SfxControllerItem::UnBind();
        GetBindings().LEAVEREGISTRATIONS();
    }
}

void SfxDispatchController_Impl::addParametersToArgs( const com::sun::star::util::URL& aURL, ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& rArgs ) const
{
    // Extract the parameter from the URL and put them into the property value sequence
    sal_Int32 nQueryIndex = aURL.Complete.indexOf( '?' );
    if ( nQueryIndex > 0 )
    {
        rtl::OUString aParamString( aURL.Complete.copy( nQueryIndex+1 ));
        sal_Int32 nIndex = 0;
        do
        {
            rtl::OUString aToken = aParamString.getToken( 0, '&', nIndex );

            sal_Int32 nParmIndex = 0;
            rtl::OUString aParamType;
            rtl::OUString aParamName = aToken.getToken( 0, '=', nParmIndex );
            rtl::OUString aValue     = (nParmIndex!=-1) ? aToken.getToken( 0, '=', nParmIndex ) : ::rtl::OUString();

            if ( aParamName.getLength() > 0 )
            {
                nParmIndex = 0;
                aToken = aParamName;
                aParamName = (nParmIndex!=-1) ? aToken.getToken( 0, ':', nParmIndex ) : ::rtl::OUString();
                aParamType = (nParmIndex!=-1) ? aToken.getToken( 0, ':', nParmIndex ) : ::rtl::OUString();
            }

            sal_Int32 nLen = rArgs.getLength();
            rArgs.realloc( nLen+1 );
            rArgs[nLen].Name = aParamName;

            if ( aParamType.getLength() == 0 )
            {
                // Default: LONG
                rArgs[nLen].Value <<= aValue.toInt32();
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_BOOL], 4 ))
            {
                // BOOL support
                rArgs[nLen].Value <<= aValue.toBoolean();
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_BYTE], 4 ))
            {
                // BYTE support
                rArgs[nLen].Value <<= sal_Int8( aValue.toInt32() );
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_LONG], 4 ))
            {
                // LONG support
                rArgs[nLen].Value <<= aValue.toInt32();
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_SHORT], 5 ))
            {
                // SHORT support
                rArgs[nLen].Value <<= sal_Int8( aValue.toInt32() );
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_HYPER], 5 ))
            {
                // HYPER support
                rArgs[nLen].Value <<= aValue.toInt64();
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_FLOAT], 5 ))
            {
                // FLOAT support
                rArgs[nLen].Value <<= aValue.toFloat();
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_STRING], 6 ))
            {
                // STRING support
                rArgs[nLen].Value <<= rtl::OUString( INetURLObject::decode( aValue, '%', INetURLObject::DECODE_WITH_CHARSET ));
            }
            else if ( aParamType.equalsAsciiL( URLTypeNames[URLType_DOUBLE], 6))
            {
                // DOUBLE support
                rArgs[nLen].Value <<= aValue.toDouble();
            }
        }
        while ( nIndex >= 0 );
    }
}

SfxMapUnit SfxDispatchController_Impl::GetCoreMetric( SfxItemPool& rPool, sal_uInt16 nSlotId )
{
    USHORT nWhich = rPool.GetWhich( nSlotId );
    return rPool.GetMetric( nWhich );
}

rtl::OUString SfxDispatchController_Impl::getSlaveCommand( const ::com::sun::star::util::URL& rURL )
{
    rtl::OUString   aSlaveCommand;
    sal_Int32       nIndex = rURL.Path.indexOf( '.' );
    if (( nIndex > 0 ) && ( nIndex < rURL.Path.getLength() ))
        aSlaveCommand = rURL.Path.copy( nIndex+1 );
    return aSlaveCommand;
}

void SAL_CALL SfxDispatchController_Impl::dispatch( const ::com::sun::star::util::URL& aURL,
        const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& aArgs,
        const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchResultListener >& rListener ) throw( ::com::sun::star::uno::RuntimeException )
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if (
        pDispatch &&
        (
         (aURL.Protocol.equalsAsciiL( ".uno:", 5 ) && aURL.Path == aDispatchURL.Path) ||
         (aURL.Protocol.equalsAsciiL( "slot:", 5 ) && aURL.Path.toInt32() == GetId())
        )
       )
	{
        /*
        if ( !IsBound() && pBindings )
        {
            pBindings->ENTERREGISTRATIONS();
            BindInternal_Impl( nSlot, pBindings );
            pBindings->LEAVEREGISTRATIONS();
        } */

        if ( !pDispatcher && pBindings )
            pDispatcher = GetBindings().GetDispatcher_Impl();

        ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > lNewArgs;
        sal_Int32 nCount = aArgs.getLength();

        // Support for URL based arguments
        INetURLObject aURLObj( aURL.Complete );
        if ( aURLObj.HasParam() )
            addParametersToArgs( aURL, lNewArgs );

        // Try to find call mode and frame name inside given arguments...
        SfxCallMode nCall = SFX_CALLMODE_STANDARD;
        sal_Int32   nMarkArg = -1;

        // Filter arguments which shouldn't be part of the sequence property value
        sal_Bool    bTemp = sal_Bool();
        sal_uInt16  nModifier(0);
        std::vector< ::com::sun::star::beans::PropertyValue > aAddArgs;
        for( sal_Int32 n=0; n<nCount; n++ )
        {
            const ::com::sun::star::beans::PropertyValue& rProp = aArgs[n];
            if( rProp.Name.equalsAsciiL("SynchronMode",12))
            {
				if( rProp.Value >>=bTemp )
                	nCall = bTemp ? SFX_CALLMODE_SYNCHRON : SFX_CALLMODE_ASYNCHRON;
            }
            else if( rProp.Name.equalsAsciiL("Bookmark",8))
            {
                nMarkArg = n;
                aAddArgs.push_back( aArgs[n] );
            }
            else if( rProp.Name.equalsAsciiL("KeyModifier",11))
                rProp.Value >>= nModifier;
            else
                aAddArgs.push_back( aArgs[n] );
        }

        // Add needed arguments to sequence property value
        sal_uInt32 nAddArgs = aAddArgs.size();
        if ( nAddArgs > 0 )
        {
            sal_uInt32 nIndex( lNewArgs.getLength() );

            lNewArgs.realloc( lNewArgs.getLength()+aAddArgs.size() );
            for ( sal_uInt32 i = 0; i < nAddArgs; i++ )
                lNewArgs[nIndex++] = aAddArgs[i];
        }

        // Overwrite possible detected sychron argument, if real listener exists (currently no other way)
        if ( rListener.is() )
            nCall = SFX_CALLMODE_SYNCHRON;

        if( GetId() == SID_JUMPTOMARK && nMarkArg == - 1 )
        {
			// we offer dispatches for SID_JUMPTOMARK if the URL points to a bookmark inside the document
			// so we must retrieve this as an argument from the parsed URL
            lNewArgs.realloc( lNewArgs.getLength()+1 );
            nMarkArg = lNewArgs.getLength()-1;
	        lNewArgs[nMarkArg].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Bookmark"));
	        lNewArgs[nMarkArg].Value <<= aURL.Mark;
        }

        css::uno::Reference< css::frame::XFrame > xFrameRef(xFrame.get(), css::uno::UNO_QUERY);
        if (! xFrameRef.is() && pDispatcher)
        {
            SfxViewFrame* pViewFrame = pDispatcher->GetFrame();
            if (pViewFrame)
                xFrameRef = pViewFrame->GetFrame()->GetFrameInterface();
        }
        SfxAllItemSet aInternalSet( SFX_APP()->GetPool() );
        if (xFrameRef.is()) // an empty set is no problem ... but an empty frame reference can be a problem !
            aInternalSet.Put( SfxUnoAnyItem( SID_FILLFRAME, css::uno::makeAny(xFrameRef) ) );

        sal_Bool bSuccess = sal_False;
        sal_Bool bFailure = sal_False;
        const SfxPoolItem* pItem = NULL;
        SfxShell* pShell( 0 );
        // #i102619# Retrieve metric from shell before execution - the shell could be destroyed after execution 
        SfxMapUnit eMapUnit( SFX_MAPUNIT_100TH_MM );
        if ( pDispatcher->GetBindings() )
        {
            if ( !pDispatcher->IsLocked( GetId() ) )
            {
                const SfxSlot *pSlot = 0;
                if ( pDispatcher->GetShellAndSlot_Impl( GetId(), &pShell, &pSlot, sal_False,
                        SFX_CALLMODE_MODAL==(nCall&SFX_CALLMODE_MODAL), FALSE ) )
                {
                    if ( bMasterSlave )
                    {
                        // Extract slave command and add argument to the args list. Master slot MUST
                        // have a argument that has the same name as the master slot and type is SfxStringItem.
                        sal_Int32 nIndex = lNewArgs.getLength();
                        lNewArgs.realloc( nIndex+1 );
                        lNewArgs[nIndex].Name   = rtl::OUString::createFromAscii( pSlot->pUnoName );
                        lNewArgs[nIndex].Value  = makeAny( SfxDispatchController_Impl::getSlaveCommand( aDispatchURL ));
                    }

                    eMapUnit = GetCoreMetric( pShell->GetPool(), GetId() );
                    SfxAllItemSet aSet( pShell->GetPool() );
                    TransformParameters( GetId(), lNewArgs, aSet, pSlot );
                    if ( aSet.Count() )
                    {
                        // execute with arguments - call directly
                        pItem = pDispatcher->Execute( GetId(), nCall, &aSet, &aInternalSet, nModifier );
                        bSuccess = (pItem != NULL);
                    }
                    else
                    {
                        // execute using bindings, enables support for toggle/enum etc.
                        SfxRequest aReq( GetId(), nCall, pShell->GetPool() );
                        aReq.SetModifier( nModifier );
                        aReq.SetInternalArgs_Impl(aInternalSet);
                        pDispatcher->GetBindings()->Execute_Impl( aReq, pSlot, pShell );
                        pItem = aReq.GetReturnValue();
                        bSuccess = aReq.IsDone() || pItem != NULL;
                        bFailure = aReq.IsCancelled();
                    }
                }
#ifdef DBG_UTIL
                else
                    DBG_WARNING("MacroPlayer: Unknown slot dispatched!");
#endif
            }
        }
        else
        {
            eMapUnit = GetCoreMetric( SFX_APP()->GetPool(), GetId() );
            // AppDispatcher
            SfxAllItemSet aSet( SFX_APP()->GetPool() );
            TransformParameters( GetId(), lNewArgs, aSet );

            if ( aSet.Count() )
                pItem = pDispatcher->Execute( GetId(), nCall, &aSet, &aInternalSet, nModifier );
            else
                // SfxRequests take empty sets as argument sets, GetArgs() returning non-zero!
                pItem = pDispatcher->Execute( GetId(), nCall, 0, &aInternalSet, nModifier );

            // no bindings, no invalidate ( usually done in SfxDispatcher::Call_Impl()! )
			if ( SfxApplication::Is_Impl() )
			{
            	SfxDispatcher* pAppDispat = SFX_APP()->GetAppDispatcher_Impl();
            	if ( pAppDispat )
            	{
                	const SfxPoolItem* pState=0;
                 	SfxItemState eState = pDispatcher->QueryState( GetId(), pState );
                	StateChanged( GetId(), eState, pState );
            	}
			}

            bSuccess = (pItem != NULL);
        }

        if ( rListener.is() )
        {
            ::com::sun::star::frame::DispatchResultEvent aEvent;
            if ( bSuccess )
                aEvent.State = com::sun::star::frame::DispatchResultState::SUCCESS;
//            else if ( bFailure )
            else
                aEvent.State = com::sun::star::frame::DispatchResultState::FAILURE;
//            else
//                aEvent.State = com::sun::star::frame::DispatchResultState::DONTKNOW;

            aEvent.Source = (::com::sun::star::frame::XDispatch*) pDispatch;
            if ( bSuccess && pItem && !pItem->ISA(SfxVoidItem) )
            {
                USHORT nSubId( 0 );
                if ( eMapUnit == SFX_MAPUNIT_TWIP )
                    nSubId |= CONVERT_TWIPS;
                pItem->QueryValue( aEvent.Result, (BYTE)nSubId );
            }

            rListener->dispatchFinished( aEvent );
        }
	}
}

SfxDispatcher* SfxDispatchController_Impl::GetDispatcher()
{
    if ( !pDispatcher && pBindings )
        pDispatcher = GetBindings().GetDispatcher_Impl();
    return pDispatcher;
}

void SAL_CALL SfxDispatchController_Impl::addStatusListener(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XStatusListener > & aListener, const ::com::sun::star::util::URL& aURL) throw ( ::com::sun::star::uno::RuntimeException )
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if ( !pDispatch )
        return;

    /*if ( !IsBound() && pBindings )
    {
        pBindings->ENTERREGISTRATIONS();
        BindInternal_Impl( nSlot, pBindings );
        pBindings->LEAVEREGISTRATIONS();
    } */

    // Use alternative QueryState call to have a valid UNO representation of the state.
    ::com::sun::star::uno::Any aState;
    if ( !pDispatcher && pBindings )
        pDispatcher = GetBindings().GetDispatcher_Impl();
    SfxItemState eState = pDispatcher->QueryState( GetId(), aState );

    if ( eState == SFX_ITEM_DONTCARE )
    {
        // Use special uno struct to transport don't care state
        ::com::sun::star::frame::status::ItemStatus aItemStatus;
        aItemStatus.State = ::com::sun::star::frame::status::ItemState::dont_care;
        aState = makeAny( aItemStatus );
    }

    ::com::sun::star::frame::FeatureStateEvent  aEvent;
    aEvent.FeatureURL = aURL;
    aEvent.Source     = (::com::sun::star::frame::XDispatch*) pDispatch;
    aEvent.Requery    = sal_False;
    if ( bVisible )
    {
	    aEvent.IsEnabled  = eState != SFX_ITEM_DISABLED;
	    aEvent.State      = aState;
    }
    else
    {
        ::com::sun::star::frame::status::Visibility aVisibilityStatus;
        aVisibilityStatus.bVisible = sal_False;

        // MBA: we might decide to *not* disable "invisible" slots, but this would be
        // a change that needs to adjust at least the testtool
        aEvent.IsEnabled           = sal_False;
        aEvent.State               = makeAny( aVisibilityStatus );
    }

    aListener->statusChanged( aEvent );
}

void SfxDispatchController_Impl::StateChanged( sal_uInt16 nSID, SfxItemState eState, const SfxPoolItem* pState, SfxSlotServer* pSlotServ )
{
    if ( !pDispatch )
        return;

    // Bindings instance notifies controller about a state change, listeners must be notified also
    // Don't cache visibility state changes as they are volatile. We need our real state to send it
    // to our controllers after visibility is set to true.
	sal_Bool bNotify = sal_True;
    if ( pState && !IsInvalidItem( pState ) )
    {
        if ( !pState->ISA( SfxVisibilityItem ) )
        {
            sal_Bool bBothAvailable = pLastState && !IsInvalidItem(pLastState);
            if ( bBothAvailable )
                bNotify = pState->Type() != pLastState->Type() || *pState != *pLastState;
            if ( pLastState && !IsInvalidItem( pLastState ) )
                delete pLastState;
            pLastState = !IsInvalidItem(pState) ? pState->Clone() : pState;
            bVisible = sal_True;
        }
        else
            bVisible = ((SfxVisibilityItem *)pState)->GetValue();
    }
    else
    {
        if ( pLastState && !IsInvalidItem( pLastState ) )
            delete pLastState;
        pLastState = pState;
    }

    ::cppu::OInterfaceContainerHelper* pContnr = pDispatch->GetListeners().getContainer ( aDispatchURL.Complete );
	if ( bNotify && pContnr )
	{
        ::com::sun::star::uno::Any aState;
        if ( ( eState >= SFX_ITEM_AVAILABLE ) && pState && !IsInvalidItem( pState ) && !pState->ISA(SfxVoidItem) )
        {
            // Retrieve metric from pool to have correct sub ID when calling QueryValue
            USHORT     nSubId( 0 );
            SfxMapUnit eMapUnit( SFX_MAPUNIT_100TH_MM );

            // retrieve the core metric
            // it's enough to check the objectshell, the only shell that does not use the pool of the document
            // is SfxViewFrame, but it hasn't any metric parameters
            // TODO/LATER: what about the FormShell? Does it use any metric data?! Perhaps it should use the Pool of the document!
            if ( pSlotServ && pDispatcher )
            {
                SfxShell* pShell = pDispatcher->GetShell( pSlotServ->GetShellLevel() );
                DBG_ASSERT( pShell, "Can't get core metric without shell!" );
                if ( pShell )
                    eMapUnit = GetCoreMetric( pShell->GetPool(), nSID );
            }

            if ( eMapUnit == SFX_MAPUNIT_TWIP )
                nSubId |= CONVERT_TWIPS;

            pState->QueryValue( aState, (BYTE)nSubId );
        }
        else if ( eState == SFX_ITEM_DONTCARE )
        {
            // Use special uno struct to transport don't care state
            ::com::sun::star::frame::status::ItemStatus aItemStatus;
            aItemStatus.State = ::com::sun::star::frame::status::ItemState::dont_care;
            aState = makeAny( aItemStatus );
        }

		::com::sun::star::frame::FeatureStateEvent aEvent;
		aEvent.FeatureURL = aDispatchURL;
        aEvent.Source = (::com::sun::star::frame::XDispatch*) pDispatch;
		aEvent.IsEnabled = eState != SFX_ITEM_DISABLED;
		aEvent.Requery = sal_False;
		aEvent.State = aState;

		::cppu::OInterfaceIteratorHelper aIt( *pContnr );
		while( aIt.hasMoreElements() )
        {
            try
            {
                ((::com::sun::star::frame::XStatusListener *)aIt.next())->statusChanged( aEvent );
            }
            catch( ::com::sun::star::uno::RuntimeException& )
            {
                aIt.remove();
            }
        }
	}
}

void SfxDispatchController_Impl::StateChanged( sal_uInt16 nSID, SfxItemState eState, const SfxPoolItem* pState )
{
    StateChanged( nSID, eState, pState, 0 );
}
