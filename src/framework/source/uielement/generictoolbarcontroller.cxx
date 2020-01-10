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
#include "precompiled_framework.hxx"

#ifndef __FRAMEWORK_UIELEMENT_GENERICTOOLBARCONTROLLER_HXX
#include "uielement/generictoolbarcontroller.hxx"
#endif

//_________________________________________________________________________________________________________________
//  my own includes
//_________________________________________________________________________________________________________________

#ifndef __FRAMEWORK_TOOLBAR_HXX_
#include "uielement/toolbar.hxx"
#endif

//_________________________________________________________________________________________________________________
//  interface includes
//_________________________________________________________________________________________________________________
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/frame/status/ItemStatus.hpp>
#include <com/sun/star/frame/status/ItemState.hpp>
#include <com/sun/star/frame/status/Visibility.hpp>

//_________________________________________________________________________________________________________________
//  other includes
//_________________________________________________________________________________________________________________
#include <svtools/toolboxcontroller.hxx>
#include <vos/mutex.hxx>
#include <vcl/svapp.hxx>
#ifndef _VCL_MNEMONIC_HXX_
#include <vcl/mnemonic.hxx>
#endif
#include <tools/urlobj.hxx>
#include <classes/resource.hrc>
#include <classes/fwkresid.hxx>
#include <dispatch/uieventloghelper.hxx>

using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::frame::status;
using namespace ::com::sun::star::util;

namespace framework
{

static sal_Bool isEnumCommand( const rtl::OUString& rCommand )
{
    INetURLObject aURL( rCommand );

    if (( aURL.GetProtocol() == INET_PROT_UNO ) &&
        ( aURL.GetURLPath().indexOf( '.' ) != -1))
        return sal_True;

    return sal_False;
}

static rtl::OUString getEnumCommand( const rtl::OUString& rCommand )
{
    INetURLObject aURL( rCommand );

    rtl::OUString   aEnumCommand;
    String          aURLPath = aURL.GetURLPath();
    xub_StrLen      nIndex   = aURLPath.Search( '.' );
    if (( nIndex > 0 ) && ( nIndex < aURLPath.Len() ))
        aEnumCommand = aURLPath.Copy( nIndex+1 );

    return aEnumCommand;
}

static rtl::OUString getMasterCommand( const rtl::OUString& rCommand )
{
    rtl::OUString aMasterCommand( rCommand );
    INetURLObject aURL( rCommand );
    if ( aURL.GetProtocol() == INET_PROT_UNO )
    {
        sal_Int32 nIndex = aURL.GetURLPath().indexOf( '.' );
        if ( nIndex )
        {
            aURL.SetURLPath( aURL.GetURLPath().copy( 0, nIndex ) );
            aMasterCommand = aURL.GetMainURL( INetURLObject::NO_DECODE );
        }
    }
    return aMasterCommand;
}

struct ExecuteInfo
{
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch >     xDispatch;
    ::com::sun::star::util::URL                                                aTargetURL;
    ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >  aArgs;
};

GenericToolbarController::GenericToolbarController( const Reference< XMultiServiceFactory >& rServiceManager,
                                                    const Reference< XFrame >&               rFrame,
                                                    ToolBox*                                 pToolbar,
                                                    USHORT                                   nID,
                                                    const ::rtl::OUString&                          aCommand ) :
    svt::ToolboxController( rServiceManager, rFrame, aCommand )
    ,   m_pToolbar( pToolbar )
    ,   m_nID( nID )
    ,   m_bEnumCommand( isEnumCommand( aCommand ))
    ,   m_bMadeInvisible( sal_False )
    ,   m_aEnumCommand( getEnumCommand( aCommand ))
{
    if ( m_bEnumCommand )
        addStatusListener( getMasterCommand( aCommand ) );
}

GenericToolbarController::~GenericToolbarController()
{
}

void SAL_CALL GenericToolbarController::dispose()
throw ( RuntimeException )
{
    vos::OGuard aSolarMutexGuard( Application::GetSolarMutex() );

    svt::ToolboxController::dispose();

    m_pToolbar = 0;
    m_nID = 0;
}

void SAL_CALL GenericToolbarController::execute( sal_Int16 KeyModifier )
throw ( RuntimeException )
{
    Reference< XDispatch >       xDispatch;
    Reference< XURLTransformer > xURLTransformer;
    ::rtl::OUString                     aCommandURL;

    {
        vos::OGuard aSolarMutexGuard( Application::GetSolarMutex() );

        if ( m_bDisposed )
            throw DisposedException();

        if ( m_bInitialized &&
             m_xFrame.is() &&
             m_xServiceManager.is() &&
             m_aCommandURL.getLength() )
        {
            xURLTransformer = Reference< XURLTransformer >( m_xServiceManager->createInstance(
                                                                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.util.URLTransformer" ))),
                                                            UNO_QUERY );

            aCommandURL = m_aCommandURL;
            URLToDispatchMap::iterator pIter = m_aListenerMap.find( m_aCommandURL );
            if ( pIter != m_aListenerMap.end() )
                xDispatch = pIter->second;
        }
    }

    if ( xDispatch.is() && xURLTransformer.is() )
    {
        com::sun::star::util::URL aTargetURL;
        Sequence<PropertyValue>   aArgs( 1 );

        // Add key modifier to argument list
        aArgs[0].Name  = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "KeyModifier" ));
        aArgs[0].Value <<= KeyModifier;

        aTargetURL.Complete = aCommandURL;
        xURLTransformer->parseStrict( aTargetURL );

        // Execute dispatch asynchronously
        ExecuteInfo* pExecuteInfo = new ExecuteInfo;
        pExecuteInfo->xDispatch     = xDispatch;
        pExecuteInfo->aTargetURL    = aTargetURL;
        pExecuteInfo->aArgs         = aArgs;
        if(::comphelper::UiEventsLogger::isEnabled()) //#i88653#
            UiEventLogHelper(::rtl::OUString::createFromAscii("GenericToolbarController")).log( m_xServiceManager, m_xFrame, aTargetURL, aArgs);
        Application::PostUserEvent( STATIC_LINK(0, GenericToolbarController , ExecuteHdl_Impl), pExecuteInfo );
    }
}

