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
#ifndef EXTENSIONS_SOURCE_PROPCTRLR_CONTROLTYPE_HXX
#define EXTENSIONS_SOURCE_PROPCTRLR_CONTROLTYPE_HXX

#include <sal/types.h>

//........................................................................
namespace pcr
{
//........................................................................

	//====================================================================
	//= control types, analogous to FormComponentType
	//====================================================================
    namespace ControlType
    {
        static const sal_Int16 FIXEDLINE      = (sal_Int16)100;
        static const sal_Int16 FORMATTEDFIELD = (sal_Int16)101;
        static const sal_Int16 PROGRESSBAR    = (sal_Int16)102;

        // need only those which are not already covered as FormComponentType
    }

//........................................................................
} // namespacepcr
//........................................................................

#endif // EXTENSIONS_SOURCE_PROPCTRLR_CONTROLTYPE_HXX

