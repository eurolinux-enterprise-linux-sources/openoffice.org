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

#include "mdrivermanager.hxx"

#include <cppuhelper/factory.hxx>

#include <stdio.h>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;

//==========================================================================
//= registration
//==========================================================================
extern "C"
{

//---------------------------------------------------------------------------------------
SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment(const sal_Char** _ppEnvTypeName, uno_Environment** /*_ppEnv*/)
{
	*_ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

//---------------------------------------------------------------------------------------
SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo(void* /*_pServiceManager*/, com::sun::star::registry::XRegistryKey* _pRegistryKey)
{


	sal_Bool bReturn = sal_False;

	try
	{
		::rtl::OUString sMainKeyName = ::rtl::OUString::createFromAscii("/");
		sMainKeyName += ::drivermanager::OSDBCDriverManager::getImplementationName_static();
		sMainKeyName += ::rtl::OUString::createFromAscii("/UNO/SERVICES");
		Reference< XRegistryKey > xMainKey = _pRegistryKey->createKey(sMainKeyName);
		if (xMainKey.is())
		{
			Sequence< ::rtl::OUString > sServices(::drivermanager::OSDBCDriverManager::getSupportedServiceNames_static());
			const ::rtl::OUString* pBegin = sServices.getConstArray();
			const ::rtl::OUString* pEnd = pBegin + sServices.getLength();
			for (;pBegin != pEnd ; ++pBegin)
				xMainKey->createKey(*pBegin);
			bReturn = sal_True;
		}
	}
	catch(InvalidRegistryException&)
	{
	}
	catch(InvalidValueException&)
	{
	}
	return bReturn;
}

//---------------------------------------------------------------------------------------
SAL_DLLPUBLIC_EXPORT void* SAL_CALL component_getFactory(const sal_Char* _pImplName, ::com::sun::star::lang::XMultiServiceFactory* _pServiceManager, void* /*_pRegistryKey*/)
{
	void* pRet = NULL;

	if (::drivermanager::OSDBCDriverManager::getImplementationName_static().compareToAscii(_pImplName) == 0)
	{
		Reference< XSingleServiceFactory > xFactory(
			::cppu::createOneInstanceFactory(
				_pServiceManager,
				::drivermanager::OSDBCDriverManager::getImplementationName_static(),
				::drivermanager::OSDBCDriverManager::Create,
				::drivermanager::OSDBCDriverManager::getSupportedServiceNames_static()
			)
		);
		if (xFactory.is())
		{
			xFactory->acquire();
			pRet = xFactory.get();
		}
	}

	return pRet;
}

}	// extern "C"


