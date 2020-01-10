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

//_______________________________________________
//	my own includes

#ifndef __FRAMEWORK_DISPATCH_INTERCEPTIONHELPER_HXX_
#include <dispatch/interceptionhelper.hxx>
#endif

//_______________________________________________
//	interface includes
#include <com/sun/star/frame/XInterceptorInfo.hpp>

//_______________________________________________
//	includes of other projects
#include <vcl/svapp.hxx>

//_______________________________________________
//	namespace

namespace framework{

//_______________________________________________
//	non exported const

sal_Bool InterceptionHelper::m_bPreferrFirstInterceptor = sal_True;

//_______________________________________________
//	non exported definitions

//_______________________________________________
//	declarations

/*-----------------------------------------------------------------------------
    31.03.2003 09:02
-----------------------------------------------------------------------------*/
DEFINE_XINTERFACE_3(InterceptionHelper                                         ,
                    OWeakObject                                                ,
                    DIRECT_INTERFACE(css::frame::XDispatchProvider            ),
                    DIRECT_INTERFACE(css::frame::XDispatchProviderInterception),
                    DIRECT_INTERFACE(css::lang::XEventListener                ))

/*-----------------------------------------------------------------------------
    31.03.2003 09:02
-----------------------------------------------------------------------------*/
InterceptionHelper::InterceptionHelper(const css::uno::Reference< css::frame::XFrame >&            xOwner,
                                       const css::uno::Reference< css::frame::XDispatchProvider >& xSlave)
    //  Init baseclasses first
    : ThreadHelpBase(&Application::GetSolarMutex())
    , OWeakObject   (                             )
    // Init member
    , m_xOwnerWeak  (xOwner                       )
    , m_xSlave      (xSlave                       )
{
}

/*-----------------------------------------------------------------------------
    31.03.2003 09:02
-----------------------------------------------------------------------------*/
InterceptionHelper::~InterceptionHelper()
{
}

/*-----------------------------------------------------------------------------
    31.03.2003 09:09
-----------------------------------------------------------------------------*/
css::uno::Reference< css::frame::XDispatch > SAL_CALL InterceptionHelper::queryDispatch(const css::util::URL&  aURL            ,
                                                                                        const ::rtl::OUString& sTargetFrameName,
                                                                                              sal_Int32        nSearchFlags    )
    throw(css::uno::RuntimeException)
{
    // SAFE {
    ReadGuard aReadLock(m_aLock);

    // a) first search an interceptor, which match to this URL by it's URL pattern registration
    //    Note: if it return NULL - it does not mean an empty interceptor list automaticly!
    css::uno::Reference< css::frame::XDispatchProvider > xInterceptor;
    InterceptorList::const_iterator pIt = m_lInterceptionRegs.findByPattern(aURL.Complete);
    if (pIt != m_lInterceptionRegs.end())
        xInterceptor = pIt->xInterceptor;

    // b) No match by registration - but a valid interceptor list.
    //    Use first interceptor everytimes.
    //    Note: it doesn't matter, which direction this helper implementation use to ask interceptor objects.
    //    Using of member m_aInterceptorList will starts at the beginning everytimes.
    //    It depends from the filling operation, in which direction it works realy!
    if (!xInterceptor.is() && m_lInterceptionRegs.size()>0)
    {
        pIt          = m_lInterceptionRegs.begin();
        xInterceptor = pIt->xInterceptor;
    }

    // c) No registered interceptor => use our direct slave.
    //    This helper exist by design and must be valid everytimes ...
    //    But to be more feature proof - we should check that .-)
    if (!xInterceptor.is() && m_xSlave.is())
        xInterceptor = m_xSlave;

    aReadLock.unlock();
    // } SAFE

    css::uno::Reference< css::frame::XDispatch > xReturn;
    if (xInterceptor.is())
        xReturn = xInterceptor->queryDispatch(aURL, sTargetFrameName, nSearchFlags);
	return xReturn;
}

/*-----------------------------------------------------------------------------
    31.03.2003 07:58
-----------------------------------------------------------------------------*/
css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > > SAL_CALL InterceptionHelper::queryDispatches( const css::uno::Sequence< css::frame::DispatchDescriptor >& lDescriptor )
    throw(css::uno::RuntimeException)
{
          sal_Int32                                                          c           = lDescriptor.getLength();
          css::uno::Sequence< css::uno::Reference< css::frame::XDispatch > > lDispatches (c);
          css::uno::Reference< css::frame::XDispatch >*                      pDispatches = lDispatches.getArray();
    const css::frame::DispatchDescriptor*                                    pDescriptor = lDescriptor.getConstArray();

    for (sal_Int32 i=0; i<c; ++i)
        pDispatches[i] = queryDispatch(pDescriptor[i].FeatureURL, pDescriptor[i].FrameName, pDescriptor[i].SearchFlags);

    return lDispatches;
}

/*-----------------------------------------------------------------------------
    31.03.2003 10:20
-----------------------------------------------------------------------------*/
void SAL_CALL InterceptionHelper::registerDispatchProviderInterceptor(const css::uno::Reference< css::frame::XDispatchProviderInterceptor >& xInterceptor)
    throw(css::uno::RuntimeException)
{
    // reject wrong calling of this interface method
    css::uno::Reference< css::frame::XDispatchProvider > xThis(static_cast< ::cppu::OWeakObject* >(this), css::uno::UNO_QUERY);
    if (!xInterceptor.is())
        throw css::uno::RuntimeException(DECLARE_ASCII("NULL references not allowed as in parameter"), xThis);

	// Fill a new info structure for new interceptor.
	// Save his reference and try to get an additional URL/pattern list from him.
	// If no list exist register these interceptor for all dispatch events with "*"!
    InterceptorInfo aInfo;

    aInfo.xInterceptor = css::uno::Reference< css::frame::XDispatchProvider >(xInterceptor, css::uno::UNO_QUERY);
    css::uno::Reference< css::frame::XInterceptorInfo > xInfo(xInterceptor, css::uno::UNO_QUERY);
    if (xInfo.is())
        aInfo.lURLPattern = xInfo->getInterceptedURLs();
	else
	{
        aInfo.lURLPattern.realloc(1);
        aInfo.lURLPattern[0] = ::rtl::OUString::createFromAscii("*");
	}

    // SAFE {
    WriteGuard aWriteLock(m_aLock);

    // a) no interceptor at all - set this instance as master for given interceptor
    //    and set our slave as it's slave - and put this interceptor to the list.
    //    It's place there doesn matter. Because this list is currently empty.
    if (m_lInterceptionRegs.empty())
    {
        xInterceptor->setMasterDispatchProvider(xThis   );
        xInterceptor->setSlaveDispatchProvider (m_xSlave);
        m_lInterceptionRegs.push_back(aInfo);
    }

    // b) OK - there is at least one interceptor already registered.
    //    It's slave and it's master must be valid references ...
    //    because we created it. But we have to look for the static bool which
    //    regulate direction of using of interceptor objects!

    // b1) If "m_bPreferrFirstInterceptor" is set to true, we have to
    //     insert it behind any other existing interceptor - means at the end of our list.
    else if (m_bPreferrFirstInterceptor)
	{
        css::uno::Reference< css::frame::XDispatchProvider >            xMasterD = m_lInterceptionRegs.rbegin()->xInterceptor;
        css::uno::Reference< css::frame::XDispatchProviderInterceptor > xMasterI (xMasterD, css::uno::UNO_QUERY);

        xInterceptor->setMasterDispatchProvider(xMasterD          );
        xInterceptor->setSlaveDispatchProvider (m_xSlave          );
        xMasterI->setSlaveDispatchProvider     (aInfo.xInterceptor);

        m_lInterceptionRegs.push_back(aInfo);
	}

    // b2) If "m_bPreferrFirstInterceptor" is set to false, we have to
    //     insert it before any other existing interceptor - means at the beginning of our list.
    else
    {
        css::uno::Reference< css::frame::XDispatchProvider >            xSlaveD = m_lInterceptionRegs.begin()->xInterceptor;
        css::uno::Reference< css::frame::XDispatchProviderInterceptor > xSlaveI (xSlaveD , css::uno::UNO_QUERY);

        xInterceptor->setMasterDispatchProvider(xThis             );
        xInterceptor->setSlaveDispatchProvider (xSlaveD           );
        xSlaveI->setMasterDispatchProvider     (aInfo.xInterceptor);

        m_lInterceptionRegs.push_front(aInfo);
    }

    css::uno::Reference< css::frame::XFrame > xOwner(m_xOwnerWeak.get(), css::uno::UNO_QUERY);

    aWriteLock.unlock();
    // } SAFE

    // Don't forget to send a frame action event "context changed".
    // Any cached dispatch objects must be validated now!
    if (xOwner.is())
		xOwner->contextChanged();
}

/*-----------------------------------------------------------------------------
    31.03.2003 10:27
-----------------------------------------------------------------------------*/
void SAL_CALL InterceptionHelper::releaseDispatchProviderInterceptor(const css::uno::Reference< css::frame::XDispatchProviderInterceptor >& xInterceptor)
    throw(css::uno::RuntimeException)
{
    // reject wrong calling of this interface method
    css::uno::Reference< css::frame::XDispatchProvider > xThis(static_cast< ::cppu::OWeakObject* >(this), css::uno::UNO_QUERY);
    if (!xInterceptor.is())
        throw css::uno::RuntimeException(DECLARE_ASCII("NULL references not allowed as in parameter"), xThis);

    // SAFE {
    WriteGuard aWriteLock(m_aLock);

    // search this interceptor ...
    // If it could be located inside cache -
    // use it's slave/master relations to update the interception list;
    // set empty references for it as new master and slave;
    // and relase it from out cache.
    InterceptorList::iterator pIt = m_lInterceptionRegs.findByReference(xInterceptor);
    if (pIt != m_lInterceptionRegs.end())
    {
        css::uno::Reference< css::frame::XDispatchProvider >            xSlaveD  (xInterceptor->getSlaveDispatchProvider() , css::uno::UNO_QUERY);
        css::uno::Reference< css::frame::XDispatchProvider >            xMasterD (xInterceptor->getMasterDispatchProvider(), css::uno::UNO_QUERY);
        css::uno::Reference< css::frame::XDispatchProviderInterceptor > xSlaveI  (xSlaveD                                  , css::uno::UNO_QUERY);
        css::uno::Reference< css::frame::XDispatchProviderInterceptor > xMasterI (xMasterD                                 , css::uno::UNO_QUERY);

        if (xMasterI.is())
            xMasterI->setSlaveDispatchProvider(xSlaveD);

        if (xSlaveI.is())
            xSlaveI->setMasterDispatchProvider(xMasterD);

        xInterceptor->setSlaveDispatchProvider (css::uno::Reference< css::frame::XDispatchProvider >());
        xInterceptor->setMasterDispatchProvider(css::uno::Reference< css::frame::XDispatchProvider >());

        m_lInterceptionRegs.erase(pIt);
    }

    css::uno::Reference< css::frame::XFrame > xOwner(m_xOwnerWeak.get(), css::uno::UNO_QUERY);

    aWriteLock.unlock();
    // } SAFE

    // Don't forget to send a frame action event "context changed".
    // Any cached dispatch objects must be validated now!
    if (xOwner.is())
		xOwner->contextChanged();
}

/*-----------------------------------------------------------------------------
    31.03.2003 10:31
-----------------------------------------------------------------------------*/
#define FORCE_DESTRUCTION_OF_INTERCEPTION_CHAIN
void SAL_CALL InterceptionHelper::disposing(const css::lang::EventObject& aEvent)
    throw(css::uno::RuntimeException)
{
    #ifdef FORCE_DESTRUCTION_OF_INTERCEPTION_CHAIN
    // SAFE ->
    ReadGuard aReadLock(m_aLock);

    // check calli ... we accept such disposing call's only from our onwer frame.
    css::uno::Reference< css::frame::XFrame > xOwner(m_xOwnerWeak.get(), css::uno::UNO_QUERY);
    if (aEvent.Source != xOwner)
        return;

    // Because every interceptor hold at least one reference to us ... and we destruct this list
    // of interception objects ... we should hold ourself alive .-)
    css::uno::Reference< css::frame::XDispatchProvider > xThis(static_cast< ::cppu::OWeakObject* >(this), css::uno::UNO_QUERY_THROW);

    // We need a full copy of all currently registered interceptor objects.
    // Otherwhise we cant iterate over this vector without the risk, that our iterator will be invalid.
    // Because this vetor will be influenced by every deregistered interceptor.
    InterceptionHelper::InterceptorList aCopy = m_lInterceptionRegs;

    aReadLock.unlock();
    // <- SAFE

    InterceptionHelper::InterceptorList::iterator pIt;
    for (  pIt  = aCopy.begin();
           pIt != aCopy.end()  ;
         ++pIt                 )
    {
        InterceptionHelper::InterceptorInfo& rInfo = *pIt;
        if (rInfo.xInterceptor.is())
        {
            css::uno::Reference< css::frame::XDispatchProviderInterceptor > xInterceptor(rInfo.xInterceptor, css::uno::UNO_QUERY_THROW);
            releaseDispatchProviderInterceptor(xInterceptor);
            rInfo.xInterceptor.clear();
        }
    }

    aCopy.clear();

    #if OSL_DEBUG_LEVEL > 0
    // SAFE ->
    aReadLock.lock();
    if (!m_lInterceptionRegs.empty() )
        OSL_ENSURE(sal_False, "There are some pending interceptor objects, which seams to be registered during (!) the destruction of a frame.");
    aReadLock.unlock();
    // <- SAFE
    #endif // ODL_DEBUG_LEVEL>0

    #endif // FORCE_DESTRUCTION_OF_INTERCEPTION_CHAIN
}

} // namespace framework
