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
#ifndef SC_XMLDRANI_HXX
#define SC_XMLDRANI_HXX

#include <xmloff/xmlictxt.hxx>
#include <xmloff/xmlimp.hxx>
#include <com/sun/star/sheet/DataImportMode.hpp>
#include <com/sun/star/sheet/SubTotalColumn.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/sheet/TableFilterField2.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/table/TableOrientation.hpp>

class ScXMLImport;

class ScXMLDatabaseRangesContext : public SvXMLImportContext
{
	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLDatabaseRangesContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList);

	virtual ~ScXMLDatabaseRangesContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

struct ScSubTotalRule
{
    sal_Int16 nSubTotalRuleGroupFieldNumber;
    com::sun::star::uno::Sequence <com::sun::star::sheet::SubTotalColumn> aSubTotalColumns;
};

class ScXMLDatabaseRangeContext : public SvXMLImportContext
{
	rtl::OUString 	sDatabaseRangeName;
    rtl::OUString   sConnectionRessource;
	rtl::OUString	sRangeAddress;
	rtl::OUString	sDatabaseName;
	rtl::OUString	sSourceObject;
	com::sun::star::uno::Sequence <com::sun::star::beans::PropertyValue> aSortSequence;
    com::sun::star::uno::Sequence <com::sun::star::sheet::TableFilterField2> aFilterFields;
    std::vector < ScSubTotalRule > aSubTotalRules;
	com::sun::star::table::CellAddress aFilterOutputPosition;
	com::sun::star::table::CellRangeAddress aFilterConditionSourceRangeAddress;
	com::sun::star::sheet::DataImportMode nSourceType;
	com::sun::star::table::TableOrientation eOrientation;
	sal_Int32		nRefresh;
	sal_Int16		nSubTotalsUserListIndex;
	sal_Int16		nSubTotalRuleGroupFieldNumber;
	sal_Bool		bContainsSort;
	sal_Bool		bContainsSubTotal;
	sal_Bool 		bNative;
	sal_Bool		bIsSelection;
	sal_Bool		bKeepFormats;
	sal_Bool		bMoveCells;
	sal_Bool		bStripData;
	sal_Bool		bContainsHeader;
	sal_Bool		bAutoFilter;
	sal_Bool		bSubTotalsBindFormatsToContent;
	sal_Bool		bSubTotalsIsCaseSensitive;
	sal_Bool		bSubTotalsInsertPageBreaks;
	sal_Bool		bSubTotalsSortGroups;
	sal_Bool		bSubTotalsEnabledUserList;
	sal_Bool		bSubTotalsAscending;
	sal_Bool		bFilterCopyOutputData;
	sal_Bool		bFilterIsCaseSensitive;
	sal_Bool		bFilterSkipDuplicates;
	sal_Bool		bFilterUseRegularExpressions;
	sal_Bool		bFilterConditionSourceRange;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLDatabaseRangeContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList);

	virtual ~ScXMLDatabaseRangeContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();

	void SetDatabaseName(const rtl::OUString sTempDatabaseName) { sDatabaseName = sTempDatabaseName; }
	void SetConnectionRessource(const rtl::OUString sTempConRes) { sConnectionRessource = sTempConRes; }
	void SetSourceObject(const rtl::OUString sTempSourceObject) { sSourceObject = sTempSourceObject; }
	void SetSourceType(const com::sun::star::sheet::DataImportMode nTempSourceType) { nSourceType = nTempSourceType; }
	void SetNative(const sal_Bool bTempNative) { bNative = bTempNative; }
	void SetSubTotalsBindFormatsToContent(const sal_Bool bTemp ) { bSubTotalsBindFormatsToContent = bTemp; }
	void SetSubTotalsIsCaseSensitive(const sal_Bool bTemp) { bSubTotalsIsCaseSensitive = bTemp; }
	void SetSubTotalsInsertPageBreaks(const sal_Bool bTemp) { bSubTotalsInsertPageBreaks = bTemp; }
	void SetSubTotalsEnabledUserList(const sal_Bool bTemp) { bSubTotalsEnabledUserList = bTemp; }
	void SetSubTotalsUserListIndex(const sal_Int16 nTemp) { nSubTotalsUserListIndex = nTemp; }
	void SetSubTotalsAscending(const sal_Bool bTemp) { bSubTotalsAscending = bTemp; }
	void SetSubTotalsSortGroups(const sal_Bool bTemp) { bSubTotalsSortGroups = bTemp; }
    void AddSubTotalRule(const ScSubTotalRule& rRule) { aSubTotalRules.push_back(rRule); }
	void SetSortSequence(const com::sun::star::uno::Sequence <com::sun::star::beans::PropertyValue>& aTempSortSequence) { aSortSequence = aTempSortSequence; }
	void SetFilterCopyOutputData(const sal_Bool bTemp) { bFilterCopyOutputData = bTemp; }
	void SetFilterIsCaseSensitive(const sal_Bool bTemp) { bFilterIsCaseSensitive = bTemp; }
	void SetFilterSkipDuplicates(const sal_Bool bTemp) { bFilterSkipDuplicates = bTemp; }
	void SetFilterUseRegularExpressions(const sal_Bool bTemp) { bFilterUseRegularExpressions = bTemp; }
	void SetFilterFields(const com::sun::star::uno::Sequence <com::sun::star::sheet::TableFilterField2>& aTemp) { aFilterFields = aTemp; }
	void SetFilterOutputPosition(const com::sun::star::table::CellAddress& aTemp) { aFilterOutputPosition = aTemp; }
	void SetFilterConditionSourceRangeAddress(const com::sun::star::table::CellRangeAddress& aTemp) { aFilterConditionSourceRangeAddress = aTemp;
																									bFilterConditionSourceRange = sal_True; }
};

class ScXMLSourceSQLContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext*  pDatabaseRangeContext;
    rtl::OUString               sDBName;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSourceSQLContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSourceSQLContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLSourceTableContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext*  pDatabaseRangeContext;
    rtl::OUString               sDBName;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSourceTableContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSourceTableContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLSourceQueryContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext*  pDatabaseRangeContext;
    rtl::OUString               sDBName;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSourceQueryContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSourceQueryContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLConResContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext*  pDatabaseRangeContext;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLConResContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLConResContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLSubTotalRulesContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext* pDatabaseRangeContext;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSubTotalRulesContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSubTotalRulesContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLSortGroupsContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext* pDatabaseRangeContext;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSortGroupsContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSortGroupsContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

class ScXMLSubTotalRuleContext : public SvXMLImportContext
{
	ScXMLDatabaseRangeContext*  pDatabaseRangeContext;
    ScSubTotalRule              aSubTotalRule;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSubTotalRuleContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext);

	virtual ~ScXMLSubTotalRuleContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();

	void AddSubTotalColumn(const com::sun::star::sheet::SubTotalColumn aSubTotalColumn)
	{ aSubTotalRule.aSubTotalColumns.realloc(aSubTotalRule.aSubTotalColumns.getLength() + 1);
    aSubTotalRule.aSubTotalColumns[aSubTotalRule.aSubTotalColumns.getLength() - 1] = aSubTotalColumn; }
};

class ScXMLSubTotalFieldContext : public SvXMLImportContext
{
	ScXMLSubTotalRuleContext* pSubTotalRuleContext;
	rtl::OUString sFieldNumber;
	rtl::OUString sFunction;

	const ScXMLImport& GetScImport() const { return (const ScXMLImport&)GetImport(); }
	ScXMLImport& GetScImport() { return (ScXMLImport&)GetImport(); }

public:

	ScXMLSubTotalFieldContext( ScXMLImport& rImport, USHORT nPrfx,
						const ::rtl::OUString& rLName,
						const ::com::sun::star::uno::Reference<
										::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLSubTotalRuleContext* pSubTotalRuleContext);

	virtual ~ScXMLSubTotalFieldContext();

	virtual SvXMLImportContext *CreateChildContext( USHORT nPrefix,
									 const ::rtl::OUString& rLocalName,
									 const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList );

	virtual void EndElement();
};

#endif
