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

#include "connectivity/CommonTools.hxx"
#include "diagnose_ex.h"
#include "TConnection.hxx"
#include "connectivity/ParameterCont.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sdb/BooleanComparisonMode.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdb/ParametersRequest.hpp>
#include <com/sun/star/sdb/RowSetVetoException.hpp>
#include <com/sun/star/sdb/SQLContext.hpp>
#include <com/sun/star/sdb/XCompletedConnection.hpp>
#include <com/sun/star/sdb/XInteractionSupplyParameters.hpp>
#include <com/sun/star/sdb/XOfficeDatabaseDocument.hpp>
#include <com/sun/star/sdb/XParametersSupplier.hpp>
#include <com/sun/star/sdb/XQueriesSupplier.hpp>
#include <com/sun/star/sdb/XSingleSelectQueryComposer.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdbc/XDataSource.hpp>
#include <com/sun/star/sdbc/XDriverManager.hpp>
#include <com/sun/star/sdbc/XParameters.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/sdbc/XRowUpdate.hpp>
#include <com/sun/star/sdbcx/Privilege.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/task/XInteractionRequest.hpp>
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <com/sun/star/uno/XNamingService.hpp>
#include <com/sun/star/util/NumberFormat.hpp>
#include <com/sun/star/util/XNumberFormatsSupplier.hpp>
#include <com/sun/star/util/XNumberFormatTypes.hpp>
/** === end UNO includes === **/

#include <comphelper/extract.hxx>
#include <comphelper/interaction.hxx>
#include <comphelper/property.hxx>
#include <connectivity/conncleanup.hxx>
#include <connectivity/dbconversion.hxx>
#include <connectivity/dbexception.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/statementcomposer.hxx>
#include <osl/diagnose.h>
#include <rtl/ustrbuf.hxx>
#include <tools/diagnose_ex.h>

#include "resource/common_res.hrc"
#include "resource/sharedresources.hxx"
#include "OSubComponent.hxx"

#include <algorithm>

using namespace ::comphelper;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::form;
using namespace connectivity;

//.........................................................................
namespace dbtools
{
//.........................................................................

	using namespace ::com::sun::star::uno;
	using namespace ::com::sun::star::beans;
	using namespace ::com::sun::star::util;
	using namespace ::com::sun::star::task;
	using namespace ::com::sun::star::uno;
	using namespace ::com::sun::star::lang;
	using namespace ::com::sun::star::sdbc;
	using namespace ::com::sun::star::task;
//	using namespace cppu;
//	using namespace osl;

//==============================================================================
//==============================================================================
namespace
{
    typedef sal_Bool (SAL_CALL XDatabaseMetaData::*FMetaDataSupport)();
}
//==============================================================================
//==============================================================================
sal_Int32 getDefaultNumberFormat(const Reference< XPropertySet >& _xColumn,
								 const Reference< XNumberFormatTypes >& _xTypes,
								 const Locale& _rLocale)
{
	OSL_ENSURE(_xTypes.is() && _xColumn.is(), "dbtools::getDefaultNumberFormat: invalid arg !");
	if (!_xTypes.is() || !_xColumn.is())
		return NumberFormat::UNDEFINED;

	sal_Int32 nDataType = 0;
	sal_Int32 nScale = 0;
	try
	{
		// determine the datatype of the column
		_xColumn->getPropertyValue(::rtl::OUString::createFromAscii("Type")) >>= nDataType;

		if (DataType::NUMERIC == nDataType || DataType::DECIMAL == nDataType)
			_xColumn->getPropertyValue(::rtl::OUString::createFromAscii("Scale")) >>= nScale;
	}
	catch (Exception&)
	{
		return NumberFormat::UNDEFINED;
	}
	return getDefaultNumberFormat(nDataType,
					nScale,
					::cppu::any2bool(_xColumn->getPropertyValue(::rtl::OUString::createFromAscii("IsCurrency"))),
					_xTypes,
					_rLocale);
}

//------------------------------------------------------------------
sal_Int32 getDefaultNumberFormat(sal_Int32 _nDataType,
								 sal_Int32 _nScale,
								 sal_Bool _bIsCurrency,
								 const Reference< XNumberFormatTypes >& _xTypes,
								 const Locale& _rLocale)
{
	OSL_ENSURE(_xTypes.is() , "dbtools::getDefaultNumberFormat: invalid arg !");
	if (!_xTypes.is())
		return NumberFormat::UNDEFINED;

	sal_Int32 nFormat = 0;
	sal_Int32 nNumberType	= _bIsCurrency ? NumberFormat::CURRENCY : NumberFormat::NUMBER;
	switch (_nDataType)
	{
		case DataType::BIT:
		case DataType::BOOLEAN:
			nFormat = _xTypes->getStandardFormat(NumberFormat::LOGICAL, _rLocale);
			break;
		case DataType::TINYINT:
		case DataType::SMALLINT:
		case DataType::INTEGER:
		case DataType::BIGINT:
		case DataType::FLOAT:
		case DataType::REAL:
		case DataType::DOUBLE:
		case DataType::NUMERIC:
		case DataType::DECIMAL:
		{
			try
			{
				nFormat = _xTypes->getStandardFormat((sal_Int16)nNumberType, _rLocale);
				if(_nScale > 0)
				{
					// generate a new format if necessary
					Reference< XNumberFormats > xFormats(_xTypes, UNO_QUERY);
					::rtl::OUString sNewFormat = xFormats->generateFormat( 0L, _rLocale, sal_False, sal_False, (sal_Int16)_nScale, sal_True);

					// and add it to the formatter if necessary
					nFormat = xFormats->queryKey(sNewFormat, _rLocale, sal_False);
					if (nFormat == (sal_Int32)-1)
						nFormat = xFormats->addNew(sNewFormat, _rLocale);
				}
			}
			catch (Exception&)
			{
				nFormat = _xTypes->getStandardFormat((sal_Int16)nNumberType, _rLocale);
			}
		}	break;
		case DataType::CHAR:
		case DataType::VARCHAR:
		case DataType::LONGVARCHAR:
			nFormat = _xTypes->getStandardFormat(NumberFormat::TEXT, _rLocale);
			break;
		case DataType::DATE:
			nFormat = _xTypes->getStandardFormat(NumberFormat::DATE, _rLocale);
			break;
		case DataType::TIME:
			nFormat = _xTypes->getStandardFormat(NumberFormat::TIME, _rLocale);
			break;
		case DataType::TIMESTAMP:
			nFormat = _xTypes->getStandardFormat(NumberFormat::DATETIME, _rLocale);
			break;
		case DataType::BINARY:
		case DataType::VARBINARY:
		case DataType::LONGVARBINARY:
		case DataType::SQLNULL:
		case DataType::OTHER:
		case DataType::OBJECT:
		case DataType::DISTINCT:
		case DataType::STRUCT:
		case DataType::ARRAY:
		case DataType::BLOB:
		case DataType::CLOB:
		case DataType::REF:
		default:
			nFormat = NumberFormat::UNDEFINED;
	}
	return nFormat;
}

//==============================================================================
//------------------------------------------------------------------------------
Reference< XConnection> findConnection(const Reference< XInterface >& xParent)
{
	Reference< XConnection> xConnection(xParent, UNO_QUERY);
	if (!xConnection.is())
	{
		Reference< XChild> xChild(xParent, UNO_QUERY);
		if (xChild.is())
			xConnection = findConnection(xChild->getParent());
	}
	return xConnection;
}

//------------------------------------------------------------------------------
Reference< XDataSource> getDataSource_allowException(
			const ::rtl::OUString& _rsTitleOrPath,
			const Reference< XMultiServiceFactory >& _rxFactory )
{
	OSL_ENSURE( _rsTitleOrPath.getLength(), "getDataSource_allowException: invalid arg !" );

	Reference< XNameAccess> xDatabaseContext(
		_rxFactory->createInstance(
			::rtl::OUString::createFromAscii( "com.sun.star.sdb.DatabaseContext" ) ),UNO_QUERY );
    OSL_ENSURE( xDatabaseContext.is(), "getDataSource_allowException: could not obtain the database context!" );

	return Reference< XDataSource >( xDatabaseContext->getByName( _rsTitleOrPath ), UNO_QUERY );
}

//------------------------------------------------------------------------------
Reference< XDataSource > getDataSource(
			const ::rtl::OUString& _rsTitleOrPath,
			const Reference< XMultiServiceFactory >& _rxFactory )
{
    Reference< XDataSource > xDS;
	try
	{
        xDS = getDataSource_allowException( _rsTitleOrPath, _rxFactory );
	}
	catch(Exception)
	{
    }

    return xDS;
}

//------------------------------------------------------------------------------
Reference< XConnection > getConnection_allowException(
			const ::rtl::OUString& _rsTitleOrPath,
			const ::rtl::OUString& _rsUser,
			const ::rtl::OUString& _rsPwd,
			const Reference< XMultiServiceFactory>& _rxFactory)
{
	Reference< XDataSource> xDataSource( getDataSource_allowException(_rsTitleOrPath, _rxFactory) );
	Reference<XConnection> xConnection;
	if (xDataSource.is())
	{
		// do it with interaction handler
		if(!_rsUser.getLength() || !_rsPwd.getLength())
		{
			Reference<XPropertySet> xProp(xDataSource,UNO_QUERY);
			::rtl::OUString sPwd, sUser;
			sal_Bool bPwdReq = sal_False;
			try
			{
				xProp->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD)) >>= sPwd;
				bPwdReq = ::cppu::any2bool(xProp->getPropertyValue(::rtl::OUString::createFromAscii("IsPasswordRequired")));
				xProp->getPropertyValue(::rtl::OUString::createFromAscii("User")) >>= sUser;
			}
			catch(Exception&)
			{
				OSL_ENSURE(sal_False, "dbtools::getConnection: error while retrieving data source properties!");
			}
			if(bPwdReq && !sPwd.getLength())
			{	// password required, but empty -> connect using an interaction handler
				Reference<XCompletedConnection> xConnectionCompletion(xProp, UNO_QUERY);
				if (xConnectionCompletion.is())
				{	// instantiate the default SDB interaction handler
					Reference< XInteractionHandler > xHandler(_rxFactory->createInstance(::rtl::OUString::createFromAscii("com.sun.star.sdb.InteractionHandler")), UNO_QUERY);
					OSL_ENSURE(xHandler.is(), "dbtools::getConnection service com.sun.star.sdb.InteractionHandler not available!");
					if (xHandler.is())
					{
						xConnection = xConnectionCompletion->connectWithCompletion(xHandler);
					}
				}
			}
			else
				xConnection = xDataSource->getConnection(sUser, sPwd);
		}
		if(!xConnection.is()) // try to get one if not already have one, just to make sure
			xConnection = xDataSource->getConnection(_rsUser, _rsPwd);
	}
	return xConnection;
}

