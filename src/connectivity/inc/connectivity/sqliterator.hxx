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
#ifndef _CONNECTIVITY_PARSE_SQLITERATOR_HXX_
#define _CONNECTIVITY_PARSE_SQLITERATOR_HXX_

#include "connectivity/dbtoolsdllapi.hxx"
#include "connectivity/sqlnode.hxx"
#include <connectivity/IParseContext.hxx>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/sdbc/SQLWarning.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include "connectivity/CommonTools.hxx"
#include <vos/ref.hxx>
#include <cppuhelper/weak.hxx>

#include <map>
#include <memory>

namespace connectivity
{

	class OSQLParseNode;
	class OSQLParser;

	enum OSQLStatementType {
		SQL_STATEMENT_UNKNOWN,
		SQL_STATEMENT_SELECT,
		SQL_STATEMENT_INSERT,
		SQL_STATEMENT_UPDATE,
		SQL_STATEMENT_DELETE,
		SQL_STATEMENT_ODBC_CALL,
		SQL_STATEMENT_CREATE_TABLE
	};

	struct OSQLParseTreeIteratorImpl;

    class OOO_DLLPUBLIC_DBTOOLS OSQLParseTreeIterator
	{
	private:
		::com::sun::star::sdbc::SQLException            m_aErrors;		    // conatins the error while iterating through the statement
		const OSQLParseNode*				            m_pParseTree;		// aktueller ParseTree
		const OSQLParser&					            m_rParser;			// if set used for general error messages from the context
		OSQLStatementType					            m_eStatementType;	// Art des Statements
		::vos::ORef<OSQLColumns>			            m_aSelectColumns;	// alle Spalten aus dem Select-Clause
		::vos::ORef<OSQLColumns>			            m_aParameters;		// all parameters
		::vos::ORef<OSQLColumns>			            m_aGroupColumns;	// the group by columns
		::vos::ORef<OSQLColumns>			            m_aOrderColumns;	// the order by columns
		::vos::ORef<OSQLColumns>			            m_aCreateColumns;	// the columns for Create table clause

        ::std::auto_ptr< OSQLParseTreeIteratorImpl >    m_pImpl;

        void				traverseParameter(const OSQLParseNode* _pParseNode,const OSQLParseNode* _pColumnRef,const ::rtl::OUString& _aColumnName,const ::rtl::OUString& _aTableRange, const ::rtl::OUString& _rColumnAlias);
		// F"ugt eine Tabelle in die Map ein
		void				traverseOneTableName( OSQLTables& _rTables,const OSQLParseNode * pTableName, const ::rtl::OUString & rTableRange );
		void				traverseORCriteria(OSQLParseNode * pSearchCondition);
		void				traverseANDCriteria(OSQLParseNode * pSearchCondition);
		void				traverseOnePredicate(
												OSQLParseNode * pColumnRef,
												::rtl::OUString& aValue,
												OSQLParseNode * pParameter);
		void traverseByColumnNames(const OSQLParseNode* pSelectNode,sal_Bool _bOrder);
        void                traverseParameters(const OSQLParseNode* pSelectNode);

		const OSQLParseNode*    getTableNode( OSQLTables& _rTables, const OSQLParseNode* pTableRef, ::rtl::OUString& aTableRange );
		void                    getQualified_join( OSQLTables& _rTables, const OSQLParseNode *pTableRef, ::rtl::OUString& aTableRange );
		void				    getSelect_statement(OSQLTables& _rTables,const OSQLParseNode* pSelect);
		::rtl::OUString		    getUniqueColumnName(const ::rtl::OUString & rColumnName)	const;

        /** finds the column with a given name, belonging to a given table, in a given tables collection
            @param  _rTables
                the tables collection to look in
            @param  rColumnName
                the column name to look for
            @param  rTableRange
                the table alias name
            @return
                the desired column object, or <NULL/> if no such column could be found
        */
        static ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet > findColumn(
            const OSQLTables& _rTables, const ::rtl::OUString & rColumnName, const ::rtl::OUString & rTableRange );

        /** finds a column with a given name, belonging to a given table
            @param  rColumnName
                the column name to look for
            @param  rTableRange
                    the table alias name
            @param  _bLookInSubTables
                <TRUE/> if and only if not only our direct tables, but also our sub tables (from sub selects)
                should be searched
            @return
        */
        ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet > findColumn(
            const ::rtl::OUString & rColumnName, const ::rtl::OUString & rTableRange, bool _bLookInSubTables );

