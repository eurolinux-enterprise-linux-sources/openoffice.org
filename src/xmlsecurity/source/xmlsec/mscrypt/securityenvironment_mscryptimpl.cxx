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

#ifdef _MSC_VER
#pragma warning(push,1)
#endif
#include "Windows.h"
#include "WinCrypt.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <sal/config.h>
#include "securityenvironment_mscryptimpl.hxx"

#ifndef _X509CERTIFICATE_NSSIMPL_HXX_
#include "x509certificate_mscryptimpl.hxx"
#endif
#include <rtl/uuid.h>

#include <xmlsec/xmlsec.h>
#include <xmlsec/keysmngr.h>
#include <xmlsec/crypto.h>
#include <xmlsec/base64.h>

#include <xmlsecurity/biginteger.hxx>

#include "xmlsec/keysmngr.h"
#include "xmlsec/mscrypto/akmngr.h"

//CP : added by CP
#include <rtl/locale.h>
#include <osl/nlsupport.h> 
#include <osl/process.h>

//CP : end

using namespace ::com::sun::star::uno ;
using namespace ::com::sun::star::lang ;
using ::com::sun::star::lang::XMultiServiceFactory ;
using ::com::sun::star::lang::XSingleServiceFactory ;
using ::rtl::OUString ;

using ::com::sun::star::xml::crypto::XSecurityEnvironment ;
using ::com::sun::star::security::XCertificate ;

extern X509Certificate_MSCryptImpl* MswcryCertContextToXCert( PCCERT_CONTEXT cert ) ;

SecurityEnvironment_MSCryptImpl :: SecurityEnvironment_MSCryptImpl( const Reference< XMultiServiceFactory >& aFactory ) : m_hProv( NULL ) , m_pszContainer( NULL ) , m_hKeyStore( NULL ), m_hCertStore( NULL ), m_tSymKeyList() , m_tPubKeyList() , m_tPriKeyList(), m_xServiceManager( aFactory ), m_bEnableDefault( sal_False ) {

}

SecurityEnvironment_MSCryptImpl :: ~SecurityEnvironment_MSCryptImpl() {

	if( m_hProv != NULL ) {
		CryptReleaseContext( m_hProv, 0 ) ;
		m_hProv = NULL ;
	}

	if( m_pszContainer != NULL ) {
		//TODO: Don't know whether or not it should be released now.
		m_pszContainer = NULL ;
	}

	if( m_hCertStore != NULL ) {
		CertCloseStore( m_hCertStore, CERT_CLOSE_STORE_FORCE_FLAG ) ;
		m_hCertStore = NULL ;
	}

	if( m_hKeyStore != NULL ) {
		CertCloseStore( m_hKeyStore, CERT_CLOSE_STORE_FORCE_FLAG ) ;
		m_hKeyStore = NULL ;
	}

	if( !m_tSymKeyList.empty()  ) {
		std::list< HCRYPTKEY >::iterator symKeyIt ;

		for( symKeyIt = m_tSymKeyList.begin() ; symKeyIt != m_tSymKeyList.end() ; symKeyIt ++ )
			CryptDestroyKey( *symKeyIt ) ;
	}

	if( !m_tPubKeyList.empty()  ) {
		std::list< HCRYPTKEY >::iterator pubKeyIt ;

		for( pubKeyIt = m_tPubKeyList.begin() ; pubKeyIt != m_tPubKeyList.end() ; pubKeyIt ++ )
			CryptDestroyKey( *pubKeyIt ) ;
	}

	if( !m_tPriKeyList.empty()  ) {
		std::list< HCRYPTKEY >::iterator priKeyIt ;

		for( priKeyIt = m_tPriKeyList.begin() ; priKeyIt != m_tPriKeyList.end() ; priKeyIt ++ )
			CryptDestroyKey( *priKeyIt ) ;
	}
	
}

/* XInitialization */
void SAL_CALL SecurityEnvironment_MSCryptImpl :: initialize( const Sequence< Any >& /*aArguments*/ ) throw( Exception, RuntimeException ) {
	//TODO
} ;

/* XServiceInfo */
OUString SAL_CALL SecurityEnvironment_MSCryptImpl :: getImplementationName() throw( RuntimeException ) {
	return impl_getImplementationName() ;
}

/* XServiceInfo */
sal_Bool SAL_CALL SecurityEnvironment_MSCryptImpl :: supportsService( const OUString& serviceName) throw( RuntimeException ) {
	Sequence< OUString > seqServiceNames = getSupportedServiceNames() ;
	const OUString* pArray = seqServiceNames.getConstArray() ;
	for( sal_Int32 i = 0 ; i < seqServiceNames.getLength() ; i ++ ) {
		if( *( pArray + i ) == serviceName )
			return sal_True ;
	}
	return sal_False ;
}

/* XServiceInfo */
Sequence< OUString > SAL_CALL SecurityEnvironment_MSCryptImpl :: getSupportedServiceNames() throw( RuntimeException ) {
	return impl_getSupportedServiceNames() ;
}

//Helper for XServiceInfo
Sequence< OUString > SecurityEnvironment_MSCryptImpl :: impl_getSupportedServiceNames() {
	::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() ) ;
	Sequence< OUString > seqServiceNames( 1 ) ;
	seqServiceNames.getArray()[0] = OUString::createFromAscii( "com.sun.star.xml.crypto.SecurityEnvironment" ) ;
	return seqServiceNames ;
}

OUString SecurityEnvironment_MSCryptImpl :: impl_getImplementationName() throw( RuntimeException ) {
	return OUString::createFromAscii( "com.sun.star.xml.security.bridge.xmlsec.SecurityEnvironment_MSCryptImpl" ) ;
}

//Helper for registry
Reference< XInterface > SAL_CALL SecurityEnvironment_MSCryptImpl :: impl_createInstance( const Reference< XMultiServiceFactory >& aServiceManager ) throw( RuntimeException ) {
	return Reference< XInterface >( *new SecurityEnvironment_MSCryptImpl( aServiceManager ) ) ;
}