//------------------------------------------------------------------------------
Reference< XConnection> getConnection_withFeedback(const ::rtl::OUString& _rDataSourceName,
		const ::rtl::OUString& _rUser, const ::rtl::OUString& _rPwd, const Reference< XMultiServiceFactory>& _rxFactory)
	SAL_THROW ( (SQLException) )
{
	Reference< XConnection > xReturn;
	try
	{
		xReturn = getConnection_allowException(_rDataSourceName, _rUser, _rPwd, _rxFactory);
	}
	catch(SQLException&)
	{
		// allowed to pass
		throw;
	}
	catch(Exception&)
	{
		OSL_ENSURE(sal_False, "::dbtools::getConnection_withFeedback: unexpected (non-SQL) exception caught!");
	}
	return xReturn;
}

//------------------------------------------------------------------------------
Reference< XConnection> getConnection(
			const ::rtl::OUString& _rsTitleOrPath,
			const ::rtl::OUString& _rsUser,
			const ::rtl::OUString& _rsPwd,
			const Reference< XMultiServiceFactory>& _rxFactory)
{
	Reference< XConnection > xReturn;
	try
	{
		xReturn = getConnection_allowException(_rsTitleOrPath, _rsUser, _rsPwd, _rxFactory);
	}
	catch(Exception&)
	{
	}

	// TODO: if there were not dozens of places which rely on getConnection not throwing an exception ....
	// I would change this ...

	return xReturn;
}

//------------------------------------------------------------------------------
Reference< XConnection> getConnection(const Reference< XRowSet>& _rxRowSet) throw (RuntimeException)
{
	Reference< XConnection> xReturn;
	Reference< XPropertySet> xRowSetProps(_rxRowSet, UNO_QUERY);
	if (xRowSetProps.is())
		xRowSetProps->getPropertyValue(::rtl::OUString::createFromAscii("ActiveConnection")) >>= xReturn;
	return xReturn;
}

//------------------------------------------------------------------------------
// helper function which allows to implement both the connectRowset and the ensureRowSetConnection semantics
// if connectRowset (which is deprecated) is removed, this function and one of its parameters are
// not needed anymore, the whole implementation can be moved into ensureRowSetConnection then)
SharedConnection lcl_connectRowSet(const Reference< XRowSet>& _rxRowSet, const Reference< XMultiServiceFactory>& _rxFactory,
        bool _bSetAsActiveConnection, bool _bAttachAutoDisposer )
    SAL_THROW ( ( SQLException, WrappedTargetException, RuntimeException ) )
{
    SharedConnection xConnection;

    do
    {
        Reference< XPropertySet> xRowSetProps(_rxRowSet, UNO_QUERY);
	    if ( !xRowSetProps.is() )
            break;

        // 1. already connected?
        Reference< XConnection > xExistingConn(
            xRowSetProps->getPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ActiveConnection" ) ) ),
            UNO_QUERY );

        if  (   xExistingConn.is()
            // 2. embedded in a database?
            ||  isEmbeddedInDatabase( _rxRowSet, xExistingConn )
            // 3. is there a connection in the parent hierarchy?
            ||  ( xExistingConn = findConnection( _rxRowSet ) ).is()
            )
        {
            if ( _bSetAsActiveConnection )
            {
                xRowSetProps->setPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ActiveConnection" ) ), makeAny( xExistingConn ) );
                // no auto disposer needed, since we did not create the connection
            }

            xConnection.reset( xExistingConn, SharedConnection::NoTakeOwnership );
            break;
        }

		// build a connection with it's current settings (4. data source name, or 5. URL)

        const ::rtl::OUString sUserProp = ::rtl::OUString::createFromAscii("User");
		::rtl::OUString sDataSourceName;
		xRowSetProps->getPropertyValue(::rtl::OUString::createFromAscii("DataSourceName")) >>= sDataSourceName;
		::rtl::OUString sURL;
		xRowSetProps->getPropertyValue(::rtl::OUString::createFromAscii("URL")) >>= sURL;

        Reference< XConnection > xPureConnection;
		if (sDataSourceName.getLength())
		{	// the row set's data source property is set
			// -> try to connect, get user and pwd setting for that
			::rtl::OUString sUser, sPwd;

			if (hasProperty(sUserProp, xRowSetProps))
				xRowSetProps->getPropertyValue(sUserProp) >>= sUser;
			if (hasProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD), xRowSetProps))
				xRowSetProps->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD)) >>= sPwd;

            xPureConnection = getConnection_allowException( sDataSourceName, sUser, sPwd, _rxFactory );
		}
		else if (sURL.getLength())
		{	// the row set has no data source, but a connection url set
			// -> try to connection with that url
			Reference< XDriverManager > xDriverManager(
				_rxFactory->createInstance( ::rtl::OUString::createFromAscii("com.sun.star.sdbc.ConnectionPool")), UNO_QUERY);
			if (xDriverManager.is())
			{
				::rtl::OUString sUser, sPwd;
				if (hasProperty(sUserProp, xRowSetProps))
					xRowSetProps->getPropertyValue(sUserProp) >>= sUser;
				if (hasProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD), xRowSetProps))
					xRowSetProps->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD)) >>= sPwd;
				if (sUser.getLength())
				{	// use user and pwd together with the url
					Sequence< PropertyValue> aInfo(2);
					aInfo.getArray()[0].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("user"));
					aInfo.getArray()[0].Value <<= sUser;
					aInfo.getArray()[1].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("password"));
					aInfo.getArray()[1].Value <<= sPwd;
                    xPureConnection = xDriverManager->getConnectionWithInfo( sURL, aInfo );
				}
				else
					// just use the url
                    xPureConnection = xDriverManager->getConnection( sURL );
			}
		}
        xConnection.reset(
            xPureConnection,
            _bAttachAutoDisposer ? SharedConnection::NoTakeOwnership : SharedConnection::TakeOwnership
            /* take ownership if and only if we're *not* going to auto-dispose the connection */
        );

        // now if we created a connection, forward it to the row set
		if ( xConnection.is() && _bSetAsActiveConnection )
		{
			try
			{
                if ( _bAttachAutoDisposer )
                {
				    OAutoConnectionDisposer* pAutoDispose = new OAutoConnectionDisposer( _rxRowSet, xConnection );
				    Reference< XPropertyChangeListener > xEnsureDelete(pAutoDispose);
                }
                else
                    xRowSetProps->setPropertyValue(
                        ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ActiveConnection" ) ),
                        makeAny( xConnection.getTyped() )
                    );
			}
			catch(Exception&)
			{
				OSL_ENSURE(0,"EXception when we set the new active connection!");
			}
		}
	}
    while ( false );

	return xConnection;
}

//------------------------------------------------------------------------------
Reference< XConnection> connectRowset(const Reference< XRowSet>& _rxRowSet, const Reference< XMultiServiceFactory>& _rxFactory,
	sal_Bool _bSetAsActiveConnection )	SAL_THROW ( ( SQLException, WrappedTargetException, RuntimeException ) )
{
    SharedConnection xConnection = lcl_connectRowSet( _rxRowSet, _rxFactory, _bSetAsActiveConnection, true );
    return xConnection.getTyped();
}

//------------------------------------------------------------------------------
SharedConnection ensureRowSetConnection(const Reference< XRowSet>& _rxRowSet, const Reference< XMultiServiceFactory>& _rxFactory,
    bool _bUseAutoConnectionDisposer )	SAL_THROW ( ( SQLException, WrappedTargetException, RuntimeException ) )
{
    return lcl_connectRowSet( _rxRowSet, _rxFactory, true, _bUseAutoConnectionDisposer );
}

//------------------------------------------------------------------------------
Reference< XNameAccess> getTableFields(const Reference< XConnection>& _rxConn,const ::rtl::OUString& _rName)
{
	Reference< XComponent > xDummy;
	return getFieldsByCommandDescriptor( _rxConn, CommandType::TABLE, _rName, xDummy );
}

//------------------------------------------------------------------------------
namespace
{
	enum FieldLookupState
	{
		HANDLE_TABLE, HANDLE_QUERY, HANDLE_SQL, RETRIEVE_OBJECT, RETRIEVE_COLUMNS, DONE, FAILED
	};
}

