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
#include "precompiled_configmgr.hxx"

#include <stdio.h>
#include <string.h>
#include <comphelper/stl_types.hxx>
#include <cppuhelper/extract.hxx>
#include <com/sun/star/uno/Type.hxx>
#include <com/sun/star/uno/TypeClass.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <cppuhelper/servicefactory.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::container;

#define ASCII_STRING(rtlOUString)	::rtl::OString((rtlOUString).getStr(), (rtlOUString).getLength(), RTL_TEXTENCODING_ASCII_US).getStr()

//=============================================================================
void explain(sal_Bool _bVerbose)
{
	cout << "cfgadduser - adding users to a registry server\n";
	cout << "\nusage :\n";
	cout << "cfgadduser [-s <server> -p <port>] [-portal] [-r <registry>] [-a <sysaccount>]";
	cout << "           [-h <homedirbase>] [-pwd] <user> [<user>]*\n";
	cout << "\nparameters\n";
	cout << "  <server>      - machine where the registry server is running\n";
	cout << "  <port>        - port the registry server is listening at\n";
	cout << "  <registry>    - registry file to use to instantiate services. Defaulted to\n";
	cout << "                  applicat.rdb\n";
	cout << "  <sysaccount>  - system account to use for the newly created user(s)\n";
	cout << "  <homedirbase> - home directory base. The concret home dir of a user will\n";
	cout << "                  be built by appending the the user name to the base dir.\n";
	cout << "  <user>        - user name to add\n";
	cout << "  -portal       - specify that the program should connect to a running portal,\n";
	cout << "                  not directly to the registry server (you need a ";
#ifdef WIN32
	cout << "portal.dll\n";
#else
	cout << "libportal.so\n";
#endif
	cout << "                  for this)\n";
	cout << "                  In this case, <server> and <port> specify the location where\n";
	cout << "                  StarPortal is running\n";
	cout << "\n";
	cout << "If no server is specified, the configuration proxy will try to bootstrap from\n";
	cout << "the initialization file (";
#ifdef WIN32
	cout << "sregistry.ini";
#else
	cout << "sregistryrc";
#endif
	cout << ")\n\n";
	cout.flush();
}

