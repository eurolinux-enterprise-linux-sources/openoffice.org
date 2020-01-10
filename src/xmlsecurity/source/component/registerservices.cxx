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
#include "precompiled_xmlsecurity.hxx"

#include <tools/debug.hxx>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>

#include <cppuhelper/factory.hxx>


#include <documentdigitalsignatures.hxx>
#include <certificatecontainer.hxx>

using namespace ::com::sun::star;

extern "C"
{
void SAL_CALL component_getImplementationEnvironment( const sal_Char ** ppEnvTypeName, uno_Environment ** )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

sal_Bool SAL_CALL component_writeInfo( void* /*pServiceManager*/, void* pRegistryKey )
{
	if (pRegistryKey)
	{
		try
		{
                    sal_Int32 nPos = 0;
                    // SERVICE DocumentDigitalSignatures
                    nPos = 0;
                    uno::Reference< registry::XRegistryKey > xNewKey(
                    reinterpret_cast< registry::XRegistryKey* >( pRegistryKey )->createKey( DocumentDigitalSignatures::GetImplementationName() ) ); 
                    
                    xNewKey = xNewKey->createKey( rtl::OUString::createFromAscii( "/UNO/SERVICES" ) );
			
                    const uno::Sequence< rtl::OUString >& rSNL = DocumentDigitalSignatures::GetSupportedServiceNames();
                    const rtl::OUString* pArray = rSNL.getConstArray();
                    for ( nPos = rSNL.getLength(); nPos--; )
                        xNewKey->createKey( pArray[nPos] );

                    // SERVICE CertificateContainer
                    nPos = 0;
                    uno::Reference< registry::XRegistryKey > xNewKeyCertificateContainer(
                    reinterpret_cast< registry::XRegistryKey* >( pRegistryKey )->createKey( CertificateContainer::impl_getStaticImplementationName() ) ); 
                    xNewKeyCertificateContainer = xNewKeyCertificateContainer->createKey( rtl::OUString::createFromAscii( "/UNO/SERVICES" ) );
			
                    const uno::Sequence< rtl::OUString >& rSNLCertificateContainer = CertificateContainer::impl_getStaticSupportedServiceNames();
                    const rtl::OUString* pArrayCertificateContainer = rSNLCertificateContainer.getConstArray();
                    for ( nPos = rSNLCertificateContainer.getLength(); nPos--; )
                        xNewKeyCertificateContainer->createKey( pArrayCertificateContainer[nPos] );    
            
                    //-----------------------------
                    
                    return sal_True;
		}
        catch (registry::InvalidRegistryException &)
		{
			DBG_ERROR( "InvalidRegistryException!" );
		}
	}
	return sal_False;
}

void* SAL_CALL component_getFactory( const sal_Char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
    void* pRet = 0;
    uno::Reference< XInterface > xFactory;
	
	//Decryptor
    rtl::OUString implName = rtl::OUString::createFromAscii( pImplName );
        
	if ( pServiceManager && implName.equals( DocumentDigitalSignatures::GetImplementationName() ) )
	{
        // DocumentDigitalSignatures
        xFactory = cppu::createSingleComponentFactory(
			DocumentDigitalSignatures_CreateInstance,
            rtl::OUString::createFromAscii( pImplName ),
            DocumentDigitalSignatures::GetSupportedServiceNames() );
	}
    else if ( pServiceManager && implName.equals( CertificateContainer::impl_getStaticImplementationName() ))
	{
        // CertificateContainer
        xFactory = cppu::createOneInstanceFactory(
            reinterpret_cast< lang::XMultiServiceFactory * >( pServiceManager ),
            rtl::OUString::createFromAscii( pImplName ), 
            CertificateContainer::impl_createInstance, 
            CertificateContainer::impl_getStaticSupportedServiceNames() );
	}
   
 	if (xFactory.is())
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}
	return pRet;
}

}   // extern "C"





