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

#ifndef INCLUDED_UNODEVTOOLS_OPTIONS_HXX
#define INCLUDED_UNODEVTOOLS_OPTIONS_HXX

#include <rtl/ustrbuf.hxx>

namespace com { namespace sun { namespace star { namespace uno {
class RuntimeException;
} } } }

namespace unodevtools {

//-------------------------------------------------------------------------------
sal_Bool readOption( rtl::OUString * pValue, const sal_Char * pOpt,
                     sal_Int32 * pnIndex, const rtl::OUString & aArg)
	throw (com::sun::star::uno::RuntimeException);

//-------------------------------------------------------------------------------
sal_Bool readOption( sal_Bool * pbOpt, const sal_Char * pOpt,
                     sal_Int32 * pnIndex, const rtl::OUString & aArg);

} // end of namespace unodevtools

#endif // INCLUDED_UNODEVTOOLS_OPTIONS_HXX
