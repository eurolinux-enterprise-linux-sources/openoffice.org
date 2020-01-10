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
#include "precompiled_lingucomponent.hxx"


#include <cppuhelper/factory.hxx>	// helper for factories
#include <rtl/string.hxx>

#include <com/sun/star/registry/XRegistryKey.hpp>

using namespace rtl;
using namespace com::sun::star::lang;
using namespace com::sun::star::registry;

////////////////////////////////////////
// declaration of external RegEntry-functions defined by the service objects
//

extern sal_Bool SAL_CALL MacSpellChecker_writeInfo(
	void * /*pServiceManager*/, XRegistryKey * pRegistryKey );

extern void * SAL_CALL MacSpellChecker_getFactory(
	const sal_Char * pImplName,
	XMultiServiceFactory * pServiceManager,
	void * /*pRegistryKey*/ );

////////////////////////////////////////
// definition of the two functions that are used to provide the services
//

extern "C"
{

sal_Bool SAL_CALL component_writeInfo(
	void * pServiceManager, XRegistryKey * pRegistryKey )
{
	return MacSpellChecker_writeInfo( pServiceManager, pRegistryKey );
}

void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

void * SAL_CALL component_getFactory(
	const sal_Char * pImplName, void * pServiceManager, void * pRegistryKey )
{
    void * pRet = NULL;
	pRet = MacSpellChecker_getFactory(
    	pImplName,
    	reinterpret_cast< XMultiServiceFactory * >( pServiceManager ),
    	pRegistryKey );

	return pRet;
}

}

///////////////////////////////////////////////////////////////////////////

