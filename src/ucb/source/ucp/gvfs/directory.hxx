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
#ifndef _GVFS_UCP_RESULTSET_HXX
#define _GVFS_UCP_RESULTSET_HXX

#include <rtl/ref.hxx>
#include <ucbhelper/resultset.hxx>
#include <ucbhelper/resultsethelper.hxx>
#include "content.hxx"

namespace gvfs {

class DynamicResultSet : public ::ucbhelper::ResultSetImplHelper
{
	rtl::Reference< Content > m_xContent;
	com::sun::star::uno::Reference<
		com::sun::star::ucb::XCommandEnvironment > m_xEnv;

private:
  	virtual void initStatic();
  	virtual void initDynamic();

public:
  	DynamicResultSet( const com::sun::star::uno::Reference<
		    			com::sun::star::lang::XMultiServiceFactory >& rxSMgr,
                      const rtl::Reference< Content >& rxContent,
				      const com::sun::star::ucb::OpenCommandArgument2& rCommand,
					  const com::sun::star::uno::Reference<
						com::sun::star::ucb::XCommandEnvironment >& rxEnv );
};


struct DataSupplier_Impl;
class DataSupplier : public ucbhelper::ResultSetDataSupplier
{
private:
	gvfs::DataSupplier_Impl *m_pImpl;
	sal_Bool getData();

public:
	DataSupplier( const com::sun::star::uno::Reference<
		      com::sun::star::lang::XMultiServiceFactory >& rxSMgr,
              const rtl::Reference< Content >& rContent,
			  sal_Int32 nOpenMode);

	virtual ~DataSupplier();

	virtual rtl::OUString queryContentIdentifierString( sal_uInt32 nIndex );
	virtual com::sun::star::uno::Reference<
				com::sun::star::ucb::XContentIdentifier >
	                      queryContentIdentifier( sal_uInt32 nIndex );
	virtual com::sun::star::uno::Reference< com::sun::star::ucb::XContent >
	                      queryContent( sal_uInt32 nIndex );

	virtual sal_Bool      getResult( sal_uInt32 nIndex );

	virtual sal_uInt32    totalCount();
	virtual sal_uInt32    currentCount();
	virtual sal_Bool      isCountFinal();

	virtual com::sun::star::uno::Reference< com::sun::star::sdbc::XRow >
	                      queryPropertyValues( sal_uInt32 nIndex  );
	virtual void          releasePropertyValues( sal_uInt32 nIndex );
	virtual void          close();
	virtual void          validate()
		throw( com::sun::star::ucb::ResultSetException );
};

}

#endif
