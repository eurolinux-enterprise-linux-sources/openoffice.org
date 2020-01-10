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
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include "xmldrani.hxx"
#include "xmlimprt.hxx"
#include "xmlfilti.hxx"
#include "xmlsorti.hxx"
#include "document.hxx"
#include "globstr.hrc"
#include "docuno.hxx"
#include "dbcolect.hxx"
#include "datauno.hxx"
#include "attrib.hxx"
#include "unonames.hxx"
#include "convuno.hxx"
#include "XMLConverter.hxx"
#include "rangeutl.hxx"

#include <xmloff/xmltkmap.hxx>
#include <xmloff/nmspmap.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlnmspe.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/xmlerror.hxx>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XDatabaseRanges.hpp>
#include <com/sun/star/sheet/XDatabaseRange.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <comphelper/extract.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/xml/sax/XLocator.hpp>

#define SC_ENABLEUSERSORTLIST	"EnableUserSortList"
#define SC_USERSORTLISTINDEX	"UserSortListIndex"
#define SC_USERLIST				"UserList"

using namespace com::sun::star;
using namespace xmloff::token;

//------------------------------------------------------------------

ScXMLDatabaseRangesContext::ScXMLDatabaseRangesContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
                                      ::com::sun::star::xml::sax::XAttributeList>& /* xAttrList */ ) :
	SvXMLImportContext( rImport, nPrfx, rLName )
{
	// has no attributes
	rImport.LockSolarMutex();
}

ScXMLDatabaseRangesContext::~ScXMLDatabaseRangesContext()
{
	GetScImport().UnlockSolarMutex();
}

SvXMLImportContext *ScXMLDatabaseRangesContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	const SvXMLTokenMap& rTokenMap = GetScImport().GetDatabaseRangesElemTokenMap();
	switch( rTokenMap.Get( nPrefix, rLName ) )
	{
		case XML_TOK_DATABASE_RANGE :
		{
			pContext = new ScXMLDatabaseRangeContext( GetScImport(), nPrefix,
													  	rLName, xAttrList);
		}
		break;
	}

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLDatabaseRangesContext::EndElement()
{
}