void GenericToolbarController::statusChanged( const FeatureStateEvent& Event )
throw ( RuntimeException )
{
    vos::OGuard aSolarMutexGuard( Application::GetSolarMutex() );

    if ( m_bDisposed )
        return;

    if ( m_pToolbar )
    {
        m_pToolbar->EnableItem( m_nID, Event.IsEnabled );

        USHORT nItemBits = m_pToolbar->GetItemBits( m_nID );
        nItemBits &= ~TIB_CHECKABLE;
        TriState eTri = STATE_NOCHECK;

        sal_Bool        bValue = sal_Bool();
        rtl::OUString   aStrValue;
        ItemStatus      aItemState;
        Visibility      aItemVisibility;

        if (( Event.State >>= bValue ) && !m_bEnumCommand )
        {
            // Boolean, treat it as checked/unchecked
            if ( m_bMadeInvisible )
                m_pToolbar->ShowItem( m_nID, TRUE );
            m_pToolbar->CheckItem( m_nID, bValue );
            if ( bValue )
                eTri = STATE_CHECK;
            nItemBits |= TIB_CHECKABLE;
        }
        else if ( Event.State >>= aStrValue )
        {
            if ( m_bEnumCommand )
            {
                if ( aStrValue == m_aEnumCommand )
                    bValue = sal_True;
                else
                    bValue = sal_False;

                m_pToolbar->CheckItem( m_nID, bValue );
                if ( bValue )
                    eTri = STATE_CHECK;
                nItemBits |= TIB_CHECKABLE;
            }
            else
            {
                // Replacement for place holders
                if ( aStrValue.matchAsciiL( "($1)", 4 ))
                {
				    String aResStr = String( FwkResId( STR_UPDATEDOC ));
                    rtl::OUString aTmp( aResStr );
                    aTmp += rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( " " ));
                    aTmp += aStrValue.copy( 4 );
                    aStrValue = aTmp;
                }
                else if ( aStrValue.matchAsciiL( "($2)", 4 ))
                {
				    String aResStr = String( FwkResId( STR_CLOSEDOC_ANDRETURN ));
                    rtl::OUString aTmp( aResStr );
                    aTmp += aStrValue.copy( 4 );
                    aStrValue = aTmp;
                }
                else if ( aStrValue.matchAsciiL( "($3)", 4 ))
                {
				    String aResStr = String( FwkResId( STR_SAVECOPYDOC ));
                    rtl::OUString aTmp( aResStr );
                    aTmp += aStrValue.copy( 4 );
                    aStrValue = aTmp;
                }
                ::rtl::OUString aText( MnemonicGenerator::EraseAllMnemonicChars( aStrValue ) );
                m_pToolbar->SetItemText( m_nID, aText );
                m_pToolbar->SetQuickHelpText( m_nID, aText );
            }

            if ( m_bMadeInvisible )
                m_pToolbar->ShowItem( m_nID, TRUE );
        }
        else if (( Event.State >>= aItemState ) && !m_bEnumCommand )
        {
            eTri = STATE_DONTKNOW;
            nItemBits |= TIB_CHECKABLE;
            if ( m_bMadeInvisible )
                m_pToolbar->ShowItem( m_nID, TRUE );
        }
        else if ( Event.State >>= aItemVisibility )
        {
            m_pToolbar->ShowItem( m_nID, aItemVisibility.bVisible );
            m_bMadeInvisible = !aItemVisibility.bVisible;
        }
        else if ( m_bMadeInvisible )
            m_pToolbar->ShowItem( m_nID, TRUE );

        m_pToolbar->SetItemState( m_nID, eTri );
        m_pToolbar->SetItemBits( m_nID, nItemBits );
    }
}

IMPL_STATIC_LINK_NOINSTANCE( GenericToolbarController, ExecuteHdl_Impl, ExecuteInfo*, pExecuteInfo )
{
   const sal_uInt32 nRef = Application::ReleaseSolarMutex();
   try
   {
        // Asynchronous execution as this can lead to our own destruction!
        // Framework can recycle our current frame and the layout manager disposes all user interface
        // elements if a component gets detached from its frame!
        pExecuteInfo->xDispatch->dispatch( pExecuteInfo->aTargetURL, pExecuteInfo->aArgs );
   }
   catch ( Exception& )
   {
   }

   Application::AcquireSolarMutex( nRef );
   delete pExecuteInfo;
   return 0;
}

} // namespace

