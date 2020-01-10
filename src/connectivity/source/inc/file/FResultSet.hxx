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

#ifndef _CONNECTIVITY_FILE_FRESULTSET_HXX_
#define _CONNECTIVITY_FILE_FRESULTSET_HXX_

#ifndef _COM_SUN_STAR_SQLC_XRESULTSET_HPP_
#include <com/sun/star/sdbc/XResultSet.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XROW_HPP_
#include <com/sun/star/sdbc/XRow.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XRESULTSETMETADATASUPPLIER_HPP_
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XCLOSEABLE_HPP_
#include <com/sun/star/sdbc/XCloseable.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XCOLUMNLOCATE_HPP_
#include <com/sun/star/sdbc/XColumnLocate.hpp>
#endif
#include <com/sun/star/util/XCancellable.hpp>
#ifndef _COM_SUN_STAR_SQLC_XWARNINGSSUPPLIER_HPP_
#include <com/sun/star/sdbc/XWarningsSupplier.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XRESULTSETUPDATE_HPP_
#include <com/sun/star/sdbc/XResultSetUpdate.hpp>
#endif
#ifndef _COM_SUN_STAR_SQLC_XROWUPDATE_HPP_
#include <com/sun/star/sdbc/XRowUpdate.hpp>
#endif
#include <cppuhelper/compbase12.hxx>
#include <comphelper/proparrhlp.hxx>
#include "file/FStatement.hxx"
#include "connectivity/CommonTools.hxx"
#include <comphelper/propertycontainer.hxx>
#include "file/fanalyzer.hxx"
#include "file/FTable.hxx"
#include "file/filedllapi.hxx"
#include <comphelper/broadcasthelper.hxx>
#include "connectivity/StdTypeDefs.hxx"
#include "TSortIndex.hxx"
#include "TSkipDeletedSet.hxx"
#include <com/sun/star/lang/XEventListener.hpp>

namespace connectivity
{
	namespace file
	{
		/*
		**	java_sql_ResultSet
		*/
        typedef ::cppu::WeakComponentImplHelper12<  ::com::sun::star::sdbc::XResultSet,
                                                    ::com::sun::star::sdbc::XRow,
                                                    ::com::sun::star::sdbc::XResultSetMetaDataSupplier,
                                                    ::com::sun::star::util::XCancellable,
                                                    ::com::sun::star::sdbc::XWarningsSupplier,
                                                    ::com::sun::star::sdbc::XResultSetUpdate,
                                                    ::com::sun::star::sdbc::XRowUpdate,
                                                    ::com::sun::star::sdbc::XCloseable,
                                                    ::com::sun::star::sdbc::XColumnLocate,
                                                    ::com::sun::star::lang::XServiceInfo,
													::com::sun::star::lang::XEventListener,
													::com::sun::star::lang::XUnoTunnel> OResultSet_BASE;