ScXMLDatabaseRangeContext::ScXMLDatabaseRangeContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
    sDatabaseRangeName(ScGlobal::GetRscString(STR_DB_NONAME)),
	aSortSequence(),
	eOrientation(table::TableOrientation_ROWS),
	nRefresh(0),
	nSubTotalsUserListIndex(0),
	bContainsSort(sal_False),
	bContainsSubTotal(sal_False),
	bNative(sal_True),
	bIsSelection(sal_False),
	bKeepFormats(sal_False),
	bMoveCells(sal_False),
	bStripData(sal_False),
	bContainsHeader(sal_True),
	bAutoFilter(sal_False),
	bSubTotalsBindFormatsToContent(sal_False),
	bSubTotalsIsCaseSensitive(sal_False),
	bSubTotalsInsertPageBreaks(sal_False),
	bSubTotalsSortGroups(sal_False),
	bSubTotalsEnabledUserList(sal_False),
	bSubTotalsAscending(sal_True),
	bFilterCopyOutputData(sal_False),
	bFilterIsCaseSensitive(sal_False),
	bFilterSkipDuplicates(sal_False),
	bFilterUseRegularExpressions(sal_False),
	bFilterConditionSourceRange(sal_False)
{
	nSourceType = sheet::DataImportMode_NONE;
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetDatabaseRangeAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_DATABASE_RANGE_ATTR_NAME :
			{
				sDatabaseRangeName = sValue;
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_IS_SELECTION :
			{
				bIsSelection = IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_ON_UPDATE_KEEP_STYLES :
			{
				bKeepFormats = IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_ON_UPDATE_KEEP_SIZE :
			{
				bMoveCells = !IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_HAS_PERSISTENT_DATA :
			{
				bStripData = !IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_ORIENTATION :
			{
				if (IsXMLToken(sValue, XML_COLUMN))
					eOrientation = table::TableOrientation_COLUMNS;
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_CONTAINS_HEADER :
			{
				bContainsHeader = IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_DISPLAY_FILTER_BUTTONS :
			{
				bAutoFilter = IsXMLToken(sValue, XML_TRUE);
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_TARGET_RANGE_ADDRESS :
			{
				sRangeAddress = sValue;
			}
			break;
			case XML_TOK_DATABASE_RANGE_ATTR_REFRESH_DELAY :
			{
				double fTime;
				if( SvXMLUnitConverter::convertTime( fTime, sValue ) )
					nRefresh = Max( (sal_Int32)(fTime * 86400.0), (sal_Int32)0 );
			}
			break;
		}
	}
}

ScXMLDatabaseRangeContext::~ScXMLDatabaseRangeContext()
{
}

SvXMLImportContext *ScXMLDatabaseRangeContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	const SvXMLTokenMap& rTokenMap = GetScImport().GetDatabaseRangeElemTokenMap();
	switch( rTokenMap.Get( nPrefix, rLName ) )
	{
		case XML_TOK_DATABASE_RANGE_SOURCE_SQL :
		{
			pContext = new ScXMLSourceSQLContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
		case XML_TOK_DATABASE_RANGE_SOURCE_TABLE :
		{
			pContext = new ScXMLSourceTableContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
		case XML_TOK_DATABASE_RANGE_SOURCE_QUERY :
		{
			pContext = new ScXMLSourceQueryContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
		case XML_TOK_FILTER :
		{
			pContext = new ScXMLFilterContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
		case XML_TOK_SORT :
		{
			bContainsSort = sal_True;
			pContext = new ScXMLSortContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
		case XML_TOK_DATABASE_RANGE_SUBTOTAL_RULES :
		{
			bContainsSubTotal = sal_True;
			pContext = new ScXMLSubTotalRulesContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
	}

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLDatabaseRangeContext::EndElement()
{
	if (GetScImport().GetModel().is())
	{
		uno::Reference <beans::XPropertySet> xPropertySet( GetScImport().GetModel(), uno::UNO_QUERY );
		ScDocument* pDoc = GetScImport().GetDocument();
		if (pDoc && xPropertySet.is())
		{
            uno::Reference <sheet::XDatabaseRanges> xDatabaseRanges(xPropertySet->getPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNO_DATABASERNG))), uno::UNO_QUERY);
			if (xDatabaseRanges.is())
			{
				table::CellRangeAddress aCellRangeAddress;
				sal_Int32 nOffset(0);
				if (ScRangeStringConverter::GetRangeFromString( aCellRangeAddress, sRangeAddress, pDoc, ::formula::FormulaGrammar::CONV_OOO, nOffset ))
				{
					sal_Bool bInsert(sal_True);
					try
					{
						xDatabaseRanges->addNewByName(sDatabaseRangeName, aCellRangeAddress);
					}
					catch ( uno::RuntimeException& rRuntimeException )
					{
						bInsert = sal_False;
						rtl::OUString sErrorMessage(RTL_CONSTASCII_USTRINGPARAM("DatabaseRange "));
						sErrorMessage += sDatabaseRangeName;
						sErrorMessage += rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(" could not be created with the range "));
						sErrorMessage += sRangeAddress;
                        uno::Sequence<rtl::OUString> aSeq(1);
                        aSeq[0] = sErrorMessage;
						uno::Reference<xml::sax::XLocator> xLocator;
						GetScImport().SetError(XMLERROR_API | XMLERROR_FLAG_ERROR, aSeq, rRuntimeException.Message, xLocator);
					}
					if (bInsert)
					{
                        uno::Reference <sheet::XDatabaseRange> xDatabaseRange(xDatabaseRanges->getByName(sDatabaseRangeName), uno::UNO_QUERY);
						if (xDatabaseRange.is())
						{
							uno::Reference <beans::XPropertySet> xDatabaseRangePropertySet (xDatabaseRange, uno::UNO_QUERY);
							if (xDatabaseRangePropertySet.is())
							{
                                xDatabaseRangePropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_KEEPFORM)), uno::makeAny(bKeepFormats));
								xDatabaseRangePropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_MOVCELLS)), uno::makeAny(bMoveCells));
								xDatabaseRangePropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_STRIPDAT)), uno::makeAny(bStripData));
							}
							uno::Sequence <beans::PropertyValue> aImportDescriptor(xDatabaseRange->getImportDescriptor());
							sal_Int32 nImportProperties = aImportDescriptor.getLength();
							for (sal_Int16 i = 0; i < nImportProperties; ++i)
							{
								if (aImportDescriptor[i].Name == rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_DBNAME)))
								{
                                    if (sDatabaseName.getLength())
                                    {
									    aImportDescriptor[i].Value <<= sDatabaseName;
                                    }
                                    else
                                    {
                                        aImportDescriptor[i].Name = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_CONRES));
                                        aImportDescriptor[i].Value <<= sConnectionRessource;
                                    }
								}
								else if (aImportDescriptor[i].Name == rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_SRCOBJ)))
									aImportDescriptor[i].Value <<= sSourceObject;
								else if (aImportDescriptor[i].Name == rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_SRCTYPE)))
									aImportDescriptor[i].Value <<= nSourceType;
                                else if (aImportDescriptor[i].Name == rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_ISNATIVE)))
									aImportDescriptor[i].Value <<= bNative;
							}
							ScDBCollection* pDBCollection = pDoc->GetDBCollection();
							sal_uInt16 nIndex;
							pDBCollection->SearchName(sDatabaseRangeName, nIndex);
							ScDBData* pDBData = (*pDBCollection)[nIndex];
							pDBData->SetImportSelection(bIsSelection);
							pDBData->SetAutoFilter(bAutoFilter);
							if (bAutoFilter)
								pDoc->ApplyFlagsTab( static_cast<SCCOL>(aCellRangeAddress.StartColumn), static_cast<SCROW>(aCellRangeAddress.StartRow),
														static_cast<SCCOL>(aCellRangeAddress.EndColumn), static_cast<SCROW>(aCellRangeAddress.StartRow),
														aCellRangeAddress.Sheet, SC_MF_AUTO );
							ScImportParam aImportParam;
							ScImportDescriptor::FillImportParam(aImportParam, aImportDescriptor);
							pDBData->SetImportParam(aImportParam);
							if (bContainsSort)
							{
								sal_uInt32 nOldSize(aSortSequence.getLength());
								aSortSequence.realloc(nOldSize + 1);
								beans::PropertyValue aProperty;
								aProperty.Name = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_ORIENT));
								aProperty.Value <<= eOrientation;
								aSortSequence[nOldSize] = aProperty;
								ScSortParam aSortParam;
								ScSortDescriptor::FillSortParam(aSortParam, aSortSequence);

								//#98317#; until now the Fields are relative to the left top edge of the range, but the
								// core wants to have the absolute position (column/row)
								SCCOLROW nFieldStart = aSortParam.bByRow ? static_cast<SCCOLROW>(aCellRangeAddress.StartColumn) : static_cast<SCCOLROW>(aCellRangeAddress.StartRow);
								for (sal_uInt16 i = 0; i < MAXSORT; ++i)
								{
									if (aSortParam.bDoSort[i])
										aSortParam.nField[i] += nFieldStart;
								}

								pDBData->SetSortParam(aSortParam);
							}
                            uno::Reference< sheet::XSheetFilterDescriptor2 > xSheetFilterDescriptor(
                                    xDatabaseRange->getFilterDescriptor(), uno::UNO_QUERY );
							if (xSheetFilterDescriptor.is())
							{
								uno::Reference <beans::XPropertySet> xFilterPropertySet (xSheetFilterDescriptor, uno::UNO_QUERY);
								if (xFilterPropertySet.is())
								{
									sal_Bool bOrientation(table::TableOrientation_COLUMNS == eOrientation);
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_ORIENT)), uno::makeAny(bOrientation));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_CONTHDR)), uno::makeAny(bContainsHeader));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_COPYOUT)), uno::makeAny(bFilterCopyOutputData));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_ISCASE)), uno::makeAny(bFilterIsCaseSensitive));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_SKIPDUP)), uno::makeAny(bFilterSkipDuplicates));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_USEREGEX)), uno::makeAny(bFilterUseRegularExpressions));
									xFilterPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_OUTPOS)), uno::makeAny(aFilterOutputPosition));
								}
                                xSheetFilterDescriptor->setFilterFields2(aFilterFields);
								if (bFilterConditionSourceRange)
								{
									ScRange aAdvSource;
									ScUnoConversion::FillScRange( aAdvSource, aFilterConditionSourceRangeAddress );
									pDBData->SetAdvancedQuerySource(&aAdvSource);
								}
							}
							if (bContainsSubTotal)
							{
								uno::Reference <sheet::XSubTotalDescriptor> xSubTotalDescriptor(xDatabaseRange->getSubTotalDescriptor());
								if (xSubTotalDescriptor.is())
								{
									uno::Reference <beans::XPropertySet> xSubTotalPropertySet (xSubTotalDescriptor, uno::UNO_QUERY);
									if( xSubTotalPropertySet.is())
									{
                                        xSubTotalPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_BINDFMT)), uno::makeAny(bSubTotalsBindFormatsToContent));
										xSubTotalPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_ENABLEUSERSORTLIST)), uno::makeAny(bSubTotalsEnabledUserList));
										xSubTotalPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_USERSORTLISTINDEX)), uno::makeAny(nSubTotalsUserListIndex));
										xSubTotalPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_INSBRK)), uno::makeAny(bSubTotalsInsertPageBreaks));
										xSubTotalPropertySet->setPropertyValue(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SC_UNONAME_ISCASE)), uno::makeAny(bSubTotalsIsCaseSensitive));
									}
									ScSubTotalParam aSubTotalParam;
									aSubTotalParam.bDoSort = bSubTotalsSortGroups;
									aSubTotalParam.bAscending = bSubTotalsAscending;
									aSubTotalParam.bUserDef = bSubTotalsEnabledUserList;
									aSubTotalParam.nUserIndex = nSubTotalsUserListIndex;
									pDBData->SetSubTotalParam(aSubTotalParam);
                                    std::vector < ScSubTotalRule >::iterator aItr(aSubTotalRules.begin());
                                    while (!aSubTotalRules.empty())
                                    {
									    xSubTotalDescriptor->addNew(aItr->aSubTotalColumns, aItr->nSubTotalRuleGroupFieldNumber);
                                        aItr = aSubTotalRules.erase(aItr);
                                    }
								}
							}
							if ( pDBData->HasImportParam() && !pDBData->HasImportSelection() )
							{
								pDBData->SetRefreshDelay( nRefresh );
								pDBData->SetRefreshHandler( pDBCollection->GetRefreshHandler() );
								pDBData->SetRefreshControl( pDoc->GetRefreshTimerControlAddress() );
							}
						}
					}
				}
			}
		}
	}
}

