/*************************************************************************
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
#include "precompiled_comphelper.hxx"

#include "comphelper_module.hxx"

//--------------------------------------------------------------------
extern void createRegistryInfo_OPropertyBag();
extern void createRegistryInfo_SequenceOutputStream();
extern void createRegistryInfo_SequenceInputStream();
extern void createRegistryInfo_UNOMemoryStream();
extern void createRegistryInfo_IndexedPropertyValuesContainer();
extern void createRegistryInfo_NamedPropertyValuesContainer();
extern void createRegistryInfo_AnyCompareFactory();
extern void createRegistryInfo_OfficeInstallationDirectories();
extern void createRegistryInfo_OInstanceLocker();
extern void createRegistryInfo_Map();
extern void createRegistryInfo_OSimpleLogRing();

//........................................................................
namespace comphelper { namespace module
{
//........................................................................

    static void initializeModule()
    {
        static bool bInitialized( false );
        if ( !bInitialized )
        {
            ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
            if ( !bInitialized )
            {
                createRegistryInfo_OPropertyBag();
                createRegistryInfo_SequenceOutputStream();
                createRegistryInfo_SequenceInputStream();
                createRegistryInfo_UNOMemoryStream();
                createRegistryInfo_IndexedPropertyValuesContainer();
                createRegistryInfo_NamedPropertyValuesContainer();
                createRegistryInfo_AnyCompareFactory();
                createRegistryInfo_OfficeInstallationDirectories();
                createRegistryInfo_OInstanceLocker();
                createRegistryInfo_Map();
                createRegistryInfo_OSimpleLogRing();
            }
        }
    }

//........................................................................
} } // namespace comphelper::module
//........................................................................

IMPLEMENT_COMPONENT_LIBRARY_API( ::comphelper::module::ComphelperModule, ::comphelper::module::initializeModule )
