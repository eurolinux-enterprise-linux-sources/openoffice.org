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

#ifndef ACCESSIBILITY_STANDARD_VCLXACCESSIBLEMENUSEPARATOR_HXX
#define ACCESSIBILITY_STANDARD_VCLXACCESSIBLEMENUSEPARATOR_HXX

#include <accessibility/standard/accessiblemenuitemcomponent.hxx>


//	----------------------------------------------------
//	class VCLXAccessibleMenuSeparator
//	----------------------------------------------------

class VCLXAccessibleMenuSeparator :	public OAccessibleMenuItemComponent
{
public:
	VCLXAccessibleMenuSeparator( Menu* pParent, sal_uInt16 nItemPos, Menu* pMenu = 0 );
	virtual ~VCLXAccessibleMenuSeparator();

	// XServiceInfo
	virtual ::rtl::OUString SAL_CALL getImplementationName() throw (::com::sun::star::uno::RuntimeException);
	virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames() throw (::com::sun::star::uno::RuntimeException);

	// XAccessibleContext
	virtual sal_Int16 SAL_CALL getAccessibleRole(  ) throw (::com::sun::star::uno::RuntimeException);
};


#endif // ACCESSIBILITY_STANDARD_VCLXACCESSIBLEMENUSEPARATOR_HXX