ScXMLSourceSQLContext::ScXMLSourceSQLContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetDatabaseRangeSourceSQLAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SOURCE_SQL_ATTR_DATABASE_NAME :
			{
                sDBName = sValue;
			}
			break;
			case XML_TOK_SOURCE_SQL_ATTR_SQL_STATEMENT :
			{
				pDatabaseRangeContext->SetSourceObject(sValue);
			}
			break;
			case XML_TOK_SOURCE_SQL_ATTR_PARSE_SQL_STATEMENT :
			{
				pDatabaseRangeContext->SetNative(IsXMLToken(sValue, XML_TRUE));
			}
			break;
		}
	}
	pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_SQL);
}

ScXMLSourceSQLContext::~ScXMLSourceSQLContext()
{
}

SvXMLImportContext *ScXMLSourceSQLContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	if ( nPrefix == XML_NAMESPACE_FORM )
	{
        if (IsXMLToken(rLName, XML_CONNECTION_RESOURCE) && (sDBName.getLength() == 0))
        {
			pContext = new ScXMLConResContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, pDatabaseRangeContext);
        }
    }

    if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSourceSQLContext::EndElement()
{
    if (sDBName.getLength())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLSourceTableContext::ScXMLSourceTableContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetDatabaseRangeSourceTableAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SOURCE_TABLE_ATTR_DATABASE_NAME :
			{
                sDBName = sValue;
			}
			break;
			case XML_TOK_SOURCE_TABLE_ATTR_TABLE_NAME :
			{
				pDatabaseRangeContext->SetSourceObject(sValue);
			}
			break;
		}
	}
	pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_TABLE);
}

