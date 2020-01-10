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
#include "precompiled_chart2.hxx"

#include "AccessibleBase.hxx"
#include "ObjectHierarchy.hxx"
#include "ObjectIdentifier.hxx"
#include "chartview/ExplicitValueProvider.hxx"
#include "macros.hxx"

#include <com/sun/star/awt/XDevice.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleEventObject.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>

#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <rtl/ustrbuf.hxx>
// for SolarMutex
#include <vcl/svapp.hxx>
#include <rtl/uuid.h>
#include <cppuhelper/queryinterface.hxx>
#include <svtools/itemset.hxx>
#include <svx/unofdesc.hxx>
#include <svx/outliner.hxx>
#include <svx/svdoutl.hxx>
#include <svx/svdetc.hxx>
#include <svx/unoshape.hxx>
#include <svx/unoprov.hxx>
#include <vcl/unohelp.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/window.hxx>

#include <algorithm>

#include "ChartElementFactory.hxx"

using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;

using ::com::sun::star::uno::UNO_QUERY;
using ::rtl::OUString;
using ::rtl::OUStringBuffer;
using ::com::sun::star::uno::Reference;
using ::osl::MutexGuard;
using ::osl::ClearableMutexGuard;
using ::osl::ResettableMutexGuard;
using ::com::sun::star::uno::RuntimeException;
using ::com::sun::star::uno::Any;