//------------------------------------------------------------------------------
Reference< XNameAccess > getFieldsByCommandDescriptor( const Reference< XConnection >& _rxConnection,
	const sal_Int32 _nCommandType, const ::rtl::OUString& _rCommand,
	Reference< XComponent >& _rxKeepFieldsAlive, SQLExceptionInfo* _pErrorInfo ) SAL_THROW( ( ) )
{
	OSL_PRECOND( _rxConnection.is(), "::dbtools::getFieldsByCommandDescriptor: invalid connection!" );
	OSL_PRECOND( ( CommandType::TABLE == _nCommandType ) || ( CommandType::QUERY == _nCommandType ) || ( CommandType::COMMAND == _nCommandType ),
		"::dbtools::getFieldsByCommandDescriptor: invalid command type!" );
	OSL_PRECOND( _rCommand.getLength(), "::dbtools::getFieldsByCommandDescriptor: invalid command (empty)!" );

	Reference< XNameAccess > xFields;

	// reset the error
	if ( _pErrorInfo )
		*_pErrorInfo = SQLExceptionInfo();
	// reset the ownership holder
	_rxKeepFieldsAlive.clear();

	// go for the fields
	try
	{
		// some kind of state machine to ease the sharing of code
		FieldLookupState eState = FAILED;
		switch ( _nCommandType )
		{
			case CommandType::TABLE:
				eState = HANDLE_TABLE;
				break;
			case CommandType::QUERY:
				eState = HANDLE_QUERY;
				break;
			case CommandType::COMMAND:
				eState = HANDLE_SQL;
				break;
		}

		// needed in various states:
		Reference< XNameAccess > xObjectCollection;
		Reference< XColumnsSupplier > xSupplyColumns;

		// go!
		while ( ( DONE != eState ) && ( FAILED != eState ) )
		{
			switch ( eState )
			{
				case HANDLE_TABLE:
				{
					// initial state for handling the tables

					// get the table objects
					Reference< XTablesSupplier > xSupplyTables( _rxConnection, UNO_QUERY );
					if ( xSupplyTables.is() )
						xObjectCollection = xSupplyTables->getTables();
					// if something went wrong 'til here, then this will be handled in the next state

					// next state: get the object
					eState = RETRIEVE_OBJECT;
				}
				break;

				case HANDLE_QUERY:
				{
					// initial state for handling the tables

					// get the table objects
					Reference< XQueriesSupplier > xSupplyQueries( _rxConnection, UNO_QUERY );
					if ( xSupplyQueries.is() )
						xObjectCollection = xSupplyQueries->getQueries();
					// if something went wrong 'til here, then this will be handled in the next state

					// next state: get the object
					eState = RETRIEVE_OBJECT;
				}
				break;

				case RETRIEVE_OBJECT:
					// here we should have an object (aka query or table) collection, and are going
					// to retrieve the desired object

					// next state: default to FAILED
					eState = FAILED;

					OSL_ENSURE( xObjectCollection.is(), "::dbtools::getFieldsByCommandDescriptor: invalid connection (no sdb.Connection, or no Tables-/QueriesSupplier)!");
					if ( xObjectCollection.is() )
					{
						if ( xObjectCollection.is() && xObjectCollection->hasByName( _rCommand ) )
						{
							xObjectCollection->getByName( _rCommand ) >>= xSupplyColumns;
								// (xSupplyColumns being NULL will be handled in the next state)

							// next: go for the columns
							eState = RETRIEVE_COLUMNS;
						}
					}
					break;

				case RETRIEVE_COLUMNS:
					OSL_ENSURE( xSupplyColumns.is(), "::dbtools::getFieldsByCommandDescriptor: could not retrieve the columns supplier!" );

					// next state: default to FAILED
					eState = FAILED;

					if ( xSupplyColumns.is() )
					{
						xFields = xSupplyColumns->getColumns();
						// that's it
						eState = DONE;
					}
					break;

				case HANDLE_SQL:
				{
					::rtl::OUString sStatementToExecute( _rCommand );

					// well, the main problem here is to handle statements which contain a parameter
					// If we would simply execute a parametrized statement, then this will fail because
					// we cannot supply any parameter values.
					// Thus, we try to analyze the statement, and to append a WHERE 0=1 filter criterion
					// This should cause every driver to not really execute the statement, but to return
					// an empty result set with the proper structure. We then can use this result set
					// to retrieve the columns.

					try
					{
						Reference< XMultiServiceFactory > xComposerFac( _rxConnection, UNO_QUERY );

						if ( xComposerFac.is() )
						{
							Reference< XSingleSelectQueryComposer > xComposer(xComposerFac->createInstance( ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.sdb.SingleSelectQueryComposer"))),UNO_QUERY);
							if ( xComposer.is() )
							{
								xComposer->setQuery( sStatementToExecute );

								// Now set the filter to a dummy restriction which will result in an empty
								// result set.
								xComposer->setFilter( ::rtl::OUString::createFromAscii( "0=1" ) );
								sStatementToExecute = xComposer->getQuery( );
							}
						}
					}
					catch( const Exception& )
					{
						// silent this error, this was just a try. If we're here, we did not change sStatementToExecute,
						// so it will still be _rCommand, which then will be executed without being touched
					}

					// now execute
					Reference< XPreparedStatement > xStatement = _rxConnection->prepareStatement( sStatementToExecute );
					// transfer ownership of this temporary object to the caller
					_rxKeepFieldsAlive = _rxKeepFieldsAlive.query( xStatement );

					// set the "MaxRows" to 0. This is just in case our attempt to append a 0=1 filter
					// failed - in this case, the MaxRows restriction should at least ensure that there
					// is no data returned (which would be potentially expensive)
					Reference< XPropertySet > xStatementProps( xStatement,UNO_QUERY );
					try
					{
						if ( xStatementProps.is() )
							xStatementProps->setPropertyValue(
								::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "MaxRows" ) ),
								makeAny( sal_Int32( 0 ) )
							);
					}
					catch( const Exception& )
					{
						OSL_ENSURE( sal_False, "::dbtools::getFieldsByCommandDescriptor: could not set the MaxRows!" );
						// oh damn. Not much of a chance to recover, we will no retrieve the complete
						// full blown result set
					}

					xSupplyColumns = xSupplyColumns.query( xStatement->executeQuery() );
					// this should have given us a result set which does not contain any data, but
					// the structural information we need

					// so the next state is to get the columns
					eState = RETRIEVE_COLUMNS;
				}
				break;

				default:
					OSL_ENSURE( sal_False, "::dbtools::getFieldsByCommandDescriptor: oops! unhandled state here!" );
					eState = FAILED;
			}
		}
	}
	catch( const SQLContext& e ) { if ( _pErrorInfo ) *_pErrorInfo = SQLExceptionInfo( e ); }
	catch( const SQLWarning& e ) { if ( _pErrorInfo ) *_pErrorInfo = SQLExceptionInfo( e ); }
	catch( const SQLException& e ) { if ( _pErrorInfo ) *_pErrorInfo = SQLExceptionInfo( e ); }
	catch( const Exception& )
	{
		OSL_ENSURE( sal_False, "::dbtools::getFieldsByCommandDescriptor: caught an exception while retrieving the fields!" );
	}

	return xFields;
}

//------------------------------------------------------------------------------
Sequence< ::rtl::OUString > getFieldNamesByCommandDescriptor( const Reference< XConnection >& _rxConnection,
	const sal_Int32 _nCommandType, const ::rtl::OUString& _rCommand,
	SQLExceptionInfo* _pErrorInfo ) SAL_THROW( ( ) )
{
	// get the container for the fields
	Reference< XComponent > xKeepFieldsAlive;
	Reference< XNameAccess > xFieldContainer = getFieldsByCommandDescriptor( _rxConnection, _nCommandType, _rCommand, xKeepFieldsAlive, _pErrorInfo );

	// get the names of the fields
	Sequence< ::rtl::OUString > aNames;
	if ( xFieldContainer.is() )
		aNames = xFieldContainer->getElementNames();

	// clean up any temporary objects which have been created
	disposeComponent( xKeepFieldsAlive );

	// outta here
	return aNames;
}

//------------------------------------------------------------------------------
SQLContext prependContextInfo(const SQLException& _rException, const Reference< XInterface >& _rxContext, const ::rtl::OUString& _rContextDescription, const ::rtl::OUString& _rContextDetails)
{
	return SQLContext( _rContextDescription, _rxContext, ::rtl::OUString(), 0, makeAny( _rException ), _rContextDetails );
}
//------------------------------------------------------------------------------
SQLException prependErrorInfo( const SQLException& _rChainedException, const Reference< XInterface >& _rxContext,
    const ::rtl::OUString& _rAdditionalError, const StandardSQLState _eSQLState, const sal_Int32 _nErrorCode )
{
    return SQLException( _rAdditionalError, _rxContext,
        _eSQLState == SQL_ERROR_UNSPECIFIED ? ::rtl::OUString() : getStandardSQLState( _eSQLState ),
        _nErrorCode, makeAny( _rChainedException ) );
}

//--------------------------------------------------------------------------
namespace
{
    struct NameComponentSupport
    {
        const bool  bCatalogs;
        const bool  bSchemas;

        NameComponentSupport( )
            :bCatalogs( true )
            ,bSchemas( true )
        {
        }

        NameComponentSupport( const bool _bCatalogs, const bool _bSchemas )
            :bCatalogs( _bCatalogs )
            ,bSchemas( _bSchemas )
        {
        }
    };

    NameComponentSupport lcl_getNameComponentSupport( const Reference< XDatabaseMetaData >& _rxMetaData, EComposeRule _eComposeRule )
    {
        OSL_PRECOND( _rxMetaData.is(), "lcl_getNameComponentSupport: invalid meta data!" );

        FMetaDataSupport pCatalogCall = &XDatabaseMetaData::supportsCatalogsInDataManipulation;
        FMetaDataSupport pSchemaCall = &XDatabaseMetaData::supportsSchemasInDataManipulation;
        bool bIgnoreMetaData = false;

	    switch ( _eComposeRule )
	    {
		    case eInTableDefinitions:
			    pCatalogCall = &XDatabaseMetaData::supportsCatalogsInTableDefinitions;
			    pSchemaCall = &XDatabaseMetaData::supportsSchemasInTableDefinitions;
			    break;
		    case eInIndexDefinitions:
			    pCatalogCall = &XDatabaseMetaData::supportsCatalogsInIndexDefinitions;
			    pSchemaCall = &XDatabaseMetaData::supportsSchemasInIndexDefinitions;
			    break;
		    case eInProcedureCalls:
			    pCatalogCall = &XDatabaseMetaData::supportsCatalogsInProcedureCalls;
			    pSchemaCall = &XDatabaseMetaData::supportsSchemasInProcedureCalls;
			    break;
		    case eInPrivilegeDefinitions:
			    pCatalogCall = &XDatabaseMetaData::supportsCatalogsInPrivilegeDefinitions;
			    pSchemaCall = &XDatabaseMetaData::supportsSchemasInPrivilegeDefinitions;
			    break;
            case eComplete:
                bIgnoreMetaData = true;
                break;
            case eInDataManipulation:
                // already properly set above
                break;
	    }
        return NameComponentSupport(
            bIgnoreMetaData ? true : (_rxMetaData.get()->*pCatalogCall)(),
            bIgnoreMetaData ? true : (_rxMetaData.get()->*pSchemaCall)()
        );
    }
}

