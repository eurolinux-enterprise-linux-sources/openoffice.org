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
#include "precompiled_svx.hxx"

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif
#include "sdbcdriverenum.hxx"
#include <comphelper/stl_types.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

//........................................................................
namespace offapp
{
//........................................................................

	using namespace ::com::sun::star::uno;
	using namespace ::com::sun::star::lang;
	using namespace ::com::sun::star::container;

	//====================================================================
	//= ODriverEnumerationImpl
	//====================================================================
	class ODriverEnumerationImpl
	{
	protected:
		::std::vector< ::rtl::OUString >	m_aImplNames;

	public:
		ODriverEnumerationImpl();

		const ::std::vector< ::rtl::OUString >& getDriverImplNames() const { return m_aImplNames; }
	};

	//--------------------------------------------------------------------
	ODriverEnumerationImpl::ODriverEnumerationImpl()
	{
		try
		{
			Reference< XMultiServiceFactory > xORB = ::comphelper::getProcessServiceFactory();
			Reference< XInterface > xDM = xORB->createInstance(::rtl::OUString::createFromAscii("com.sun.star.sdbc.DriverManager"));
			OSL_ENSURE(xDM.is(), "ODriverEnumerationImpl::ODriverEnumerationImpl: no access to the SDBC driver manager!");

			Reference< XEnumerationAccess > xEnumAccess(xDM, UNO_QUERY);
			OSL_ENSURE(xEnumAccess.is() || !xDM.is(), "ODriverEnumerationImpl::ODriverEnumerationImpl: can't enumerate SDBC drivers (missing the interface)!");
			if (xEnumAccess.is())
			{
				Reference< XEnumeration > xEnumDrivers = xEnumAccess->createEnumeration();
				OSL_ENSURE(xEnumDrivers.is(), "ODriverEnumerationImpl::ODriverEnumerationImpl: invalid enumeration object!");

				Reference< XServiceInfo > xDriverSI;
				while (xEnumDrivers->hasMoreElements())
				{
					xEnumDrivers->nextElement() >>= xDriverSI;
					OSL_ENSURE(xDriverSI.is(), "ODriverEnumerationImpl::ODriverEnumerationImpl: driver without service info!");
					if (xDriverSI.is())
						m_aImplNames.push_back(xDriverSI->getImplementationName());
				}
			}
		}
		catch(const Exception&)
		{
			OSL_ENSURE(sal_False, "ODriverEnumerationImpl::ODriverEnumerationImpl: caught an exception while enumerating the drivers!");
		}
	}

	//====================================================================
	//= ODriverEnumeration
	//====================================================================
	//--------------------------------------------------------------------
	ODriverEnumeration::ODriverEnumeration() throw()
		:m_pImpl(new ODriverEnumerationImpl)
	{
	}

	//--------------------------------------------------------------------
	ODriverEnumeration::~ODriverEnumeration() throw()
	{
		delete m_pImpl;
	}

	//--------------------------------------------------------------------
	ODriverEnumeration::const_iterator ODriverEnumeration::begin() const throw()
	{
		return m_pImpl->getDriverImplNames().begin();
	}

	//--------------------------------------------------------------------
	ODriverEnumeration::const_iterator ODriverEnumeration::end() const throw()
	{
		return m_pImpl->getDriverImplNames().end();
	}

	//--------------------------------------------------------------------
	sal_Int32 ODriverEnumeration::size() const throw()
	{
		return m_pImpl->getDriverImplNames().size();
	}

//........................................................................
}	// namespace offapp
//........................................................................


