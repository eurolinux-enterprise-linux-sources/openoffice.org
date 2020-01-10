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

#include "MacabTables.hxx"
#include "MacabTable.hxx"
#include "MacabCatalog.hxx"
#include "MacabConnection.hxx"
#include <comphelper/types.hxx>

using namespace connectivity::macab;
using namespace connectivity;
using namespace ::comphelper;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;

sdbcx::ObjectType MacabTables::createObject(const ::rtl::OUString& _rName)
{
	::rtl::OUString aName,aSchema;
	aSchema = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("%"));
	aName = _rName;

	Sequence< ::rtl::OUString > aTypes(1);
	aTypes[0] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("%"));
	::rtl::OUString sEmpty;

	Reference< XResultSet > xResult = m_xMetaData->getTables(Any(), aSchema, aName, aTypes);

	sdbcx::ObjectType xRet = NULL;
	if (xResult.is())
	{
        Reference< XRow > xRow(xResult, UNO_QUERY);
		if (xResult->next()) // there can be only one table with this name
		{
			MacabTable* pRet = new MacabTable(
					this,
					static_cast<MacabCatalog&>(m_rParent).getConnection(),
					aName,
					xRow->getString(4),
					xRow->getString(5),
					sEmpty);
			xRet = pRet;
		}
	}
	::comphelper::disposeComponent(xResult);

	return xRet;
}
// -------------------------------------------------------------------------
void MacabTables::impl_refresh(  ) throw(RuntimeException)
{
	static_cast<MacabCatalog&>(m_rParent).refreshTables();
}
// -------------------------------------------------------------------------
void MacabTables::disposing(void)
{
m_xMetaData.clear();
	OCollection::disposing();
}
