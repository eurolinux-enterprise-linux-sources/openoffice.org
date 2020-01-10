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

#ifndef _CONNECTIVITY_SDBCX_COLUMN_HXX_
#include "connectivity/PColumn.hxx"
#endif
#include "connectivity/dbtools.hxx"
#include "TConnection.hxx"
#include <comphelper/types.hxx>

using namespace ::comphelper;
using namespace connectivity;
using namespace dbtools;
using namespace connectivity::parse;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;

// -------------------------------------------------------------------------
OParseColumn::OParseColumn(const Reference<XPropertySet>& _xColumn,sal_Bool		_bCase)
	: connectivity::sdbcx::OColumn(	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)))
								,	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPENAME)))
								,	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISNULLABLE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRECISION)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE)))
								,	getBOOL(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT)))
								,	sal_False
								,	getBOOL(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))
								,	_bCase
								)
	, m_bFunction(sal_False)
	, m_bDbasePrecisionChanged(sal_False)
	, m_bAggregateFunction(sal_False)
    , m_bIsSearchable( sal_True )
{
	construct();
}

// -------------------------------------------------------------------------
OParseColumn::OParseColumn(	const ::rtl::OUString& _Name,
					const ::rtl::OUString& _TypeName,
					const ::rtl::OUString& _DefaultValue,
					sal_Int32		_IsNullable,
					sal_Int32		_Precision,
					sal_Int32		_Scale,
					sal_Int32		_Type,
					sal_Bool		_IsAutoIncrement,
					sal_Bool		_IsCurrency,
					sal_Bool		_bCase
				) : connectivity::sdbcx::OColumn(_Name,
								  _TypeName,
								  _DefaultValue,
								  _IsNullable,
								  _Precision,
								  _Scale,
								  _Type,
								  _IsAutoIncrement,
								  sal_False,
								  _IsCurrency,
								  _bCase)
	, m_bFunction(sal_False)
	, m_bDbasePrecisionChanged(sal_False)
	, m_bAggregateFunction(sal_False)
    , m_bIsSearchable( sal_True )
{
	construct();
}

// -------------------------------------------------------------------------
::vos::ORef< OSQLColumns > OParseColumn::createColumnsForResultSet( const Reference< XResultSetMetaData >& _rxResMetaData,
    const Reference< XDatabaseMetaData >& _rxDBMetaData )
{
    sal_Int32 nColumnCount = _rxResMetaData->getColumnCount();
    ::vos::ORef< OSQLColumns > aReturn( new OSQLColumns ); aReturn->get().reserve( nColumnCount );

    for ( sal_Int32 i = 1; i <= nColumnCount; ++i )
        aReturn->get().push_back( createColumnForResultSet( _rxResMetaData, _rxDBMetaData, i ) );

    return aReturn;
}

// -------------------------------------------------------------------------
OParseColumn* OParseColumn::createColumnForResultSet( const Reference< XResultSetMetaData >& _rxResMetaData,
    const Reference< XDatabaseMetaData >& _rxDBMetaData, sal_Int32 _nColumnPos )
{
	OParseColumn* pColumn = new OParseColumn(
        _rxResMetaData->getColumnName( _nColumnPos ),
		_rxResMetaData->getColumnTypeName( _nColumnPos ),
        ::rtl::OUString(),
        _rxResMetaData->isNullable( _nColumnPos ),
        _rxResMetaData->getPrecision( _nColumnPos ),
        _rxResMetaData->getScale( _nColumnPos ),
        _rxResMetaData->getColumnType( _nColumnPos ),
        _rxResMetaData->isAutoIncrement( _nColumnPos ),
        _rxResMetaData->isCurrency( _nColumnPos ),
        _rxDBMetaData->storesMixedCaseQuotedIdentifiers()
    );
    pColumn->setTableName(  ::dbtools::composeTableName( _rxDBMetaData,
        _rxResMetaData->getCatalogName( _nColumnPos ),
        _rxResMetaData->getSchemaName( _nColumnPos ),
        _rxResMetaData->getTableName( _nColumnPos ),
        sal_False,
        eComplete
    ) );
    pColumn->setIsSearchable( _rxResMetaData->isSearchable( _nColumnPos ) );
    return pColumn;
}

