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
#include "TDatabaseMetaDataBase.hxx"
#include "RowFunctionParser.hxx"

#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/evtlistenerhlp.hxx>
#include <com/sun/star/lang/XComponent.hpp>
#include "resource/sharedresources.hxx"
#include "resource/common_res.hrc"
#include <connectivity/dbexception.hxx>

using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace comphelper;
using namespace connectivity;


ODatabaseMetaDataBase::ODatabaseMetaDataBase(const Reference< XConnection >& _rxConnection,const Sequence< PropertyValue >& _rInfo)
    : m_aConnectionInfo(_rInfo)
    ,m_isCatalogAtStart(false,sal_False)
    ,m_sCatalogSeparator(false,::rtl::OUString())
    ,m_sIdentifierQuoteString(false,::rtl::OUString())
    ,m_supportsCatalogsInTableDefinitions(false,sal_False)
    ,m_supportsSchemasInTableDefinitions(false,sal_False)
    ,m_supportsCatalogsInDataManipulation(false,sal_False)
    ,m_supportsSchemasInDataManipulation(false,sal_False)
    ,m_supportsMixedCaseQuotedIdentifiers(false,sal_False)
    ,m_supportsAlterTableWithAddColumn(false,sal_False)
    ,m_supportsAlterTableWithDropColumn(false,sal_False)
    ,m_MaxStatements(false,0)
    ,m_MaxTablesInSelect(false,0)
    ,m_storesMixedCaseQuotedIdentifiers(false,sal_False)
	, m_xConnection(_rxConnection)
{
	osl_incrementInterlockedCount( &m_refCount );
	{
		m_xListenerHelper = new OEventListenerHelper(this);
		Reference<XComponent> xCom(m_xConnection,UNO_QUERY);
		if(xCom.is())
			xCom->addEventListener(m_xListenerHelper);
	}
	osl_decrementInterlockedCount( &m_refCount );
}
// -------------------------------------------------------------------------
ODatabaseMetaDataBase::~ODatabaseMetaDataBase()
{
}

// -----------------------------------------------------------------------------
Sequence< PropertyValue > SAL_CALL ODatabaseMetaDataBase::getConnectionInfo(  ) throw (RuntimeException)
{
    return m_aConnectionInfo;
}

// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseMetaDataBase::disposing( const EventObject& /*Source*/ ) throw(RuntimeException)
{
	// cut off all references to the connection
m_xConnection.clear();
m_xListenerHelper.clear();
}
// -----------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getTypeInfo(  ) throw(SQLException, RuntimeException)
{
    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_aTypeInfoRows.empty() )
    {
        Reference< XResultSet > xRet = impl_getTypeInfo_throw();
        Reference< XRow > xRow(xRet,UNO_QUERY);
        ::comphelper::SequenceAsHashMap aMap(m_aConnectionInfo);
        Sequence< Any > aTypeInfoSettings;
        aTypeInfoSettings = aMap.getUnpackedValueOrDefault(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("TypeInfoSettings")),aTypeInfoSettings);
        
        if ( xRow.is() )
        {
            static sal_Int32 pTypes[] = {
                                        DataType::VARCHAR
                                        ,DataType::INTEGER
                                        ,DataType::INTEGER
                                        ,DataType::VARCHAR
                                        ,DataType::VARCHAR
                                        ,DataType::VARCHAR
                                        ,DataType::INTEGER
                                        ,DataType::BOOLEAN
                                        ,DataType::INTEGER
                                        ,DataType::BOOLEAN
                                        ,DataType::BOOLEAN
                                        ,DataType::BOOLEAN
                                        ,DataType::VARCHAR
                                        ,DataType::INTEGER
                                        ,DataType::INTEGER
                                        ,DataType::INTEGER
                                        ,DataType::INTEGER
                                        ,DataType::INTEGER
                                    };
            ::std::vector<ExpressionNodeSharedPtr> aConditions;
            if ( aTypeInfoSettings.getLength() > 1 && ((aTypeInfoSettings.getLength() % 2) == 0) )
            {
                const Any* pIter = aTypeInfoSettings.getConstArray();
                const Any* pEnd	 = pIter + aTypeInfoSettings.getLength();
                try
                {
                    for(;pIter != pEnd;++pIter)
                        aConditions.push_back(FunctionParser::parseFunction(::comphelper::getString(*pIter)));
                }
                catch(ParseError&)
                {
                    ::connectivity::SharedResources aResources;
                    const ::rtl::OUString sError( aResources.getResourceString(STR_FORMULA_WRONG));
                    ::dbtools::throwGenericSQLException(sError,*this);
                }
            }
            
            while( xRet->next() )
            {
                ::connectivity::ODatabaseMetaDataResultSet::ORow aRow;
                aRow.push_back(ODatabaseMetaDataResultSet::getEmptyValue());
                sal_Int32* pType = pTypes;
                for (sal_Int32 i = 1; i <= sal_Int32(sizeof(pTypes)/sizeof(pTypes[0])); ++i,++pType)
                {
                    ORowSetValue aValue;
                    aValue.fill(i,*pType,xRow);
                    aRow.push_back(new ORowSetValueDecorator(aValue));
                }
                
                ::std::vector<ExpressionNodeSharedPtr>::iterator aIter = aConditions.begin();
                ::std::vector<ExpressionNodeSharedPtr>::iterator aEnd = aConditions.end();
                for (; aIter != aEnd; ++aIter)
                {
                    if ( (*aIter)->evaluate(aRow)->getValue().getBool() )
                    {
                        ++aIter;
                        (*aIter)->fill(aRow);
                    }
                    else
                        ++aIter;
                }
                m_aTypeInfoRows.push_back(aRow);
            }
        }
    }
    ::connectivity::ODatabaseMetaDataResultSet* pResult = new ::connectivity::ODatabaseMetaDataResultSet(::connectivity::ODatabaseMetaDataResultSet::eTypeInfo);
	Reference< XResultSet > xRet = pResult;
	pResult->setRows(m_aTypeInfoRows);
    return xRet;
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getExportedKeys(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eExportedKeys );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getImportedKeys(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eImportedKeys );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getPrimaryKeys(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::ePrimaryKeys );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getIndexInfo(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/,
        sal_Bool /*unique*/, sal_Bool /*approximate*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eIndexInfo );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getBestRowIdentifier(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/, sal_Int32 /*scope*/,
        sal_Bool /*nullable*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eBestRowIdentifier );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getCrossReference(
        const Any& /*primaryCatalog*/, const ::rtl::OUString& /*primarySchema*/,
        const ::rtl::OUString& /*primaryTable*/, const Any& /*foreignCatalog*/,
        const ::rtl::OUString& /*foreignSchema*/, const ::rtl::OUString& /*foreignTable*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eCrossReference );
}
// -------------------------------------------------------------------------
Reference< XConnection > SAL_CALL ODatabaseMetaDataBase::getConnection(  ) throw(SQLException, RuntimeException)
{
    return m_xConnection;
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getProcedureColumns(
        const Any& /*catalog*/, const ::rtl::OUString& /*schemaPattern*/,
        const ::rtl::OUString& /*procedureNamePattern*/, const ::rtl::OUString& /*columnNamePattern*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eProcedureColumns );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getProcedures(
        const Any& /*catalog*/, const ::rtl::OUString& /*schemaPattern*/,
        const ::rtl::OUString& /*procedureNamePattern*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eProcedures );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getVersionColumns(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eVersionColumns );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getSchemas(  ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eSchemas );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getColumnPrivileges(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/,
        const ::rtl::OUString& /*columnNamePattern*/ ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eColumnPrivileges );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getTablePrivileges(
        const Any& /*catalog*/, const ::rtl::OUString& /*schema*/, const ::rtl::OUString& /*table*/) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eTablePrivileges );
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL ODatabaseMetaDataBase::getCatalogs(  ) throw(SQLException, RuntimeException)
{
    return new ODatabaseMetaDataResultSet( ODatabaseMetaDataResultSet::eCatalogs );
}
// -----------------------------------------------------------------------------
::rtl::OUString SAL_CALL ODatabaseMetaDataBase::getIdentifierQuoteString(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_sIdentifierQuoteString,::std::mem_fun_t< ::rtl::OUString ,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_getIdentifierQuoteString_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::isCatalogAtStart(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_isCatalogAtStart,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_isCatalogAtStart_throw));
}
// -----------------------------------------------------------------------------
::rtl::OUString SAL_CALL ODatabaseMetaDataBase::getCatalogSeparator(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_sCatalogSeparator,::std::mem_fun_t< ::rtl::OUString,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_getCatalogSeparator_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsCatalogsInTableDefinitions(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsCatalogsInTableDefinitions,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsCatalogsInTableDefinitions_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsSchemasInTableDefinitions(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsSchemasInTableDefinitions,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsSchemasInTableDefinitions_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsCatalogsInDataManipulation(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsCatalogsInDataManipulation,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsCatalogsInDataManipulation_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsSchemasInDataManipulation(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsSchemasInDataManipulation,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsSchemasInDataManipulation_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsMixedCaseQuotedIdentifiers(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsMixedCaseQuotedIdentifiers,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsMixedCaseQuotedIdentifiers_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsAlterTableWithAddColumn(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsAlterTableWithAddColumn,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsAlterTableWithAddColumn_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::supportsAlterTableWithDropColumn(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_supportsAlterTableWithDropColumn,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_supportsAlterTableWithDropColumn_throw));
}
// -----------------------------------------------------------------------------
sal_Int32 SAL_CALL ODatabaseMetaDataBase::getMaxStatements(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_MaxStatements,::std::mem_fun_t< sal_Int32,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_getMaxStatements_throw));
}
// -----------------------------------------------------------------------------
sal_Int32 SAL_CALL ODatabaseMetaDataBase::getMaxTablesInSelect(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_MaxTablesInSelect,::std::mem_fun_t< sal_Int32,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_getMaxTablesInSelect_throw));
}
// -----------------------------------------------------------------------------
sal_Bool SAL_CALL ODatabaseMetaDataBase::storesMixedCaseQuotedIdentifiers(  ) throw(SQLException, RuntimeException)
{
    return callImplMethod(m_storesMixedCaseQuotedIdentifiers,::std::mem_fun_t< sal_Bool,ODatabaseMetaDataBase>(&ODatabaseMetaDataBase::impl_storesMixedCaseQuotedIdentifiers_throw));
}
// -----------------------------------------------------------------------------