Reference< XSingleServiceFactory > SecurityEnvironment_MSCryptImpl :: impl_createFactory( const Reference< XMultiServiceFactory >& aServiceManager ) {
	return ::cppu::createSingleFactory( aServiceManager , impl_getImplementationName() , impl_createInstance , impl_getSupportedServiceNames() ) ;
}

/* XUnoTunnel */
sal_Int64 SAL_CALL SecurityEnvironment_MSCryptImpl :: getSomething( const Sequence< sal_Int8 >& aIdentifier ) 
	throw( RuntimeException )
{
	if( aIdentifier.getLength() == 16 && 0 == rtl_compareMemory( getUnoTunnelId().getConstArray(), aIdentifier.getConstArray(), 16 ) ) { 
		return ( sal_Int64 )this ;
	}
	return 0 ;
}

/* XUnoTunnel extension */
const Sequence< sal_Int8>& SecurityEnvironment_MSCryptImpl :: getUnoTunnelId() {
	static Sequence< sal_Int8 >* pSeq = 0 ;
	if( !pSeq ) {
		::osl::Guard< ::osl::Mutex > aGuard( ::osl::Mutex::getGlobalMutex() ) ;
		if( !pSeq ) {
			static Sequence< sal_Int8> aSeq( 16 ) ;
			rtl_createUuid( ( sal_uInt8* )aSeq.getArray() , 0 , sal_True ) ;
			pSeq = &aSeq ;
		}
	}
	return *pSeq ;
}

/* XUnoTunnel extension */
SecurityEnvironment_MSCryptImpl* SecurityEnvironment_MSCryptImpl :: getImplementation( const Reference< XInterface > xObj ) {
	Reference< XUnoTunnel > xUT( xObj , UNO_QUERY ) ;
	if( xUT.is() ) {
		return ( SecurityEnvironment_MSCryptImpl* )xUT->getSomething( getUnoTunnelId() ) ;
	} else
		return NULL ;
}

/* Native methods */
HCRYPTPROV SecurityEnvironment_MSCryptImpl :: getCryptoProvider() throw( ::com::sun::star::uno::Exception , ::com::sun::star::uno::RuntimeException ) {
	return m_hProv ;
}

void SecurityEnvironment_MSCryptImpl :: setCryptoProvider( HCRYPTPROV aProv ) throw( ::com::sun::star::uno::Exception , ::com::sun::star::uno::RuntimeException ) {
	if( m_hProv != NULL ) {
		CryptReleaseContext( m_hProv, 0 ) ;
		m_hProv = NULL ;
	}

	if( aProv != NULL ) {
		/*- Replaced by direct adopt for WINNT support ----
		if( !CryptContextAddRef( aProv, NULL, NULL ) )
			throw Exception() ;
		else
			m_hProv = aProv ;
		----*/
		m_hProv = aProv ;
	}
}

LPCTSTR SecurityEnvironment_MSCryptImpl :: getKeyContainer() throw( ::com::sun::star::uno::Exception , ::com::sun::star::uno::RuntimeException ) {
	return m_pszContainer ;
}

void SecurityEnvironment_MSCryptImpl :: setKeyContainer( LPCTSTR aKeyContainer ) throw( ::com::sun::star::uno::Exception , ::com::sun::star::uno::RuntimeException ) {
	//TODO: Don't know whether or not it should be copied.
	m_pszContainer = aKeyContainer ;
}


HCERTSTORE SecurityEnvironment_MSCryptImpl :: getCryptoSlot() throw( Exception , RuntimeException ) {
	return m_hKeyStore ;
}

void SecurityEnvironment_MSCryptImpl :: setCryptoSlot( HCERTSTORE aSlot) throw( Exception , RuntimeException ) {
	if( m_hKeyStore != NULL ) {
		CertCloseStore( m_hKeyStore, CERT_CLOSE_STORE_FORCE_FLAG ) ;
		m_hKeyStore = NULL ;
	}

	if( aSlot != NULL ) {
		m_hKeyStore = CertDuplicateStore( aSlot ) ;
	}
}

HCERTSTORE SecurityEnvironment_MSCryptImpl :: getCertDb() throw( Exception , RuntimeException ) {
	return m_hCertStore ;
}

void SecurityEnvironment_MSCryptImpl :: setCertDb( HCERTSTORE aCertDb ) throw( Exception , RuntimeException ) {
	if( m_hCertStore != NULL ) {
		CertCloseStore( m_hCertStore, CERT_CLOSE_STORE_FORCE_FLAG ) ;
		m_hCertStore = NULL ;
	}

	if( aCertDb != NULL ) {
		m_hCertStore = CertDuplicateStore( aCertDb ) ;
	}
}

void SecurityEnvironment_MSCryptImpl :: adoptSymKey( HCRYPTKEY aSymKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY	symkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aSymKey != NULL ) {
		//First try to find the key in the list
		for( keyIt = m_tSymKeyList.begin() ; keyIt != m_tSymKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aSymKey )
				return ;
		}

		//If we do not find the key in the list, add a new node
		/*- Replaced with directly adopt for WINNT 4.0 support ----
		if( !CryptDuplicateKey( aSymKey, NULL, 0, &symkey ) )
			throw RuntimeException() ;
		----*/
		symkey = aSymKey ;

		try {
			m_tSymKeyList.push_back( symkey ) ;
		} catch ( Exception& ) {
			CryptDestroyKey( symkey ) ;
		}
	}
}

void SecurityEnvironment_MSCryptImpl :: rejectSymKey( HCRYPTKEY aSymKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY symkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aSymKey != NULL ) {
		for( keyIt = m_tSymKeyList.begin() ; keyIt != m_tSymKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aSymKey ) {
				symkey = *keyIt ;
				CryptDestroyKey( symkey ) ;
				m_tSymKeyList.erase( keyIt ) ;
				break ;
			}
		}
	}
}

HCRYPTKEY SecurityEnvironment_MSCryptImpl :: getSymKey( unsigned int position ) throw( Exception , RuntimeException ) {
	HCRYPTKEY symkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;
	unsigned int pos ;

	symkey = NULL ;
	for( pos = 0, keyIt = m_tSymKeyList.begin() ; pos < position && keyIt != m_tSymKeyList.end() ; pos ++ , keyIt ++ ) ;

	if( pos == position && keyIt != m_tSymKeyList.end() )
		symkey = *keyIt ;

	return symkey ;
}