	  protected:
		void setSelectColumnName(::vos::ORef<OSQLColumns>& _rColumns,const ::rtl::OUString & rColumnName,const ::rtl::OUString & rColumnAlias, const ::rtl::OUString & rTableRange,sal_Bool bFkt=sal_False,sal_Int32 _nType = com::sun::star::sdbc::DataType::VARCHAR,sal_Bool bAggFkt=sal_False);
		void appendColumns(::vos::ORef<OSQLColumns>& _rColumns,const ::rtl::OUString& _rTableAlias,const OSQLTable& _rTable);
		// Weitere Member-Variable, die in den "set"-Funktionen zur
		// Verfuegung stehen sollen, koennen in der abgeleiteten Klasse
		// definiert werden und z. B. in deren Konstruktor initialisiert
		// bzw. nach Benutzung der "traverse"-Routinen mit Hilfe weiterer
		// Funktionen abgefragt werden.


      private:
        OSQLParseTreeIterator();                                        // never implemented
		OSQLParseTreeIterator(const OSQLParseTreeIterator & rIter);     // never implemented

	  public:
		OSQLParseTreeIterator(
            const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XConnection >& _rxConnection,
            const ::com::sun::star::uno::Reference< ::com::sun::star::container::XNameAccess >& _rxTables,
            const OSQLParser& _rParser,
            const OSQLParseNode* pRoot = NULL );
		~OSQLParseTreeIterator();

		inline static void * SAL_CALL operator new( size_t nSize ) SAL_THROW( () )
			{ return ::rtl_allocateMemory( nSize ); }
		inline static void * SAL_CALL operator new( size_t,void* _pHint ) SAL_THROW( () )
			{ return _pHint; }
		inline static void SAL_CALL operator delete( void * pMem ) SAL_THROW( () )
			{ ::rtl_freeMemory( pMem ); }
		inline static void SAL_CALL operator delete( void *,void* ) SAL_THROW( () )
			{  }

		void dispose();
        bool isCaseSensitive() const;
		// Der zu analysierende/zu traversierende Parse Tree:
		// bei "Ubergabe von NULL wird der aktuelle Parsetree gel"oscht und der Fehlerstatus gecleared
		void setParseTree(const OSQLParseNode * pNewParseTree);
//		void setParser(const OSQLParser* _pParser) { m_pParser = _pParser; }
		const OSQLParseNode * getParseTree() const { return m_pParseTree; };

		// Teilbaueme bei einem select statement
		const OSQLParseNode* getWhereTree() const;
		const OSQLParseNode* getOrderTree() const;
		const OSQLParseNode* getGroupByTree() const;
		const OSQLParseNode* getHavingTree() const;

		const OSQLParseNode* getSimpleWhereTree() const;
		const OSQLParseNode* getSimpleOrderTree() const;
		const OSQLParseNode* getSimpleGroupByTree() const;
		const OSQLParseNode* getSimpleHavingTree() const;

        /** returns the errors which occured during parsing.

            The returned object contains a chain (via SQLException::NextException) of SQLExceptions.
        */
		inline const ::com::sun::star::sdbc::SQLException&   getErrors() const { return m_aErrors; }
        inline bool hasErrors() const { return m_aErrors.Message.getLength() > 0; }

		// Statement-Typ (wird bereits in setParseTree gesetzt):
		OSQLStatementType getStatementType() const { return m_eStatementType; }

        /** traverses the complete statement tree, and fills all our data with
            the information obatined during traversal.

            Implemented by calling the single traverse* methods in the proper
            order (depending on the statement type).
        */
        void traverseAll();

        enum TraversalParts
        {
            Parameters      = 0x0001,
            TableNames      = 0x0002,
            SelectColumns   = 0x0006,   // note that this includes TableNames. No SelectColumns without TableNames

            // Those are not implemented currently
            // GroupColumns    = 0x0008,
            // OrderColumns    = 0x0010,
            // SelectColumns   = 0x0020,
            // CreateColumns   = 0x0040,

            All             = 0xFFFF
        };
        /** traverses selected parts of the statement tree, and fills our data with
            the information obtained during traversal

            @param _nIncludeMask
                set of TraversalParts bits, specifying which information is to be collected.
                Note TraversalParts is currently not 
        */
        void traverseSome( sal_uInt32 _nIncludeMask );

		// Die TableRangeMap enth"alt alle Tabellen unter dem zugeh"origen Rangenamen der zuerst gefunden wird
		const OSQLTables& getTables() const;

		::vos::ORef<OSQLColumns> getSelectColumns() const { return m_aSelectColumns;}
		::vos::ORef<OSQLColumns> getGroupColumns() const { return m_aGroupColumns;}
		::vos::ORef<OSQLColumns> getOrderColumns() const { return m_aOrderColumns;}
		::vos::ORef<OSQLColumns> getParameters()	const { return m_aParameters; }
		::vos::ORef<OSQLColumns> getCreateColumns() const { return m_aCreateColumns;}

		/** return the columname and the table range
			@param	_pColumnRef
				The column ref parse node.
			@param	_rColumnName
				The column name to be set.
			@param	_rTableRange
				The table range to be set.
		*/
		void getColumnRange(	const OSQLParseNode* _pColumnRef,
								::rtl::OUString &_rColumnName,
								::rtl::OUString& _rTableRange) const;

