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

#ifndef CONNECTIVITY_DBTOOLS_PARSER_SIMPLE_HXX
#define CONNECTIVITY_DBTOOLS_PARSER_SIMPLE_HXX

#include <connectivity/virtualdbtools.hxx>
#include "refbase.hxx"
#include <connectivity/sqlparse.hxx>

//........................................................................
namespace connectivity
{
//........................................................................

	//================================================================
	//= OSimpleSQLParser
	//================================================================
	class OSimpleSQLParser
			:public simple::ISQLParser
			,public ORefBase
	{
	protected:
		OSQLParser		m_aFullParser;

	public:
		OSimpleSQLParser(const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& _rxServiceFactory,const IParseContext* _pContext);

		// ISQLParser
		virtual ::rtl::Reference< simple::ISQLParseNode > predicateTree(
			::rtl::OUString& rErrorMessage,
			const ::rtl::OUString& rStatement,
			const ::com::sun::star::uno::Reference< ::com::sun::star::util::XNumberFormatter >& _rxFormatter,
			const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& _rxField
		) const;

		virtual const IParseContext& getContext() const;

		// disambiguate IReference
		virtual oslInterlockedCount SAL_CALL acquire();
		virtual oslInterlockedCount SAL_CALL release();
	};

//........................................................................
}	// namespace connectivity
//........................................................................

#endif // CONNECTIVITY_DBTOOLS_PARSER_SIMPLE_HXX