//--------------------------------------------------------------------------
static ::rtl::OUString impl_doComposeTableName( const Reference< XDatabaseMetaData >& _rxMetaData,
                const ::rtl::OUString& _rCatalog, const ::rtl::OUString& _rSchema, const ::rtl::OUString& _rName,
                sal_Bool _bQuote, EComposeRule _eComposeRule )
{
	OSL_ENSURE(_rxMetaData.is(), "impl_doComposeTableName : invalid meta data !");
    if ( !_rxMetaData.is() )
        return ::rtl::OUString();
	OSL_ENSURE(_rName.getLength(), "impl_doComposeTableName : at least the name should be non-empty !");

	const ::rtl::OUString sQuoteString = _rxMetaData->getIdentifierQuoteString();
    const NameComponentSupport aNameComps( lcl_getNameComponentSupport( _rxMetaData, _eComposeRule ) );

    ::rtl::OUStringBuffer aComposedName;

    ::rtl::OUString sCatalogSep;
	sal_Bool bCatlogAtStart = sal_True;
	if ( _rCatalog.getLength() && aNameComps.bCatalogs )
	{
		sCatalogSep		= _rxMetaData->getCatalogSeparator();
		bCatlogAtStart	= _rxMetaData->isCatalogAtStart();

		if ( bCatlogAtStart && sCatalogSep.getLength())
		{
            aComposedName.append( _bQuote ? quoteName( sQuoteString, _rCatalog ) : _rCatalog );
			aComposedName.append( sCatalogSep );
		}
	}

	if ( _rSchema.getLength() && aNameComps.bSchemas )
	{
        aComposedName.append( _bQuote ? quoteName( sQuoteString, _rSchema ) : _rSchema );
        aComposedName.appendAscii( "." );
	}

    aComposedName.append( _bQuote ? quoteName( sQuoteString, _rName ) : _rName );

	if  (   _rCatalog.getLength()
        &&  !bCatlogAtStart
        &&  sCatalogSep.getLength()
        &&  aNameComps.bCatalogs
        )
	{
		aComposedName.append( sCatalogSep );
        aComposedName.append( _bQuote ? quoteName( sQuoteString, _rCatalog ) : _rCatalog );
	}

    return aComposedName.makeStringAndClear();
}

//------------------------------------------------------------------------------
::rtl::OUString quoteTableName(const Reference< XDatabaseMetaData>& _rxMeta
							   , const ::rtl::OUString& _rName
							   , EComposeRule _eComposeRule)
{
	::rtl::OUString sCatalog, sSchema, sTable;
	qualifiedNameComponents(_rxMeta,_rName,sCatalog,sSchema,sTable,_eComposeRule);
	return impl_doComposeTableName( _rxMeta, sCatalog, sSchema, sTable, sal_True, _eComposeRule );
}

//------------------------------------------------------------------------------
void qualifiedNameComponents(const Reference< XDatabaseMetaData >& _rxConnMetaData, const ::rtl::OUString& _rQualifiedName, ::rtl::OUString& _rCatalog, ::rtl::OUString& _rSchema, ::rtl::OUString& _rName,EComposeRule _eComposeRule)
{
	OSL_ENSURE(_rxConnMetaData.is(), "QualifiedNameComponents : invalid meta data!");

    NameComponentSupport aNameComps( lcl_getNameComponentSupport( _rxConnMetaData, _eComposeRule ) );

	::rtl::OUString sSeparator = _rxConnMetaData->getCatalogSeparator();

	::rtl::OUString sName(_rQualifiedName);
	// do we have catalogs ?
	if ( aNameComps.bCatalogs )
	{
		if (_rxConnMetaData->isCatalogAtStart())
		{
			// search for the catalog name at the beginning
			sal_Int32 nIndex = sName.indexOf(sSeparator);
			if (-1 != nIndex)
			{
				_rCatalog = sName.copy(0, nIndex);
				sName = sName.copy(nIndex + 1);
			}
		}
		else
		{
			// Katalogname am Ende
			sal_Int32 nIndex = sName.lastIndexOf(sSeparator);
			if (-1 != nIndex)
			{
				_rCatalog = sName.copy(nIndex + 1);
				sName = sName.copy(0, nIndex);
			}
		}
	}

	if ( aNameComps.bSchemas )
	{
		sal_Int32 nIndex = sName.indexOf((sal_Unicode)'.');
		//	OSL_ENSURE(-1 != nIndex, "QualifiedNameComponents : no schema separator!");
		if ( nIndex != -1 )
			_rSchema = sName.copy(0, nIndex);
		sName = sName.copy(nIndex + 1);
	}

	_rName = sName;
}

//------------------------------------------------------------------------------
Reference< XNumberFormatsSupplier> getNumberFormats(
			const Reference< XConnection>& _rxConn,
			sal_Bool _bAlloweDefault,
			const Reference< XMultiServiceFactory>& _rxFactory)
{
	// ask the parent of the connection (should be an DatabaseAccess)
	Reference< XNumberFormatsSupplier> xReturn;
	Reference< XChild> xConnAsChild(_rxConn, UNO_QUERY);
	::rtl::OUString sPropFormatsSupplier = ::rtl::OUString::createFromAscii("NumberFormatsSupplier");
	if (xConnAsChild.is())
	{
		Reference< XPropertySet> xConnParentProps(xConnAsChild->getParent(), UNO_QUERY);
		if (xConnParentProps.is() && hasProperty(sPropFormatsSupplier, xConnParentProps))
			xConnParentProps->getPropertyValue(sPropFormatsSupplier) >>= xReturn;
	}
	else if(_bAlloweDefault && _rxFactory.is())
	{
		xReturn = Reference< XNumberFormatsSupplier>(_rxFactory->createInstance(::rtl::OUString::createFromAscii("com.sun.star.util.NumberFormatsSupplier")),UNO_QUERY);
	}
	return xReturn;
}

