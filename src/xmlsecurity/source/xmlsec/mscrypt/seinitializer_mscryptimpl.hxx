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

#ifndef _SEINITIALIZERIMPL_HXX
#define _SEINITIALIZERIMPL_HXX

#include <com/sun/star/xml/crypto/XXMLSecurityContext.hpp>
#ifndef _COM_SUN_STAR_XML_CRYPTO_SEINITIALIZER_HPP_
#include <com/sun/star/xml/crypto/XSEInitializer.hpp>
#endif
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <cppuhelper/implbase2.hxx>

#include <libxml/tree.h>

class SEInitializer_MSCryptImpl : public cppu::WeakImplHelper2 
< 
	com::sun::star::xml::crypto::XSEInitializer,
	com::sun::star::lang::XServiceInfo
>
/****** SEInitializer_MSCryptImpl.hxx/CLASS SEInitializer_MSCryptImpl ***********
 *
 *   NAME
 *	SEInitializer_MSCryptImpl -- Class to initialize a Security Context
 *	instance
 *
 *   FUNCTION
 *	Use this class to initialize a XmlSec based Security Context
 *	instance. After this instance is used up, use this class to free this
 *	instance.
 *
 *   HISTORY
 *	05.01.2004 -	Interface supported: XSEInitializer, XSEInitializer
 *
 *   AUTHOR
 *	Michael Mi
 *	Email: michael.mi@sun.com
 ******************************************************************************/
{
private:
	com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory > mxMSF;
	
public:
	SEInitializer_MSCryptImpl(const com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory > &rxMSF);
	virtual ~SEInitializer_MSCryptImpl();

	/* XSEInitializer */
	virtual com::sun::star::uno::Reference< 
		com::sun::star::xml::crypto::XXMLSecurityContext >
		SAL_CALL createSecurityContext( const rtl::OUString& certDB )
		throw (com::sun::star::uno::RuntimeException);
		
	virtual void SAL_CALL freeSecurityContext( const com::sun::star::uno::Reference<
		com::sun::star::xml::crypto::XXMLSecurityContext >& securityContext )
		throw (com::sun::star::uno::RuntimeException);
	
	/* XServiceInfo */
	virtual rtl::OUString SAL_CALL getImplementationName(  ) 
		throw (com::sun::star::uno::RuntimeException);
		
	virtual sal_Bool SAL_CALL supportsService( const rtl::OUString& ServiceName ) 
		throw (com::sun::star::uno::RuntimeException);
		
	virtual com::sun::star::uno::Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames(  ) 
		throw (com::sun::star::uno::RuntimeException);
};

rtl::OUString SEInitializer_MSCryptImpl_getImplementationName()
	throw ( com::sun::star::uno::RuntimeException );

sal_Bool SAL_CALL SEInitializer_MSCryptImpl_supportsService( const rtl::OUString& ServiceName ) 
	throw ( com::sun::star::uno::RuntimeException );

com::sun::star::uno::Sequence< rtl::OUString > SAL_CALL SEInitializer_MSCryptImpl_getSupportedServiceNames(  ) 
	throw ( com::sun::star::uno::RuntimeException );

com::sun::star::uno::Reference< com::sun::star::uno::XInterface >
SAL_CALL SEInitializer_MSCryptImpl_createInstance( const com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory > & rSMgr)
	throw ( com::sun::star::uno::Exception );

#endif