ScXMLSourceTableContext::~ScXMLSourceTableContext()
{
}

SvXMLImportContext *ScXMLSourceTableContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	if ( nPrefix == XML_NAMESPACE_FORM )
	{
        if (IsXMLToken(rLName, XML_CONNECTION_RESOURCE) && (sDBName.getLength() == 0))
        {
			pContext = new ScXMLConResContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, pDatabaseRangeContext);
        }
    }

    if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSourceTableContext::EndElement()
{
    if (sDBName.getLength())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLSourceQueryContext::ScXMLSourceQueryContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetDatabaseRangeSourceQueryAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SOURCE_QUERY_ATTR_DATABASE_NAME :
			{
                sDBName = sValue;
			}
			break;
			case XML_TOK_SOURCE_QUERY_ATTR_QUERY_NAME :
			{
				pDatabaseRangeContext->SetSourceObject(sValue);
			}
			break;
		}
	}
	pDatabaseRangeContext->SetSourceType(sheet::DataImportMode_QUERY);
}

ScXMLSourceQueryContext::~ScXMLSourceQueryContext()
{
}

SvXMLImportContext *ScXMLSourceQueryContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	if ( nPrefix == XML_NAMESPACE_FORM )
	{
        if (IsXMLToken(rLName, XML_CONNECTION_RESOURCE) && (sDBName.getLength() == 0))
        {
			pContext = new ScXMLConResContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, pDatabaseRangeContext);
        }
    }

    if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSourceQueryContext::EndElement()
{
    if (sDBName.getLength())
        pDatabaseRangeContext->SetDatabaseName(sDBName);
}

