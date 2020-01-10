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
#ifndef SC_VBA_COMMANDBARS_HXX
#define SC_VBA_COMMANDBARS_HXX

#include <ooo/vba/XCommandBar.hpp>
#include <ooo/vba/XCommandBars.hpp>
#include <com/sun/star/container/XNameAccess.hpp>

#include <cppuhelper/implbase1.hxx>

#include "vbahelperinterface.hxx"
#include "vbacollectionimpl.hxx"

typedef CollTestImplHelper< ov::XCommandBars > CommandBars_BASE;

class ScVbaCommandBars : public CommandBars_BASE
{
private:
    css::uno::Reference< css::container::XNameAccess > m_xNameAccess;
    rtl::OUString m_sModuleName;
    void retrieveObjects() throw( css::uno::RuntimeException );
public:
    ScVbaCommandBars( const css::uno::Reference< ov::XHelperInterface > xParent, const css::uno::Reference< css::uno::XComponentContext > xContext, const css::uno::Reference< css::container::XIndexAccess > xIndexAccess );

    sal_Bool checkToolBarExist( rtl::OUString sToolBarName );
    rtl::OUString GetModuleName(){ return m_sModuleName; };
    css::uno::Reference< css::container::XNameAccess > GetWindows() 
    { 
        retrieveObjects();
        return m_xNameAccess; 
    };
    // XCommandBars
    virtual css::uno::Reference< ov::XCommandBar > SAL_CALL Add( const css::uno::Any& Name, const css::uno::Any& Position, const css::uno::Any& MenuBar, const css::uno::Any& Temporary ) throw (css::script::BasicErrorException, css::uno::RuntimeException);
    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() throw (css::uno::RuntimeException);
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() throw (css::uno::RuntimeException);
    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource );

    virtual sal_Int32 SAL_CALL getCount() throw(css::uno::RuntimeException);
    virtual css::uno::Any SAL_CALL Item( const css::uno::Any& aIndex, const css::uno::Any& /*aIndex2*/ ) throw( css::uno::RuntimeException);
    // XHelperInterface
    virtual rtl::OUString& getServiceImplName();
    virtual css::uno::Sequence<rtl::OUString> getServiceNames();
};

#endif//SC_VBA_COMMANDBARS_HXX
