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

#include <stdio.h>
#include <cppuhelper/factory.hxx>
#include "ZPoolCollection.hxx"


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;
using namespace connectivity;
//==========================================================================
//= registration
//==========================================================================
extern "C"
{

//---------------------------------------------------------------------------------------
	void SAL_CALL component_getImplementationEnvironment(const sal_Char** _ppEnvTypeName, uno_Environment** /*_ppEnv*/)
{
	*_ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

//---------------------------------------------------------------------------------------
sal_Bool SAL_CALL component_writeInfo(void* /*_pServiceManager*/, com::sun::star::registry::XRegistryKey* _pRegistryKey)
{
	::rtl::OUString sMainKeyName = ::rtl::OUString::createFromAscii("/");
	sMainKeyName += OPoolCollection::getImplementationName_Static();
	sMainKeyName += ::rtl::OUString::createFromAscii("/UNO/SERVICES");

	try
	{
		Reference< XRegistryKey > xMainKey = _pRegistryKey->createKey(sMainKeyName);
		if (!xMainKey.is())
			return sal_False;

		Sequence< ::rtl::OUString > sServices = OPoolCollection::getSupportedServiceNames_Static();
		const ::rtl::OUString* pServices = sServices.getConstArray();
		for (sal_Int32 i=0; i<sServices.getLength(); ++i, ++pServices)
			xMainKey->createKey(*pServices);
	}
	catch(InvalidRegistryException&)
	{
		return sal_False;
	}
	catch(InvalidValueException&)
	{
		return sal_False;
	}
	return sal_True;
}

//---------------------------------------------------------------------------------------
void* SAL_CALL component_getFactory(const sal_Char* _pImplName, ::com::sun::star::lang::XMultiServiceFactory* _pServiceManager, void* /*_pRegistryKey*/)
{
	void* pRet = NULL;

	if (OPoolCollection::getImplementationName_Static().compareToAscii(_pImplName) == 0)
	{
		Reference< XSingleServiceFactory > xFactory(
			::cppu::createOneInstanceFactory(
				_pServiceManager,
				OPoolCollection::getImplementationName_Static(),
				OPoolCollection::CreateInstance,
				OPoolCollection::getSupportedServiceNames_Static()
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