ScXMLConResContext::ScXMLConResContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext( pTempDatabaseRangeContext )
{
    rtl::OUString sConRes;
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	for( sal_Int16 i=0; i < nAttrCount; i++ )
	{
		rtl::OUString sAttrName = xAttrList->getNameByIndex( i );
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		rtl::OUString sValue = xAttrList->getValueByIndex( i );

        if (nPrefix == XML_NAMESPACE_XLINK)
        {
            if (IsXMLToken(aLocalName, XML_HREF))
                sConRes = sValue;
        }
	}
    if (sConRes.getLength())
        pDatabaseRangeContext->SetConnectionRessource(sConRes);
}

ScXMLConResContext::~ScXMLConResContext()
{
}

SvXMLImportContext *ScXMLConResContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
                                        ::com::sun::star::xml::sax::XAttributeList>& /* xAttrList */ )
{
	SvXMLImportContext *pContext = 0;

    if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLConResContext::EndElement()
{
}

ScXMLSubTotalRulesContext::ScXMLSubTotalRulesContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetDatabaseRangeSubTotalRulesAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SUBTOTAL_RULES_ATTR_BIND_STYLES_TO_CONTENT :
			{
				pDatabaseRangeContext->SetSubTotalsBindFormatsToContent(IsXMLToken(sValue, XML_TRUE));
			}
			break;
			case XML_TOK_SUBTOTAL_RULES_ATTR_CASE_SENSITIVE :
			{
				pDatabaseRangeContext->SetSubTotalsIsCaseSensitive(IsXMLToken(sValue, XML_TRUE));
			}
			break;
			case XML_TOK_SUBTOTAL_RULES_ATTR_PAGE_BREAKS_ON_GROUP_CHANGE :
			{
				pDatabaseRangeContext->SetSubTotalsInsertPageBreaks(IsXMLToken(sValue, XML_TRUE));
			}
			break;
		}
	}
}

ScXMLSubTotalRulesContext::~ScXMLSubTotalRulesContext()
{
}

SvXMLImportContext *ScXMLSubTotalRulesContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	const SvXMLTokenMap& rTokenMap = GetScImport().GetDatabaseRangeSubTotalRulesElemTokenMap();
	switch( rTokenMap.Get( nPrefix, rLName ) )
	{
		case XML_TOK_SUBTOTAL_RULES_SORT_GROUPS :
		{
			pContext = new ScXMLSortGroupsContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, pDatabaseRangeContext);
		}
		break;
		case XML_TOK_SUBTOTAL_RULES_SUBTOTAL_RULE :
		{
			pContext = new ScXMLSubTotalRuleContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, pDatabaseRangeContext);
		}
		break;
	}

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSubTotalRulesContext::EndElement()
{
}

ScXMLSortGroupsContext::ScXMLSortGroupsContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	pDatabaseRangeContext->SetSubTotalsSortGroups(sal_True);
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetSubTotalRulesSortGroupsAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SORT_GROUPS_ATTR_DATA_TYPE :
			{
				if (sValue.getLength() > 8)
				{
					rtl::OUString sTemp = sValue.copy(0, 8);
					if (sTemp.compareToAscii(SC_USERLIST) == 0)
					{
						pDatabaseRangeContext->SetSubTotalsEnabledUserList(sal_True);
						sTemp = sValue.copy(8);
						pDatabaseRangeContext->SetSubTotalsUserListIndex(static_cast<sal_Int16>(sTemp.toInt32()));
					}
					else
					{
						//if (IsXMLToken(sValue, XML_AUTOMATIC))
							//aSortField.FieldType = util::SortFieldType_AUTOMATIC;
							// is not supported by StarOffice
					}
				}
				else
				{
					//if (IsXMLToken(sValue, XML_TEXT))
						//aSortField.FieldType = util::SortFieldType_ALPHANUMERIC;
						// is not supported by StarOffice
					//else if (IsXMLToken(sValue, XML_NUMBER))
						//aSortField.FieldType = util::SortFieldType_NUMERIC;
						// is not supported by StarOffice
				}
			}
			break;
			case XML_TOK_SORT_GROUPS_ATTR_ORDER :
			{
				if (IsXMLToken(sValue, XML_ASCENDING))
					pDatabaseRangeContext->SetSubTotalsAscending(sal_True);
				else
					pDatabaseRangeContext->SetSubTotalsAscending(sal_False);
			}
			break;
		}
	}
}

