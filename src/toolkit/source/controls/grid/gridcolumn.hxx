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
#include "precompiled_toolkit.hxx"
#include <com/sun/star/awt/grid/XGridColumn.hpp>
#include <com/sun/star/lang/XEventListener.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implbase3.hxx>
#include <rtl/ref.hxx>
#include <vector>
#include <toolkit/helper/mutexandbroadcasthelper.hxx>

using ::rtl::OUString;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::awt::grid;
using namespace ::com::sun::star::lang;

namespace toolkit
{

class GridColumn : public ::cppu::WeakImplHelper2< XGridColumn, XServiceInfo >,
							 public MutexAndBroadcastHelper
{
public:
	GridColumn();
	virtual ~GridColumn();

	// XGridColumn
	virtual ::com::sun::star::uno::Any SAL_CALL getIdentifier() throw (::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL setIdentifier(const ::com::sun::star::uno::Any & value) throw (::com::sun::star::uno::RuntimeException);
	virtual ::sal_Int32 SAL_CALL getColumnWidth() throw (::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL setColumnWidth(::sal_Int32 the_value) throw (::com::sun::star::uno::RuntimeException);
	virtual ::rtl::OUString SAL_CALL getTitle() throw (::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL setTitle(const ::rtl::OUString & value) throw (::com::sun::star::uno::RuntimeException);

	// XComponent
    virtual void SAL_CALL dispose(  ) throw (RuntimeException);
    virtual void SAL_CALL addEventListener( const Reference< XEventListener >& xListener ) throw (RuntimeException);
    virtual void SAL_CALL removeEventListener( const Reference< XEventListener >& aListener ) throw (RuntimeException);

    // XServiceInfo
	virtual ::rtl::OUString SAL_CALL getImplementationName(  ) throw (RuntimeException);
	virtual ::sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw (RuntimeException);
	virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) throw (RuntimeException);


private:
	Any identifier;
	sal_Int32 columnWidth;
	::rtl::OUString title;
};

}
