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
#include "cppuhelper/implementationentry.hxx"
#include "com/sun/star/lang/XMultiServiceFactory.hpp"
#include "com/sun/star/registry/XRegistryKey.hpp"
#include "comphelper/servicedecl.hxx"

// =============================================================================
// component exports
// =============================================================================
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace sdecl = comphelper::service_decl;

// reference service helper(s)
namespace  range
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace  workbook
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace  worksheet
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace window 
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace globals
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace  userform
{
extern sdecl::ServiceDecl const serviceDecl;
}
namespace hyperlink 
{
extern sdecl::ServiceDecl const serviceDecl;
}

extern "C"
{
    SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment( 
        const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
    {
		OSL_TRACE("In component_getImplementationEnv");
        *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
    }

    SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo( 
        lang::XMultiServiceFactory * pServiceManager, registry::XRegistryKey * pRegistryKey )
    {
		OSL_TRACE("In component_writeInfo");

	// Component registration
        if ( component_writeInfoHelper( pServiceManager, pRegistryKey, 
		range::serviceDecl, workbook::serviceDecl, worksheet::serviceDecl, globals::serviceDecl, userform::serviceDecl, window::serviceDecl, hyperlink::serviceDecl ) )
		{
			// Singleton registration
			try
			{
				registry::XRegistryKey * pKey =
					reinterpret_cast< registry::XRegistryKey * >(pRegistryKey);

				Reference< registry::XRegistryKey >xKey = pKey->createKey(
					rtl::OUString::createFromAscii( ("ooo.vba.Globals/UNO/SINGLETONS/ooo.vba.theGlobals") ) );
				xKey->setStringValue( ::rtl::OUString::createFromAscii( 
					("ooo.vba.Globals") ) );
				return sal_True;
			}
			catch( uno::Exception& /*e*/ )
			{
				//recomp & friends will detect false returned and fail
			}
		}
		return sal_False;
    }

    SAL_DLLPUBLIC_EXPORT void * SAL_CALL component_getFactory( 
        const sal_Char * pImplName, lang::XMultiServiceFactory * pServiceManager,
        registry::XRegistryKey * pRegistryKey )
    {
		OSL_TRACE("In component_getFactory for %s", pImplName );
	void* pRet =  component_getFactoryHelper(
        	pImplName, pServiceManager, pRegistryKey, range::serviceDecl, workbook::serviceDecl, worksheet::serviceDecl, globals::serviceDecl, userform::serviceDecl, window::serviceDecl, hyperlink::serviceDecl );
	OSL_TRACE("Ret is 0x%x", pRet);
	return pRet;
    }
}
