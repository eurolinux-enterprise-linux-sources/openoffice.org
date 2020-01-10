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

#include "MacabStatement.hxx"
#include "MacabConnection.hxx"
#include "MacabAddressBook.hxx"
#include "MacabDriver.hxx"
#include "MacabResultSet.hxx"
#include "MacabResultSetMetaData.hxx"
#include "macabcondition.hxx"
#include "macaborder.hxx"
#include "TConnection.hxx"
#include <connectivity/dbexception.hxx>
#include "resource/sharedresources.hxx"
#include "resource/macab_res.hrc"

#if OSL_DEBUG_LEVEL > 0
# define OUtoCStr( x ) ( ::rtl::OUStringToOString ( (x), RTL_TEXTENCODING_ASCII_US).getStr())
#else /* OSL_DEBUG_LEVEL */
# define OUtoCStr( x ) ("dummy")
#endif /* OSL_DEBUG_LEVEL */

using namespace connectivity::macab;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;
using namespace com::sun::star::container;
using namespace com::sun::star::io;
using namespace com::sun::star::util;

namespace connectivity
{
	namespace macab
	{
    void impl_throwError(sal_uInt16 _nErrorId)
    {
        ::connectivity::SharedResources aResources;
        const ::rtl::OUString sError( aResources.getResourceString(_nErrorId) );
        ::dbtools::throwGenericSQLException(sError,NULL);
    }
	}
}