		class OOO_DLLPUBLIC_FILE OResultSet :
                            public	comphelper::OBaseMutex,
							public	::connectivity::IResultSetHelper,
							public	OResultSet_BASE,
							public	::comphelper::OPropertyContainer,
							public	::comphelper::OPropertyArrayUsageHelper<OResultSet>
		{

		protected:
			::std::vector<void*>					m_aBindVector;
			::std::vector<sal_Int32>				m_aColMapping; // pos 0 is unused so we don't have to decrement 1 everytime

			::std::vector<sal_Int32>				m_aOrderbyColumnNumber;
            ::std::vector<TAscendingOrder>          m_aOrderbyAscending;

			OValueRefRow							m_aSelectRow;
			OValueRefRow							m_aRow;
			OValueRefRow							m_aEvaluateRow; // contains all values of a row
			OValueRefRow							m_aParameterRow;
			OValueRefRow							m_aInsertRow;	// needed for insert by cursor
			ORefAssignValues						m_aAssignValues; // needed for insert,update and parameters
																	// to compare with the restrictions
			TIntVector*								m_pEvaluationKeySet;
			TIntVector::iterator					m_aEvaluateIter;


//			TInt2IntMap								m_aBookmarks;		  // map from bookmarks to logical position
//			::std::vector<TInt2IntMap::iterator>	m_aBookmarksPositions;// vector of iterators to bookmark map, the order is the logical position
			OSkipDeletedSet							m_aSkipDeletedSet;

			::vos::ORef<OKeySet>					m_pFileSet;
			OKeySet::Vector::iterator				m_aFileSetIter;



			OSortIndex*								m_pSortIndex;
			::vos::ORef<connectivity::OSQLColumns>	m_xColumns;	// this are the select columns
			::vos::ORef<connectivity::OSQLColumns>	m_xParamColumns;
			OFileTable*								m_pTable;
			connectivity::OSQLParseNode*			m_pParseTree;

			OSQLAnalyzer*							m_pSQLAnalyzer;
			connectivity::OSQLParseTreeIterator&	m_aSQLIterator;

			sal_Int32								m_nFetchSize;
			sal_Int32								m_nResultSetType;
			sal_Int32								m_nFetchDirection;
			sal_Int32								m_nResultSetConcurrency;

			::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface>			m_xStatement;
            ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XResultSetMetaData>	m_xMetaData;
            ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XDatabaseMetaData>	m_xDBMetaData;
			::com::sun::star::uno::Reference< ::com::sun::star::container::XNameAccess>		m_xColNames; // table columns
			::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexAccess>	m_xColsIdx; // table columns


			::rtl::OUString							m_aTableRange;
			rtl_TextEncoding						m_nTextEncoding;
			sal_Int32								m_nRowPos;
			sal_Int32								m_nFilePos;
			sal_Int32								m_nLastVisitedPos;
			sal_Int32								m_nRowCountResult;
			sal_Int32								m_nCurrentPosition;		// current position of the resultset is returned when ask for getRow()
            sal_Int32								m_nColumnCount;
			sal_Bool								m_bWasNull;
			sal_Bool								m_bEOF;					// after last record
			sal_Bool								m_bLastRecord;
			sal_Bool								m_bInserted;			// true when moveToInsertRow was called
																			// set to false when cursor moved or cancel
			sal_Bool								m_bRowUpdated;
			sal_Bool								m_bRowInserted;
			sal_Bool								m_bRowDeleted;
			sal_Bool								m_bShowDeleted;
            sal_Bool								m_bIsCount;

			void initializeRow(OValueRefRow& _rRow,sal_Int32 _nColumnCount);
			void construct();
			sal_Bool evaluate();

			BOOL ExecuteRow(IResultSetHelper::Movement eFirstCursorPosition,
								INT32 nOffset = 1,
								BOOL bEvaluate = TRUE,
								BOOL bRetrieveData = TRUE);

			OKeyValue* GetOrderbyKeyValue(OValueRefRow& _rRow);
			BOOL IsSorted() const { return !m_aOrderbyColumnNumber.empty() && m_aOrderbyColumnNumber[0] != SQL_COLUMN_NOTFOUND;}

			// return true when the select statement is "select count(*) from table"
            inline sal_Bool isCount() const { return m_bIsCount; }
			void checkIndex(sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException);

			const ORowSetValue& getValue(sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException);
			void updateValue(sal_Int32 columnIndex,const ORowSetValue& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// clear insert row
			void clearInsertRow();
			void sortRows();
		protected:

			using OResultSet_BASE::rBHelper;

			BOOL Move(IResultSetHelper::Movement eCursorPosition, INT32 nOffset, BOOL bRetrieveData);
			virtual sal_Bool fillIndexValues(const ::com::sun::star::uno::Reference< ::com::sun::star::sdbcx::XColumnsSupplier> &_xIndex);

			// OPropertyArrayUsageHelper
			virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const;
			// OPropertySetHelper
			virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper();

			virtual ~OResultSet();
		public:
			DECLARE_SERVICE_INFO();
			// ein Konstruktor, der fuer das Returnen des Objektes benoetigt wird:
			OResultSet( OStatement_Base* pStmt,connectivity::OSQLParseTreeIterator&	_aSQLIterator);

			::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > operator *()
			{
				return ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >(*(OResultSet_BASE*)this);
			}

			// ::cppu::OComponentHelper
			virtual void SAL_CALL disposing(void);
			// XInterface
			virtual ::com::sun::star::uno::Any SAL_CALL queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException);
			virtual void SAL_CALL acquire() throw();
            virtual void SAL_CALL release() throw();
			//XTypeProvider
			virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Type > SAL_CALL getTypes(  ) throw(::com::sun::star::uno::RuntimeException);
			// XPropertySet
			virtual ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw(::com::sun::star::uno::RuntimeException);
			// XResultSet
            virtual sal_Bool SAL_CALL next(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isBeforeFirst(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isAfterLast(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isFirst(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL isLast(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL beforeFirst(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL afterLast(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL first(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL last(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int32 SAL_CALL getRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL absolute( sal_Int32 row ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL relative( sal_Int32 rows ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL previous(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL refreshRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL rowUpdated(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL rowInserted(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL rowDeleted(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL getStatement(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XRow
            virtual sal_Bool SAL_CALL wasNull(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::rtl::OUString SAL_CALL getString( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL getBoolean( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int8 SAL_CALL getByte( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int16 SAL_CALL getShort( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int32 SAL_CALL getInt( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int64 SAL_CALL getLong( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual float SAL_CALL getFloat( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual double SAL_CALL getDouble( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Sequence< sal_Int8 > SAL_CALL getBytes( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::util::Date SAL_CALL getDate( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::util::Time SAL_CALL getTime( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::util::DateTime SAL_CALL getTimestamp( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getBinaryStream( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL getCharacterStream( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Any SAL_CALL getObject( sal_Int32 columnIndex, const ::com::sun::star::uno::Reference< ::com::sun::star::container::XNameAccess >& typeMap ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XRef > SAL_CALL getRef( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XBlob > SAL_CALL getBlob( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XClob > SAL_CALL getClob( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XArray > SAL_CALL getArray( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XResultSetMetaDataSupplier
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XResultSetMetaData > SAL_CALL getMetaData(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XCancellable
			virtual void SAL_CALL cancel(  ) throw(::com::sun::star::uno::RuntimeException);
			// XCloseable
            virtual void SAL_CALL close(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XWarningsSupplier
            virtual ::com::sun::star::uno::Any SAL_CALL getWarnings(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL clearWarnings(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XResultSetUpdate
            virtual void SAL_CALL insertRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL deleteRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL cancelRowUpdates(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL moveToInsertRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL moveToCurrentRow(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XRowUpdate
            virtual void SAL_CALL updateNull( sal_Int32 columnIndex ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateBoolean( sal_Int32 columnIndex, sal_Bool x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateByte( sal_Int32 columnIndex, sal_Int8 x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateShort( sal_Int32 columnIndex, sal_Int16 x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateInt( sal_Int32 columnIndex, sal_Int32 x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateLong( sal_Int32 columnIndex, sal_Int64 x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateFloat( sal_Int32 columnIndex, float x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateDouble( sal_Int32 columnIndex, double x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateString( sal_Int32 columnIndex, const ::rtl::OUString& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateBytes( sal_Int32 columnIndex, const ::com::sun::star::uno::Sequence< sal_Int8 >& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateDate( sal_Int32 columnIndex, const ::com::sun::star::util::Date& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateTime( sal_Int32 columnIndex, const ::com::sun::star::util::Time& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateTimestamp( sal_Int32 columnIndex, const ::com::sun::star::util::DateTime& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateBinaryStream( sal_Int32 columnIndex, const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& x, sal_Int32 length ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateCharacterStream( sal_Int32 columnIndex, const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& x, sal_Int32 length ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateObject( sal_Int32 columnIndex, const ::com::sun::star::uno::Any& x ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual void SAL_CALL updateNumericObject( sal_Int32 columnIndex, const ::com::sun::star::uno::Any& x, sal_Int32 scale ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XColumnLocate
            virtual sal_Int32 SAL_CALL findColumn( const ::rtl::OUString& columnName ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// com::sun::star::lang::XUnoTunnel
			virtual sal_Int64 SAL_CALL getSomething( const ::com::sun::star::uno::Sequence< sal_Int8 >& aIdentifier ) throw(::com::sun::star::uno::RuntimeException);
			static ::com::sun::star::uno::Sequence< sal_Int8 > getUnoTunnelImplementationId();
			//XEventlistener
			virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);

			// special methods
			inline sal_Int32 mapColumn(sal_Int32	column);
			virtual BOOL OpenImpl();
			virtual void doTableSpecials(const OSQLTable& _xTable);

			inline sal_Int32 getRowCountResult() const { return m_nRowCountResult; }
            inline void setParameterRow(const OValueRefRow& _rParaRow)					{ m_aParameterRow = _rParaRow; }
			inline void setEvaluationRow(const OValueRefRow& _aRow)						{ m_aEvaluateRow = _aRow; }
			inline void setParameterColumns(const ::vos::ORef<connectivity::OSQLColumns>&	_xParamColumns) { m_xParamColumns = _xParamColumns; }
			inline void setAssignValues(const ORefAssignValues& _aAssignValues)			{ m_aAssignValues = _aAssignValues; }
			inline void setBindingRow(const OValueRefRow& _aRow)						{ m_aRow = _aRow; }
			inline void setSelectRow(const OValueRefRow& _rRow)							
            { 
                m_aSelectRow = _rRow; 
                m_nColumnCount = m_aSelectRow->get().size();
            }
			inline void setColumnMapping(const ::std::vector<sal_Int32>& _aColumnMapping)	{ m_aColMapping = _aColumnMapping; }
			inline void setSqlAnalyzer(OSQLAnalyzer* _pSQLAnalyzer)						{ m_pSQLAnalyzer = _pSQLAnalyzer; }

			inline void setOrderByColumns(const ::std::vector<sal_Int32>& _aColumnOrderBy)	{ m_aOrderbyColumnNumber = _aColumnOrderBy; }
            inline void setOrderByAscending(const ::std::vector<TAscendingOrder>& _aOrderbyAsc)    { m_aOrderbyAscending = _aOrderbyAsc; }
			inline void setEvaluationKeySet(TIntVector* _pEvaluationKeySet)				{ m_pEvaluationKeySet = _pEvaluationKeySet; }
            inline void setMetaData(const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XResultSetMetaData>& _xMetaData) { m_xMetaData = _xMetaData;}

			// clears the resultset so it can be reused by a preparedstatement
			void clear();
			static void setBoundedColumns(const OValueRefRow& _rRow,
									const OValueRefRow& _rSelectRow,
									const ::vos::ORef<connectivity::OSQLColumns>& _rxColumns,
									const ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexAccess>& _xNames,
									sal_Bool _bSetColumnMapping,
									const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XDatabaseMetaData>& _xMetaData,
									::std::vector<sal_Int32>& _rColMapping);

			// IResultSetHelper
			virtual sal_Bool move(IResultSetHelper::Movement _eCursorPosition, sal_Int32 _nOffset, sal_Bool _bRetrieveData);
			virtual sal_Int32 getDriverPos() const;
			virtual sal_Bool deletedVisible() const;
			virtual sal_Bool isRowDeleted() const;
		};
		// -------------------------------------------------------------------------
		inline sal_Int32 OResultSet::mapColumn(sal_Int32 column)
		{
			sal_Int32	map = column;

			OSL_ENSURE(column > 0, "file::OResultSet::mapColumn: invalid column index!");
				// the first column (index 0) is for convenience only. The first real select column is no 1.
			if ((column > 0) && (column < (sal_Int32)m_aColMapping.size()))
				map = m_aColMapping[column];

			return map;
		}
	}
}
#endif // _CONNECTIVITY_FILE_ORESULTSET_HXX_


