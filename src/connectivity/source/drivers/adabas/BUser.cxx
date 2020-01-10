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
#include "adabas/BUser.hxx"
#include "adabas/BGroups.hxx"
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include "adabas/BConnection.hxx"
#include "connectivity/dbtools.hxx"
#include "connectivity/dbexception.hxx"
#include <com/sun/star/sdbcx/Privilege.hpp>
#include <com/sun/star/sdbcx/PrivilegeObject.hpp>
#include "resource/adabas_res.hrc"

using namespace connectivity::adabas;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
// -------------------------------------------------------------------------
OAdabasUser::OAdabasUser(	OAdabasConnection* _pConnection) : connectivity::sdbcx::OUser(sal_True)
				,m_pConnection(_pConnection)
{
	construct();
}
// -------------------------------------------------------------------------
OAdabasUser::OAdabasUser(   OAdabasConnection* _pConnection,
				const ::rtl::OUString& _Name
			) : connectivity::sdbcx::OUser(_Name,sal_True)
				,m_pConnection(_pConnection)
{
	construct();
}
// -------------------------------------------------------------------------
void OAdabasUser::refreshGroups()
{
	if(!m_pConnection)
		return;

	TStringVector aVector;
	aVector.reserve(7); // we don't know the excatly count of users but this should fit the normal need
	Reference< XStatement > xStmt = m_pConnection->createStatement(  );
	::rtl::OUString aSql = ::rtl::OUString::createFromAscii("SELECT DISTINCT GROUPNAME FROM DOMAIN.USERS WHERE GROUPNAME IS NOT NULL AND GROUPNAME <> ' ' AND USERNAME = '");
	aSql += getName( );
	aSql += ::rtl::OUString::createFromAscii("'");

    Reference< XResultSet > xResult = xStmt->executeQuery(aSql);
	if(xResult.is())
	{
                Reference< XRow > xRow(xResult,UNO_QUERY);
		while(xResult->next())
			aVector.push_back(xRow->getString(1));
		::comphelper::disposeComponent(xResult);
	}
	::comphelper::disposeComponent(xStmt);

	if(m_pGroups)
		m_pGroups->reFill(aVector);
	else
		m_pGroups = new OGroups(*this,m_aMutex,aVector,m_pConnection,this);
}
// -------------------------------------------------------------------------
OUserExtend::OUserExtend(	OAdabasConnection* _pConnection) : OAdabasUser(_pConnection)
{
	construct();
}
// -------------------------------------------------------------------------
typedef connectivity::sdbcx::OUser	OUser_TYPEDEF;
void OUserExtend::construct()
{

	registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD),	PROPERTY_ID_PASSWORD,0,&m_Password,::getCppuType(reinterpret_cast< ::rtl::OUString*>(NULL)));
}
// -----------------------------------------------------------------------------
cppu::IPropertyArrayHelper* OUserExtend::createArrayHelper() const
{
	Sequence< Property > aProps;
	describeProperties(aProps);
	return new cppu::OPropertyArrayHelper(aProps);
}
// -------------------------------------------------------------------------
cppu::IPropertyArrayHelper & OUserExtend::getInfoHelper()
{
	return *OUserExtend_PROP::getArrayHelper();
}
typedef connectivity::sdbcx::OUser_BASE OUser_BASE_RBHELPER;
// -----------------------------------------------------------------------------
sal_Int32 SAL_CALL OAdabasUser::getPrivileges( const ::rtl::OUString& objName, sal_Int32 objType ) throw(SQLException, RuntimeException)
{
    if ( objType != PrivilegeObject::TABLE )
        return 0;

	::osl::MutexGuard aGuard(m_aMutex);
	checkDisposed(OUser_BASE_RBHELPER::rBHelper.bDisposed);

	sal_Int32 nRights,nRightsWithGrant;
	getAnyTablePrivileges(objName,nRights,nRightsWithGrant);
	return nRights;
}
// -----------------------------------------------------------------------------
void OAdabasUser::getAnyTablePrivileges(const ::rtl::OUString& objName, sal_Int32& nRights,sal_Int32& nRightsWithGrant) throw(SQLException, RuntimeException)
{
	nRightsWithGrant = nRights = 0;
	// first we need to create the sql stmt to select the privs
	Reference<XDatabaseMetaData> xMeta = m_pConnection->getMetaData();
	::rtl::OUString sCatalog,sSchema,sTable;
	::dbtools::qualifiedNameComponents(xMeta,objName,sCatalog,sSchema,sTable,::dbtools::eInDataManipulation);
	Reference<XStatement> xStmt = m_pConnection->createStatement();
	::rtl::OUString sSql = ::rtl::OUString::createFromAscii("SELECT REFTABLENAME,PRIVILEGES FROM DOMAIN.USR_USES_TAB WHERE REFOBJTYPE <> 'SYSTEM' AND DEFUSERNAME = '");
	sSql += m_Name;
	sSql += ::rtl::OUString::createFromAscii("' AND REFTABLENAME = '");
	sSql += sTable;
	sSql += ::rtl::OUString::createFromAscii("'");
	if(xStmt.is())
	{
		Reference<XResultSet> xRes = xStmt->executeQuery(sSql);
		if(xRes.is())
		{
			Reference<XRow> xRow(xRes,UNO_QUERY);
			if(xRow.is() && xRes->next())
			{
				::rtl::OUString sPrivs = xRow->getString(2);

                struct _priv_nam
                {
                    const sal_Char* pAsciiName;
                    sal_Int32       nNumericValue;
                } privileges[] =
                {
                    { "INS", Privilege::INSERT },
                    { "DEL", Privilege::DELETE },
                    { "UPD", Privilege::UPDATE },
                    { "ALT", Privilege::ALTER },
                    { "SEL", Privilege::SELECT },
                    { "REF", Privilege::REFERENCE }
                };
                for ( size_t i = 0; i < sizeof( privileges ) / sizeof( privileges[0] ); ++i )
                {
                    sal_Int32 nIndex = sPrivs.indexOf( ::rtl::OUString::createFromAscii( privileges[i].pAsciiName ) );
                    if ( nIndex == -1 )
                        continue;

                    nRights |= privileges[i].nNumericValue;
                    if ( sPrivs.copy( nIndex + 2, 1 ).equalsAscii( "+" ) )
                        nRightsWithGrant |= privileges[i].nNumericValue;
                }
			}
			::comphelper::disposeComponent(xRes);
		}
		::comphelper::disposeComponent(xStmt);
	}
}
// -------------------------------------------------------------------------
sal_Int32 SAL_CALL OAdabasUser::getGrantablePrivileges( const ::rtl::OUString& objName, sal_Int32 objType ) throw(SQLException, RuntimeException)
{
    if ( objType != PrivilegeObject::TABLE )
        return 0;

	::osl::MutexGuard aGuard(m_aMutex);
	checkDisposed(OUser_BASE_RBHELPER::rBHelper.bDisposed);

	sal_Int32 nRights,nRightsWithGrant;
	getAnyTablePrivileges(objName,nRights,nRightsWithGrant);
	return nRightsWithGrant;
}
// -------------------------------------------------------------------------
void SAL_CALL OAdabasUser::grantPrivileges( const ::rtl::OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges ) throw(SQLException, RuntimeException)
{
    if ( objType != PrivilegeObject::TABLE )
        m_pConnection->throwGenericSQLException(STR_PRIVILEGE_NOT_GRANTED,*this);

	::osl::MutexGuard aGuard(m_aMutex);
	::rtl::OUString sPrivs = getPrivilegeString(objPrivileges);
	if(sPrivs.getLength())
	{
		::rtl::OUString sGrant;
		sGrant += ::rtl::OUString::createFromAscii("GRANT ");
		sGrant += sPrivs;
		sGrant += ::rtl::OUString::createFromAscii(" ON ");
		Reference<XDatabaseMetaData> xMeta = m_pConnection->getMetaData();
		sGrant += ::dbtools::quoteTableName(xMeta,objName,::dbtools::eInDataManipulation);
		sGrant += ::rtl::OUString::createFromAscii(" TO ");
		sGrant += m_Name;

		Reference<XStatement> xStmt = m_pConnection->createStatement();
		if(xStmt.is())
			xStmt->execute(sGrant);
		::comphelper::disposeComponent(xStmt);
	}
}
// -------------------------------------------------------------------------
void SAL_CALL OAdabasUser::revokePrivileges( const ::rtl::OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges ) throw(SQLException, RuntimeException)
{
    if ( objType != PrivilegeObject::TABLE )
        m_pConnection->throwGenericSQLException(STR_PRIVILEGE_NOT_REVOKED,*this);

	::osl::MutexGuard aGuard(m_aMutex);
	checkDisposed(OUser_BASE_RBHELPER::rBHelper.bDisposed);
    ::rtl::OUString sPrivs = getPrivilegeString(objPrivileges);
	if(sPrivs.getLength())
	{
		::rtl::OUString sGrant;
		sGrant += ::rtl::OUString::createFromAscii("REVOKE ");
		sGrant += sPrivs;
		sGrant += ::rtl::OUString::createFromAscii(" ON ");
		Reference<XDatabaseMetaData> xMeta = m_pConnection->getMetaData();
		sGrant += ::dbtools::quoteTableName(xMeta,objName,::dbtools::eInDataManipulation);
		sGrant += ::rtl::OUString::createFromAscii(" FROM ");
		sGrant += m_Name;

		Reference<XStatement> xStmt = m_pConnection->createStatement();
		if(xStmt.is())
			xStmt->execute(sGrant);
		::comphelper::disposeComponent(xStmt);
	}
}
// -----------------------------------------------------------------------------
// XUser
void SAL_CALL OAdabasUser::changePassword( const ::rtl::OUString& objPassword, const ::rtl::OUString& newPassword ) throw(SQLException, RuntimeException)
{
	::osl::MutexGuard aGuard(m_aMutex);
	checkDisposed(OUser_BASE_RBHELPER::rBHelper.bDisposed);
	::rtl::OUString sAlterPwd;
	sAlterPwd = ::rtl::OUString::createFromAscii("ALTER PASSWORD \"");
	sAlterPwd += objPassword.toAsciiUpperCase();
	sAlterPwd += ::rtl::OUString::createFromAscii("\" TO \"") ;
	sAlterPwd += newPassword.toAsciiUpperCase();
	sAlterPwd += ::rtl::OUString::createFromAscii("\"") ;

	sal_Bool bDisposeConnection = sal_False;
	Reference<XConnection> xConnection = m_pConnection;
	if(m_pConnection->getMetaData()->getUserName() != m_Name)
	{
		OAdabasConnection* pNewConnection = new OAdabasConnection(m_pConnection->getDriverHandle(),m_pConnection->getDriver());
		xConnection = pNewConnection;
		if(pNewConnection)
		{
			Sequence< PropertyValue> aSeq(2);
			aSeq.getArray()[0].Name		= ::rtl::OUString::createFromAscii("user") ;
			aSeq.getArray()[0].Value	<<= m_Name;
			aSeq.getArray()[1].Name		= ::rtl::OUString::createFromAscii("password") ;
			aSeq.getArray()[1].Value	<<= objPassword;
			pNewConnection->Construct(m_pConnection->getMetaData()->getURL(),aSeq);
		}
		bDisposeConnection = sal_True;
	}
	if(xConnection.is())
	{
		Reference<XStatement> xStmt = xConnection->createStatement();
		if(xStmt.is())
			xStmt->execute(sAlterPwd);
		::comphelper::disposeComponent(xStmt);
		if(bDisposeConnection)
			::comphelper::disposeComponent(xConnection);
	}
	else
		::dbtools::throwFunctionSequenceException(*this);
}
// -----------------------------------------------------------------------------
::rtl::OUString OAdabasUser::getPrivilegeString(sal_Int32 nRights) const
{
	::rtl::OUString sPrivs;
	if((nRights & Privilege::INSERT) == Privilege::INSERT)
		sPrivs += ::rtl::OUString::createFromAscii("INSERT");

	if((nRights & Privilege::DELETE) == Privilege::DELETE)
	{
		if(sPrivs.getLength())
			sPrivs += ::rtl::OUString::createFromAscii(",");
		sPrivs += ::rtl::OUString::createFromAscii("DELETE");
	}

	if((nRights & Privilege::UPDATE) == Privilege::UPDATE)
	{
		if(sPrivs.getLength())
			sPrivs += ::rtl::OUString::createFromAscii(",");
		sPrivs += ::rtl::OUString::createFromAscii("UPDATE");
	}

	if((nRights & Privilege::ALTER) == Privilege::ALTER)
	{
		if(sPrivs.getLength())
			sPrivs += ::rtl::OUString::createFromAscii(",");
		sPrivs += ::rtl::OUString::createFromAscii("ALTER");
	}

	if((nRights & Privilege::SELECT) == Privilege::SELECT)
	{
		if(sPrivs.getLength())
			sPrivs += ::rtl::OUString::createFromAscii(",");
		sPrivs += ::rtl::OUString::createFromAscii("SELECT");
	}

	if((nRights & Privilege::REFERENCE) == Privilege::REFERENCE)
	{
		if(sPrivs.getLength())
			sPrivs += ::rtl::OUString::createFromAscii(",");
		sPrivs += ::rtl::OUString::createFromAscii("REFERENCES");
	}

	return sPrivs;
}
// -----------------------------------------------------------------------------