namespace chart
{

/** @param bMayHaveChildren is false per default
 */
AccessibleBase::AccessibleBase(
    const AccessibleElementInfo & rAccInfo,
    bool bMayHaveChildren,
    bool bAlwaysTransparent /* default: false */ ) :
        impl::AccessibleBase_Base( m_aMutex ),
        m_bIsDisposed( false ),
        m_bMayHaveChildren( bMayHaveChildren ),
        m_bChildrenInitialized( false ),
        m_nEventNotifierId(0),
        m_pStateSetHelper( new ::utl::AccessibleStateSetHelper() ),
        m_aStateSet( m_pStateSetHelper ),
        m_aAccInfo( rAccInfo ),
        m_bAlwaysTransparent( bAlwaysTransparent ),
        m_bStateSetInitialized( false )
{
    // initialize some states
    OSL_ASSERT( m_pStateSetHelper );
    m_pStateSetHelper->AddState( AccessibleStateType::ENABLED );
    m_pStateSetHelper->AddState( AccessibleStateType::SHOWING );
    m_pStateSetHelper->AddState( AccessibleStateType::VISIBLE );
    m_pStateSetHelper->AddState( AccessibleStateType::SELECTABLE );
    m_pStateSetHelper->AddState( AccessibleStateType::FOCUSABLE );
}

AccessibleBase::~AccessibleBase()
{
    OSL_ASSERT( m_bIsDisposed );
}

// ________ public ________

bool AccessibleBase::CheckDisposeState( bool bThrowException /* default: true */ ) const
    throw (lang::DisposedException)
{
    if( bThrowException &&
        m_bIsDisposed )
    {
        throw lang::DisposedException(
            C2U("component has state DEFUNC" ),
            static_cast< uno::XWeak * >( const_cast< AccessibleBase * >( this )));
    }
    return m_bIsDisposed;
}

bool AccessibleBase::NotifyEvent( EventType eEventType, const AccessibleUniqueId & rId )
{
    if( GetId() == rId )
    {
        // event is addressed to this object

        ::com::sun::star::uno::Any aEmpty;
        ::com::sun::star::uno::Any aSelected;
        aSelected <<= AccessibleStateType::SELECTED;
        switch( eEventType )
        {
            case OBJECT_CHANGE:
                {
                    BroadcastAccEvent( AccessibleEventId::VISIBLE_DATA_CHANGED, aEmpty, aEmpty );
#if OSL_DEBUG_LEVEL > 1
                    OSL_TRACE(
                        ::rtl::OUStringToOString(
                            OUString( RTL_CONSTASCII_USTRINGPARAM(
                                          "Visible data event sent by: " )) +
                            getAccessibleName(),
                            RTL_TEXTENCODING_ASCII_US ).getStr() );
#endif
                }
                break;

            case GOT_SELECTION:
                {
                    AddState( AccessibleStateType::SELECTED );
                    BroadcastAccEvent( AccessibleEventId::STATE_CHANGED, aSelected, aEmpty );

                    AddState( AccessibleStateType::FOCUSED );
                    aSelected <<= AccessibleStateType::FOCUSED;
                    BroadcastAccEvent( AccessibleEventId::STATE_CHANGED, aSelected, aEmpty, true );
#if OSL_DEBUG_LEVEL > 1
                    OSL_TRACE(
                        ::rtl::OUStringToOString(
                            OUString( RTL_CONSTASCII_USTRINGPARAM(
                                          "Selection acquired by: " )) +
                            getAccessibleName(),
                            RTL_TEXTENCODING_ASCII_US ).getStr() );
#endif
                }
                break;

            case LOST_SELECTION:
                {
                    RemoveState( AccessibleStateType::SELECTED );
                    BroadcastAccEvent( AccessibleEventId::STATE_CHANGED, aEmpty, aSelected );

                    AddState( AccessibleStateType::FOCUSED );
                    aSelected <<= AccessibleStateType::FOCUSED;
                    BroadcastAccEvent( AccessibleEventId::STATE_CHANGED, aEmpty, aSelected, true );
#if OSL_DEBUG_LEVEL > 1
                    OSL_TRACE(
                        ::rtl::OUStringToOString(
                            OUString( RTL_CONSTASCII_USTRINGPARAM(
                                          "Selection lost by: " )) +
                            getAccessibleName(),
                            RTL_TEXTENCODING_ASCII_US ).getStr() );
#endif
                }
                break;

            case PROPERTY_CHANGE:
                {
                    //not implemented --> rebuild all
                }
                break;
        }
        return true;
    }
    else if( m_bMayHaveChildren )
    {
        bool bStop = false;
        // /--
        ClearableMutexGuard aGuard( GetMutex() );
        // make local copy for notification
        ChildListVectorType aLocalChildList( m_aChildList );
        aGuard.clear();
        // \--

        ChildListVectorType::iterator aEndIter = aLocalChildList.end();
        for( ChildListVectorType::iterator aIter = aLocalChildList.begin() ;
             ( aIter != aEndIter ) && ( ! bStop ) ;
             ++aIter )
        {
            // Note: at this place we must be sure to have an AccessibleBase
            // object in the UNO reference to XAccessible !
            bStop = (*static_cast< AccessibleBase * >
                     ( (*aIter).get() )).NotifyEvent( eEventType, rId );
        }
        return bStop;
    }

    return false;
}

void AccessibleBase::AddState( sal_Int16 aState )
    throw (RuntimeException)
{
    CheckDisposeState();
    OSL_ASSERT( m_pStateSetHelper );
    m_pStateSetHelper->AddState( aState );
}

void AccessibleBase::RemoveState( sal_Int16 aState )
    throw (RuntimeException)
{
    CheckDisposeState();
    OSL_ASSERT( m_pStateSetHelper );
    m_pStateSetHelper->RemoveState( aState );
}

// ________ protected ________

bool AccessibleBase::UpdateChildren()
{
    bool bMustUpdateChildren = false;
    {
        // /--
        MutexGuard aGuard( GetMutex() );
        if( ! m_bMayHaveChildren ||
            m_bIsDisposed )
            return false;

        bMustUpdateChildren = ( m_bMayHaveChildren &&
                                ! m_bChildrenInitialized );
        // \--
    }

    // update unguarded
    if( bMustUpdateChildren )
        m_bChildrenInitialized = ImplUpdateChildren();

    return m_bChildrenInitialized;
}

bool AccessibleBase::ImplUpdateChildren()
{
    bool bResult = false;

    if( m_aAccInfo.m_spObjectHierarchy )
    {
        ObjectHierarchy::tChildContainer aModelChildren(
            m_aAccInfo.m_spObjectHierarchy->getChildren( GetId() ));
        ::std::vector< ChildCIDMap::key_type > aAccChildren;
        aAccChildren.reserve( aModelChildren.size());
        ::std::transform( m_aChildCIDMap.begin(), m_aChildCIDMap.end(),
                          ::std::back_inserter( aAccChildren ),
                          ::std::select1st< ChildCIDMap::value_type >());

        ::std::sort( aModelChildren.begin(), aModelChildren.end());

        ::std::vector< OUString > aChildrenToRemove, aChildrenToAdd;
        ::std::set_difference( aModelChildren.begin(), aModelChildren.end(),
                               aAccChildren.begin(), aAccChildren.end(),
                               ::std::back_inserter( aChildrenToAdd ));
        ::std::set_difference( aAccChildren.begin(), aAccChildren.end(),
                               aModelChildren.begin(), aModelChildren.end(),
                               ::std::back_inserter( aChildrenToRemove ));

        ::std::vector< OUString >::const_iterator aIt( aChildrenToRemove.begin());
        for( ; aIt != aChildrenToRemove.end(); ++aIt )
        {
            RemoveChildById( *aIt );
        }

        AccessibleElementInfo aAccInfo( GetInfo());
        aAccInfo.m_pParent = this;

        for( aIt = aChildrenToAdd.begin(); aIt != aChildrenToAdd.end(); ++aIt )
        {
            aAccInfo.m_aCID = *aIt;
            AddChild( ChartElementFactory::CreateChartElement( aAccInfo ));
        }
        bResult = true;
    }

    return bResult;
}

void AccessibleBase::AddChild( AccessibleBase * pChild  )
{
    OSL_ENSURE( pChild != NULL, "Invalid Child" );
    if( pChild )
    {
        // /--
        ClearableMutexGuard aGuard( GetMutex() );

        Reference< XAccessible > xChild( pChild );
        m_aChildList.push_back( xChild );

        m_aChildCIDMap[ pChild->GetId() ] = xChild;

        // inform listeners of new child
        if( m_bChildrenInitialized )
        {
            Any aEmpty, aNew;
            aNew <<= xChild;

            aGuard.clear();
            // \-- (1st)
            BroadcastAccEvent( AccessibleEventId::CHILD, aNew, aEmpty );
        }
        // \-- (2nd)
    }
}

/** in this method we imply that the Reference< XAccessible > elements in the
    vector are AccessibleBase objects !
 */
void AccessibleBase::RemoveChildById( const ::rtl::OUString & rId )
{
    // /--
    ClearableMutexGuard aGuard( GetMutex() );

    ChildCIDMap::iterator aIt( m_aChildCIDMap.find( rId ));
    if( aIt != m_aChildCIDMap.end())
    {
        Reference< XAccessible > xChild( aIt->second );

        // remove from map
        m_aChildCIDMap.erase( aIt );

        // search child in vector
        ChildListVectorType::iterator aVecIter =
            ::std::find( m_aChildList.begin(), m_aChildList.end(), xChild );

        OSL_ENSURE( aVecIter != m_aChildList.end(),
                    "Inconsistent ChildMap" );

        // remove child from vector
        m_aChildList.erase( aVecIter );
        bool bInitialized = m_bChildrenInitialized;

        // call listeners unguarded
        aGuard.clear();
        // \-- (1st)

        // inform listeners of removed child
        if( bInitialized )
        {
            Any aEmpty, aOld;
            aOld <<= xChild;

            BroadcastAccEvent( AccessibleEventId::CHILD, aEmpty, aOld );
        }

        // dispose the child
        Reference< lang::XComponent > xComp( xChild, UNO_QUERY );
        if( xComp.is())
            xComp->dispose();
    }
}

awt::Point AccessibleBase::GetUpperLeftOnScreen() const
{
    awt::Point aResult;
    if( m_aAccInfo.m_pParent )
    {
        // /--
        ClearableMutexGuard aGuard( GetMutex() );
        AccessibleBase * pParent = m_aAccInfo.m_pParent;
        aGuard.clear();
        // \--

        if( pParent )
        {
            aResult = pParent->GetUpperLeftOnScreen();
        }
        else
            OSL_ENSURE( false, "Default position used is probably incorrect." );
    }

    return aResult;
}

void AccessibleBase::BroadcastAccEvent(
    sal_Int16 nId,
    const Any & rNew,
    const Any & rOld,
    bool bSendGlobally ) const
{
    // /--
    ClearableMutexGuard aGuard( GetMutex() );

    if ( !m_nEventNotifierId && !bSendGlobally )
        return;
        // if we don't have a client id for the notifier, then we don't have listeners, then
        // we don't need to notify anything
        //except SendGlobally for focus handling?

    // the const cast is needed, because UNO parameters are never const
    const AccessibleEventObject aEvent(
        const_cast< uno::XWeak * >( static_cast< const uno::XWeak * >( this )),
        nId, rNew, rOld );

    if ( m_nEventNotifierId ) // let the notifier handle this event
        ::comphelper::AccessibleEventNotifier::addEvent( m_nEventNotifierId, aEvent );

    aGuard.clear();
    // \--

    // send event to global message queue
    if( bSendGlobally )
    {
        ::vcl::unohelper::NotifyAccessibleStateEventGlobally( aEvent );
    }
}

void AccessibleBase::KillAllChildren()
{
    // /--
    ClearableMutexGuard aGuard( GetMutex() );

    // make local copy for notification
    ChildListVectorType aLocalChildList( m_aChildList );

    // remove all children
    m_aChildList.clear();
    m_aChildCIDMap.clear();

    aGuard.clear();
    // \--

    // call dispose for all children
    // and notify listeners
    Reference< lang::XComponent > xComp;
    Any aEmpty, aOld;
    ChildListVectorType::const_iterator aEndIter = aLocalChildList.end();
    for( ChildListVectorType::const_iterator aIter = aLocalChildList.begin();
         aIter != aEndIter; ++aIter )
    {
        aOld <<= (*aIter);
        BroadcastAccEvent( AccessibleEventId::CHILD, aEmpty, aOld );

        xComp.set( *aIter, UNO_QUERY );
        if( xComp.is())
            xComp->dispose();
    }
    m_bChildrenInitialized = false;
}

AccessibleElementInfo AccessibleBase::GetInfo() const
{
    return m_aAccInfo;
}

void AccessibleBase::SetInfo( const AccessibleElementInfo & rNewInfo )
{
    m_aAccInfo = rNewInfo;
    if( m_bMayHaveChildren )
    {
        KillAllChildren();
    }
    BroadcastAccEvent( AccessibleEventId::INVALIDATE_ALL_CHILDREN, uno::Any(), uno::Any(),
                       true /* global notification */ );
}

AccessibleUniqueId AccessibleBase::GetId() const
{
    return m_aAccInfo.m_aCID;
}

// ____________________________________
// ____________________________________
//
//             Interfaces
// ____________________________________
// ____________________________________

// ________ (XComponent::dispose) ________
void SAL_CALL AccessibleBase::disposing()
{
    // /--
    ClearableMutexGuard aGuard( GetMutex() );
    OSL_ENSURE( ! m_bIsDisposed, "dispose() called twice" );

    // notify disposing to all AccessibleEvent listeners asynchron
    if ( m_nEventNotifierId )
    {
        ::comphelper::AccessibleEventNotifier::revokeClientNotifyDisposing( m_nEventNotifierId, *this );
        m_nEventNotifierId = 0;
    }

    // reset pointers
    m_aAccInfo.m_pParent = NULL;

    // invalidate implementation for helper, but keep UNO reference to still
    // allow a tool to query the DEFUNC state.
    // Note: The object will be deleted when the last reference is released
    m_pStateSetHelper = NULL;

    // attach new empty state set helper to member reference
    ::utl::AccessibleStateSetHelper * pHelper = new ::utl::AccessibleStateSetHelper();
    pHelper->AddState( AccessibleStateType::DEFUNC );
    // release old helper and attach new one
    m_aStateSet.set( pHelper );

    m_bIsDisposed = true;

    // call listeners unguarded
    aGuard.clear();
    // \--

    if( m_bMayHaveChildren )
    {
        KillAllChildren();
    }
    else
        OSL_ENSURE( m_aChildList.empty(), "Child list should be empty" );
}

// ________ XAccessible ________
Reference< XAccessibleContext > SAL_CALL AccessibleBase::getAccessibleContext()
    throw (RuntimeException)
{
    return this;
}

// ________ AccessibleBase::XAccessibleContext ________
sal_Int32 SAL_CALL AccessibleBase::getAccessibleChildCount()
    throw (RuntimeException)
{
    // /--
    ClearableMutexGuard aGuard( GetMutex() );
    if( ! m_bMayHaveChildren ||
        m_bIsDisposed )
        return 0;

    bool bMustUpdateChildren = ( m_bMayHaveChildren &&
                                 ! m_bChildrenInitialized );

    aGuard.clear();
    // \--

    // update unguarded
    if( bMustUpdateChildren )
        UpdateChildren();

    return ImplGetAccessibleChildCount();
}

sal_Int32 AccessibleBase::ImplGetAccessibleChildCount() const
    throw (RuntimeException)
{
    return m_aChildList.size();
}

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleChild( sal_Int32 i )
    throw (lang::IndexOutOfBoundsException, RuntimeException)
{
    CheckDisposeState();
    Reference< XAccessible > xResult;

    // /--
    ResettableMutexGuard aGuard( GetMutex() );
    bool bMustUpdateChildren = ( m_bMayHaveChildren &&
                                 ! m_bChildrenInitialized );

    aGuard.clear();
    // \--

    if( bMustUpdateChildren )
        UpdateChildren();

    xResult.set( ImplGetAccessibleChildById( i ));

    return xResult;
    // \--
}

Reference< XAccessible > AccessibleBase::ImplGetAccessibleChildById( sal_Int32 i ) const
    throw (lang::IndexOutOfBoundsException, RuntimeException)
{
    Reference< XAccessible > xResult;
    // /--
    MutexGuard aGuard( GetMutex());
    if( ! m_bMayHaveChildren ||
        i < 0 ||
        static_cast< ChildListVectorType::size_type >( i ) >= m_aChildList.size() )
    {
        OUStringBuffer aBuf;
        aBuf.appendAscii( RTL_CONSTASCII_STRINGPARAM( "Index " ));
        aBuf.append( i );
        aBuf.appendAscii( RTL_CONSTASCII_STRINGPARAM( " is invalid for range [ 0, " ));
        aBuf.append( static_cast< sal_Int32 >( m_aChildList.size() - 1 ) );
        aBuf.appendAscii( RTL_CONSTASCII_STRINGPARAM( " ]" ) );
        lang::IndexOutOfBoundsException aEx( aBuf.makeStringAndClear(),
                                             const_cast< ::cppu::OWeakObject * >(
                                                 static_cast< const ::cppu::OWeakObject * >( this )));
        throw aEx;
    }
    else
        xResult.set( m_aChildList[ i ] );

    return xResult;
}

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleParent()
    throw (RuntimeException)
{
    CheckDisposeState();
    Reference< XAccessible > aResult;
    if( m_aAccInfo.m_pParent )
        aResult.set( m_aAccInfo.m_pParent );

    return aResult;
}

sal_Int32 SAL_CALL AccessibleBase::getAccessibleIndexInParent()
    throw (RuntimeException)
{
    CheckDisposeState();

    if( m_aAccInfo.m_spObjectHierarchy )
        return m_aAccInfo.m_spObjectHierarchy->getIndexInParent( GetId() );
    return -1;
}

sal_Int16 SAL_CALL AccessibleBase::getAccessibleRole()
    throw (RuntimeException)
{
    return AccessibleRole::LIST_ITEM; // #i73747# role SHAPE seems more appropriate, but is not read
}

Reference< XAccessibleRelationSet > SAL_CALL AccessibleBase::getAccessibleRelationSet()
    throw (RuntimeException)
{
    Reference< XAccessibleRelationSet > aResult;
    return aResult;
}

Reference< XAccessibleStateSet > SAL_CALL AccessibleBase::getAccessibleStateSet()
    throw (RuntimeException)
{
    if( ! m_bStateSetInitialized )
    {
        OUString aSelCID;
        Reference< view::XSelectionSupplier > xSelSupp( GetInfo().m_xSelectionSupplier );
        if( xSelSupp.is() &&
            ( xSelSupp->getSelection() >>= aSelCID ) &&
            GetId().equals( aSelCID ) )
        {
            AddState( AccessibleStateType::SELECTED );
            AddState( AccessibleStateType::FOCUSED );
        }
        m_bStateSetInitialized = true;
    }

    return m_aStateSet;
}


lang::Locale SAL_CALL AccessibleBase::getLocale()
    throw (IllegalAccessibleComponentStateException, RuntimeException)
{
    CheckDisposeState();

    return Application::GetSettings().GetLocale();
}

// ________ AccessibleBase::XAccessibleComponent ________
sal_Bool SAL_CALL AccessibleBase::containsPoint( const awt::Point& aPoint )
    throw (RuntimeException)
{
    awt::Rectangle aRect( getBounds() );

    // contains() works with relative coordinates
    aRect.X = 0;
    aRect.Y = 0;

    return ( aPoint.X >= aRect.X &&
             aPoint.Y >= aRect.Y &&
             aPoint.X < (aRect.X + aRect.Width) &&
             aPoint.Y < (aRect.Y + aRect.Height) );
}

Reference< XAccessible > SAL_CALL AccessibleBase::getAccessibleAtPoint( const awt::Point& aPoint )
    throw (RuntimeException)
{
    CheckDisposeState();
    Reference< XAccessible > aResult;
    awt::Rectangle aRect( getBounds());

    // children are positioned relative to this object, so translate bound rect
    aRect.X = 0;
    aRect.Y = 0;

    // children must be inside the own bound rect
    if( ( aRect.X <= aPoint.X && aPoint.X <= (aRect.X + aRect.Width) ) &&
        ( aRect.Y <= aPoint.Y && aPoint.Y <= (aRect.Y + aRect.Height)))
    {
        // /--
        ClearableMutexGuard aGuard( GetMutex() );
        ChildListVectorType aLocalChildList( m_aChildList );
        aGuard.clear();
        // \--

        Reference< XAccessibleComponent > aComp;
        for( ChildListVectorType::const_iterator aIter = aLocalChildList.begin();
             aIter != aLocalChildList.end(); ++aIter )
        {
            aComp.set( *aIter, UNO_QUERY );
            if( aComp.is())
            {
                aRect = aComp->getBounds();
                if( ( aRect.X <= aPoint.X && aPoint.X <= (aRect.X + aRect.Width) ) &&
                    ( aRect.Y <= aPoint.Y && aPoint.Y <= (aRect.Y + aRect.Height)))
                {
                    aResult = (*aIter);
                    break;
                }
            }
        }
    }

    return aResult;
}

awt::Rectangle SAL_CALL AccessibleBase::getBounds()
    throw (RuntimeException)
{
    ExplicitValueProvider *pExplicitValueProvider(
        ExplicitValueProvider::getExplicitValueProvider( m_aAccInfo.m_xView ));
    if( pExplicitValueProvider )
    {
        Window* pWindow( VCLUnoHelper::GetWindow( m_aAccInfo.m_xWindow ));
        awt::Rectangle aLogicRect( pExplicitValueProvider->getRectangleOfObject( m_aAccInfo.m_aCID ));
        if( pWindow )
        {
            Rectangle aRect( aLogicRect.X, aLogicRect.Y,
                             aLogicRect.X + aLogicRect.Width,
                             aLogicRect.Y + aLogicRect.Height );
            // /-- solar
            ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
            aRect = pWindow->LogicToPixel( aRect );

            // aLogicRect ist relative to the page, but we need a value relative
            // to the parent object
            awt::Point aParentLocOnScreen;
            uno::Reference< XAccessibleComponent > xParent( getAccessibleParent(), uno::UNO_QUERY );
            if( xParent.is() )
                aParentLocOnScreen = xParent->getLocationOnScreen();

            // aOffset = aParentLocOnScreen - GetUpperLeftOnScreen()
            awt::Point aULOnScreen = GetUpperLeftOnScreen();
            awt::Point aOffset( aParentLocOnScreen.X - aULOnScreen.X,
                                aParentLocOnScreen.Y - aULOnScreen.Y );

            return awt::Rectangle( aRect.getX() - aOffset.X, aRect.getY() - aOffset.Y,
                                   aRect.getWidth(), aRect.getHeight());
            // \-- solar
        }
	}

    return awt::Rectangle();
}

awt::Point SAL_CALL AccessibleBase::getLocation()
    throw (RuntimeException)
{
    CheckDisposeState();
    awt::Rectangle aBBox( getBounds() );
    return awt::Point( aBBox.X, aBBox.Y );
}

awt::Point SAL_CALL AccessibleBase::getLocationOnScreen()
    throw (RuntimeException)
{
    CheckDisposeState();

    if( m_aAccInfo.m_pParent != NULL )
    {
        AccessibleBase * pParent = m_aAccInfo.m_pParent;
        awt::Point aLocThisRel( getLocation());
        awt::Point aUpperLeft;

        if( pParent != NULL )
            aUpperLeft = pParent->getLocationOnScreen();

        return  awt::Point( aUpperLeft.X + aLocThisRel.X,
                            aUpperLeft.Y + aLocThisRel.Y );
    }
    else
        return getLocation();
}

awt::Size SAL_CALL AccessibleBase::getSize()
    throw (RuntimeException)
{
    CheckDisposeState();
    awt::Rectangle aBBox( getBounds() );
    return awt::Size( aBBox.Width, aBBox.Height );
}

void SAL_CALL AccessibleBase::grabFocus()
    throw (RuntimeException)
{
    CheckDisposeState();

    OUString aSelCID;
    Reference< view::XSelectionSupplier > xSelSupp( GetInfo().m_xSelectionSupplier );
    if( xSelSupp.is())
        xSelSupp->select( uno::makeAny( GetId()));
}

sal_Int32 SAL_CALL AccessibleBase::getForeground()
    throw (RuntimeException)
{
    return getColor( ACC_BASE_FOREGROUND );
}

sal_Int32 SAL_CALL AccessibleBase::getBackground()
    throw (RuntimeException)
{
    return getColor( ACC_BASE_BACKGROUND );
}

sal_Int32 AccessibleBase::getColor( eColorType eColType )
{
    sal_Int32 nResult = static_cast< sal_Int32 >( Color( COL_TRANSPARENT ).GetColor());
    if( m_bAlwaysTransparent )
        return nResult;

    ObjectType eType( ObjectIdentifier::getObjectType( m_aAccInfo.m_aCID ));
    Reference< beans::XPropertySet > xObjProp;
    OUString aObjectCID = m_aAccInfo.m_aCID;
    if( eType == OBJECTTYPE_LEGEND_ENTRY )
    {
        // for colors get the data series/point properties
        OUString aParentParticle( ObjectIdentifier::getFullParentParticle( aObjectCID ));
        aObjectCID = ObjectIdentifier::createClassifiedIdentifierForParticle( aParentParticle );
    }

    xObjProp.set(
        ObjectIdentifier::getObjectPropertySet(
            aObjectCID, Reference< chart2::XChartDocument >( m_aAccInfo.m_xChartDocument )), uno::UNO_QUERY );
    if( xObjProp.is())
    {
        try
        {
            OUString aPropName;
            OUString aStylePropName;

            switch( eType )
            {
                case OBJECTTYPE_LEGEND_ENTRY:
                case OBJECTTYPE_DATA_SERIES:
                case OBJECTTYPE_DATA_POINT:
                    if( eColType == ACC_BASE_FOREGROUND )
                    {
                        aPropName = C2U("BorderColor");
                        aStylePropName = C2U("BorderTransparency");
                    }
                    else
                    {
                        aPropName = C2U("Color");
                        aStylePropName = C2U("Transparency");
                    }
                    break;
                default:
                    if( eColType == ACC_BASE_FOREGROUND )
                    {
                        aPropName = C2U("LineColor");
                        aStylePropName = C2U("LineTransparence");
                    }
                    else
                    {
                        aPropName = C2U("FillColor");
                        aStylePropName = C2U("FillTransparence");
                    }
                    break;
            }

            bool bTransparent = m_bAlwaysTransparent;
            Reference< beans::XPropertySetInfo > xInfo( xObjProp->getPropertySetInfo(), uno::UNO_QUERY );
            if( xInfo.is() &&
                xInfo->hasPropertyByName( aStylePropName ))
            {
                if( eColType == ACC_BASE_FOREGROUND )
                {
                    drawing::LineStyle aLStyle;
                    if( xObjProp->getPropertyValue( aStylePropName ) >>= aLStyle )
                        bTransparent = (aLStyle == drawing::LineStyle_NONE);
                }
                else
                {
                    drawing::FillStyle aFStyle;
                    if( xObjProp->getPropertyValue( aStylePropName ) >>= aFStyle )
                        bTransparent = (aFStyle == drawing::FillStyle_NONE);
                }
            }

            if( !bTransparent &&
                xInfo.is() &&
                xInfo->hasPropertyByName( aPropName ))
            {
                xObjProp->getPropertyValue( aPropName ) >>= nResult;
            }
        }
        catch( const uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return nResult;
}

// ________ AccessibleBase::XServiceInfo ________
OUString SAL_CALL AccessibleBase::getImplementationName()
    throw (RuntimeException)
{
    return OUString( RTL_CONSTASCII_USTRINGPARAM( "AccessibleBase" ));
}

sal_Bool SAL_CALL AccessibleBase::supportsService( const OUString& ServiceName )
    throw (RuntimeException)
{
    return SvxServiceInfoHelper::supportsService( ServiceName, getSupportedServiceNames() );
}

uno::Sequence< OUString > SAL_CALL AccessibleBase::getSupportedServiceNames()
    throw (RuntimeException)
{
    uno::Sequence< ::rtl::OUString > aSeq( 2 );
    ::rtl::OUString* pStr = aSeq.getArray();
    pStr[ 0 ] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.accessibility.Accessible" ));
    pStr[ 1 ] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.accessibility.AccessibleContext" ));

    return aSeq;
}

// ________ AccessibleBase::XEventListener ________
void SAL_CALL AccessibleBase::disposing( const lang::EventObject& /*Source*/ )
    throw (RuntimeException)
{
}

// ________ XAccessibleEventBroadcasters ________
void SAL_CALL AccessibleBase::addEventListener( const Reference< XAccessibleEventListener >& xListener )
    throw (RuntimeException)
{
    MutexGuard aGuard( GetMutex() );

    if ( xListener.is() )
    {
        if ( !m_nEventNotifierId )
            m_nEventNotifierId = ::comphelper::AccessibleEventNotifier::registerClient();

        ::comphelper::AccessibleEventNotifier::addEventListener( m_nEventNotifierId, xListener );
    }
}

void SAL_CALL AccessibleBase::removeEventListener( const Reference< XAccessibleEventListener >& xListener )
    throw (RuntimeException)
{
    MutexGuard aGuard( GetMutex() );

    if ( xListener.is() )
    {
        sal_Int32 nListenerCount = ::comphelper::AccessibleEventNotifier::removeEventListener( m_nEventNotifierId, xListener );
        if ( !nListenerCount )
        {
            // no listeners anymore
            ::comphelper::AccessibleEventNotifier::revokeClient( m_nEventNotifierId );
            m_nEventNotifierId = 0;
        }
    }
}

} // namespace chart