//==============================================================================
//------------------------------------------------------------------------------
void TransferFormComponentProperties(
			const Reference< XPropertySet>& xOldProps,
			const Reference< XPropertySet>& xNewProps,
			const Locale& _rLocale)
{
try
{
    OSL_ENSURE( xOldProps.is() && xNewProps.is(), "TransferFormComponentProperties: invalid source/dest!" );
    if ( !xOldProps.is() || !xNewProps.is() )
        return;

	// kopieren wir erst mal alle Props, die in Quelle und Ziel vorhanden sind und identische Beschreibungen haben
	Reference< XPropertySetInfo> xOldInfo( xOldProps->getPropertySetInfo());
	Reference< XPropertySetInfo> xNewInfo( xNewProps->getPropertySetInfo());

	Sequence< Property> aOldProperties = xOldInfo->getProperties();
	Sequence< Property> aNewProperties = xNewInfo->getProperties();
	int nNewLen = aNewProperties.getLength();

	Property* pOldProps = aOldProperties.getArray();
	Property* pNewProps = aNewProperties.getArray();

	::rtl::OUString sPropDefaultControl(::rtl::OUString::createFromAscii("DefaultControl"));
	::rtl::OUString sPropLabelControl(::rtl::OUString::createFromAscii("LabelControl"));
	::rtl::OUString sPropFormatsSupplier(::rtl::OUString::createFromAscii("FormatsSupplier"));
	::rtl::OUString sPropCurrencySymbol(::rtl::OUString::createFromAscii("CurrencySymbol"));
	::rtl::OUString sPropDecimals(::rtl::OUString::createFromAscii("Decimals"));
	::rtl::OUString sPropEffectiveMin(::rtl::OUString::createFromAscii("EffectiveMin"));
	::rtl::OUString sPropEffectiveMax(::rtl::OUString::createFromAscii("EffectiveMax"));
	::rtl::OUString sPropEffectiveDefault(::rtl::OUString::createFromAscii("EffectiveDefault"));
	::rtl::OUString sPropDefaultText(::rtl::OUString::createFromAscii("DefaultText"));
	::rtl::OUString sPropDefaultDate(::rtl::OUString::createFromAscii("DefaultDate"));
	::rtl::OUString sPropDefaultTime(::rtl::OUString::createFromAscii("DefaultTime"));
	::rtl::OUString sPropValueMin(::rtl::OUString::createFromAscii("ValueMin"));
	::rtl::OUString sPropValueMax(::rtl::OUString::createFromAscii("ValueMax"));
	::rtl::OUString sPropDecimalAccuracy(::rtl::OUString::createFromAscii("DecimalAccuracy"));
	::rtl::OUString sPropClassId(::rtl::OUString::createFromAscii("ClassId"));
	::rtl::OUString sFormattedServiceName( ::rtl::OUString::createFromAscii( "com.sun.star.form.component.FormattedField" ) );

	for (sal_Int16 i=0; i<aOldProperties.getLength(); ++i)
	{
		if	(	(!pOldProps[i].Name.equals(sPropDefaultControl))
			&&	(!pOldProps[i].Name.equals(sPropLabelControl))
			)
		{
			// binaere Suche
			Property* pResult = ::std::lower_bound(pNewProps, pNewProps + nNewLen,pOldProps[i].Name, ::comphelper::PropertyStringLessFunctor());
			if (    pResult
				&& ( pResult != pNewProps + nNewLen && pResult->Name == pOldProps[i].Name )
				&& ( (pResult->Attributes & PropertyAttribute::READONLY) == 0 )
				&& ( pResult->Type.equals(pOldProps[i].Type)) )
			{	// Attribute stimmen ueberein und Property ist nicht read-only
				try
				{
					xNewProps->setPropertyValue(pResult->Name, xOldProps->getPropertyValue(pResult->Name));
				}
				catch(IllegalArgumentException& e)
				{
					OSL_UNUSED( e );
#ifdef DBG_UTIL
					::rtl::OUString sMessage = ::rtl::OUString::createFromAscii("TransferFormComponentProperties : could not transfer the value for property \"");
					sMessage += pResult->Name;
					sMessage += ::rtl::OUString::createFromAscii("\"");;
					OSL_ENSURE(sal_False, ::rtl::OUStringToOString(sMessage, RTL_TEXTENCODING_ASCII_US));
#endif
				}
			}
		}
	}


	// fuer formatierte Felder (entweder alt oder neu) haben wir ein paar Sonderbehandlungen
	Reference< XServiceInfo > xSI( xOldProps, UNO_QUERY );
	sal_Bool bOldIsFormatted = xSI.is() && xSI->supportsService( sFormattedServiceName );
	xSI = Reference< XServiceInfo >( xNewProps, UNO_QUERY );
	sal_Bool bNewIsFormatted = xSI.is() && xSI->supportsService( sFormattedServiceName );

	if (!bOldIsFormatted && !bNewIsFormatted)
		return;	// nothing to do

	if (bOldIsFormatted && bNewIsFormatted)
		// nein, wenn beide formatierte Felder sind, dann machen wir keinerlei Konvertierungen
		// Das geht zu weit ;)
		return;

	if (bOldIsFormatted)
	{
		// aus dem eingestellten Format ein paar Properties rausziehen und zum neuen Set durchschleifen
		Any aFormatKey( xOldProps->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_FORMATKEY)) );
		if (aFormatKey.hasValue())
		{
			Reference< XNumberFormatsSupplier> xSupplier;
			xOldProps->getPropertyValue(sPropFormatsSupplier) >>= xSupplier;
			if (xSupplier.is())
			{
				Reference< XNumberFormats> xFormats(xSupplier->getNumberFormats());
				Reference< XPropertySet> xFormat(xFormats->getByKey(getINT32(aFormatKey)));
				if (hasProperty(sPropCurrencySymbol, xFormat))
				{
					Any aVal( xFormat->getPropertyValue(sPropCurrencySymbol) );
					if (aVal.hasValue() && hasProperty(sPropCurrencySymbol, xNewProps))
						// (wenn die Quelle das nicht gesetzt hat, dann auch nicht kopieren, um den
						// Default-Wert nicht zu ueberschreiben
						xNewProps->setPropertyValue(sPropCurrencySymbol, aVal);
				}
				if (hasProperty(sPropDecimals, xFormat) && hasProperty(sPropDecimals, xNewProps))
					xNewProps->setPropertyValue(sPropDecimals, xFormat->getPropertyValue(sPropDecimals));
			}
		}

		// eine eventuelle-Min-Max-Konvertierung
		Any aEffectiveMin( xOldProps->getPropertyValue(sPropEffectiveMin) );
		if (aEffectiveMin.hasValue())
		{	// im Gegensatz zu ValueMin kann EffectiveMin void sein
			if (hasProperty(sPropValueMin, xNewProps))
			{
				OSL_ENSURE(aEffectiveMin.getValueType().getTypeClass() == TypeClass_DOUBLE,
					"TransferFormComponentProperties : invalid property type !");
				xNewProps->setPropertyValue(sPropValueMin, aEffectiveMin);
			}
		}
		Any aEffectiveMax( xOldProps->getPropertyValue(sPropEffectiveMax) );
		if (aEffectiveMax.hasValue())
		{	// analog
			if (hasProperty(sPropValueMax, xNewProps))
			{
				OSL_ENSURE(aEffectiveMax.getValueType().getTypeClass() == TypeClass_DOUBLE,
					"TransferFormComponentProperties : invalid property type !");
				xNewProps->setPropertyValue(sPropValueMax, aEffectiveMax);
			}
		}

		// dann koennen wir noch Default-Werte konvertieren und uebernehmen
		Any aEffectiveDefault( xOldProps->getPropertyValue(sPropEffectiveDefault) );
		if (aEffectiveDefault.hasValue())
		{
			sal_Bool bIsString = aEffectiveDefault.getValueType().getTypeClass() == TypeClass_STRING;
			OSL_ENSURE(bIsString || aEffectiveDefault.getValueType().getTypeClass() == TypeClass_DOUBLE,
				"TransferFormComponentProperties : invalid property type !");
				// die Effective-Properties sollten immer void oder string oder double sein ....

			if (hasProperty(sPropDefaultDate, xNewProps) && !bIsString)
			{	// (einen ::rtl::OUString in ein Datum zu konvertieren muss nicht immer klappen, denn das ganze kann ja an
				// eine Textspalte gebunden gewesen sein, aber mit einem double koennen wir was anfangen)
				Date aDate = DBTypeConversion::toDate(getDouble(aEffectiveDefault));
				xNewProps->setPropertyValue(sPropDefaultDate, makeAny(aDate));
			}

			if (hasProperty(sPropDefaultTime, xNewProps) && !bIsString)
			{	// voellig analog mit Zeit
				Time aTime = DBTypeConversion::toTime(getDouble(aEffectiveDefault));
				xNewProps->setPropertyValue(sPropDefaultTime, makeAny(aTime));
			}

			if (hasProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE), xNewProps) && !bIsString)
			{	// hier koennen wir einfach das double durchreichen
				xNewProps->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE), aEffectiveDefault);
			}

			if (hasProperty(sPropDefaultText, xNewProps) && bIsString)
			{	// und hier den ::rtl::OUString
				xNewProps->setPropertyValue(sPropDefaultText, aEffectiveDefault);
			}

			// nyi: die Uebersetzung zwischen doubles und ::rtl::OUString wuerde noch mehr Moeglichkeien eroeffnen
		}
	}

	// die andere Richtung : das neu Control soll formatiert sein
	if (bNewIsFormatted)
	{
		// zuerst die Formatierung
		// einen Supplier koennen wir nicht setzen, also muss das neue Set schon einen mitbringen
		Reference< XNumberFormatsSupplier> xSupplier;
		xNewProps->getPropertyValue(sPropFormatsSupplier) >>= xSupplier;
		if (xSupplier.is())
		{
			Reference< XNumberFormats> xFormats(xSupplier->getNumberFormats());

			// Dezimal-Stellen
			sal_Int16 nDecimals = 2;
			if (hasProperty(sPropDecimalAccuracy, xOldProps))
				xOldProps->getPropertyValue(sPropDecimalAccuracy) >>= nDecimals;

			// Grund-Format (je nach ClassId des alten Sets)
			sal_Int32 nBaseKey = 0;
			if (hasProperty(sPropClassId, xOldProps))
			{
				Reference< XNumberFormatTypes> xTypeList(xFormats, UNO_QUERY);
				if (xTypeList.is())
				{
					sal_Int16 nClassId = 0;
					xOldProps->getPropertyValue(sPropClassId) >>= nClassId;
					switch (nClassId)
					{
						case FormComponentType::DATEFIELD :
							nBaseKey = xTypeList->getStandardFormat(NumberFormat::DATE, _rLocale);
							break;

						case FormComponentType::TIMEFIELD :
							nBaseKey = xTypeList->getStandardFormat(NumberFormat::TIME, _rLocale);
							break;

						case FormComponentType::CURRENCYFIELD :
							nBaseKey = xTypeList->getStandardFormat(NumberFormat::CURRENCY, _rLocale);
							break;
					}
				}
			}

			// damit koennen wir ein neues Format basteln ...
			::rtl::OUString sNewFormat = xFormats->generateFormat(nBaseKey, _rLocale, sal_False, sal_False, nDecimals, 0);
				// kein Tausender-Trennzeichen, negative Zahlen nicht in Rot, keine fuehrenden Nullen

			// ... und zum FormatsSupplier hinzufuegen (wenn noetig)
			sal_Int32 nKey = xFormats->queryKey(sNewFormat, _rLocale, sal_False);
			if (nKey == (sal_Int32)-1)
			{	// noch nicht vorhanden in meinem Formatter ...
				nKey = xFormats->addNew(sNewFormat, _rLocale);
			}

			xNewProps->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_FORMATKEY), makeAny((sal_Int32)nKey));
		}

		// min-/max-Werte
		Any aNewMin, aNewMax;
		if (hasProperty(sPropValueMin, xOldProps))
			aNewMin = xOldProps->getPropertyValue(sPropValueMin);
		if (hasProperty(sPropValueMax, xOldProps))
			aNewMax = xOldProps->getPropertyValue(sPropValueMax);
		xNewProps->setPropertyValue(sPropEffectiveMin, aNewMin);
		xNewProps->setPropertyValue(sPropEffectiveMax, aNewMax);

		// Default-Wert
		Any aNewDefault;
		if (hasProperty(sPropDefaultDate, xOldProps))
		{
			Any aDate( xOldProps->getPropertyValue(sPropDefaultDate) );
			if (aDate.hasValue())
				aNewDefault <<= DBTypeConversion::toDouble(*(Date*)aDate.getValue());
		}

		if (hasProperty(sPropDefaultTime, xOldProps))
		{
			Any aTime( xOldProps->getPropertyValue(sPropDefaultTime) );
			if (aTime.hasValue())
				aNewDefault <<= DBTypeConversion::toDouble(*(Time*)aTime.getValue());
		}

		// double oder ::rtl::OUString werden direkt uebernommen
		if (hasProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE), xOldProps))
			aNewDefault = xOldProps->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DEFAULTVALUE));
		if (hasProperty(sPropDefaultText, xOldProps))
			aNewDefault = xOldProps->getPropertyValue(sPropDefaultText);

		if (aNewDefault.hasValue())
			xNewProps->setPropertyValue(sPropEffectiveDefault, aNewDefault);
	}
}
catch(const Exception&)
{
	OSL_ENSURE( sal_False, "TransferFormComponentProperties: caught an exception!" );
}
}

//------------------------------------------------------------------------------
sal_Bool canInsert(const Reference< XPropertySet>& _rxCursorSet)
{
	return ((_rxCursorSet.is() && (getINT32(_rxCursorSet->getPropertyValue(::rtl::OUString::createFromAscii("Privileges"))) & Privilege::INSERT) != 0));
}

//------------------------------------------------------------------------------
sal_Bool canUpdate(const Reference< XPropertySet>& _rxCursorSet)
{
	return ((_rxCursorSet.is() && (getINT32(_rxCursorSet->getPropertyValue(::rtl::OUString::createFromAscii("Privileges"))) & Privilege::UPDATE) != 0));
}

//------------------------------------------------------------------------------
sal_Bool canDelete(const Reference< XPropertySet>& _rxCursorSet)
{
	return ((_rxCursorSet.is() && (getINT32(_rxCursorSet->getPropertyValue(::rtl::OUString::createFromAscii("Privileges"))) & Privilege::DELETE) != 0));
}
// -----------------------------------------------------------------------------
Reference< XDataSource> findDataSource(const Reference< XInterface >& _xParent)
{
	Reference< XOfficeDatabaseDocument> xDatabaseDocument(_xParent, UNO_QUERY);
	Reference< XDataSource> xDataSource;
	if ( xDatabaseDocument.is() )
		xDataSource = xDatabaseDocument->getDataSource();
	if ( !xDataSource.is() )
		xDataSource.set(_xParent, UNO_QUERY);
	if (!xDataSource.is())
	{
		Reference< XChild> xChild(_xParent, UNO_QUERY);
		if ( xChild.is() )
			xDataSource = findDataSource(xChild->getParent());
	}
	return xDataSource;
}