//=============================================================================
#if (defined UNX) || (defined OS2)
int main( int argc, char * argv[] )
#else
int _cdecl main( int argc, char * argv[] )
#endif
{
	sal_Char* pPort = NULL;
	sal_Char* pServer = NULL;
	sal_Char* pRegistry = NULL;
	sal_Char* pSysAccount = NULL;
	sal_Char* pHomeDirBase = NULL;
	sal_Bool bPortal = sal_False;

	::std::vector< sal_Char* > aUsers;

	// collect some parameters
	sal_Char** pArgs = argv + 1;
	for (sal_Int32 i=1; i<argc; ++i, ++pArgs)
	{
		sal_Char* pCurArg = *pArgs;
		sal_Int32 nLen = strlen(pCurArg);
		sal_Bool bInvalidArg = sal_True;
		if (nLen && ('-' == *pCurArg))
		{	// it's a switch
			sal_Char* pSwitch = pCurArg + 1;
			switch (nLen)
			{
				case 2:
					switch (*pSwitch)
					{
						case '?':
							explain(sal_True);
							return 1;
						case 'h':
							pHomeDirBase = *++pArgs;
							++i;
							bInvalidArg = sal_False;
							break;
						case 'a':
							pSysAccount = *++pArgs;
							++i;
							bInvalidArg = sal_False;
							break;
						case 'p':
							pPort = *++pArgs;
							++i;
							bInvalidArg = sal_False;
							break;
						case 's':
							pServer = *++pArgs;
							++i;
							bInvalidArg = sal_False;
							break;
						case 'r':
							pRegistry = *++pArgs;
							++i;
							bInvalidArg = sal_False;
							break;
					}
					break;
				case 7:
					if (0 == strncmp(pSwitch, "portal", 6))
					{
						bInvalidArg = sal_False;
						bPortal = sal_True;
					}
					break;
			}
		}
		else
		{
			if ((1 == nLen) && ('?' == *pCurArg))
			{
				explain(sal_True);
				return 1;
			}
			else
			{
				bInvalidArg = sal_False;
				aUsers.push_back(pCurArg);
			}
		}
		if (bInvalidArg)
		{
			explain(sal_False);
			return 1;
		}
	}

	if ((!pServer && pPort) || (!pPort && pServer))
	{
		explain(sal_False);
		return 1;
	}

	if (0 == aUsers.size())
	{
		explain(sal_False);
		return 1;
	}

	// refine some params
	::rtl::OUString sHomeDirBase, sSystemAccountName;
	if (pHomeDirBase)
	{
		sHomeDirBase = ::rtl::OUString::createFromAscii(pHomeDirBase);
		if (!sHomeDirBase.getLength() || ('/' != sHomeDirBase.getStr()[sHomeDirBase.getLength() - 1]))
			sHomeDirBase += ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("/"));
	}
	if (pSysAccount)
		sSystemAccountName = ::rtl::OUString::createFromAscii(pSysAccount);

	try
	{
		::rtl::OUString const sServiceRegistry = ::rtl::OUString::createFromAscii( pRegistry ? pRegistry : "applicat.rdb" );
		Reference< XMultiServiceFactory > xORB = ::cppu::createRegistryServiceFactory(
			sServiceRegistry,
			::rtl::OUString()
			);
		if (!xORB.is())
		{
			cerr << "Could not create the service factory !\n\n";
			return 1;
		}

		// collect the params for the config provider
		Sequence< Any > aProviderArgs(3 + (pServer ? 2 : 0));
		aProviderArgs[0] = makeAny(PropertyValue(
				::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("servertype")),
				0,
				makeAny(::rtl::OUString::createFromAscii(bPortal ? "portal" : "remote")),
				PropertyState_DIRECT_VALUE
				));
		aProviderArgs[1] = makeAny(PropertyValue(
				::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("user")),
				0,
				makeAny(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Administrator"))),
				PropertyState_DIRECT_VALUE
				));
		aProviderArgs[2] = makeAny(PropertyValue(
				::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("password")),
				0,
				makeAny(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("unused"))),
				PropertyState_DIRECT_VALUE
				));
		if (pServer)
		{
			aProviderArgs[3] = makeAny(PropertyValue(
					::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("server")),
					0,
					makeAny(::rtl::OUString::createFromAscii(pServer)),
					PropertyState_DIRECT_VALUE
					));

			sal_Int32 nPort = ::rtl::OUString::createFromAscii(pPort).toInt32();
			aProviderArgs[4] = makeAny(PropertyValue(
					::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("port")),
					0,
					makeAny(nPort),
					PropertyState_DIRECT_VALUE
					));
		}

		Reference< XMultiServiceFactory > xCfgProvider(
			xORB->createInstanceWithArguments(::rtl::OUString::createFromAscii("com.sun.star.configuration.ConfigurationProvider"),
			aProviderArgs),
			UNO_QUERY);
		if (!xCfgProvider.is())
		{
			cerr << "Could not create the configuration provider !\n\n";
			return 3;
		}

		Reference< XInterface > xIFace = xCfgProvider->createInstance(
			::rtl::OUString::createFromAscii("com.sun.star.configuration.UserAdministration"));
		if (!xIFace.is())
		{
			cerr << "Could not create the configuration provider !\n\n";
			return 4;
		}

		Reference< XChangesBatch > xUserChanges(xIFace, UNO_QUERY);
		Reference< XNameContainer > xUserContainer(xIFace, UNO_QUERY);
		Reference< XSingleServiceFactory> xUserFactory(xIFace, UNO_QUERY);
		if (!xUserChanges.is() || !xUserContainer.is() || !xUserFactory.is())
		{
			cerr << "the user admin access does not provide the necessary interfaces !\n\n";
			return 5;
		}

		cout << "going to add the users ..." << endl << endl;
		for (	::std::vector< sal_Char* >::const_iterator aUserLoop = aUsers.begin();
				aUserLoop != aUsers.end();
				++aUserLoop
			)
		{
			cout << *aUserLoop << " ... ";
			sal_Bool bHadLinebreak = sal_False;
			try
			{
				Reference< XInterface > xNewUser = xUserFactory->createInstance();

				// the user name as unicode string use more than once)
				::rtl::OUString sUserName = ::rtl::OUString::createFromAscii(*aUserLoop);

				// the XNameContainer access to the Security node in the user profile data
				Reference< XNameReplace > xSecurityDataAccess;
				Reference< XNameAccess > xUserDataAccess(xNewUser, UNO_QUERY);
				if (xUserDataAccess.is())
				{
					Any aSecurity = xUserDataAccess->getByName(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Security")));
					::cppu::extractInterface(xSecurityDataAccess, aSecurity);
				}

				if (!xSecurityDataAccess.is())
					throw Exception(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("The user administration service did not provide a valid user template.")), NULL);

				// set the home directory
				if (sHomeDirBase.getLength())
				{
					::rtl::OUString sHomeDir(sHomeDirBase);
					sHomeDir += sUserName;
					xSecurityDataAccess->replaceByName(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("HomeDirectory")), makeAny(sHomeDir));
					cout << "\n\thome dir      : " << ASCII_STRING(sHomeDir) << " ... ";
					cout.flush();
					bHadLinebreak = sal_True;
				}

				if (sSystemAccountName.getLength())
				{
					xSecurityDataAccess->replaceByName(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("SystemAccount")), makeAny(sSystemAccountName));
					cout << "\n\tsystem account: " << ASCII_STRING(sSystemAccountName) << " ... ";
					cout.flush();
					bHadLinebreak = sal_True;
				}

				xUserContainer->insertByName(sUserName, makeAny(xNewUser));
				xUserChanges->commitChanges();
				if (bHadLinebreak)
					cout << "\n";
				cout << "done.\n";
				cout.flush();
			}
			catch(Exception& e)
			{
				cout << "\n";
				if (!bHadLinebreak)
					cout << "\t";
				cerr << "unable to add the user named " << *aUserLoop << endl;
				if (!bHadLinebreak)
					cout << "\t";
				cerr << "(exception message: " << ::rtl::OString(e.Message.getStr(), e.Message.getLength(), RTL_TEXTENCODING_ASCII_US).getStr() << ")" << endl;
			}
			cout << "\n";
		}
	}
	catch(Exception& e)
	{
		cerr << "Caught exception: " << ASCII_STRING(e.Message) << endl;
		return 2;
	}

	return 0;
}
