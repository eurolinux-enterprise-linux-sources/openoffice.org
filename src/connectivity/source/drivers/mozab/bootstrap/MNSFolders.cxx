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
#include "precompiled_connectivity.hxx"
#include <MNSFolders.hxx>

#ifdef UNIX
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#endif // End UNIX

#ifdef WNT
#include "pre_include_windows.h"
#include <windows.h>
#include <stdlib.h>
#include <shlobj.h>
#include <objidl.h>
#include "post_include_windows.h"
#endif // End WNT
#include <osl/security.hxx>
#include <osl/file.hxx>
#include <osl/thread.h>

using namespace ::com::sun::star::mozilla;

namespace
{
    #if defined(XP_MAC) || defined(XP_MACOSX) || defined(MACOSX) 
        #define APP_REGISTRY_NAME "Application Registry"
    #elif defined(XP_WIN) || defined(XP_OS2)
        #define APP_REGISTRY_NAME "registry.dat"
    #else
        #define APP_REGISTRY_NAME "appreg"
    #endif

    // -------------------------------------------------------------------
    static ::rtl::OUString lcl_getUserDataDirectory()
    {
        ::osl::Security   aSecurity;
        ::rtl::OUString   aConfigPath;

        aSecurity.getConfigDir( aConfigPath );
        return aConfigPath + ::rtl::OUString::createFromAscii( "/" );
    }

    // -------------------------------------------------------------------
    static const char* DefaultProductDir[3][3] =
    {
    #if defined(XP_WIN)
        { "Mozilla/", NULL, NULL },
        { "Mozilla/Firefox/", NULL, NULL },
        { "Thunderbird/", "Mozilla/Thunderbird/", NULL }
    #elif(MACOSX)
        { "../Mozilla/", NULL, NULL },
        { "Firefox/", NULL, NULL },
        { "../Thunderbird/", NULL, NULL }
    #else
        { ".mozilla/", NULL, NULL },
        { ".mozilla/firefox/", NULL, NULL },
        { ".thunderbird/", ".mozilla-thunderbird/", ".mozilla/thunderbird/" }
    #endif
    };

    static const char* ProductRootEnvironmentVariable[3] =
    {
        "MOZILLA_PROFILE_ROOT",
        "MOZILLA_FIREFOX_PROFILE_ROOT",
        "MOZILLA_THUNDERBIRD_PROFILE_ROOT",
    };

    // -------------------------------------------------------------------
    static ::rtl::OUString lcl_guessProfileRoot( MozillaProductType _product )
    {
        size_t productIndex = _product - 1;

        static ::rtl::OUString s_productDirectories[3];

        if ( !s_productDirectories[ productIndex ].getLength() )
        {
            ::rtl::OUString sProductPath;

            // check whether we have an anevironment variable which helps us
            const char* pProfileByEnv = getenv( ProductRootEnvironmentVariable[ productIndex ] );
            if ( pProfileByEnv )
            {
                sProductPath = ::rtl::OUString( pProfileByEnv, rtl_str_getLength( pProfileByEnv ), osl_getThreadTextEncoding() );
                // asume that this is fine, no further checks
            }
            else
            {
                ::rtl::OUString sProductDirCandidate;
                const char* pProfileRegistry = ( _product == MozillaProductType_Mozilla ) ? APP_REGISTRY_NAME : "profiles.ini";

                // check all possible candidates
                for ( size_t i=0; i<3; ++i )
                {
                    if ( NULL == DefaultProductDir[ productIndex ][ i ] )
                        break;

                    sProductDirCandidate = lcl_getUserDataDirectory() +
                        ::rtl::OUString::createFromAscii( DefaultProductDir[ productIndex ][ i ] );

                    // check existence
                    ::osl::DirectoryItem aRegistryItem;
                    ::osl::FileBase::RC result = ::osl::DirectoryItem::get( sProductDirCandidate + ::rtl::OUString::createFromAscii( pProfileRegistry ), aRegistryItem );
                    if ( result == ::osl::FileBase::E_None  )
                    {
                        ::osl::FileStatus aStatus( FileStatusMask_Validate );
                        result = aRegistryItem.getFileStatus( aStatus );
                        if ( result == ::osl::FileBase::E_None  )
                        {
                            // the registry file exists
                            break;
                        }
                    }
                }

                ::osl::FileBase::getSystemPathFromFileURL( sProductDirCandidate, sProductPath );
            }

            s_productDirectories[ productIndex ] = sProductPath;
        }

        return s_productDirectories[ productIndex ];
    }
}

// -----------------------------------------------------------------------
::rtl::OUString getRegistryDir(MozillaProductType product)
{
	if (product == MozillaProductType_Default)
		return ::rtl::OUString();

    return lcl_guessProfileRoot( product );
}
#ifndef MINIMAL_PROFILEDISCOVER
// -----------------------------------------------------------------------
::rtl::OUString getRegistryFileName(MozillaProductType product)
{
	if (product == MozillaProductType_Default)
		return ::rtl::OUString();

	return getRegistryDir(product) + ::rtl::OUString::createFromAscii(APP_REGISTRY_NAME);
}
#endif
