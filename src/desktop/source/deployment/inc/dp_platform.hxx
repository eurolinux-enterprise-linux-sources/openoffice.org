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

#if ! defined INCLUDED_DP_PLATFORM_HXX
#define INCLUDED_DP_PLATFORM_HXX


#ifndef INCLUDED_DESKTOP_SOURCE_DEPLOYMENT_INC_DP_MISC_API_HXX
#include "dp_misc_api.hxx"
#endif

#include "com/sun/star/uno/Sequence.hxx"
#include "rtl/ustring.hxx"

namespace dp_misc 
{
 

DESKTOP_DEPLOYMENTMISC_DLLPUBLIC ::rtl::OUString const & getPlatformString();

DESKTOP_DEPLOYMENTMISC_DLLPUBLIC
    bool platform_fits( ::rtl::OUString const & platform_string );

/** determines if the current platform corresponds to one of the platform strings.

*/
DESKTOP_DEPLOYMENTMISC_DLLPUBLIC
bool hasValidPlatform( ::com::sun::star::uno::Sequence< ::rtl::OUString > const & platformStrings);

}

#endif
