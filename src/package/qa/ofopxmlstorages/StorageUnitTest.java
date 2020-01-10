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
package complex.ofopxmlstorages;

import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.lang.XMultiComponentFactory;
import com.sun.star.connection.XConnector;
import com.sun.star.connection.XConnection;

import com.sun.star.bridge.XUnoUrlResolver;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.uno.XNamingService;
import com.sun.star.uno.XComponentContext;

import com.sun.star.container.*;
import com.sun.star.beans.*;
import com.sun.star.lang.*;

import complexlib.ComplexTestCase;

import complex.ofopxmlstorages.*;

import util.utils;
import java.util.*;
import java.io.*;

/* This unit test for storage objects is designed to
 * test most important statements from storage service
 * specification.
 *
 * Regression tests are added to extend the tested
 * functionalities.
 */
public class StorageUnitTest  extends ComplexTestCase
{
	private XMultiServiceFactory m_xMSF = null;
	private XSingleServiceFactory m_xStorageFactory = null;

    public String[] getTestMethodNames()
	{
        return new String[] {
								"ExecuteTest01",
								"ExecuteTest02",
								"ExecuteTest03",
								"ExecuteTest04",
								"ExecuteTest05",
								"ExecuteTest06",
								"ExecuteTest07",
								"ExecuteTest08"
							};
    }

    public String getTestObjectName()
	{
        return "StorageUnitTest";
    }

    public void before()
	{
        m_xMSF = (XMultiServiceFactory)param.getMSF();
		if ( m_xMSF == null )
		{
			failed( "Can't create service factory!" );
			return;
		}

		try {
			Object oStorageFactory = m_xMSF.createInstance( "com.sun.star.embed.StorageFactory" );
			m_xStorageFactory = (XSingleServiceFactory)UnoRuntime.queryInterface( XSingleServiceFactory.class,
																				oStorageFactory );
		}
		catch( Exception e )
		{
			failed( "Can't create storage factory!" );
			return;
		}

		if ( m_xStorageFactory == null )
		{
			failed( "Can't create service factory!" );
			return;
		}
    }

	public void ExecuteTest01()
	{
		StorageTest aTest = new Test01( m_xMSF, m_xStorageFactory, log );
		assure( "Test01 failed!", aTest.test() );
	}

	public void ExecuteTest02()
	{
		StorageTest aTest = new Test02( m_xMSF, m_xStorageFactory, log );
		assure( "Test02 failed!", aTest.test() );
	}

	public void ExecuteTest03()
	{
		StorageTest aTest = new Test03( m_xMSF, m_xStorageFactory, log );
		assure( "Test03 failed!", aTest.test() );
	}

	public void ExecuteTest04()
	{
		StorageTest aTest = new Test04( m_xMSF, m_xStorageFactory, log );
		assure( "Test04 failed!", aTest.test() );
	}

	public void ExecuteTest05()
	{
		StorageTest aTest = new Test05( m_xMSF, m_xStorageFactory, log );
		assure( "Test05 failed!", aTest.test() );
	}

	public void ExecuteTest06()
	{
		StorageTest aTest = new Test06( m_xMSF, m_xStorageFactory, log );
		assure( "Test06 failed!", aTest.test() );
	}

	public void ExecuteTest07()
	{
		StorageTest aTest = new Test07( m_xMSF, m_xStorageFactory, log );
		assure( "Test07 failed!", aTest.test() );
	}

	public void ExecuteTest08()
	{
		StorageTest aTest = new Test08( m_xMSF, m_xStorageFactory, log );
		assure( "Test08 failed!", aTest.test() );
	}
}

