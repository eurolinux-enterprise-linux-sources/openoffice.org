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
#ifndef SC_VBA_OLEOBJECTS_HXX
#define SC_VBA_OLEOBJECTS_HXX

#include <cppuhelper/implbase1.hxx>
#include <ooo/vba/excel/XOLEObjects.hpp>

#include "vbacollectionimpl.hxx"
#include "vbahelper.hxx"

typedef CollTestImplHelper< ov::excel::XOLEObjects > OLEObjectsImpl_BASE;

class ScVbaOLEObjects : public OLEObjectsImpl_BASE
{
protected:
    virtual css::uno::Any getItemByStringIndex( const rtl::OUString& sIndex ) throw (css::uno::RuntimeException);
	virtual rtl::OUString& getServiceImplName();
	virtual css::uno::Sequence<rtl::OUString> getServiceNames();
public:
    ScVbaOLEObjects( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext, 
                    const css::uno::Reference< css::container::XIndexAccess >& xIndexAccess );

    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() throw (css::uno::RuntimeException);
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() throw (css::uno::RuntimeException);

    // ScVbaCollectionBaseImpl
    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource );
    
};
#endif //SC_VBA_OLEOBJECTS_HXX