void SecurityEnvironment_MSCryptImpl :: adoptPubKey( HCRYPTKEY aPubKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY	pubkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aPubKey != NULL ) {
		//First try to find the key in the list
		for( keyIt = m_tPubKeyList.begin() ; keyIt != m_tPubKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aPubKey )
				return ;
		}

		//If we do not find the key in the list, add a new node
		/*- Replaced with directly adopt for WINNT 4.0 support ----
		if( !CryptDuplicateKey( aPubKey, NULL, 0, &pubkey ) )
			throw RuntimeException() ;
		----*/
		pubkey = aPubKey ;

		try {
			m_tPubKeyList.push_back( pubkey ) ;
		} catch ( Exception& ) {
			CryptDestroyKey( pubkey ) ;
		}
	}
}

void SecurityEnvironment_MSCryptImpl :: rejectPubKey( HCRYPTKEY aPubKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY pubkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aPubKey != NULL ) {
		for( keyIt = m_tPubKeyList.begin() ; keyIt != m_tPubKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aPubKey ) {
				pubkey = *keyIt ;
				CryptDestroyKey( pubkey ) ;
				m_tPubKeyList.erase( keyIt ) ;
				break ;
			}
		}
	}
}

HCRYPTKEY SecurityEnvironment_MSCryptImpl :: getPubKey( unsigned int position ) throw( Exception , RuntimeException ) {
	HCRYPTKEY pubkey ;
	std::list< HCRYPTKEY >::iterator keyIt ;
	unsigned int pos ;

	pubkey = NULL ;
	for( pos = 0, keyIt = m_tPubKeyList.begin() ; pos < position && keyIt != m_tPubKeyList.end() ; pos ++ , keyIt ++ ) ;

	if( pos == position && keyIt != m_tPubKeyList.end() )
		pubkey = *keyIt ;

	return pubkey ;
}

void SecurityEnvironment_MSCryptImpl :: adoptPriKey( HCRYPTKEY aPriKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY	prikey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aPriKey != NULL ) {
		//First try to find the key in the list
		for( keyIt = m_tPriKeyList.begin() ; keyIt != m_tPriKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aPriKey )
				return ;
		}

		//If we do not find the key in the list, add a new node
		/*- Replaced with directly adopt for WINNT 4.0 support ----
		if( !CryptDuplicateKey( aPriKey, NULL, 0, &prikey ) )
			throw RuntimeException() ;
		----*/
		prikey = aPriKey ;

		try {
			m_tPriKeyList.push_back( prikey ) ;
		} catch ( Exception& ) {
			CryptDestroyKey( prikey ) ;
		}
	}
}

void SecurityEnvironment_MSCryptImpl :: rejectPriKey( HCRYPTKEY aPriKey ) throw( Exception , RuntimeException ) {
	HCRYPTKEY	prikey ;
	std::list< HCRYPTKEY >::iterator keyIt ;

	if( aPriKey != NULL ) {
		for( keyIt = m_tPriKeyList.begin() ; keyIt != m_tPriKeyList.end() ; keyIt ++ ) {
			if( *keyIt == aPriKey ) {
				prikey = *keyIt ;
				CryptDestroyKey( prikey ) ;
				m_tPriKeyList.erase( keyIt ) ;
				break ;
			}
		}
	}
}

HCRYPTKEY SecurityEnvironment_MSCryptImpl :: getPriKey( unsigned int position ) throw( Exception , RuntimeException ) {
	HCRYPTKEY prikey ;
	std::list< HCRYPTKEY >::iterator keyIt ;
	unsigned int pos ;

	prikey = NULL ;
	for( pos = 0, keyIt = m_tPriKeyList.begin() ; pos < position && keyIt != m_tPriKeyList.end() ; pos ++ , keyIt ++ ) ;

	if( pos == position && keyIt != m_tPriKeyList.end() )
		prikey = *keyIt ;

	return prikey ;
}