//------------------------------------------------------------------------------
::rtl::OUString getComposedRowSetStatement( const Reference< XPropertySet >& _rxRowSet, const Reference< XMultiServiceFactory>& _rxFactory,
                                   sal_Bool _bUseRowSetFilter, sal_Bool _bUseRowSetOrder, Reference< XSingleSelectQueryComposer >* _pxComposer )
    SAL_THROW( ( SQLException ) )
{
    ::rtl::OUString sStatement;
	try
	{
        Reference< XConnection> xConn = connectRowset( Reference< XRowSet >( _rxRowSet, UNO_QUERY ), _rxFactory, sal_True );
		if ( xConn.is() )		// implies _rxRowSet.is()
		{
			// build the statement the row set is based on (can't use the ActiveCommand property of the set
			// as this reflects the status after the last execute, not the currently set properties)

            sal_Int32 nCommandType = CommandType::COMMAND;
            ::rtl::OUString sCommand;
            sal_Bool bEscapeProcessing = sal_False;

            OSL_VERIFY( _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "CommandType" )      ) >>= nCommandType      );
			OSL_VERIFY( _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "Command" )          ) >>= sCommand          );
			OSL_VERIFY( _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "EscapeProcessing" ) ) >>= bEscapeProcessing );

            StatementComposer aComposer( xConn, sCommand, nCommandType, bEscapeProcessing );
			// append sort
            if ( _bUseRowSetOrder )
				aComposer.setOrder( getString( _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "Order" ) ) ) );

            // append filter
            if ( _bUseRowSetFilter )
            {
				sal_Bool bApplyFilter = sal_True;
                _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "ApplyFilter" ) ) >>= bApplyFilter;
				if ( bApplyFilter )
					aComposer.setFilter( getString( _rxRowSet->getPropertyValue( ::rtl::OUString::createFromAscii( "Filter" ) ) ) );
            }

            sStatement = aComposer.getQuery();

            if ( _pxComposer )
            {
                *_pxComposer = aComposer.getComposer();
                aComposer.setDisposeComposer( false );
            }
		}
	}
	catch( const SQLException& )
	{
		throw;
	}
	catch( const Exception& )
	{
        DBG_UNHANDLED_EXCEPTION();
	}

    return sStatement;
}

//------------------------------------------------------------------------------
::rtl::OUString getComposedRowSetStatement(
                    const Reference< XPropertySet >& _rxRowSet, const Reference< XMultiServiceFactory>& _rxFactory,
                    sal_Bool _bUseRowSetFilter, sal_Bool _bUseRowSetOrder )
{
    return getComposedRowSetStatement( _rxRowSet, _rxFactory, _bUseRowSetFilter, _bUseRowSetOrder, NULL );
}

//------------------------------------------------------------------------------
Reference< XSingleSelectQueryComposer > getCurrentSettingsComposer(
				const Reference< XPropertySet>& _rxRowSetProps,
				const Reference< XMultiServiceFactory>& _rxFactory)
{
	Reference< XSingleSelectQueryComposer > xReturn;
	try
	{
        getComposedRowSetStatement( _rxRowSetProps, _rxFactory, sal_True, sal_True, &xReturn );
	}
	catch( const SQLException& )
	{
		throw;
	}
	catch( const Exception& )
	{
		OSL_ENSURE( sal_False, "::getCurrentSettingsComposer : caught an exception !" );
	}

	return xReturn;
}
//--------------------------------------------------------------------------
::rtl::OUString composeTableName( const Reference< XDatabaseMetaData >& _rxMetaData,
						const ::rtl::OUString& _rCatalog,
						const ::rtl::OUString& _rSchema,
						const ::rtl::OUString& _rName,
						sal_Bool _bQuote,
						EComposeRule _eComposeRule)
{
    return impl_doComposeTableName( _rxMetaData, _rCatalog, _rSchema, _rName, _bQuote, _eComposeRule );
}

// -----------------------------------------------------------------------------
::rtl::OUString composeTableNameForSelect( const Reference< XConnection >& _rxConnection,
    const ::rtl::OUString& _rCatalog, const ::rtl::OUString& _rSchema, const ::rtl::OUString& _rName )
{
	sal_Bool bUseCatalogInSelect = isDataSourcePropertyEnabled( _rxConnection, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "UseCatalogInSelect" ) ), sal_True );
	sal_Bool bUseSchemaInSelect = isDataSourcePropertyEnabled( _rxConnection, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "UseSchemaInSelect" ) ), sal_True );

    return impl_doComposeTableName(
        _rxConnection->getMetaData(),
        bUseCatalogInSelect ? _rCatalog : ::rtl::OUString(),
        bUseSchemaInSelect ? _rSchema : ::rtl::OUString(),
        _rName,
        true,
        eInDataManipulation
    );
}

// -----------------------------------------------------------------------------
namespace
{
    static void lcl_getTableNameComponents( const Reference<XPropertySet>& _xTable,
        ::rtl::OUString& _out_rCatalog, ::rtl::OUString& _out_rSchema, ::rtl::OUString& _out_rName )
    {
        ::dbtools::OPropertyMap& rPropMap = OMetaConnection::getPropMap();
        Reference< XPropertySetInfo > xInfo = _xTable->getPropertySetInfo();
        if (	xInfo.is() 
	        &&	xInfo->hasPropertyByName(rPropMap.getNameByIndex(PROPERTY_ID_CATALOGNAME)) 
	        &&	xInfo->hasPropertyByName(rPropMap.getNameByIndex(PROPERTY_ID_SCHEMANAME)) 
	        &&	xInfo->hasPropertyByName(rPropMap.getNameByIndex(PROPERTY_ID_NAME)) )
        {
        	
	        ::rtl::OUString aCatalog;
	        ::rtl::OUString aSchema;
	        ::rtl::OUString aTable;
	        _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_CATALOGNAME))	>>= _out_rCatalog;
	        _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_SCHEMANAME))	>>= _out_rSchema;
	        _xTable->getPropertyValue(rPropMap.getNameByIndex(PROPERTY_ID_NAME))		>>= _out_rName;
        }
        else
            OSL_ENSURE( false, "::dbtools::lcl_getTableNameComponents: this is no table object!" );
    }
}

// -----------------------------------------------------------------------------
::rtl::OUString composeTableNameForSelect( const Reference< XConnection >& _rxConnection, const Reference<XPropertySet>& _xTable )
{
    ::rtl::OUString sCatalog, sSchema, sName;
    lcl_getTableNameComponents( _xTable, sCatalog, sSchema, sName );

    return composeTableNameForSelect( _rxConnection, sCatalog, sSchema, sName );
}

// -----------------------------------------------------------------------------
::rtl::OUString composeTableName(const Reference<XDatabaseMetaData>& _xMetaData,
								 const Reference<XPropertySet>& _xTable,
								 EComposeRule _eComposeRule,
                                 bool _bSuppressCatalog,
                                 bool _bSuppressSchema,
                                 bool _bQuote )
{
    ::rtl::OUString sCatalog, sSchema, sName;
    lcl_getTableNameComponents( _xTable, sCatalog, sSchema, sName );

    return impl_doComposeTableName(
            _xMetaData,
            _bSuppressCatalog ? ::rtl::OUString() : sCatalog,
            _bSuppressSchema ? ::rtl::OUString() : sSchema,
            sName,
            _bQuote,
            _eComposeRule
        );
}
// -----------------------------------------------------------------------------
sal_Int32 getSearchColumnFlag( const Reference< XConnection>& _rxConn,sal_Int32 _nDataType)
{
	sal_Int32 nSearchFlag = 0;
	Reference<XResultSet> xSet = _rxConn->getMetaData()->getTypeInfo();
	if(xSet.is())
	{
		Reference<XRow> xRow(xSet,UNO_QUERY);
		while(xSet->next())
		{
			if(xRow->getInt(2) == _nDataType)
			{
				nSearchFlag = xRow->getInt(9);
				break;
			}
		}
	}
	return nSearchFlag;
}

// -----------------------------------------------------------------------------
::rtl::OUString createUniqueName( const Sequence< ::rtl::OUString >& _rNames, const ::rtl::OUString& _rBaseName, sal_Bool _bStartWithNumber )
{
    ::std::set< ::rtl::OUString > aUsedNames;
    ::std::copy(
        _rNames.getConstArray(),
        _rNames.getConstArray() + _rNames.getLength(),
        ::std::insert_iterator< ::std::set< ::rtl::OUString > >( aUsedNames, aUsedNames.end() )
    );

	::rtl::OUString sName( _rBaseName );
	sal_Int32 nPos = 1;
	if ( _bStartWithNumber )
		sName += ::rtl::OUString::valueOf( nPos );

    while ( aUsedNames.find( sName ) != aUsedNames.end() )
	{
		sName = _rBaseName;
		sName += ::rtl::OUString::valueOf( ++nPos );
	}
	return sName;
}

// -----------------------------------------------------------------------------
::rtl::OUString createUniqueName(const Reference<XNameAccess>& _rxContainer,const ::rtl::OUString& _rBaseName,sal_Bool _bStartWithNumber)
{
    Sequence< ::rtl::OUString > aElementNames;

    OSL_ENSURE( _rxContainer.is(), "createUniqueName: invalid container!" );
	if ( _rxContainer.is() )
        aElementNames = _rxContainer->getElementNames();

    return createUniqueName( aElementNames, _rBaseName, _bStartWithNumber );
}

// -----------------------------------------------------------------------------
void showError(const SQLExceptionInfo& _rInfo,
			   const Reference< XWindow>& _xParent,
			   const Reference< XMultiServiceFactory >& _xFactory)
{
	if (_rInfo.isValid())
	{
		try
		{
			Sequence< Any > aArgs(2);
			aArgs[0] <<= PropertyValue(::rtl::OUString::createFromAscii("SQLException"), 0, _rInfo.get(), PropertyState_DIRECT_VALUE);
			aArgs[1] <<= PropertyValue(::rtl::OUString::createFromAscii("ParentWindow"), 0, makeAny(_xParent), PropertyState_DIRECT_VALUE);

			static ::rtl::OUString s_sDialogServiceName = ::rtl::OUString::createFromAscii("com.sun.star.sdb.ErrorMessageDialog");
			Reference< XExecutableDialog > xErrorDialog(
				_xFactory->createInstanceWithArguments(s_sDialogServiceName, aArgs), UNO_QUERY);
			if (xErrorDialog.is())
				xErrorDialog->execute();
			else
			{
				OSL_ENSURE(0,"dbtools::showError: no XExecutableDialog found!");
			}
		}
		catch(Exception&)
		{
			OSL_ENSURE(0,"showError: could not display the error message!");
		}
	}
}