		/** retrieves a column's name, table range, and alias

            @param	_pColumnRef
				The column_ref parse node.
			@param	_out_rColumnName
				The column name to be set.
			@param	_out_rTableRange
				The table range to be set.
            @param _out_rColumnAliasIfPresent
                If the column specified by _pColumnRef is part of the select columns, and contains a column alias there,
                this alias is returned here.
		*/
        void getColumnRange(    const OSQLParseNode* _pColumnRef,
								::rtl::OUString& _out_rColumnName,
								::rtl::OUString& _out_rTableRange,
                                ::rtl::OUString& _out_rColumnAliasIfPresent
                                ) const;
		
		/** return the alias name of a column
			@param	_pDerivedColumn
				The parse node where SQL_ISRULE(_pDerivedColumn,derived_column) must be true
			@return
				The alias name of the column or an empty string.
		*/
		static ::rtl::OUString getColumnAlias(const OSQLParseNode* _pDerivedColumn);

		/** return the columname and the table range
			@param	_pColumnRef
				The column ref parse node.
			@param	_xMetaData
				The database meta data.
			@param	_rColumnName
				The column name to be set.
			@param	_rTableRange
				The table range to be set.
		*/
		static void getColumnRange(	const OSQLParseNode* _pColumnRef,
									const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XConnection >& _rxConnection,
									::rtl::OUString &_rColumnName,
									::rtl::OUString& _rTableRange);

		// Ermittelt fuer eine Funktion, Spalten den zugehoeren TableRange,
		// wenn nicht eindeutig, dann leer
		sal_Bool getColumnTableRange(const OSQLParseNode* pNode, ::rtl::OUString &rTableRange) const;

		// return true when the tableNode is a rule like catalog_name, schema_name or table_name
		sal_Bool isTableNode(const OSQLParseNode* _pTableNode) const;
    private:
        /** traverses the list of table names, and filles _rTables
        */
		bool traverseTableNames( OSQLTables& _rTables );

        /// traverses columns in a SELECT statement
		bool traverseSelectColumnNames(const OSQLParseNode* pSelectNode);
        /// traverses columns in a CREATE TABLE statement
		void traverseCreateColumns(const OSQLParseNode* pSelectNode);

		bool traverseOrderByColumnNames(const OSQLParseNode* pSelectNode);
		bool traverseGroupByColumnNames(const OSQLParseNode* pSelectNode);

		bool traverseSelectionCriteria(const OSQLParseNode* pSelectNode);

    private:
        /** constructs a new iterator, which inherits some of the settings from a parent iterator
        */
		OSQLParseTreeIterator(
            const OSQLParseTreeIterator& _rParentIterator,
            const OSQLParser& _rParser,
            const OSQLParseNode* pRoot );

        /** creates a table object and inserts it into our tables collection

            only used when we're iterating through a CREATE TABLE statement
        */
		OSQLTable   impl_createTableObject(
            const ::rtl::OUString& rTableName, const ::rtl::OUString& rCatalogName, const ::rtl::OUString& rSchemaName );

        /** locates a record source (a table or query) with the given name
        */
        OSQLTable   impl_locateRecordSource(
            const ::rtl::OUString& _rComposedName
        );

        /** implementation for both traverseAll and traverseSome
        */
        void    impl_traverse( sal_uInt32 _nIncludeMask );

        /** retrieves the parameter columns of the given query
        */
        void    impl_getQueryParameterColumns( const OSQLTable& _rQuery );

        void setOrderByColumnName(const ::rtl::OUString & rColumnName, const ::rtl::OUString & rTableRange, sal_Bool bAscending);
		void setGroupByColumnName(const ::rtl::OUString & rColumnName, const ::rtl::OUString & rTableRange);

    private:
        /** appends an SQLException corresponding to the given error code to our error collection

            @param  _eError
                the code of the error which occured
            @param  _pReplaceToken1
                if not <NULL/>, the first occurance of '#' in the error message will be replaced
                with the given token
            @param  _pReplaceToken2
                if not <NULL/>, and if _rReplaceToken1 is not <NULL/>, the second occurance of '#'
                in the error message will be replaced with _rReplaceToken2
        */
        void impl_appendError( IParseContext::ErrorCode _eError,
            const ::rtl::OUString* _pReplaceToken1 = NULL, const ::rtl::OUString* _pReplaceToken2 = NULL );

        /** appends an SQLException corresponding to the given error code to our error collection
        */
        void impl_appendError( const ::com::sun::star::sdbc::SQLException& _rError );

        /** resets our errors
        */
        inline void impl_resetErrors()
        {
            m_aErrors = ::com::sun::star::sdbc::SQLException();
        }
	};
}

#endif // _CONNECTIVITY_PARSE_SQLITERATOR_HXX_

