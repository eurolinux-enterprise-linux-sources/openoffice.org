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
#include <accelerators/moduleacceleratorconfiguration.hxx>

//_______________________________________________
// own includes
#include <threadhelp/readguard.hxx>
#include <threadhelp/writeguard.hxx>

#ifndef __FRAMEWORK_ACCELERATORCONST_H_
#include <acceleratorconst.h>
#endif
#include <services.h>

//_______________________________________________
// interface includes
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/embed/ElementModes.hpp>

//_______________________________________________
// other includes

#ifndef _COMPHELPER_SEQUENCEASHASHMAP_HXX
#include <comphelper/sequenceashashmap.hxx>
#endif
#include <vcl/svapp.hxx>

#ifndef _COMPHELPER_CONFIGURATIONHELPER_HXX_
#include <comphelper/configurationhelper.hxx>
#endif

#ifndef _COM_SUN_STAR_UTIL_XCHANGESNOTIFIER_HPP_
#include <com/sun/star/util/XChangesNotifier.hpp>
#endif

#ifndef _RTL_LOGFILE_HXX_
#include <rtl/logfile.hxx>
#endif

#ifndef _RTL_LOGFILE_HXX_
#include <rtl/logfile.h>
#endif

//_______________________________________________
// const

namespace framework
{

//-----------------------------------------------    
// XInterface, XTypeProvider, XServiceInfo
DEFINE_XINTERFACE_2(ModuleAcceleratorConfiguration              ,
                    XCUBasedAcceleratorConfiguration                    ,
                    DIRECT_INTERFACE(css::lang::XServiceInfo)   ,
                    DIRECT_INTERFACE(css::lang::XInitialization))

DEFINE_XTYPEPROVIDER_2_WITH_BASECLASS(ModuleAcceleratorConfiguration,
                                      XCUBasedAcceleratorConfiguration      ,
                                      css::lang::XServiceInfo       ,
                                      css::lang::XInitialization    )
                       
DEFINE_XSERVICEINFO_MULTISERVICE(ModuleAcceleratorConfiguration                   ,
                                 ::cppu::OWeakObject                              ,
                                 SERVICENAME_MODULEACCELERATORCONFIGURATION       ,
                                 IMPLEMENTATIONNAME_MODULEACCELERATORCONFIGURATION)

DEFINE_INIT_SERVICE(ModuleAcceleratorConfiguration,
                    {
                        /*Attention
                        I think we don't need any mutex or lock here ... because we are called by our own static method impl_createInstance()
                        to create a new instance of this class by our own supported service factory.
                        see macro DEFINE_XSERVICEINFO_MULTISERVICE and "impl_initService()" for further informations!
                        */
                    }
                   )
                                    
//-----------------------------------------------    
ModuleAcceleratorConfiguration::ModuleAcceleratorConfiguration(const css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR)
    : XCUBasedAcceleratorConfiguration(xSMGR)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "framework", "Ocke.Janssen@sun.com", "ModuleAcceleratorConfiguration::ModuleAcceleratorConfiguration" );
}

//-----------------------------------------------    
ModuleAcceleratorConfiguration::~ModuleAcceleratorConfiguration()
{
   // m_aPresetHandler.removeStorageListener(this);
}

//-----------------------------------------------    
void SAL_CALL ModuleAcceleratorConfiguration::initialize(const css::uno::Sequence< css::uno::Any >& lArguments)
    throw(css::uno::Exception       ,
          css::uno::RuntimeException)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "framework", "Ocke.Janssen@sun.com", "ModuleAcceleratorConfiguration::initialize" );
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);
    
	::comphelper::SequenceAsHashMap lArgs(lArguments);
    m_sModule = lArgs.getUnpackedValueOrDefault(::rtl::OUString::createFromAscii("ModuleIdentifier"), ::rtl::OUString());
	m_sLocale = lArgs.getUnpackedValueOrDefault(::rtl::OUString::createFromAscii("Locale")          , ::rtl::OUString::createFromAscii("x-default"));
    
    if (!m_sModule.getLength())
        throw css::uno::RuntimeException(
                ::rtl::OUString::createFromAscii("The module dependend accelerator configuration service was initialized with an empty module identifier!"),
                static_cast< ::cppu::OWeakObject* >(this));
    
    aWriteLock.unlock();
    // <- SAFE ----------------------------------
    
    impl_ts_fillCache();
}
          
//-----------------------------------------------    
void ModuleAcceleratorConfiguration::impl_ts_fillCache()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "framework", "Ocke.Janssen@sun.com", "ModuleAcceleratorConfiguration::impl_ts_fillCache" );
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);
    ::rtl::OUString sModule = m_sModule;
	m_sModuleCFG = m_sModule;
    aReadLock.unlock();
    // <- SAFE ----------------------------------

    // get current office locale ... but dont cache it.
    // Otherwise we must be listener on the configuration layer
    // which seems to superflous for this small implementation .-)
	::comphelper::Locale aLocale = ::comphelper::Locale(m_sLocale);
    
    // May be the current app module does not have any
    // accelerator config? Handle it gracefully :-)
    try
    {
        m_sGlobalOrModules = CFG_ENTRY_MODULES;
		XCUBasedAcceleratorConfiguration::reload();

		css::uno::Reference< css::util::XChangesNotifier > xBroadcaster(m_xCfg, css::uno::UNO_QUERY_THROW);
		xBroadcaster->addChangesListener(static_cast< css::util::XChangesListener* >(this));
	}
    catch(const css::uno::RuntimeException& exRun)
        { throw exRun; }
    catch(const css::uno::Exception&)
        {}
}

} // namespace framework