IMPLEMENT_SERVICE_INFO(MacabStatement, "com.sun.star.sdbc.drivers.MacabStatement", "com.sun.star.sdbc.Statement");
//------------------------------------------------------------------------------
MacabCommonStatement::MacabCommonStatement(MacabConnection* _pConnection ) 
	: MacabCommonStatement_BASE(m_aMutex),
	OPropertySetHelper(MacabCommonStatement_BASE::rBHelper),
    m_aParser(_pConnection->getDriver()->getMSFactory()),
    m_aSQLIterator(_pConnection, _pConnection->createCatalog()->getTables(), m_aParser, NULL ),
	m_pParseTree(NULL),
	m_pConnection(_pConnection),
    rBHelper(MacabCommonStatement_BASE::rBHelper)
{
	m_pConnection->acquire();
}
// -----------------------------------------------------------------------------
MacabCommonStatement::~MacabCommonStatement()
{
}
// -----------------------------------------------------------------------------
void MacabCommonStatement::disposing()
{
	MacabCommonStatement_BASE::disposing();
}
// -----------------------------------------------------------------------------
void MacabCommonStatement::resetParameters() const throw(::com::sun::star::sdbc::SQLException)
{
    impl_throwError(STR_PARA_ONLY_PREPARED);
}
// -----------------------------------------------------------------------------
void MacabCommonStatement::getNextParameter(::rtl::OUString &) const throw(::com::sun::star::sdbc::SQLException)
{
	impl_throwError(STR_PARA_ONLY_PREPARED);
}
// -----------------------------------------------------------------------------
MacabCondition *MacabCommonStatement::analyseWhereClause(const OSQLParseNode *pParseNode) const throw(SQLException)
{
	if (pParseNode->count() == 3)
	{
		const OSQLParseNode *pLeft = pParseNode->getChild(0),
		                    *pMiddle = pParseNode->getChild(1),
		                    *pRight = pParseNode->getChild(2);

		// WHERE ( ... ) ?
		if (SQL_ISPUNCTUATION(pLeft, "(") && SQL_ISPUNCTUATION(pRight, ")"))
		{
		  	return analyseWhereClause(pMiddle);
		}
		else if (SQL_ISRULE(pParseNode, comparison_predicate))
		{
			if (pLeft->isToken() && pRight->isToken())
			{
				switch (pMiddle->getNodeType())
				{
					case SQL_NODE_EQUAL:
						// WHERE 0 = 1
						return new MacabConditionConstant(pLeft->getTokenValue() == pRight->getTokenValue());

					case SQL_NODE_NOTEQUAL:
						// WHERE 0 <> 1
						// (might not be correct SQL... don't care, handling anyway)
						return new MacabConditionConstant(pLeft->getTokenValue() != pRight->getTokenValue());

					default:
                        break;
				}
			}
			else if (SQL_ISRULE(pLeft, column_ref))
			{
				::rtl::OUString sColumnName,
				                sTableRange;

				m_aSQLIterator.getColumnRange(pLeft, sColumnName, sTableRange);

				if (pRight->isToken() || SQL_ISRULE(pRight, parameter))
				{
					::rtl::OUString sMatchString;

					if (pRight->isToken())						// WHERE Name = 'Doe'
						sMatchString = pRight->getTokenValue();
					else if (SQL_ISRULE(pRight, parameter))		// WHERE Name = ?
						getNextParameter(sMatchString);

					switch (pMiddle->getNodeType())
					{
						case SQL_NODE_EQUAL:
							// WHERE Name = 'Smith'
							return new MacabConditionEqual(m_pHeader, sColumnName, sMatchString);

			 			case SQL_NODE_NOTEQUAL:
							// WHERE Name <> 'Jones'
							return new MacabConditionDifferent(m_pHeader, sColumnName, sMatchString);

						default:
							break;
					}
				}
			}
		}
		else if (SQL_ISRULE(pParseNode, search_condition))
		{
			if (SQL_ISTOKEN(pMiddle, OR))
			{
				// WHERE Name = 'Smith' OR Name = 'Jones'
				return new MacabConditionOr(
					analyseWhereClause(pLeft),
					analyseWhereClause(pRight));
			}
		}
		else if (SQL_ISRULE(pParseNode, boolean_term))
		{
			if (SQL_ISTOKEN(pMiddle, AND))
			{
				// WHERE Name = 'Smith' AND "Given Name" = 'Peter'
				return new MacabConditionAnd(
					analyseWhereClause(pLeft),
					analyseWhereClause(pRight));
			}
		}
	}
	else if (pParseNode->count() == 4)
	{
		const OSQLParseNode *pLeft = pParseNode->getChild(0),
		                    *pMiddleLeft = pParseNode->getChild(1),
		                    *pMiddleRight = pParseNode->getChild(2),
		                    *pRight = pParseNode->getChild(3);

		if (SQL_ISRULE(pParseNode, test_for_null))
		{
			if (SQL_ISRULE(pLeft, column_ref) &&
                            SQL_ISTOKEN(pMiddleLeft, IS) &&
                            SQL_ISTOKEN(pRight, NULL))
			{
				::rtl::OUString sColumnName,
				                sTableRange;

				m_aSQLIterator.getColumnRange(pLeft, sColumnName, sTableRange);

				if (SQL_ISTOKEN(pMiddleRight, NOT))
				{
					// WHERE "Mobile Phone" IS NOT NULL
					return new MacabConditionNotNull(m_pHeader, sColumnName);
				}
				else
				{
					// WHERE "Mobile Phone" IS NULL
					return new MacabConditionNull(m_pHeader, sColumnName);
				}
			}
		}
		else if (SQL_ISRULE(pParseNode, like_predicate))
		{
			if (SQL_ISRULE(pLeft, column_ref))
			{
				::rtl::OUString sColumnName,
				                sTableRange;

				m_aSQLIterator.getColumnRange(pLeft, sColumnName, sTableRange);

				if (pMiddleRight->isToken() || SQL_ISRULE(pMiddleRight, parameter))
				{
					::rtl::OUString sMatchString;

					if (pMiddleRight->isToken())					// WHERE Name LIKE 'Sm%'
						sMatchString = pMiddleRight->getTokenValue();
					else if (SQL_ISRULE(pMiddleRight, parameter))	// WHERE Name LIKE ?
						getNextParameter(sMatchString);

					return new MacabConditionSimilar(m_pHeader, sColumnName, sMatchString);
				}
			}
		}
	}
	impl_throwError(STR_QUERY_TOO_COMPLEX);
    // Unreachable:
    OSL_ASSERT(false);
    return 0;
}
// -----------------------------------------------------------------------------
MacabOrder *MacabCommonStatement::analyseOrderByClause(const OSQLParseNode *pParseNode) const throw(SQLException)
{
	if (SQL_ISRULE(pParseNode, ordering_spec_commalist))
	{
		MacabComplexOrder *list = new MacabComplexOrder();
		sal_uInt32 n = pParseNode->count();

		// Iterate through the ordering columns
		for (sal_uInt32 i = 0; i < n; i++)
		{
			list->addOrder
				(analyseOrderByClause(pParseNode->getChild(i)));
		}

		return list;
	}
	else if (SQL_ISRULE(pParseNode, ordering_spec))
	{
		if (pParseNode->count() == 2)
		{
			OSQLParseNode* pColumnRef = pParseNode->getChild(0);
			OSQLParseNode* pAscendingDescending = pParseNode->getChild(1);

			if (SQL_ISRULE(pColumnRef, column_ref))
			{
				if (pColumnRef->count() == 3)
					pColumnRef = pColumnRef->getChild(2);

				if (pColumnRef->count() == 1)
				{
					::rtl::OUString sColumnName =
						pColumnRef->getChild(0)->getTokenValue();
					sal_Bool bAscending =
						SQL_ISTOKEN(pAscendingDescending, DESC)?
						sal_False:
						sal_True;

					return new MacabSimpleOrder(m_pHeader, sColumnName, bAscending);
				}
			}
		}
	}
	impl_throwError(STR_QUERY_TOO_COMPLEX);
    // Unreachable:
    OSL_ASSERT(false);
    return 0;
}
//------------------------------------------------------------------------------
::rtl::OUString MacabCommonStatement::getTableName() const
{
	const OSQLTables& xTabs = m_aSQLIterator.getTables();

	if( xTabs.empty() )
		return ::rtl::OUString();

	// can only deal with one table at a time
	if(xTabs.size() > 1 || m_aSQLIterator.hasErrors() )
		return ::rtl::OUString();

	return xTabs.begin()->first;
}
//------------------------------------------------------------------------------
void MacabCommonStatement::setMacabFields(MacabResultSet *pResult) const throw(SQLException)
{
	::vos::ORef<connectivity::OSQLColumns> xColumns;	// selected columns
	MacabResultSetMetaData *pMeta;				// meta information - holds the list of AddressBook fields

	xColumns = m_aSQLIterator.getSelectColumns();
	if (!xColumns.isValid())
	{
		::connectivity::SharedResources aResources;
        const ::rtl::OUString sError( aResources.getResourceString(
                STR_INVALID_COLUMN_SELECTION
             ) );
	    ::dbtools::throwGenericSQLException(sError,NULL);
	}
	pMeta = static_cast<MacabResultSetMetaData *>(pResult->getMetaData().get());
	pMeta->setMacabFields(xColumns);
}
// -------------------------------------------------------------------------
void MacabCommonStatement::selectRecords(MacabResultSet *pResult) const throw(SQLException)
{
	const OSQLParseNode *pParseNode;
	MacabCondition *pCondition;

	pParseNode = m_aSQLIterator.getWhereTree();
	if (pParseNode != NULL)
	{
		if (SQL_ISRULE(pParseNode, where_clause))
		{
			// Since we don't support parameters, don't reset them. If we ever
			// support them, uncomment this line and fix resetParameters.
			//resetParameters();
			pParseNode = pParseNode->getChild(1);
			pCondition = analyseWhereClause(pParseNode);
			if (pCondition->isAlwaysTrue())
				pResult->allMacabRecords();
			else if (!pCondition->isAlwaysFalse())
				pResult->someMacabRecords(pCondition);
			delete pCondition;
			return;
		}
	}

	// no WHERE clause: get all rows
	pResult->allMacabRecords();
}
// -------------------------------------------------------------------------
void MacabCommonStatement::sortRecords(MacabResultSet *pResult) const throw(SQLException)
{
	const OSQLParseNode *pParseNode;
	MacabOrder *pOrder;

	pParseNode = m_aSQLIterator.getOrderTree();
	if (pParseNode != NULL)
	{
		if (SQL_ISRULE(pParseNode, opt_order_by_clause))
		{
			pParseNode = pParseNode->getChild(2);
			pOrder = analyseOrderByClause(pParseNode);
			pResult->sortMacabRecords(pOrder);
			delete pOrder;
		}
	}
}
//-----------------------------------------------------------------------------
Any SAL_CALL MacabCommonStatement::queryInterface( const Type & rType ) throw(RuntimeException)
{
	Any aRet = MacabCommonStatement_BASE::queryInterface(rType);
	if (!aRet.hasValue())
		aRet = OPropertySetHelper::queryInterface(rType);
	return aRet;
}
// -------------------------------------------------------------------------
Sequence< Type > SAL_CALL MacabCommonStatement::getTypes(  ) throw(RuntimeException)
{
	::cppu::OTypeCollection aTypes(	::getCppuType( (const Reference< XMultiPropertySet > *)0 ),
									::getCppuType( (const Reference< XFastPropertySet > *)0 ),
									::getCppuType( (const Reference< XPropertySet > *)0 ));

	return comphelper::concatSequences(aTypes.getTypes(),MacabCommonStatement_BASE::getTypes());
}
// -------------------------------------------------------------------------
void SAL_CALL MacabCommonStatement::cancel(  ) throw(RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );

	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);
	// cancel the current sql statement
}
// -------------------------------------------------------------------------
void SAL_CALL MacabCommonStatement::close(  ) throw(SQLException, RuntimeException)
{
	{
		::osl::MutexGuard aGuard( m_aMutex );
		checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);
			
	}
	dispose();
}
// -------------------------------------------------------------------------
sal_Bool SAL_CALL MacabCommonStatement::execute(
		const ::rtl::OUString& sql ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);

	Reference< XResultSet > xRS = executeQuery(sql);

	return xRS.is();
}
// -------------------------------------------------------------------------
Reference< XResultSet > SAL_CALL MacabCommonStatement::executeQuery(
		const ::rtl::OUString& sql ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);