// -------------------------------------------------------------------------
sal_Bool implUpdateObject(const Reference< XRowUpdate >& _rxUpdatedObject,
	const sal_Int32 _nColumnIndex, const Any& _rValue) SAL_THROW ( ( SQLException, RuntimeException ) )
{
	sal_Bool bSuccessfullyReRouted = sal_True;
	switch (_rValue.getValueTypeClass())
	{
		case TypeClass_ANY:
		{
			Any aInnerValue;
			_rValue >>= aInnerValue;
			bSuccessfullyReRouted = implUpdateObject(_rxUpdatedObject, _nColumnIndex, aInnerValue);
		}
		break;

		case TypeClass_VOID:
			_rxUpdatedObject->updateNull(_nColumnIndex);
			break;

		case TypeClass_STRING:
			_rxUpdatedObject->updateString(_nColumnIndex, *(rtl::OUString*)_rValue.getValue());
			break;

		case TypeClass_BOOLEAN:
			_rxUpdatedObject->updateBoolean(_nColumnIndex, *(sal_Bool *)_rValue.getValue());
			break;

		case TypeClass_BYTE:
			_rxUpdatedObject->updateByte(_nColumnIndex, *(sal_Int8 *)_rValue.getValue());
			break;

		case TypeClass_UNSIGNED_SHORT:
		case TypeClass_SHORT:
			_rxUpdatedObject->updateShort(_nColumnIndex, *(sal_Int16*)_rValue.getValue());
			break;

		case TypeClass_CHAR:
			_rxUpdatedObject->updateString(_nColumnIndex,::rtl::OUString((sal_Unicode *)_rValue.getValue(),1));
			break;

		case TypeClass_UNSIGNED_LONG:
		case TypeClass_LONG:
			_rxUpdatedObject->updateInt(_nColumnIndex, *(sal_Int32*)_rValue.getValue());
			break;

        case TypeClass_HYPER:
		{
			sal_Int64 nValue = 0;
			OSL_VERIFY( _rValue >>= nValue );
			_rxUpdatedObject->updateLong( _nColumnIndex, nValue );
		}
		break;

		case TypeClass_FLOAT:
			_rxUpdatedObject->updateFloat(_nColumnIndex, *(float*)_rValue.getValue());
			break;

		case TypeClass_DOUBLE:
			_rxUpdatedObject->updateDouble(_nColumnIndex, *(double*)_rValue.getValue());
			break;

		case TypeClass_SEQUENCE:
			if (_rValue.getValueType() == ::getCppuType((const Sequence< sal_Int8 > *)0))
				_rxUpdatedObject->updateBytes(_nColumnIndex, *(Sequence<sal_Int8>*)_rValue.getValue());
			else
				bSuccessfullyReRouted = sal_False;
			break;
		case TypeClass_STRUCT:
			if (_rValue.getValueType() == ::getCppuType((const DateTime*)0))
				_rxUpdatedObject->updateTimestamp(_nColumnIndex, *(DateTime*)_rValue.getValue());
			else if (_rValue.getValueType() == ::getCppuType((const Date*)0))
				_rxUpdatedObject->updateDate(_nColumnIndex, *(Date*)_rValue.getValue());
			else if (_rValue.getValueType() == ::getCppuType((const Time*)0))
				_rxUpdatedObject->updateTime(_nColumnIndex, *(Time*)_rValue.getValue());
			else
				bSuccessfullyReRouted = sal_False;
			break;

		case TypeClass_INTERFACE:
			if (_rValue.getValueType() == ::getCppuType(static_cast<Reference< XInputStream>*>(NULL)))
			{
				Reference< XInputStream >  xStream;
				_rValue >>= xStream;
				_rxUpdatedObject->updateBinaryStream(_nColumnIndex, xStream, xStream->available());
				break;
			}
			// run through
		default:
			bSuccessfullyReRouted = sal_False;
	}

	return bSuccessfullyReRouted;
}
// -------------------------------------------------------------------------
sal_Bool implSetObject(	const Reference< XParameters >& _rxParameters,
						const sal_Int32 _nColumnIndex, const Any& _rValue) SAL_THROW ( ( SQLException, RuntimeException ) )
{
	sal_Bool bSuccessfullyReRouted = sal_True;
	switch (_rValue.getValueTypeClass())
	{
		case TypeClass_HYPER:
		{
			sal_Int64 nValue = 0;
			OSL_VERIFY( _rValue >>= nValue );
			_rxParameters->setLong( _nColumnIndex, nValue );
		}
		break;

		case TypeClass_ANY:
		{
			Any aInnerValue;
			_rValue >>= aInnerValue;
			bSuccessfullyReRouted = implSetObject(_rxParameters, _nColumnIndex, aInnerValue);
		}
		break;

		case TypeClass_VOID:
			_rxParameters->setNull(_nColumnIndex,DataType::VARCHAR);
			break;

		case TypeClass_STRING:
			_rxParameters->setString(_nColumnIndex, *(rtl::OUString*)_rValue.getValue());
			break;

		case TypeClass_BOOLEAN:
			_rxParameters->setBoolean(_nColumnIndex, *(sal_Bool *)_rValue.getValue());
			break;

		case TypeClass_BYTE:
			_rxParameters->setByte(_nColumnIndex, *(sal_Int8 *)_rValue.getValue());
			break;

		case TypeClass_UNSIGNED_SHORT:
		case TypeClass_SHORT:
			_rxParameters->setShort(_nColumnIndex, *(sal_Int16*)_rValue.getValue());
			break;

		case TypeClass_CHAR:
			_rxParameters->setString(_nColumnIndex, ::rtl::OUString((sal_Unicode *)_rValue.getValue(),1));
			break;

		case TypeClass_UNSIGNED_LONG:
		case TypeClass_LONG:
			_rxParameters->setInt(_nColumnIndex, *(sal_Int32*)_rValue.getValue());
			break;

		case TypeClass_FLOAT:
			_rxParameters->setFloat(_nColumnIndex, *(float*)_rValue.getValue());
			break;

		case TypeClass_DOUBLE:
			_rxParameters->setDouble(_nColumnIndex, *(double*)_rValue.getValue());
			break;

		case TypeClass_SEQUENCE:
			if (_rValue.getValueType() == ::getCppuType((const Sequence< sal_Int8 > *)0))
			{
				_rxParameters->setBytes(_nColumnIndex, *(Sequence<sal_Int8>*)_rValue.getValue());
			}
			else
				bSuccessfullyReRouted = sal_False;
			break;
		case TypeClass_STRUCT:
			if (_rValue.getValueType() == ::getCppuType((const DateTime*)0))
				_rxParameters->setTimestamp(_nColumnIndex, *(DateTime*)_rValue.getValue());
			else if (_rValue.getValueType() == ::getCppuType((const Date*)0))
				_rxParameters->setDate(_nColumnIndex, *(Date*)_rValue.getValue());
			else if (_rValue.getValueType() == ::getCppuType((const Time*)0))
				_rxParameters->setTime(_nColumnIndex, *(Time*)_rValue.getValue());
			else
				bSuccessfullyReRouted = sal_False;
			break;

		case TypeClass_INTERFACE:
			if (_rValue.getValueType() == ::getCppuType(static_cast<Reference< XInputStream>*>(NULL)))
			{
				Reference< XInputStream >  xStream;
				_rValue >>= xStream;
				_rxParameters->setBinaryStream(_nColumnIndex, xStream, xStream->available());
				break;
			}
			// run through
		default:
			bSuccessfullyReRouted = sal_False;

	}

	return bSuccessfullyReRouted;
}

//..................................................................
namespace
{
    class OParameterWrapper : public ::cppu::WeakImplHelper1< XIndexAccess > 
    {
        ::std::bit_vector       m_aSet;
        Reference<XIndexAccess> m_xSource;
    public:
        OParameterWrapper(const ::std::bit_vector& _aSet,const Reference<XIndexAccess>& _xSource) : m_aSet(_aSet),m_xSource(_xSource){}
    private:
        // ::com::sun::star::container::XElementAccess
        virtual Type SAL_CALL getElementType() throw(RuntimeException)
        {
            return m_xSource->getElementType();
        }
		virtual sal_Bool SAL_CALL hasElements(  ) throw(RuntimeException)
        {
            if ( m_aSet.empty() )
                return m_xSource->hasElements();
            return ::std::count(m_aSet.begin(),m_aSet.end(),false) != 0;
        }
        // ::com::sun::star::container::XIndexAccess
        virtual sal_Int32 SAL_CALL getCount(  ) throw(RuntimeException)
        {
            if ( m_aSet.empty() )
                return m_xSource->getCount();
            return ::std::count(m_aSet.begin(),m_aSet.end(),false);
        }
        virtual Any SAL_CALL getByIndex( sal_Int32 Index ) throw(IndexOutOfBoundsException, WrappedTargetException, RuntimeException)
        {
            if ( m_aSet.empty() )
                return m_xSource->getByIndex(Index);
            if ( m_aSet.size() < (size_t)Index )
                throw IndexOutOfBoundsException();

            ::std::bit_vector::iterator aIter = m_aSet.begin();
            ::std::bit_vector::iterator aEnd = m_aSet.end();
            sal_Int32 i = 0;
            sal_Int32 nParamPos = -1;
            for(; aIter != aEnd && i <= Index; ++aIter)
            {
                ++nParamPos;
                if ( !*aIter )
                {
                    ++i;
                }
            }
            return m_xSource->getByIndex(nParamPos);
        }
    };
}