//Methods from XSecurityEnvironment
Sequence< Reference < XCertificate > > SecurityEnvironment_MSCryptImpl :: getPersonalCertificates() throw( SecurityException , RuntimeException ) 
{
	sal_Int32 length ;
	X509Certificate_MSCryptImpl* xcert ;
	std::list< X509Certificate_MSCryptImpl* > certsList ;
	PCCERT_CONTEXT pCertContext = NULL;

	//firstly, we try to find private keys in given key store.
	if( m_hKeyStore != NULL ) {
		pCertContext = CertEnumCertificatesInStore( m_hKeyStore, pCertContext );
		while (pCertContext)
        {
			xcert = MswcryCertContextToXCert( pCertContext ) ;
			if( xcert != NULL )
				certsList.push_back( xcert ) ;
            pCertContext = CertEnumCertificatesInStore( m_hKeyStore, pCertContext );
		}
	}

	//secondly, we try to find certificate from registered private keys.
	if( !m_tPriKeyList.empty()  ) {
		//TODO: Don't know whether or not it is necessary ans possible.
	}

	//Thirdly, we try to find certificate from system default key store.
	if( m_bEnableDefault ) {
		HCERTSTORE hSystemKeyStore ;
		DWORD      dwKeySpec;
		HCRYPTPROV hCryptProv;

		/*
		hSystemKeyStore = CertOpenStore(
				CERT_STORE_PROV_SYSTEM ,
				0 ,
				NULL ,
				CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG ,
				L"MY"
			) ;
		*/
		hSystemKeyStore = CertOpenSystemStore( 0, "MY" ) ;
		if( hSystemKeyStore != NULL ) {
			pCertContext = CertEnumCertificatesInStore( hSystemKeyStore, pCertContext );
			while (pCertContext)
            {
				// Add By CP for checking whether the certificate is a personal certificate or not.
				if(!(CryptAcquireCertificatePrivateKey(pCertContext,
						CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
						NULL,
						&hCryptProv,
						&dwKeySpec,
						NULL)))
				{
					// Not Privatekey found. SKIP this one; By CP
                    pCertContext = CertEnumCertificatesInStore( hSystemKeyStore, pCertContext );
					continue;
				}
				// then TODO : Check the personal cert is valid or not.

				// end CP
				xcert = MswcryCertContextToXCert( pCertContext ) ;
				if( xcert != NULL )
					certsList.push_back( xcert ) ;
                pCertContext = CertEnumCertificatesInStore( hSystemKeyStore, pCertContext );
			}
		}

		CertCloseStore( hSystemKeyStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
	}

	length = certsList.size() ;
	if( length != 0 ) {
		int i ;
		std::list< X509Certificate_MSCryptImpl* >::iterator xcertIt ;
		Sequence< Reference< XCertificate > > certSeq( length ) ;

		for( i = 0, xcertIt = certsList.begin(); xcertIt != certsList.end(); xcertIt ++, i++ ) {
			certSeq[i] = *xcertIt ;
		}

		return certSeq ;
	}

	return Sequence< Reference< XCertificate > >() ;
}


Reference< XCertificate > SecurityEnvironment_MSCryptImpl :: getCertificate( const OUString& issuerName, const Sequence< sal_Int8 >& serialNumber ) throw( SecurityException , RuntimeException ) {
	unsigned int i ;
//	sal_Int8 found = 0 ;
	LPSTR	pszName ;
	X509Certificate_MSCryptImpl *xcert = NULL ;
	PCCERT_CONTEXT pCertContext = NULL ;
	HCERTSTORE hCertStore = NULL ;
	CRYPT_INTEGER_BLOB cryptSerialNumber ;
	CERT_INFO certInfo ;

	// By CP , for correct encoding
	sal_uInt16 encoding ;
	rtl_Locale *pLocale = NULL ;
	osl_getProcessLocale( &pLocale ) ;
	encoding = osl_getTextEncodingFromLocale( pLocale ) ;
	// CP end

	//Create cert info from issue and serial
	rtl::OString oissuer = rtl::OUStringToOString( issuerName , encoding ) ;
	pszName = ( char* )oissuer.getStr() ;

	if( ! ( CertStrToName(
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING ,
		pszName ,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG | CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG,
		NULL ,
		NULL ,
		&certInfo.Issuer.cbData, NULL ) )
	) {
		return NULL ;
	}

    certInfo.Issuer.pbData = ( BYTE* )malloc( certInfo.Issuer.cbData );
	if(!certInfo.Issuer.pbData)
		throw RuntimeException() ;

	if( ! ( CertStrToName(
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING ,
		pszName ,
		CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG | CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG,
		NULL ,
		( BYTE* )certInfo.Issuer.pbData ,
		&certInfo.Issuer.cbData, NULL ) )
	) {
		free( certInfo.Issuer.pbData ) ;
		return NULL ;
	}

	//Get the SerialNumber
	cryptSerialNumber.cbData = serialNumber.getLength() ;
    cryptSerialNumber.pbData = ( BYTE* )malloc( cryptSerialNumber.cbData); 
	if (!cryptSerialNumber.pbData) 
	{
		free( certInfo.Issuer.pbData ) ;
		throw RuntimeException() ;
	}
	for( i = 0; i < cryptSerialNumber.cbData; i ++ )
		cryptSerialNumber.pbData[i] = serialNumber[ cryptSerialNumber.cbData - i - 1 ] ;

	certInfo.SerialNumber.cbData = cryptSerialNumber.cbData ;
	certInfo.SerialNumber.pbData = cryptSerialNumber.pbData ;
		
	// Get the Cert from all store.
	for( i = 0 ; i < 6 ; i ++ )
	{
		switch(i)
		{
		case 0:
			if(m_hKeyStore == NULL) continue ;
			hCertStore = m_hKeyStore ;
			break;
		case 1:
			if(m_hCertStore == NULL) continue ;
			hCertStore = m_hCertStore ;
			break;
		case 2:
			hCertStore = CertOpenSystemStore( 0, "MY" ) ;
			if(hCertStore == NULL || !m_bEnableDefault) continue ;
			break;
		case 3:
			hCertStore = CertOpenSystemStore( 0, "Root" ) ;
			if(hCertStore == NULL || !m_bEnableDefault) continue ;
			break;
		case 4:
			hCertStore = CertOpenSystemStore( 0, "Trust" ) ;
			if(hCertStore == NULL || !m_bEnableDefault) continue ;
			break;
		case 5:
			hCertStore = CertOpenSystemStore( 0, "CA" ) ;
			if(hCertStore == NULL || !m_bEnableDefault) continue ;
			break;
		default:
			i=6;
			continue;
		}

/******************************************************************************* 
 * This code reserved for remind us there are another way to find one cert by 
 * IssuerName&serialnumber. You can use the code to replaced the function 
 * CertFindCertificateInStore IF and ONLY IF you must find one special cert in
 * certStore but can not be found by CertFindCertificateInStore , then , you 
 * should also change the same part in libxmlsec/.../src/mscrypto/x509vfy.c#875.
 * By Chandler Peng(chandler.peng@sun.com)
 *****/
/*******************************************************************************
		pCertContext = NULL ;
		found = 0;
		do{
			//	1. enum the certs has same string in the issuer string.
			pCertContext = CertEnumCertificatesInStore( hCertStore , pCertContext ) ;
			if( pCertContext != NULL )
			{
				// 2. check the cert's issuer name .
				char* issuer = NULL ;
				DWORD cbIssuer = 0 ;

				cbIssuer = CertNameToStr(
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING ,
					&( pCertContext->pCertInfo->Issuer ),
					CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG ,
					NULL, 0
				) ;
				
				if( cbIssuer == 0 ) continue ; // discard this cert;

				issuer = (char *)malloc( cbIssuer ) ;
				if( issuer == NULL )  // discard this cert;
				{
					free( cryptSerialNumber.pbData) ;
					free( certInfo.Issuer.pbData ) ;
					CertFreeCertificateContext( pCertContext ) ;
					if(i != 0 && i != 1) CertCloseStore( hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
					throw RuntimeException() ;
				}

				cbIssuer = CertNameToStr(
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING ,
					&( pCertContext->pCertInfo->Issuer ),
					CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG ,
					issuer, cbIssuer
				) ;

				if( cbIssuer <= 0 )
				{
					free( issuer ) ;
					continue ;// discard this cert;
				}

				if(strncmp(pszName , issuer , cbIssuer) != 0) 
				{
					free( issuer ) ;
					continue ;// discard this cert;
				}
				free( issuer ) ;

				// 3. check the serial number.
				if( memcmp( cryptSerialNumber.pbData , pCertContext->pCertInfo->SerialNumber.pbData  , cryptSerialNumber.cbData ) != 0 )
				{
					continue ;// discard this cert;
				}

				// 4. confirm and break;
				found = 1;
				break ;
			}

		}while(pCertContext);
		
		if(i != 0 && i != 1) CertCloseStore( hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
		if( found != 0 ) break; // Found the certificate.
********************************************************************************/

		pCertContext = CertFindCertificateInStore(
			hCertStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			0,
			CERT_FIND_SUBJECT_CERT,
			&certInfo,
			NULL
		) ;
	
		if(i != 0 && i != 1) CertCloseStore( hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
		if( pCertContext != NULL ) break ; // Found the certificate.

	}

	if( cryptSerialNumber.pbData ) free( cryptSerialNumber.pbData ) ;
	if( certInfo.Issuer.pbData ) free( certInfo.Issuer.pbData ) ;

	if( pCertContext != NULL ) {
		xcert = MswcryCertContextToXCert( pCertContext ) ;
		if( pCertContext ) CertFreeCertificateContext( pCertContext ) ;
	} else {
		xcert = NULL ;
	}

	return xcert ;
}

Reference< XCertificate > SecurityEnvironment_MSCryptImpl :: getCertificate( const OUString& issuerName, const OUString& serialNumber ) throw( SecurityException , RuntimeException ) {
	Sequence< sal_Int8 > serial = numericStringToBigInteger( serialNumber ) ;
	return getCertificate( issuerName, serial ) ;
}

Sequence< Reference < XCertificate > > SecurityEnvironment_MSCryptImpl :: buildCertificatePath( const Reference< XCertificate >& begin ) throw( SecurityException , RuntimeException ) {
	PCCERT_CHAIN_CONTEXT pChainContext ;
	PCCERT_CONTEXT pCertContext ;
	const X509Certificate_MSCryptImpl* xcert ;

	CERT_ENHKEY_USAGE	enhKeyUsage ;
	CERT_USAGE_MATCH	certUsage ;
	CERT_CHAIN_PARA		chainPara ;

	enhKeyUsage.cUsageIdentifier = 0 ;
	enhKeyUsage.rgpszUsageIdentifier = NULL ;
	certUsage.dwType = USAGE_MATCH_TYPE_AND ;
	certUsage.Usage = enhKeyUsage ;
	chainPara.cbSize = sizeof( CERT_CHAIN_PARA ) ;
	chainPara.RequestedUsage = certUsage ;

	Reference< XUnoTunnel > xCertTunnel( begin, UNO_QUERY ) ;
	if( !xCertTunnel.is() ) {
		throw RuntimeException() ;
	}

	xcert = ( X509Certificate_MSCryptImpl* )xCertTunnel->getSomething( X509Certificate_MSCryptImpl::getUnoTunnelId() ) ;
	if( xcert == NULL ) {
		throw RuntimeException() ;
	}

	pCertContext = xcert->getMswcryCert() ;

	pChainContext = NULL ;

    BOOL bChain = FALSE;
    if( pCertContext != NULL )
    {
        HCERTSTORE hAdditionalStore = NULL;
        HCERTSTORE hCollectionStore = NULL;
        if (m_hCertStore && m_hKeyStore)
        {
            //Merge m_hCertStore and m_hKeyStore into one store.
            hCollectionStore = CertOpenStore(
				CERT_STORE_PROV_COLLECTION ,
				0 ,
				NULL ,
				0 ,
				NULL
                ) ;
            if (hCollectionStore != NULL)
            {
                CertAddStoreToCollection (
 					hCollectionStore ,
 					m_hCertStore ,
 					CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG ,
 					0) ;
                CertAddStoreToCollection (
 					hCollectionStore ,
 					m_hCertStore ,
 					CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG ,
 					0) ;
                hAdditionalStore = hCollectionStore;
            }

        }

        //if the merge of both stores failed then we add only m_hCertStore
        if (hAdditionalStore == NULL && m_hCertStore)
            hAdditionalStore = m_hCertStore;
        else if (hAdditionalStore == NULL && m_hKeyStore)
            hAdditionalStore = m_hKeyStore;
        else
            hAdditionalStore = NULL;

        //CertGetCertificateChain searches by default in MY, CA, ROOT and TRUST
    	bChain = CertGetCertificateChain(
		    NULL ,
		    pCertContext ,
		    NULL , //use current system time
		    hAdditionalStore,
		    &chainPara ,
		    CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_TIMESTAMP_TIME ,
		    NULL ,
		    &pChainContext);
        if (!bChain)
			pChainContext = NULL;
		
        //Close the additional store
       CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }

	if(bChain &&  pChainContext != NULL && pChainContext->cChain > 0 )
    {
		PCCERT_CONTEXT pCertInChain ;
		PCERT_SIMPLE_CHAIN pCertChain ;
		X509Certificate_MSCryptImpl* pCert ;

		pCertChain = pChainContext->rgpChain[0] ;
		if( pCertChain->cElement ) {
			Sequence< Reference< XCertificate > > xCertChain( pCertChain->cElement ) ;

			for( unsigned int i = 0 ; i < pCertChain->cElement ; i ++ ) {
				if( pCertChain->rgpElement[i] )
					pCertInChain = pCertChain->rgpElement[i]->pCertContext ;
				else
					pCertInChain = NULL ;

				if( pCertInChain != NULL ) {
					pCert = MswcryCertContextToXCert( pCertInChain ) ;
					if( pCert != NULL )
						xCertChain[i] = pCert ;
				}
			}

			CertFreeCertificateChain( pChainContext ) ;
			pChainContext = NULL ;

			return xCertChain ;
		}
	}
    if (pChainContext)
        CertFreeCertificateChain(pChainContext);

	return NULL ;
}

Reference< XCertificate > SecurityEnvironment_MSCryptImpl :: createCertificateFromRaw( const Sequence< sal_Int8 >& rawCertificate ) throw( SecurityException , RuntimeException ) {
	X509Certificate_MSCryptImpl* xcert ;

	if( rawCertificate.getLength() > 0 ) {
		xcert = new X509Certificate_MSCryptImpl() ;
		if( xcert == NULL )
			throw RuntimeException() ;

		xcert->setRawCert( rawCertificate ) ;
	} else {
		xcert = NULL ;
	}

	return xcert ;
}

Reference< XCertificate > SecurityEnvironment_MSCryptImpl :: createCertificateFromAscii( const OUString& asciiCertificate ) throw( SecurityException , RuntimeException ) {
	xmlChar* chCert ;
	xmlSecSize certSize ;

	rtl::OString oscert = rtl::OUStringToOString( asciiCertificate , RTL_TEXTENCODING_ASCII_US ) ;

	chCert = xmlStrndup( ( const xmlChar* )oscert.getStr(), ( int )oscert.getLength() ) ;

	certSize = xmlSecBase64Decode( chCert, ( xmlSecByte* )chCert, xmlStrlen( chCert ) ) ;

	Sequence< sal_Int8 > rawCert( certSize ) ;
	for( unsigned int i = 0 ; i < certSize ; i ++ )
		rawCert[i] = *( chCert + i ) ;

	xmlFree( chCert ) ;

	return createCertificateFromRaw( rawCert ) ;
}


HCERTSTORE getCertStoreForIntermediatCerts(
    const Sequence< Reference< ::com::sun::star::security::XCertificate > >& seqCerts)
{
    HCERTSTORE store = NULL;
    store = CertOpenStore(
        CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL);
    if (store == NULL)
        return NULL;

    for (int i = 0; i < seqCerts.getLength(); i++)
    {
        Sequence<sal_Int8> data = seqCerts[i]->getEncoded();
        PCCERT_CONTEXT cert = CertCreateCertificateContext(
            X509_ASN_ENCODING, ( const BYTE* )&data[0], data.getLength());
        //Adding the certificate creates a copy and not just increases the ref count
        //Therefore we free later the certificate that we now add
        CertAddCertificateContextToStore(store, cert, CERT_STORE_ADD_ALWAYS, NULL);
        CertFreeCertificateContext(cert);
    }
    return store;
}
sal_Int32 SecurityEnvironment_MSCryptImpl :: verifyCertificate( 
    const Reference< ::com::sun::star::security::XCertificate >& aCert,
    const Sequence< Reference< ::com::sun::star::security::XCertificate > >& seqCerts) 
    throw( ::com::sun::star::uno::SecurityException, ::com::sun::star::uno::RuntimeException ) 
{
	sal_Int32 validity = 0;
	PCCERT_CHAIN_CONTEXT pChainContext = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	const X509Certificate_MSCryptImpl* xcert = NULL;
	DWORD chainStatus ;

	CERT_ENHKEY_USAGE	enhKeyUsage ;
	CERT_USAGE_MATCH	certUsage ;
	CERT_CHAIN_PARA		chainPara ;

	Reference< XUnoTunnel > xCertTunnel( aCert, UNO_QUERY ) ;
	if( !xCertTunnel.is() ) {
		throw RuntimeException() ;
	}

	xcert = ( X509Certificate_MSCryptImpl* )xCertTunnel->getSomething( X509Certificate_MSCryptImpl::getUnoTunnelId() ) ;
	if( xcert == NULL ) {
		throw RuntimeException() ;
	}

	pCertContext = xcert->getMswcryCert() ;

    //Prepare parameter for CertGetCertificateChain
	enhKeyUsage.cUsageIdentifier = 0 ;
	enhKeyUsage.rgpszUsageIdentifier = NULL ;
	certUsage.dwType = USAGE_MATCH_TYPE_AND ;
	certUsage.Usage = enhKeyUsage ;
	chainPara.cbSize = sizeof( CERT_CHAIN_PARA ) ;
	chainPara.RequestedUsage = certUsage ;


    HCERTSTORE hCollectionStore = NULL;
    HCERTSTORE hIntermediateCertsStore = NULL;
	BOOL bChain = FALSE;
	if( pCertContext != NULL )
    {
        hIntermediateCertsStore =
            getCertStoreForIntermediatCerts(seqCerts);

        //Merge m_hCertStore and m_hKeyStore and the store of the intermediate 
        //certificates into one store.
        hCollectionStore = CertOpenStore(
			CERT_STORE_PROV_COLLECTION ,
			0 ,
			NULL ,
			0 ,
			NULL
            ) ;
        if (hCollectionStore != NULL)
        {
            CertAddStoreToCollection (
				hCollectionStore ,
				m_hCertStore ,
				CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG ,
				0) ;
            CertAddStoreToCollection (
				hCollectionStore ,
				m_hCertStore ,
				CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG ,
				0) ;
            CertAddStoreToCollection (
                hCollectionStore,
                hIntermediateCertsStore,
                CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                0);
                
        }

        //CertGetCertificateChain searches by default in MY, CA, ROOT and TRUST
        bChain = CertGetCertificateChain(
            NULL ,
            pCertContext ,
            NULL , //use current system time
            hCollectionStore,
            &chainPara ,
            CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_TIMESTAMP_TIME ,
            NULL ,
        	&pChainContext);

    	if (!bChain)
			pChainContext = NULL;

    }

	if(bChain && pChainContext != NULL )
    {
		chainStatus = pChainContext->TrustStatus.dwErrorStatus ;

		// JL & TKR: Until we have a test suite to test all error types we just say that the cert is
		// valid or invalid with no further separation.
		// Error CERT_TRUST_IS_OFFLINE_REVOCATION and CERT_TRUST_REVOCATION_STATUS_UNKNOWN are treated separate
		// because they are ignored ( Bad! ) in the currently situation 

		if( chainStatus == CERT_TRUST_NO_ERROR ) 
		{
			validity = ::com::sun::star::security::CertificateValidity::VALID ;
		} 
		
		if ( ( chainStatus & CERT_TRUST_IS_OFFLINE_REVOCATION ) == CERT_TRUST_IS_OFFLINE_REVOCATION ) {
			validity |= ::com::sun::star::security::CertificateValidity::UNKNOWN_REVOKATION ;
		} 		
		
		if ( ( chainStatus & CERT_TRUST_REVOCATION_STATUS_UNKNOWN ) == CERT_TRUST_REVOCATION_STATUS_UNKNOWN ) {
			validity |= ::com::sun::star::security::CertificateValidity::UNKNOWN_REVOKATION ;
		}

		if (chainStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE
			|| chainStatus & CERT_TRUST_IS_CYCLIC
			|| chainStatus & CERT_TRUST_INVALID_POLICY_CONSTRAINTS
			|| chainStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS
			|| chainStatus & CERT_TRUST_INVALID_NAME_CONSTRAINTS
			|| chainStatus & CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_TIME_VALID
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE
			|| chainStatus & CERT_TRUST_IS_NOT_TIME_VALID
			|| chainStatus & CERT_TRUST_IS_NOT_TIME_NESTED
			|| chainStatus & CERT_TRUST_IS_REVOKED
			|| chainStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID
			|| chainStatus & CERT_TRUST_IS_UNTRUSTED_ROOT
			|| chainStatus & CERT_TRUST_INVALID_EXTENSION
			|| chainStatus & CERT_TRUST_IS_PARTIAL_CHAIN )
		{
			validity = ::com::sun::star::security::CertificateValidity::INVALID;
		}
/*		
		
		if( ( chainStatus & CERT_TRUST_IS_NOT_TIME_VALID ) == CERT_TRUST_IS_NOT_TIME_VALID ) {
			validity |= ::com::sun::star::security::CertificateValidity::TIME_INVALID ;
		}

		if( ( chainStatus & CERT_TRUST_IS_NOT_TIME_NESTED ) == CERT_TRUST_IS_NOT_TIME_NESTED ) {
			validity |= ::com::sun::star::security::CertificateValidity::NOT_TIME_NESTED;
		}

		if( ( chainStatus & CERT_TRUST_IS_REVOKED ) == CERT_TRUST_IS_REVOKED ) {
			validity |= ::com::sun::star::security::CertificateValidity::REVOKED ;
		}

		//JL My interpretation is that CERT_TRUST_IS_OFFLINE_REVOCATION  does not mean that the certificate was revoked.
        //Instead the CRL cannot be retrieved from the net, or an available CRL is stale (too old).
        //This error may also occurs if the certificate does not contain the CDP (Certificate Distribution Point)extension
		if( ( chainStatus & CERT_TRUST_IS_OFFLINE_REVOCATION ) == CERT_TRUST_IS_OFFLINE_REVOCATION ) {
			validity |= ::com::sun::star::security::CertificateValidity::UNKNOWN_REVOKATION ;
		}

		if( ( chainStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID ) == CERT_TRUST_IS_NOT_SIGNATURE_VALID ) {
			validity |= ::com::sun::star::security::CertificateValidity::SIGNATURE_INVALID ;
		}

		if( ( chainStatus & CERT_TRUST_IS_UNTRUSTED_ROOT ) == CERT_TRUST_IS_UNTRUSTED_ROOT ) {
			validity |= ::com::sun::star::security::CertificateValidity::ROOT_UNTRUSTED ;
		}

		if( ( chainStatus & CERT_TRUST_REVOCATION_STATUS_UNKNOWN ) == CERT_TRUST_REVOCATION_STATUS_UNKNOWN ) {
			validity |= ::com::sun::star::security::CertificateValidity::UNKNOWN_REVOKATION ;
		}

		if( ( chainStatus & CERT_TRUST_INVALID_EXTENSION ) == CERT_TRUST_INVALID_EXTENSION ) {
			validity |= ::com::sun::star::security::CertificateValidity::EXTENSION_INVALID ;
		}

		if( ( chainStatus & CERT_TRUST_IS_PARTIAL_CHAIN ) == CERT_TRUST_IS_PARTIAL_CHAIN ) {
			validity |= ::com::sun::star::security::CertificateValidity::CHAIN_INCOMPLETE ;
		}
		//todo 
		if (chainStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE
			|| chainStatus & CERT_TRUST_IS_CYCLIC
			|| chainStatus & CERT_TRUST_INVALID_POLICY_CONSTRAINTS
			|| chainStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS
			|| chainStatus & CERT_TRUST_INVALID_NAME_CONSTRAINTS
			|| chainStatus & CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT
			|| chainStatus & CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_TIME_VALID
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID
			|| chainStatus & CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE)
		{
			validity = ::com::sun::star::security::CertificateValidity::INVALID;
		}
*/
	} else {
		validity = ::com::sun::star::security::CertificateValidity::INVALID ;
	}

	if (pChainContext)
		CertFreeCertificateChain(pChainContext);

    //Close the additional store, do not destroy the contained certs
    CertCloseStore(hCollectionStore, CERT_CLOSE_STORE_CHECK_FLAG);
    //Close the temporary store containing the intermediate certificates and make
    //sure all certificates are deleted.
    CertCloseStore(hIntermediateCertsStore, CERT_CLOSE_STORE_CHECK_FLAG);

	return validity ;
}

sal_Int32 SecurityEnvironment_MSCryptImpl :: getCertificateCharacters( const ::com::sun::star::uno::Reference< ::com::sun::star::security::XCertificate >& aCert ) throw( ::com::sun::star::uno::SecurityException, ::com::sun::star::uno::RuntimeException ) {
	sal_Int32 characters ;
	PCCERT_CONTEXT pCertContext ;
	const X509Certificate_MSCryptImpl* xcert ;

	Reference< XUnoTunnel > xCertTunnel( aCert, UNO_QUERY ) ;
	if( !xCertTunnel.is() ) {
		throw RuntimeException() ;
	}

	xcert = ( X509Certificate_MSCryptImpl* )xCertTunnel->getSomething( X509Certificate_MSCryptImpl::getUnoTunnelId() ) ;
	if( xcert == NULL ) {
		throw RuntimeException() ;
	}

	pCertContext = xcert->getMswcryCert() ;

	characters = 0x00000000 ;

	//Firstly, make sentence whether or not the cert is self-signed.
	if( CertCompareCertificateName( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &(pCertContext->pCertInfo->Subject), &(pCertContext->pCertInfo->Issuer) ) ) {
		characters |= ::com::sun::star::security::CertificateCharacters::SELF_SIGNED ;
	} else {
		characters &= ~ ::com::sun::star::security::CertificateCharacters::SELF_SIGNED ;
	}

	//Secondly, make sentence whether or not the cert has a private key.
	{
		BOOL	fCallerFreeProv ;
		DWORD	dwKeySpec ;
		HCRYPTPROV	hProv ;
		if( CryptAcquireCertificatePrivateKey( pCertContext ,
				   0 ,
				   NULL ,
				   &( hProv ) ,
				   &( dwKeySpec ) ,
				   &( fCallerFreeProv ) )
		) {
			characters |=  ::com::sun::star::security::CertificateCharacters::HAS_PRIVATE_KEY ;

			if( hProv != NULL && fCallerFreeProv )
				CryptReleaseContext( hProv, 0 ) ;
		} else {
			characters &= ~ ::com::sun::star::security::CertificateCharacters::HAS_PRIVATE_KEY ;
		}
	}
	return characters ;
}

void SecurityEnvironment_MSCryptImpl :: enableDefaultCrypt( sal_Bool enable ) throw( Exception, RuntimeException ) {
	m_bEnableDefault = enable ;
}

sal_Bool SecurityEnvironment_MSCryptImpl :: defaultEnabled() throw( Exception, RuntimeException ) {
	return m_bEnableDefault ;
}

X509Certificate_MSCryptImpl* MswcryCertContextToXCert( PCCERT_CONTEXT cert )
{
	X509Certificate_MSCryptImpl* xcert ;

	if( cert != NULL ) {
		xcert = new X509Certificate_MSCryptImpl() ;
		if( xcert != NULL ) {
			xcert->setMswcryCert( cert ) ;
		}
	} else {
		xcert = NULL ;
	}

	return xcert ;
}

::rtl::OUString SecurityEnvironment_MSCryptImpl::getSecurityEnvironmentInformation() throw( ::com::sun::star::uno::RuntimeException )
{
	return rtl::OUString::createFromAscii("Microsoft Crypto API");
}

/* Native methods */
xmlSecKeysMngrPtr SecurityEnvironment_MSCryptImpl :: createKeysManager() throw( Exception, RuntimeException ) {

	unsigned int i ;
	HCRYPTKEY symKey ;
	HCRYPTKEY pubKey ;
	HCRYPTKEY priKey ;
	xmlSecKeysMngrPtr pKeysMngr = NULL ;

	/*-
	 * The following lines is based on the of xmlsec-mscrypto crypto engine
	 */
	pKeysMngr = xmlSecMSCryptoAppliedKeysMngrCreate( m_hKeyStore , m_hCertStore ) ;
	if( pKeysMngr == NULL )
		throw RuntimeException() ;

	/*-
	 * Adopt symmetric key into keys manager
	 */
	for( i = 0 ; ( symKey = getSymKey( i ) ) != NULL ; i ++ ) {
		if( xmlSecMSCryptoAppliedKeysMngrSymKeyLoad( pKeysMngr, symKey ) < 0 ) {
			throw RuntimeException() ;
		}
	}

	/*-
	 * Adopt asymmetric public key into keys manager
	 */
	for( i = 0 ; ( pubKey = getPubKey( i ) ) != NULL ; i ++ ) {
		if( xmlSecMSCryptoAppliedKeysMngrPubKeyLoad( pKeysMngr, pubKey ) < 0 ) {
			throw RuntimeException() ;
		}
	}

	/*-
	 * Adopt asymmetric private key into keys manager
	 */
	for( i = 0 ; ( priKey = getPriKey( i ) ) != NULL ; i ++ ) {
		if( xmlSecMSCryptoAppliedKeysMngrPriKeyLoad( pKeysMngr, priKey ) < 0 ) {
			throw RuntimeException() ;
		}
	}

	/*-
	 * Adopt system default certificate store.
	 */
	if( defaultEnabled() ) {
		HCERTSTORE hSystemStore ;

		//Add system key store into the keys manager.
		hSystemStore = CertOpenSystemStore( 0, "MY" ) ;
		if( hSystemStore != NULL ) {
			if( xmlSecMSCryptoAppliedKeysMngrAdoptKeyStore( pKeysMngr, hSystemStore ) < 0 ) {
				CertCloseStore( hSystemStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
				throw RuntimeException() ;
			}
		}

		//Add system root store into the keys manager.
		hSystemStore = CertOpenSystemStore( 0, "Root" ) ;
		if( hSystemStore != NULL ) {
			if( xmlSecMSCryptoAppliedKeysMngrAdoptTrustedStore( pKeysMngr, hSystemStore ) < 0 ) {
				CertCloseStore( hSystemStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
				throw RuntimeException() ;
			}
		}

		//Add system trusted store into the keys manager.
		hSystemStore = CertOpenSystemStore( 0, "Trust" ) ;
		if( hSystemStore != NULL ) {
			if( xmlSecMSCryptoAppliedKeysMngrAdoptUntrustedStore( pKeysMngr, hSystemStore ) < 0 ) {
				CertCloseStore( hSystemStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
				throw RuntimeException() ;
			}
		}

		//Add system CA store into the keys manager.
		hSystemStore = CertOpenSystemStore( 0, "CA" ) ;
		if( hSystemStore != NULL ) {
			if( xmlSecMSCryptoAppliedKeysMngrAdoptUntrustedStore( pKeysMngr, hSystemStore ) < 0 ) {
				CertCloseStore( hSystemStore, CERT_CLOSE_STORE_CHECK_FLAG ) ;
				throw RuntimeException() ;
			}
		}
	}

	return pKeysMngr ;
}
void SecurityEnvironment_MSCryptImpl :: destroyKeysManager(xmlSecKeysMngrPtr pKeysMngr) throw( Exception, RuntimeException ) {
	if( pKeysMngr != NULL ) {
		xmlSecKeysMngrDestroy( pKeysMngr ) ;
	}
}