OSL_TRACE("Mac OS Address book - SQL Request: %s", OUtoCStr(sql));

	MacabResultSet* pResult = new MacabResultSet(this);
	Reference< XResultSet > xRS = pResult;
	::rtl::OUString aErr;

	m_pParseTree = m_aParser.parseTree(aErr, sql);
	if (m_pParseTree == NULL)
		throw SQLException(aErr, *this, aErr, 0, Any());

	m_aSQLIterator.setParseTree(m_pParseTree);
	m_aSQLIterator.traverseAll();
	switch (m_aSQLIterator.getStatementType())
	{
		case SQL_STATEMENT_SELECT:
			{
			::rtl::OUString sTableName = getTableName(); // FROM which table ?
			if (sTableName.getLength() != 0) // a match
			{
				MacabRecords *aRecords;
				aRecords = m_pConnection->getAddressBook()->getMacabRecords(sTableName);

				// In case, somehow, we don't have anything with the name m_sTableName
				if(aRecords == NULL)
				{
                    impl_throwError(STR_NO_TABLE);
				}
				else
				{
					m_pHeader = aRecords->getHeader();

					pResult->setTableName(sTableName);

					setMacabFields(pResult);		// SELECT which columns ?
					selectRecords(pResult);	// WHERE which condition ?
					sortRecords(pResult);	// ORDER BY which columns ?
				}
// To be continued: DISTINCT
//				    etc...
			}
			}
			break;

		default:
// To be continued: UPDATE
//				    DELETE
//				    etc...
            impl_throwError(STR_QUERY_TOO_COMPLEX);
	}

	m_xResultSet = Reference<XResultSet>(pResult);
	return xRS;
}
// -------------------------------------------------------------------------
Reference< XConnection > SAL_CALL MacabCommonStatement::getConnection(  ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);
		
	// just return our connection here
	return (Reference< XConnection >) m_pConnection;
}
// -------------------------------------------------------------------------
sal_Int32 SAL_CALL MacabCommonStatement::executeUpdate( const ::rtl::OUString& ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);
		
	// the return values gives information about how many rows are affected by executing the sql statement		
	return 0;
}
// -------------------------------------------------------------------------
Any SAL_CALL MacabCommonStatement::getWarnings(  ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);

	return makeAny(m_aLastWarning);
}
// -------------------------------------------------------------------------
void SAL_CALL MacabCommonStatement::clearWarnings(  ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard( m_aMutex );
	checkDisposed(MacabCommonStatement_BASE::rBHelper.bDisposed);

	m_aLastWarning = SQLWarning();
}
// -------------------------------------------------------------------------
::cppu::IPropertyArrayHelper* MacabCommonStatement::createArrayHelper( ) const
{
	// this properties are defined by the service statement
	// they must be in alphabetic order
	Sequence< Property > aProps(10);
	Property* pProperties = aProps.getArray();
	sal_Int32 nPos = 0;
	DECL_PROP0(CURSORNAME,	::rtl::OUString);
	DECL_BOOL_PROP0(ESCAPEPROCESSING);
	DECL_PROP0(FETCHDIRECTION,sal_Int32);
	DECL_PROP0(FETCHSIZE,	sal_Int32);
	DECL_PROP0(MAXFIELDSIZE,sal_Int32);
	DECL_PROP0(MAXROWS,		sal_Int32);
	DECL_PROP0(QUERYTIMEOUT,sal_Int32);
	DECL_PROP0(RESULTSETCONCURRENCY,sal_Int32);
	DECL_PROP0(RESULTSETTYPE,sal_Int32);
	DECL_BOOL_PROP0(USEBOOKMARKS);

	return new ::cppu::OPropertyArrayHelper(aProps);
}
// -------------------------------------------------------------------------
::cppu::IPropertyArrayHelper & MacabCommonStatement::getInfoHelper()
{
	return *const_cast<MacabCommonStatement*>(this)->getArrayHelper();
}
// -------------------------------------------------------------------------
sal_Bool MacabCommonStatement::convertFastPropertyValue(
		Any &,
		Any &,
		sal_Int32,
		const Any&) throw (::com::sun::star::lang::IllegalArgumentException)
{
	sal_Bool bConverted = sal_False;
	// here we have to try to convert 
	return bConverted;
}
// -------------------------------------------------------------------------
void MacabCommonStatement::setFastPropertyValue_NoBroadcast(sal_Int32 nHandle,const Any&) throw (Exception)
{
	// set the value to whatever is nescessary
	switch (nHandle)
	{
		case PROPERTY_ID_QUERYTIMEOUT:
		case PROPERTY_ID_MAXFIELDSIZE:
		case PROPERTY_ID_MAXROWS:
		case PROPERTY_ID_CURSORNAME:
		case PROPERTY_ID_RESULTSETCONCURRENCY:
		case PROPERTY_ID_RESULTSETTYPE:
		case PROPERTY_ID_FETCHDIRECTION:
		case PROPERTY_ID_FETCHSIZE:
		case PROPERTY_ID_ESCAPEPROCESSING:
		case PROPERTY_ID_USEBOOKMARKS:
		default:
			;
	}
}
// -------------------------------------------------------------------------
void MacabCommonStatement::getFastPropertyValue(Any&,sal_Int32 nHandle) const
{
	switch (nHandle)
	{
		case PROPERTY_ID_QUERYTIMEOUT:
		case PROPERTY_ID_MAXFIELDSIZE:
		case PROPERTY_ID_MAXROWS:
		case PROPERTY_ID_CURSORNAME:
		case PROPERTY_ID_RESULTSETCONCURRENCY:
		case PROPERTY_ID_RESULTSETTYPE:
		case PROPERTY_ID_FETCHDIRECTION:
		case PROPERTY_ID_FETCHSIZE:
		case PROPERTY_ID_ESCAPEPROCESSING:
		case PROPERTY_ID_USEBOOKMARKS:
		default:
			;
	}
}
// -----------------------------------------------------------------------------
void SAL_CALL MacabCommonStatement::acquire() throw()
{
	MacabCommonStatement_BASE::acquire();
}
// -----------------------------------------------------------------------------
void SAL_CALL MacabCommonStatement::release() throw()
{
	MacabCommonStatement_BASE::release();
}
// -----------------------------------------------------------------------------
Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL MacabCommonStatement::getPropertySetInfo(  ) throw(RuntimeException)
{
	return ::cppu::OPropertySetHelper::createPropertySetInfo(getInfoHelper());
}
// -----------------------------------------------------------------------------
MacabStatement::MacabStatement(MacabConnection* _pConnection)
	: MacabStatement_BASE(_pConnection)
{
}