// -----------------------------------------------------------------------------
void askForParameters(const Reference< XSingleSelectQueryComposer >& _xComposer,
					  const Reference<XParameters>& _xParameters,
					  const Reference< XConnection>& _xConnection,
					  const Reference< XInteractionHandler >& _rxHandler,
                      const ::std::bit_vector& _aParametersSet)
{
	OSL_ENSURE(_xComposer.is(),"dbtools::askForParameters XSQLQueryComposer is null!");
	OSL_ENSURE(_xParameters.is(),"dbtools::askForParameters XParameters is null!");
	OSL_ENSURE(_xConnection.is(),"dbtools::askForParameters XConnection is null!");
	OSL_ENSURE(_rxHandler.is(),"dbtools::askForParameters XInteractionHandler is null!");

	// we have to set this here again because getCurrentSettingsComposer can force a setpropertyvalue
	Reference<XParametersSupplier>  xParameters = Reference<XParametersSupplier> (_xComposer, UNO_QUERY);

	Reference<XIndexAccess>  xParamsAsIndicies = xParameters.is() ? xParameters->getParameters() : Reference<XIndexAccess>();
	Reference<XNameAccess>   xParamsAsNames(xParamsAsIndicies, UNO_QUERY);
	sal_Int32 nParamCount = xParamsAsIndicies.is() ? xParamsAsIndicies->getCount() : 0;
    if ( (nParamCount && _aParametersSet.empty()) || ::std::count(_aParametersSet.begin(),_aParametersSet.end(),true) != nParamCount )
	{
		// build an interaction request
		// two continuations (Ok and Cancel)
		OInteractionAbort* pAbort = new OInteractionAbort;
		OParameterContinuation* pParams = new OParameterContinuation;
		// the request
		ParametersRequest aRequest;
        Reference<XIndexAccess> xWrappedParameters = new OParameterWrapper(_aParametersSet,xParamsAsIndicies);
		aRequest.Parameters = xWrappedParameters;
		aRequest.Connection = _xConnection;
		OInteractionRequest* pRequest = new OInteractionRequest(makeAny(aRequest));
		Reference< XInteractionRequest > xRequest(pRequest);
		// some knittings
		pRequest->addContinuation(pAbort);
		pRequest->addContinuation(pParams);

		// execute the request
		_rxHandler->handle(xRequest);

		if (!pParams->wasSelected())
		{
			// canceled by the user (i.e. (s)he canceled the dialog)
			RowSetVetoException e;
			e.ErrorCode = ParameterInteractionCancelled;
			throw e;
		}

		// now transfer the values from the continuation object to the parameter columns
		Sequence< PropertyValue > aFinalValues = pParams->getValues();
		const PropertyValue* pFinalValues = aFinalValues.getConstArray();
		for (sal_Int32 i=0; i<aFinalValues.getLength(); ++i, ++pFinalValues)
		{
			Reference< XPropertySet > xParamColumn(xWrappedParameters->getByIndex(i),UNO_QUERY);
			if (xParamColumn.is())
			{
#ifdef DBG_UTIL
				::rtl::OUString sName;
				xParamColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= sName;
				OSL_ENSURE(sName.equals(pFinalValues->Name), "::dbaui::askForParameters: inconsistent parameter names!");
#endif
				// determine the field type and ...
				sal_Int32 nParamType = 0;
				xParamColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE)) >>= nParamType;
				// ... the scale of the parameter column
				sal_Int32 nScale = 0;
				if (hasProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE), xParamColumn))
					xParamColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE)) >>= nScale;
				// and set the value
                ::std::bit_vector::const_iterator aIter = _aParametersSet.begin();
                ::std::bit_vector::const_iterator aEnd = _aParametersSet.end();
                sal_Int32 j = 0;
                sal_Int32 nParamPos = -1;
                for(; aIter != aEnd && j <= i; ++aIter)
                {
                    ++nParamPos;
                    if ( !*aIter )
                    {
                        ++j;
                    }
                }
				_xParameters->setObjectWithInfo(nParamPos + 1, pFinalValues->Value, nParamType, nScale);
					// (the index of the parameters is one-based)
			}
		}
	}
}
// -----------------------------------------------------------------------------
void setObjectWithInfo(const Reference<XParameters>& _xParams,
					   sal_Int32 parameterIndex,
					   const Any& x,
					   sal_Int32 sqlType,
					   sal_Int32 /*scale*/)  throw(SQLException, RuntimeException)
{
	if(!x.hasValue())
		_xParams->setNull(parameterIndex,sqlType);
	else
	{
		switch(sqlType)
		{
            case DataType::DECIMAL:
            case DataType::NUMERIC:
                _xParams->setObjectWithInfo(parameterIndex,x,sqlType,0);
                break;
			case DataType::CHAR:
			case DataType::VARCHAR:
			//case DataType::DECIMAL:
			//case DataType::NUMERIC:
			case DataType::LONGVARCHAR:
				_xParams->setString(parameterIndex,::comphelper::getString(x));
				break;
			case DataType::BIGINT:
				{
					sal_Int64 nValue = 0;
					if(x >>= nValue)
					{
						_xParams->setLong(parameterIndex,nValue);
						break;
					}
				}
				break;

			case DataType::FLOAT:
			case DataType::REAL:
				{
					float nValue = 0;
					if(x >>= nValue)
					{
						_xParams->setFloat(parameterIndex,nValue);
						break;
					}
				}
				// run through if we couldn't set a float value
			case DataType::DOUBLE:
				_xParams->setDouble(parameterIndex,::comphelper::getDouble(x));
				break;
			case DataType::DATE:
				{
					::com::sun::star::util::Date aValue;
					if(x >>= aValue)
						_xParams->setDate(parameterIndex,aValue);
				}
				break;
			case DataType::TIME:
				{
					::com::sun::star::util::Time aValue;
					if(x >>= aValue)
						_xParams->setTime(parameterIndex,aValue);
				}
				break;
			case DataType::TIMESTAMP:
				{
					::com::sun::star::util::DateTime aValue;
					if(x >>= aValue)
						_xParams->setTimestamp(parameterIndex,aValue);
				}
				break;
			case DataType::BINARY:
			case DataType::VARBINARY:
			case DataType::LONGVARBINARY:
				{
					Sequence< sal_Int8> aBytes;
					if(x >>= aBytes)
						_xParams->setBytes(parameterIndex,aBytes);
					else
					{
						Reference< XBlob > xBlob;
						if(x >>= xBlob)
							_xParams->setBlob(parameterIndex,xBlob);
						else
						{
							Reference< XClob > xClob;
							if(x >>= xClob)
								_xParams->setClob(parameterIndex,xClob);
							else
							{
								Reference< ::com::sun::star::io::XInputStream > xBinStream;
								if(x >>= xBinStream)
									_xParams->setBinaryStream(parameterIndex,xBinStream,xBinStream->available());
							}
						}
					}
				}
				break;
			case DataType::BIT:
			case DataType::BOOLEAN:
				_xParams->setBoolean(parameterIndex,::cppu::any2bool(x));
				break;
			case DataType::TINYINT:
				_xParams->setByte(parameterIndex,(sal_Int8)::comphelper::getINT32(x));
				break;
			case DataType::SMALLINT:
				_xParams->setShort(parameterIndex,(sal_Int16)::comphelper::getINT32(x));
				break;
			case DataType::INTEGER:
				_xParams->setInt(parameterIndex,::comphelper::getINT32(x));
				break;
			default:
				{
                    ::connectivity::SharedResources aResources;
                    const ::rtl::OUString sError( aResources.getResourceStringWithSubstitution(
                            STR_UNKNOWN_PARA_TYPE,
                            "$position$", ::rtl::OUString::valueOf(parameterIndex)
                         ) );
		            ::dbtools::throwGenericSQLException(sError,NULL);
				}
		}
	}
}

// --------------------------------------------------------------------
void getBoleanComparisonPredicate( const ::rtl::OUString& _rExpression, const sal_Bool _bValue, const sal_Int32 _nBooleanComparisonMode,
    ::rtl::OUStringBuffer& _out_rSQLPredicate )
{
    switch ( _nBooleanComparisonMode )
    {
    case BooleanComparisonMode::IS_LITERAL:
        _out_rSQLPredicate.append( _rExpression );
        if ( _bValue )
            _out_rSQLPredicate.appendAscii( " IS TRUE" );
        else
            _out_rSQLPredicate.appendAscii( " IS FALSE" );
        break;

    case BooleanComparisonMode::EQUAL_LITERAL:
        _out_rSQLPredicate.append( _rExpression );
        _out_rSQLPredicate.appendAscii( _bValue ? " = TRUE" : " = FALSE" );
        break;

    case BooleanComparisonMode::ACCESS_COMPAT:
        if ( _bValue )
        {
            _out_rSQLPredicate.appendAscii( " NOT ( ( " );
            _out_rSQLPredicate.append( _rExpression );
            _out_rSQLPredicate.appendAscii( " = 0 ) OR ( " );
            _out_rSQLPredicate.append( _rExpression );
            _out_rSQLPredicate.appendAscii( " IS NULL ) )" );
        }
        else
        {
            _out_rSQLPredicate.append( _rExpression );
            _out_rSQLPredicate.appendAscii( " = 0" );
        }
        break;

    case BooleanComparisonMode::EQUAL_INTEGER:
        // fall through
    default:
        _out_rSQLPredicate.append( _rExpression );
        _out_rSQLPredicate.appendAscii( _bValue ? " = 1" : " = 0" );
        break;
    }
}

//.........................................................................
}	// namespace dbtools
//.........................................................................

//.........................................................................
namespace connectivity
{
//.........................................................................

void release(oslInterlockedCount& _refCount,
			 ::cppu::OBroadcastHelper& rBHelper,
			 ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >& _xInterface,
			 ::com::sun::star::lang::XComponent* _pObject)
{
	if (osl_decrementInterlockedCount( &_refCount ) == 0)
	{
		osl_incrementInterlockedCount( &_refCount );

		if (!rBHelper.bDisposed && !rBHelper.bInDispose)
		{
			// remember the parent
			::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > xParent;
			{
				::osl::MutexGuard aGuard( rBHelper.rMutex );
				xParent = _xInterface;
				_xInterface = NULL;
			}

			// First dispose
			_pObject->dispose();

			// only the alive ref holds the object
			OSL_ASSERT( _refCount == 1 );

			// release the parent in the ~
			if (xParent.is())
			{
				::osl::MutexGuard aGuard( rBHelper.rMutex );
				_xInterface = xParent;
			}

//					// destroy the object if xHoldAlive decrement the refcount to 0
//					m_pDerivedImplementation->WEAK::release();
		}
	}
	else
		osl_incrementInterlockedCount( &_refCount );
}

void checkDisposed(sal_Bool _bThrow) throw ( DisposedException )
{
	if (_bThrow)
		throw DisposedException();

}
// -------------------------------------------------------------------------
	OSQLColumns::Vector::const_iterator find(	OSQLColumns::Vector::const_iterator __first,
										OSQLColumns::Vector::const_iterator __last,
										const ::rtl::OUString& _rVal,
										const ::comphelper::UStringMixEqual& _rCase)
	{
		::rtl::OUString sName = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME);
		return find(__first,__last,sName,_rVal,_rCase);
	}
	// -------------------------------------------------------------------------
	OSQLColumns::Vector::const_iterator findRealName(	OSQLColumns::Vector::const_iterator __first,
										OSQLColumns::Vector::const_iterator __last,
										const ::rtl::OUString& _rVal,
										const ::comphelper::UStringMixEqual& _rCase)
	{
		::rtl::OUString sRealName = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_REALNAME);
		return find(__first,__last,sRealName,_rVal,_rCase);
	}
	// -------------------------------------------------------------------------
	OSQLColumns::Vector::const_iterator find(	OSQLColumns::Vector::const_iterator __first,
										OSQLColumns::Vector::const_iterator __last,
										const ::rtl::OUString& _rProp,
										const ::rtl::OUString& _rVal,
										const ::comphelper::UStringMixEqual& _rCase)
	{
		while (__first != __last && !_rCase(getString((*__first)->getPropertyValue(_rProp)),_rVal))
			++__first;
		return __first;
	}

// -----------------------------------------------------------------------------
} //namespace connectivity
// -----------------------------------------------------------------------------