// -------------------------------------------------------------------------
OParseColumn::~OParseColumn()
{
}
// -------------------------------------------------------------------------
void OParseColumn::construct()
{
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_FUNCTION),				PROPERTY_ID_FUNCTION,				0,  &m_bFunction,		        ::getCppuType(reinterpret_cast< sal_Bool*>(NULL)));
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_AGGREGATEFUNCTION),		PROPERTY_ID_AGGREGATEFUNCTION,		0,  &m_bAggregateFunction,		::getCppuType(reinterpret_cast< sal_Bool*>(NULL)));
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TABLENAME),				PROPERTY_ID_TABLENAME,				0,  &m_aTableName,		        ::getCppuType(reinterpret_cast< ::rtl::OUString*>(NULL)));
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_REALNAME),				PROPERTY_ID_REALNAME,				0,  &m_aRealName,		        ::getCppuType(reinterpret_cast< ::rtl::OUString*>(NULL)));
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DBASEPRECISIONCHANGED),	PROPERTY_ID_DBASEPRECISIONCHANGED,	0,  &m_bDbasePrecisionChanged,	::getCppuType(reinterpret_cast<sal_Bool*>(NULL)));
    registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISSEARCHABLE),	        PROPERTY_ID_ISSEARCHABLE,			0,  &m_bIsSearchable,           ::getCppuType(reinterpret_cast< sal_Bool*>(NULL)));

}
// -----------------------------------------------------------------------------
::cppu::IPropertyArrayHelper* OParseColumn::createArrayHelper() const
{
    return doCreateArrayHelper();
}
// -----------------------------------------------------------------------------
::cppu::IPropertyArrayHelper & SAL_CALL OParseColumn::getInfoHelper()
{
    OSL_ENSURE( !isNew(), "OParseColumn::OOrderColumn: a *new* OrderColumn?" );
	return *OParseColumn_PROP::getArrayHelper();
}
// -----------------------------------------------------------------------------
OOrderColumn::OOrderColumn(	const Reference<XPropertySet>& _xColumn
									 ,sal_Bool	_bCase
									 ,sal_Bool _bAscending)
	: connectivity::sdbcx::OColumn(	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)))
								,	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPENAME)))
								,	getString(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISNULLABLE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRECISION)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE)))
								,	getINT32(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE)))
								,	getBOOL(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISAUTOINCREMENT)))
								,	sal_False
								,	getBOOL(_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))
								,	_bCase
								)
	, m_bAscending(_bAscending)
{
	construct();
}
// -------------------------------------------------------------------------
OOrderColumn::OOrderColumn(	const ::rtl::OUString& _Name,
					const ::rtl::OUString& _TypeName,
					const ::rtl::OUString& _DefaultValue,
					sal_Int32		_IsNullable,
					sal_Int32		_Precision,
					sal_Int32		_Scale,
					sal_Int32		_Type,
					sal_Bool		_IsAutoIncrement,
					sal_Bool		_IsCurrency,
					sal_Bool		_bCase
					,sal_Bool _bAscending
				) : connectivity::sdbcx::OColumn(_Name,
								  _TypeName,
								  _DefaultValue,
								  _IsNullable,
								  _Precision,
								  _Scale,
								  _Type,
								  _IsAutoIncrement,
								  sal_False,
								  _IsCurrency,
								  _bCase)
	, m_bAscending(_bAscending)
{
	construct();
}
// -------------------------------------------------------------------------
OOrderColumn::~OOrderColumn()
{
}
// -------------------------------------------------------------------------
void OOrderColumn::construct()
{
	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISASCENDING),PROPERTY_ID_ISASCENDING,0,&m_bAscending,		::getCppuType(reinterpret_cast< sal_Bool*>(NULL)));
}
// -----------------------------------------------------------------------------
::cppu::IPropertyArrayHelper* OOrderColumn::createArrayHelper() const
{
    return doCreateArrayHelper();
}    
// -----------------------------------------------------------------------------
::cppu::IPropertyArrayHelper & SAL_CALL OOrderColumn::getInfoHelper()
{
    OSL_ENSURE( !isNew(), "OOrderColumn::OOrderColumn: a *new* OrderColumn?" );
	return *OOrderColumn_PROP::getArrayHelper();
}
// -----------------------------------------------------------------------------
::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL OOrderColumn::getSupportedServiceNames(  ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Sequence< ::rtl::OUString > aSupported(1);
	if ( m_bOrder )
		aSupported[0] = ::rtl::OUString::createFromAscii("com.sun.star.sdb.OrderColumn");
	else
		aSupported[0] = ::rtl::OUString::createFromAscii("com.sun.star.sdb.GroupColumn");

	return aSupported;
}
// -----------------------------------------------------------------------------