ScXMLSortGroupsContext::~ScXMLSortGroupsContext()
{
}

SvXMLImportContext *ScXMLSortGroupsContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
                                        ::com::sun::star::xml::sax::XAttributeList>& /* xAttrList */ )
{
	SvXMLImportContext *pContext = 0;

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSortGroupsContext::EndElement()
{
}

ScXMLSubTotalRuleContext::ScXMLSubTotalRuleContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLDatabaseRangeContext* pTempDatabaseRangeContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
    pDatabaseRangeContext(pTempDatabaseRangeContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetSubTotalRulesSubTotalRuleAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SUBTOTAL_RULE_ATTR_GROUP_BY_FIELD_NUMBER :
			{
				aSubTotalRule.nSubTotalRuleGroupFieldNumber = static_cast<sal_Int16>(sValue.toInt32());
			}
			break;
		}
	}
}

ScXMLSubTotalRuleContext::~ScXMLSubTotalRuleContext()
{
}

SvXMLImportContext *ScXMLSubTotalRuleContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
									  	::com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	SvXMLImportContext *pContext = 0;

	const SvXMLTokenMap& rTokenMap = GetScImport().GetSubTotalRulesSubTotalRuleElemTokenMap();
	switch( rTokenMap.Get( nPrefix, rLName ) )
	{
		case XML_TOK_SUBTOTAL_RULE_SUBTOTAL_FIELD :
		{
			pContext = new ScXMLSubTotalFieldContext( GetScImport(), nPrefix,
													  	rLName, xAttrList, this);
		}
		break;
	}

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSubTotalRuleContext::EndElement()
{
    if (pDatabaseRangeContext)
        pDatabaseRangeContext->AddSubTotalRule(aSubTotalRule);
}

ScXMLSubTotalFieldContext::ScXMLSubTotalFieldContext( ScXMLImport& rImport,
									  USHORT nPrfx,
									  const ::rtl::OUString& rLName,
									  const ::com::sun::star::uno::Reference<
									  ::com::sun::star::xml::sax::XAttributeList>& xAttrList,
										ScXMLSubTotalRuleContext* pTempSubTotalRuleContext) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
    pSubTotalRuleContext(pTempSubTotalRuleContext)
{
	sal_Int16 nAttrCount = xAttrList.is() ? xAttrList->getLength() : 0;
	const SvXMLTokenMap& rAttrTokenMap = GetScImport().GetSubTotalRuleSubTotalFieldAttrTokenMap();
	for( sal_Int16 i=0; i < nAttrCount; ++i )
	{
		const rtl::OUString& sAttrName(xAttrList->getNameByIndex( i ));
		rtl::OUString aLocalName;
		USHORT nPrefix = GetScImport().GetNamespaceMap().GetKeyByAttrName(
											sAttrName, &aLocalName );
		const rtl::OUString& sValue(xAttrList->getValueByIndex( i ));

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_SUBTOTAL_FIELD_ATTR_FIELD_NUMBER :
			{
				sFieldNumber = sValue;
			}
			break;
			case XML_TOK_SUBTOTAL_FIELD_ATTR_FUNCTION :
			{
				sFunction = sValue;
			}
			break;
		}
	}
}

ScXMLSubTotalFieldContext::~ScXMLSubTotalFieldContext()
{
}

SvXMLImportContext *ScXMLSubTotalFieldContext::CreateChildContext( USHORT nPrefix,
											const ::rtl::OUString& rLName,
											const ::com::sun::star::uno::Reference<
                                        ::com::sun::star::xml::sax::XAttributeList>& /* xAttrList */ )
{
	SvXMLImportContext *pContext = 0;

	if( !pContext )
		pContext = new SvXMLImportContext( GetImport(), nPrefix, rLName );

	return pContext;
}

void ScXMLSubTotalFieldContext::EndElement()
{
	sheet::SubTotalColumn aSubTotalColumn;
	aSubTotalColumn.Column = sFieldNumber.toInt32();
	aSubTotalColumn.Function = ScXMLConverter::GetFunctionFromString( sFunction );
	pSubTotalRuleContext->AddSubTotalColumn(aSubTotalColumn);
}

