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

#include "externalrefmgr.hxx"
#include "document.hxx"
#include "token.hxx"
#include "tokenarray.hxx"
#include "address.hxx"
#include "tablink.hxx"
#include "docsh.hxx"
#include "scextopt.hxx"
#include "rangenam.hxx"
#include "cell.hxx"
#include "viewdata.hxx"
#include "tabvwsh.hxx"
#include "sc.hrc"

#include "sfx2/app.hxx"
#include "sfx2/docfilt.hxx"
#include "sfx2/docfile.hxx"
#include "sfx2/fcontnr.hxx"
#include "sfx2/sfxsids.hrc"
#include "sfx2/objsh.hxx"
#include "svtools/broadcast.hxx"
#include "svtools/smplhint.hxx"
#include "svtools/itemset.hxx"
#include "svtools/stritem.hxx"
#include "svtools/urihelper.hxx"
#include "svtools/zformat.hxx"
#include "svx/linkmgr.hxx"
#include "tools/urlobj.hxx"
#include "unotools/ucbhelper.hxx"

#include <memory>
#include <algorithm>

using ::std::auto_ptr;
using ::com::sun::star::uno::Any;
using ::rtl::OUString;
using ::std::vector;
using ::std::find;
using ::std::find_if;
using ::std::distance;
using ::std::pair;
using ::std::list;
using ::std::unary_function;
using namespace formula;

#define SRCDOC_LIFE_SPAN     6000       // 1 minute (in 100th of a sec)
#define SRCDOC_SCAN_INTERVAL 1000*5     // every 5 seconds (in msec)

namespace {

class TabNameSearchPredicate : public unary_function<bool, ScExternalRefCache::TableName>
{
public:
    explicit TabNameSearchPredicate(const String& rSearchName) :
        maSearchName(ScGlobal::pCharClass->upper(rSearchName))
    {
    }

    bool operator()(const ScExternalRefCache::TableName& rTabNameSet) const
    {
        // Ok, I'm doing case insensitive search here.
        return rTabNameSet.maUpperName.Equals(maSearchName);
    }

private:
    String maSearchName;
};

class FindSrcFileByName : public unary_function<ScExternalRefManager::SrcFileData, bool>
{
public:
    FindSrcFileByName(const String& rMatchName) :
        mrMatchName(rMatchName)
    {
    }

    bool operator()(const ScExternalRefManager::SrcFileData& rSrcData) const
    {
        return rSrcData.maFileName.Equals(mrMatchName);
    }

private:
    const String& mrMatchName;
};

class NotifyLinkListener : public unary_function<ScExternalRefManager::LinkListener*,  void>
{
public:
    NotifyLinkListener(sal_uInt16 nFileId, ScExternalRefManager::LinkUpdateType eType) :
        mnFileId(nFileId), meType(eType) {}

    NotifyLinkListener(const NotifyLinkListener& r) :
        mnFileId(r.mnFileId), meType(r.meType) {}

    void operator() (ScExternalRefManager::LinkListener* p) const
    {
        p->notify(mnFileId, meType);
    }
private:
    sal_uInt16 mnFileId;
    ScExternalRefManager::LinkUpdateType meType;
};

}

// ============================================================================

ScExternalRefCache::Table::Table()
    : meReferenced( REFERENCED_MARKED )
      // Prevent accidental data loss due to lack of knowledge.
{
}

ScExternalRefCache::Table::~Table()
{
}

void ScExternalRefCache::Table::setReferencedFlag( ScExternalRefCache::Table::ReferencedFlag eFlag )
{
    meReferenced = eFlag;
}

void ScExternalRefCache::Table::setReferenced( bool bReferenced )
{
    if (meReferenced != REFERENCED_PERMANENT)
        meReferenced = (bReferenced ? REFERENCED_MARKED : UNREFERENCED);
}

ScExternalRefCache::Table::ReferencedFlag ScExternalRefCache::Table::getReferencedFlag() const
{
    return meReferenced;
}

bool ScExternalRefCache::Table::isReferenced() const
{
    return meReferenced != UNREFERENCED;
}

void ScExternalRefCache::Table::setCell(SCCOL nCol, SCROW nRow, TokenRef pToken, sal_uInt32 nFmtIndex)
{
    using ::std::pair;
    RowsDataType::iterator itrRow = maRows.find(nRow);
    if (itrRow == maRows.end())
    {
        // This row does not exist yet.
        pair<RowsDataType::iterator, bool> res = maRows.insert(
            RowsDataType::value_type(nRow, RowDataType()));

        if (!res.second)
            return;

        itrRow = res.first;
    }

    // Insert this token into the specified column location.  I don't need to
    // check for existing data.  Just overwrite it.
    RowDataType& rRow = itrRow->second;
    ScExternalRefCache::Cell aCell;
    aCell.mxToken = pToken;
    aCell.mnFmtIndex = nFmtIndex;
    rRow.insert(RowDataType::value_type(nCol, aCell));
}

ScExternalRefCache::TokenRef ScExternalRefCache::Table::getCell(SCCOL nCol, SCROW nRow, sal_uInt32* pnFmtIndex) const
{
    RowsDataType::const_iterator itrTable = maRows.find(nRow);
    if (itrTable == maRows.end())
    {
        // this table doesn't have the specified row.
        return TokenRef();
    }

    const RowDataType& rRowData = itrTable->second;
    RowDataType::const_iterator itrRow = rRowData.find(nCol);
    if (itrRow == rRowData.end())
    {
        // this row doesn't have the specified column.
        return TokenRef();
    }

    const Cell& rCell = itrRow->second;
    if (pnFmtIndex)
        *pnFmtIndex = rCell.mnFmtIndex;

    return rCell.mxToken;
}

bool ScExternalRefCache::Table::hasRow( SCROW nRow ) const
{
    RowsDataType::const_iterator itrRow = maRows.find(nRow);
    return itrRow != maRows.end();
}

void ScExternalRefCache::Table::getAllRows(vector<SCROW>& rRows) const
{
    vector<SCROW> aRows;
    aRows.reserve(maRows.size());
    RowsDataType::const_iterator itr = maRows.begin(), itrEnd = maRows.end();
    for (; itr != itrEnd; ++itr)
        aRows.push_back(itr->first);

    // hash map is not ordered, so we need to explicitly sort it.
    ::std::sort(aRows.begin(), aRows.end());
    rRows.swap(aRows);
}

void ScExternalRefCache::Table::getAllCols(SCROW nRow, vector<SCCOL>& rCols) const
{
    RowsDataType::const_iterator itrRow = maRows.find(nRow);
    if (itrRow == maRows.end())
        // this table doesn't have the specified row.
        return;

    const RowDataType& rRowData = itrRow->second;
    vector<SCCOL> aCols;
    aCols.reserve(rRowData.size());
    RowDataType::const_iterator itrCol = rRowData.begin(), itrColEnd = rRowData.end();
    for (; itrCol != itrColEnd; ++itrCol)
        aCols.push_back(itrCol->first);

    // hash map is not ordered, so we need to explicitly sort it.
    ::std::sort(aCols.begin(), aCols.end());
    rCols.swap(aCols);
}

void ScExternalRefCache::Table::getAllNumberFormats(vector<sal_uInt32>& rNumFmts) const
{
    RowsDataType::const_iterator itrRow = maRows.begin(), itrRowEnd = maRows.end();
    for (; itrRow != itrRowEnd; ++itrRow)
    {
        const RowDataType& rRowData = itrRow->second;
        RowDataType::const_iterator itrCol = rRowData.begin(), itrColEnd = rRowData.end();
        for (; itrCol != itrColEnd; ++itrCol)
        {
            const Cell& rCell = itrCol->second;
            rNumFmts.push_back(rCell.mnFmtIndex);
        }
    }
}

// ----------------------------------------------------------------------------

ScExternalRefCache::TableName::TableName(const String& rUpper, const String& rReal) :
    maUpperName(rUpper), maRealName(rReal)
{
}

// ----------------------------------------------------------------------------

ScExternalRefCache::CellFormat::CellFormat() :
    mbIsSet(false), mnType(NUMBERFORMAT_ALL), mnIndex(0)
{
}

// ----------------------------------------------------------------------------

ScExternalRefCache::ScExternalRefCache()
{
}
ScExternalRefCache::~ScExternalRefCache()
{
}

const String* ScExternalRefCache::getRealTableName(sal_uInt16 nFileId, const String& rTabName) const
{
    DocDataType::const_iterator itrDoc = maDocs.find(nFileId);
    if (itrDoc == maDocs.end())
    {
        // specified document is not cached.
        return NULL;
    }

    const DocItem& rDoc = itrDoc->second;
    TableNameIndexMap::const_iterator itrTabId = rDoc.maTableNameIndex.find(
        ScGlobal::pCharClass->upper(rTabName));
    if (itrTabId == rDoc.maTableNameIndex.end())
    {
        // the specified table is not in cache.
        return NULL;
    }

    return &rDoc.maTableNames[itrTabId->second].maRealName;
}

const String* ScExternalRefCache::getRealRangeName(sal_uInt16 nFileId, const String& rRangeName) const
{
    DocDataType::const_iterator itrDoc = maDocs.find(nFileId);
    if (itrDoc == maDocs.end())
    {
        // specified document is not cached.
        return NULL;
    }

    const DocItem& rDoc = itrDoc->second;
    NamePairMap::const_iterator itr = rDoc.maRealRangeNameMap.find(
        ScGlobal::pCharClass->upper(rRangeName));
    if (itr == rDoc.maRealRangeNameMap.end())
        // range name not found.
        return NULL;

    return &itr->second;
}

ScExternalRefCache::TokenRef ScExternalRefCache::getCellData(
    sal_uInt16 nFileId, const String& rTabName, SCCOL nCol, SCROW nRow, 
    bool bEmptyCellOnNull, bool bWriteEmpty, sal_uInt32* pnFmtIndex)
{
    DocDataType::const_iterator itrDoc = maDocs.find(nFileId);
    if (itrDoc == maDocs.end())
    {
        // specified document is not cached.
        return TokenRef();
    }

    const DocItem& rDoc = itrDoc->second;
    TableNameIndexMap::const_iterator itrTabId = rDoc.maTableNameIndex.find(
        ScGlobal::pCharClass->upper(rTabName));
    if (itrTabId == rDoc.maTableNameIndex.end())
    {
        // the specified table is not in cache.
        return TokenRef();
    }

    const TableTypeRef& pTableData = rDoc.maTables[itrTabId->second];
    if (!pTableData.get())
    {
        // the table data is not instantiated yet.
        return TokenRef();
    }

    TokenRef pToken = pTableData->getCell(nCol, nRow, pnFmtIndex);
    if (!pToken && bEmptyCellOnNull)
    {
        pToken.reset(new ScEmptyCellToken(false, false));
        if (bWriteEmpty)
            pTableData->setCell(nCol, nRow, pToken);
    }
    return pToken;
}

ScExternalRefCache::TokenArrayRef ScExternalRefCache::getCellRangeData(
    sal_uInt16 nFileId, const String& rTabName, const ScRange& rRange, bool bEmptyCellOnNull, bool bWriteEmpty)
{
    DocDataType::iterator itrDoc = maDocs.find(nFileId);
    if (itrDoc == maDocs.end())
        // specified document is not cached.
        return TokenArrayRef();

    DocItem& rDoc = itrDoc->second;

    TableNameIndexMap::iterator itrTabId = rDoc.maTableNameIndex.find(
        ScGlobal::pCharClass->upper(rTabName));
    if (itrTabId == rDoc.maTableNameIndex.end())
        // the specified table is not in cache.
        return TokenArrayRef();

    const ScAddress& s = rRange.aStart;
    const ScAddress& e = rRange.aEnd;

    SCTAB nTab1 = s.Tab(), nTab2 = e.Tab();
    SCCOL nCol1 = s.Col(), nCol2 = e.Col();
    SCROW nRow1 = s.Row(), nRow2 = e.Row();

    // Make sure I have all the tables cached.
    size_t nTabFirstId = itrTabId->second;
    size_t nTabLastId  = nTabFirstId + nTab2 - nTab1;
    if (nTabLastId >= rDoc.maTables.size())
        // not all tables are cached.
        return TokenArrayRef();

    ScRange aCacheRange( nCol1, nRow1, static_cast<SCTAB>(nTabFirstId), nCol2, nRow2, static_cast<SCTAB>(nTabLastId));
    RangeArrayMap::const_iterator itrRange = rDoc.maRangeArrays.find( aCacheRange);
    if (itrRange != rDoc.maRangeArrays.end())
    {
        return itrRange->second;
    }

    TokenArrayRef pArray(new ScTokenArray);
    bool bFirstTab = true;
    for (size_t nTab = nTabFirstId; nTab <= nTabLastId; ++nTab)
    {
        TableTypeRef pTab = rDoc.maTables[nTab];
        if (!pTab.get())
            return TokenArrayRef();

        ScMatrixRef xMat = new ScMatrix(
            static_cast<SCSIZE>(nCol2-nCol1+1), static_cast<SCSIZE>(nRow2-nRow1+1));

        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
        {
            for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
            {
                TokenRef pToken = pTab->getCell(nCol, nRow);
                if (!pToken)
                {
                    if (bEmptyCellOnNull)
                    {
                        pToken.reset(new ScEmptyCellToken(false, false));
                        if (bWriteEmpty)
                            pTab->setCell(nCol, nRow, pToken);
                    }
                    else
                        return TokenArrayRef();
                }

                SCSIZE nC = nCol - nCol1, nR = nRow - nRow1;
                switch (pToken->GetType())
                {
                    case svDouble:
                        xMat->PutDouble(pToken->GetDouble(), nC, nR);
                    break;
                    case svString:
                        xMat->PutString(pToken->GetString(), nC, nR);
                    break;
                    default:
                        xMat->PutEmpty(nC, nR);
                }
            }
        }

        if (!bFirstTab)
            pArray->AddOpCode(ocSep);

        ScMatrix* pMat2 = xMat;
        ScMatrixToken aToken(pMat2);
        pArray->AddToken(aToken);

        bFirstTab = false;
    }
    rDoc.maRangeArrays.insert( RangeArrayMap::value_type( aCacheRange, pArray));
    return pArray;
}

ScExternalRefCache::TokenArrayRef ScExternalRefCache::getRangeNameTokens(sal_uInt16 nFileId, const String& rName)
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return TokenArrayRef();

    RangeNameMap& rMap = pDoc->maRangeNames;
    RangeNameMap::const_iterator itr = rMap.find(
        ScGlobal::pCharClass->upper(rName));
    if (itr == rMap.end())
        return TokenArrayRef();

    return itr->second;
}

void ScExternalRefCache::setRangeNameTokens(sal_uInt16 nFileId, const String& rName, TokenArrayRef pArray)
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return;

    String aUpperName = ScGlobal::pCharClass->upper(rName);
    RangeNameMap& rMap = pDoc->maRangeNames;
    rMap.insert(RangeNameMap::value_type(aUpperName, pArray));
    pDoc->maRealRangeNameMap.insert(NamePairMap::value_type(aUpperName, rName));
}

void ScExternalRefCache::setCellData(sal_uInt16 nFileId, const String& rTabName, SCROW nRow, SCCOL nCol,
                                     TokenRef pToken, sal_uInt32 nFmtIndex)
{
    if (!isDocInitialized(nFileId))
        return;

    using ::std::pair;
    DocItem* pDocItem = getDocItem(nFileId);
    if (!pDocItem)
        return;

    DocItem& rDoc = *pDocItem;

    // See if the table by this name already exists.
    TableNameIndexMap::iterator itrTabName = rDoc.maTableNameIndex.find(
        ScGlobal::pCharClass->upper(rTabName));
    if (itrTabName == rDoc.maTableNameIndex.end())
        // Table not found.  Maybe the table name or the file id is wrong ???
        return;

    TableTypeRef& pTableData = rDoc.maTables[itrTabName->second];
    if (!pTableData.get())
        pTableData.reset(new Table);

    pTableData->setCell(nCol, nRow, pToken, nFmtIndex);
}

void ScExternalRefCache::setCellRangeData(sal_uInt16 nFileId, const ScRange& rRange, const vector<SingleRangeData>& rData,
                                          TokenArrayRef pArray)
{
    using ::std::pair;
    if (rData.empty() || !isDocInitialized(nFileId))
        // nothing to cache
        return;

    // First, get the document item for the given file ID.
    DocItem* pDocItem = getDocItem(nFileId);
    if (!pDocItem)
        return;

    DocItem& rDoc = *pDocItem;

    // Now, find the table position of the first table to cache.
    const String& rFirstTabName = rData.front().maTableName;
    TableNameIndexMap::iterator itrTabName = rDoc.maTableNameIndex.find(
        ScGlobal::pCharClass->upper(rFirstTabName));
    if (itrTabName == rDoc.maTableNameIndex.end())
    {
        // table index not found.
        return;
    }

    size_t nTabFirstId = itrTabName->second;
    SCROW nRow1 = rRange.aStart.Row(), nRow2 = rRange.aEnd.Row();
    SCCOL nCol1 = rRange.aStart.Col(), nCol2 = rRange.aEnd.Col();
    vector<SingleRangeData>::const_iterator itrDataBeg = rData.begin(), itrDataEnd = rData.end();
    for (vector<SingleRangeData>::const_iterator itrData = itrDataBeg; itrData != itrDataEnd; ++itrData)
    {
        size_t i = nTabFirstId + ::std::distance(itrDataBeg, itrData);
        TableTypeRef& pTabData = rDoc.maTables[i];
        if (!pTabData.get())
            pTabData.reset(new Table);

        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
        {
            for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
            {
                SCSIZE nC = nCol - nCol1, nR = nRow - nRow1;
                TokenRef pToken;
                const ScMatrixRef& pMat = itrData->mpRangeData;
                if (pMat->IsValue(nC, nR))
                    pToken.reset(new formula::FormulaDoubleToken(pMat->GetDouble(nC, nR)));
                else if (pMat->IsString(nC, nR))
                    pToken.reset(new formula::FormulaStringToken(pMat->GetString(nC, nR)));
                else
                    pToken.reset(new ScEmptyCellToken(false, false));

                pTabData->setCell(nCol, nRow, pToken);
            }
        }
    }

    size_t nTabLastId = nTabFirstId + rRange.aEnd.Tab() - rRange.aStart.Tab();
    ScRange aCacheRange( nCol1, nRow1, static_cast<SCTAB>(nTabFirstId), nCol2, nRow2, static_cast<SCTAB>(nTabLastId));
    rDoc.maRangeArrays.insert( RangeArrayMap::value_type( aCacheRange, pArray));
}

bool ScExternalRefCache::isDocInitialized(sal_uInt16 nFileId)
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return false;

    return pDoc->mbInitFromSource;
}

static bool lcl_getTableDataIndex(const ScExternalRefCache::TableNameIndexMap& rMap, const String& rName, size_t& rIndex)
{
    ScExternalRefCache::TableNameIndexMap::const_iterator itr = rMap.find(rName);
    if (itr == rMap.end())
        return false;

    rIndex = itr->second;
    return true;
}

void ScExternalRefCache::initializeDoc(sal_uInt16 nFileId, const vector<String>& rTabNames)
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return;

    size_t n = rTabNames.size();

    // table name list - the list must include all table names in the source
    // document and only to be populated when loading the source document, not
    // when loading cached data from, say, Excel XCT/CRN records.
    vector<TableName> aNewTabNames;
    aNewTabNames.reserve(n);
    for (vector<String>::const_iterator itr = rTabNames.begin(), itrEnd = rTabNames.end();
          itr != itrEnd; ++itr)
    {
        TableName aNameItem(ScGlobal::pCharClass->upper(*itr), *itr);
        aNewTabNames.push_back(aNameItem);
    }
    pDoc->maTableNames.swap(aNewTabNames);

    // data tables - preserve any existing data that may have been set during
    // file import.
    vector<TableTypeRef> aNewTables(n);
    for (size_t i = 0; i < n; ++i)
    {
        size_t nIndex;
        if (lcl_getTableDataIndex(pDoc->maTableNameIndex, pDoc->maTableNames[i].maUpperName, nIndex))
        {
            aNewTables[i] = pDoc->maTables[nIndex];
        }
    }
    pDoc->maTables.swap(aNewTables);

    // name index map
    TableNameIndexMap aNewNameIndex;
    for (size_t i = 0; i < n; ++i)
        aNewNameIndex.insert(TableNameIndexMap::value_type(pDoc->maTableNames[i].maUpperName, i));
    pDoc->maTableNameIndex.swap(aNewNameIndex);

    pDoc->mbInitFromSource = true;
}

String ScExternalRefCache::getTableName(sal_uInt16 nFileId, size_t nCacheId) const
{
    if( DocItem* pDoc = getDocItem( nFileId ) )
        if( nCacheId < pDoc->maTableNames.size() )
            return pDoc->maTableNames[ nCacheId ].maRealName;
    return EMPTY_STRING;
}

void ScExternalRefCache::getAllTableNames(sal_uInt16 nFileId, vector<String>& rTabNames) const
{
    rTabNames.clear();
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return;

    size_t n = pDoc->maTableNames.size();
    rTabNames.reserve(n);
    for (vector<TableName>::const_iterator itr = pDoc->maTableNames.begin(), itrEnd = pDoc->maTableNames.end();
          itr != itrEnd; ++itr)
        rTabNames.push_back(itr->maRealName);
}

SCsTAB ScExternalRefCache::getTabSpan( sal_uInt16 nFileId, const String& rStartTabName, const String& rEndTabName ) const
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return -1;

    vector<TableName>::const_iterator itrBeg = pDoc->maTableNames.begin();
    vector<TableName>::const_iterator itrEnd = pDoc->maTableNames.end();

    vector<TableName>::const_iterator itrStartTab = ::std::find_if( itrBeg, itrEnd,
            TabNameSearchPredicate( rStartTabName));
    if (itrStartTab == itrEnd)
        return -1;

    vector<TableName>::const_iterator itrEndTab = ::std::find_if( itrBeg, itrEnd,
            TabNameSearchPredicate( rEndTabName));
    if (itrEndTab == itrEnd)
        return 0;

    size_t nStartDist = ::std::distance( itrBeg, itrStartTab);
    size_t nEndDist = ::std::distance( itrBeg, itrEndTab);
    return nStartDist <= nEndDist ? static_cast<SCsTAB>(nEndDist - nStartDist + 1) : -static_cast<SCsTAB>(nStartDist - nEndDist + 1);
}

void ScExternalRefCache::getAllNumberFormats(vector<sal_uInt32>& rNumFmts) const
{
    using ::std::sort;
    using ::std::unique;

    vector<sal_uInt32> aNumFmts;
    for (DocDataType::const_iterator itrDoc = maDocs.begin(), itrDocEnd = maDocs.end();
          itrDoc != itrDocEnd; ++itrDoc)
    {
        const vector<TableTypeRef>& rTables = itrDoc->second.maTables;
        for (vector<TableTypeRef>::const_iterator itrTab = rTables.begin(), itrTabEnd = rTables.end();
              itrTab != itrTabEnd; ++itrTab)
        {
            TableTypeRef pTab = *itrTab;
            if (!pTab)
                continue;

            pTab->getAllNumberFormats(aNumFmts);
        }
    }

    // remove duplicates.
    sort(aNumFmts.begin(), aNumFmts.end());
    aNumFmts.erase(unique(aNumFmts.begin(), aNumFmts.end()), aNumFmts.end());
    rNumFmts.swap(aNumFmts);
}

bool ScExternalRefCache::hasCacheTable(sal_uInt16 nFileId, const String& rTabName) const
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
        return false;

    String aUpperName = ScGlobal::pCharClass->upper(rTabName);
    vector<TableName>::const_iterator itrBeg = pDoc->maTableNames.begin(), itrEnd = pDoc->maTableNames.end();
    vector<TableName>::const_iterator itr = ::std::find_if(
        itrBeg, itrEnd, TabNameSearchPredicate(aUpperName));

    return itr != itrEnd;
}

size_t ScExternalRefCache::getCacheTableCount(sal_uInt16 nFileId) const
{
    DocItem* pDoc = getDocItem(nFileId);
    return pDoc ? pDoc->maTables.size() : 0;
}

bool ScExternalRefCache::setCacheDocReferenced( sal_uInt16 nFileId )
{
    DocItem* pDocItem = getDocItem(nFileId);
    if (!pDocItem)
        return areAllCacheTablesReferenced();

    for (::std::vector<TableTypeRef>::iterator itrTab = pDocItem->maTables.begin();
            itrTab != pDocItem->maTables.end(); ++itrTab)
    {
        if ((*itrTab).get())
            (*itrTab)->setReferenced( true);
    }
    addCacheDocToReferenced( nFileId);
    return areAllCacheTablesReferenced();
}

bool ScExternalRefCache::setCacheTableReferenced( sal_uInt16 nFileId, const String& rTabName, size_t nSheets, bool bPermanent )
{
    DocItem* pDoc = getDocItem(nFileId);
    if (pDoc)
    {
        size_t nIndex = 0;
        String aTabNameUpper = ScGlobal::pCharClass->upper( rTabName);
        if (lcl_getTableDataIndex( pDoc->maTableNameIndex, aTabNameUpper, nIndex))
        {
            size_t nStop = ::std::min( nIndex + nSheets, pDoc->maTables.size());
            for (size_t i = nIndex; i < nStop; ++i)
            {
                TableTypeRef pTab = pDoc->maTables[i];
                if (pTab.get())
                {
                    Table::ReferencedFlag eNewFlag = (bPermanent ? 
                            Table::REFERENCED_PERMANENT : 
                            Table::REFERENCED_MARKED);
                    Table::ReferencedFlag eOldFlag = pTab->getReferencedFlag();
                    if (eOldFlag != Table::REFERENCED_PERMANENT && eNewFlag != eOldFlag)
                    {
                        pTab->setReferencedFlag( eNewFlag);
                        addCacheTableToReferenced( nFileId, i);
                    }
                }
            }
        }
    }
    return areAllCacheTablesReferenced();
}

void ScExternalRefCache::setCacheTableReferencedPermanently( sal_uInt16 nFileId, const String& rTabName, size_t nSheets )
{
    DocItem* pDoc = getDocItem(nFileId);
    if (pDoc)
    {
        size_t nIndex = 0;
        String aTabNameUpper = ScGlobal::pCharClass->upper( rTabName);
        if (lcl_getTableDataIndex( pDoc->maTableNameIndex, aTabNameUpper, nIndex))
        {
            size_t nStop = ::std::min( nIndex + nSheets, pDoc->maTables.size());
            for (size_t i = nIndex; i < nStop; ++i)
            {
                TableTypeRef pTab = pDoc->maTables[i];
                if (pTab.get())
                    pTab->setReferencedFlag( Table::REFERENCED_PERMANENT);
            }
        }
    }
}

void ScExternalRefCache::setAllCacheTableReferencedStati( bool bReferenced )
{
    if (bReferenced)
    {
        maReferenced.reset(0);
        for (DocDataType::iterator itrDoc = maDocs.begin(); itrDoc != maDocs.end(); ++itrDoc)
        {
            ScExternalRefCache::DocItem& rDocItem = (*itrDoc).second;
            for (::std::vector<TableTypeRef>::iterator itrTab = rDocItem.maTables.begin();
                    itrTab != rDocItem.maTables.end(); ++itrTab)
            {
                if ((*itrTab).get())
                    (*itrTab)->setReferenced( true);
            }
        }
    }
    else
    {
        size_t nDocs = 0;
        for (DocDataType::const_iterator itrDoc = maDocs.begin(); itrDoc != maDocs.end(); ++itrDoc)
        {
            if (nDocs <= (*itrDoc).first)
                nDocs  = (*itrDoc).first + 1;
        }
        maReferenced.reset( nDocs);

        for (DocDataType::iterator itrDoc = maDocs.begin(); itrDoc != maDocs.end(); ++itrDoc)
        {
            ScExternalRefCache::DocItem& rDocItem = (*itrDoc).second;
            sal_uInt16 nFileId = (*itrDoc).first;
            size_t nTables = rDocItem.maTables.size();
            ReferencedStatus::DocReferenced & rDocReferenced = maReferenced.maDocs[nFileId];
            // All referenced => non-existing tables evaluate as completed.
            rDocReferenced.maTables.resize( nTables, true);
            for (size_t i=0; i < nTables; ++i)
            {
                TableTypeRef & xTab = rDocItem.maTables[i];
                if (xTab.get())
                {
                    if (xTab->getReferencedFlag() == Table::REFERENCED_PERMANENT)
                        addCacheTableToReferenced( nFileId, i);
                    else
                    {
                        xTab->setReferencedFlag( Table::UNREFERENCED);
                        rDocReferenced.maTables[i] = false;
                        rDocReferenced.mbAllTablesReferenced = false;
                        // An addCacheTableToReferenced() actually may have 
                        // resulted in mbAllReferenced been set. Clear it.
                        maReferenced.mbAllReferenced = false;
                    }
                }
            }
        }
    }
}

void ScExternalRefCache::addCacheTableToReferenced( sal_uInt16 nFileId, size_t nIndex )
{
    if (nFileId >= maReferenced.maDocs.size())
        return;

    ::std::vector<bool> & rTables = maReferenced.maDocs[nFileId].maTables;
    size_t nTables = rTables.size();
    if (nIndex >= nTables)
        return;

    if (!rTables[nIndex])
    {
        rTables[nIndex] = true;
        size_t i = 0;
        while (i < nTables && rTables[i])
            ++i;
        if (i == nTables)
        {
            maReferenced.maDocs[nFileId].mbAllTablesReferenced = true;
            maReferenced.checkAllDocs();
        }
    }
}

void ScExternalRefCache::addCacheDocToReferenced( sal_uInt16 nFileId )
{
    if (nFileId >= maReferenced.maDocs.size())
        return;

    if (!maReferenced.maDocs[nFileId].mbAllTablesReferenced)
    {
        ::std::vector<bool> & rTables = maReferenced.maDocs[nFileId].maTables;
        size_t nSize = rTables.size();
        for (size_t i=0; i < nSize; ++i)
            rTables[i] = true;
        maReferenced.maDocs[nFileId].mbAllTablesReferenced = true;
        maReferenced.checkAllDocs();
    }
}

bool ScExternalRefCache::areAllCacheTablesReferenced() const
{
    return maReferenced.mbAllReferenced;
}

ScExternalRefCache::ReferencedStatus::ReferencedStatus() :
    mbAllReferenced(false)
{
    reset(0);
}

ScExternalRefCache::ReferencedStatus::ReferencedStatus( size_t nDocs ) :
    mbAllReferenced(false)
{
    reset( nDocs);
}

void ScExternalRefCache::ReferencedStatus::reset( size_t nDocs )
{
    if (nDocs)
    {
        mbAllReferenced = false;
        DocReferencedVec aRefs( nDocs);
        maDocs.swap( aRefs);
    }
    else
    {
        mbAllReferenced = true;
        DocReferencedVec aRefs;
        maDocs.swap( aRefs);
    }
}

void ScExternalRefCache::ReferencedStatus::checkAllDocs()
{
    for (DocReferencedVec::const_iterator itr = maDocs.begin(); itr != maDocs.end(); ++itr)
    {
        if (!(*itr).mbAllTablesReferenced)
            return;
    }
    mbAllReferenced = true;
}

ScExternalRefCache::TableTypeRef ScExternalRefCache::getCacheTable(sal_uInt16 nFileId, size_t nTabIndex) const
{
    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc || nTabIndex >= pDoc->maTables.size())
        return TableTypeRef();

    return pDoc->maTables[nTabIndex];
}

ScExternalRefCache::TableTypeRef ScExternalRefCache::getCacheTable(sal_uInt16 nFileId, const String& rTabName, bool bCreateNew, size_t* pnIndex)
{
    // In API, the index is transported as cached sheet ID of type sal_Int32 in 
    // sheet::SingleReference.Sheet or sheet::ComplexReference.Reference1.Sheet 
    // in a sheet::FormulaToken, choose a sensible value for N/A. Effectively 
    // being 0xffffffff
    const size_t nNotAvailable = static_cast<size_t>( static_cast<sal_Int32>( -1));

    DocItem* pDoc = getDocItem(nFileId);
    if (!pDoc)
    {
        if (pnIndex) *pnIndex = nNotAvailable;
        return TableTypeRef();
    }

    DocItem& rDoc = *pDoc;

    size_t nIndex;
    String aTabNameUpper = ScGlobal::pCharClass->upper(rTabName);
    if (lcl_getTableDataIndex(rDoc.maTableNameIndex, aTabNameUpper, nIndex))
    {
        // specified table found.
        if( pnIndex ) *pnIndex = nIndex;
        return rDoc.maTables[nIndex];
    }

    if (!bCreateNew)
    {
        if (pnIndex) *pnIndex = nNotAvailable;
        return TableTypeRef();
    }

    // Specified table doesn't exist yet.  Create one.
    nIndex = rDoc.maTables.size();
    if( pnIndex ) *pnIndex = nIndex;
    TableTypeRef pTab(new Table);
    rDoc.maTables.push_back(pTab);
    rDoc.maTableNames.push_back(TableName(aTabNameUpper, rTabName));
    rDoc.maTableNameIndex.insert(
        TableNameIndexMap::value_type(aTabNameUpper, nIndex));
    return pTab;
}

void ScExternalRefCache::clearCache(sal_uInt16 nFileId)
{
    maDocs.erase(nFileId);
}

ScExternalRefCache::DocItem* ScExternalRefCache::getDocItem(sal_uInt16 nFileId) const
{
    using ::std::pair;
    DocDataType::iterator itrDoc = maDocs.find(nFileId);
    if (itrDoc == maDocs.end())
    {
        // specified document is not cached.
        pair<DocDataType::iterator, bool> res = maDocs.insert(
                DocDataType::value_type(nFileId, DocItem()));

        if (!res.second)
            // insertion failed.
            return NULL;

        itrDoc = res.first;
    }

    return &itrDoc->second;
}

// ============================================================================

ScExternalRefLink::ScExternalRefLink(ScDocument* pDoc, sal_uInt16 nFileId, const String& rFilter) :
    ::sfx2::SvBaseLink(::sfx2::LINKUPDATE_ONCALL, FORMAT_FILE),
    mnFileId(nFileId),
    maFilterName(rFilter),
    mpDoc(pDoc),
    mbDoRefresh(true)
{
}

ScExternalRefLink::~ScExternalRefLink()
{
}

void ScExternalRefLink::Closed()
{
    ScExternalRefManager* pMgr = mpDoc->GetExternalRefManager();
    pMgr->breakLink(mnFileId);
}

void ScExternalRefLink::DataChanged(const String& /*rMimeType*/, const Any& /*rValue*/)
{
    if (!mbDoRefresh)
        return;

    String aFile, aFilter;
    mpDoc->GetLinkManager()->GetDisplayNames(this, NULL, &aFile, NULL, &aFilter);
    ScExternalRefManager* pMgr = mpDoc->GetExternalRefManager();
    const String* pCurFile = pMgr->getExternalFileName(mnFileId);
    if (!pCurFile)
        return;

    if (pCurFile->Equals(aFile))
    {
        // Refresh the current source document.
        pMgr->refreshNames(mnFileId);
    }
    else
    {
        // The source document has changed.
        pMgr->switchSrcFile(mnFileId, aFile, aFilter);
        maFilterName = aFilter;
    }
}

void ScExternalRefLink::Edit(Window* pParent, const Link& /*rEndEditHdl*/)
{
    SvBaseLink::Edit(pParent, LINK(this, ScExternalRefLink, ExternalRefEndEditHdl));
}

void ScExternalRefLink::SetDoReferesh(bool b)
{
    mbDoRefresh = b;
}

IMPL_LINK( ScExternalRefLink, ExternalRefEndEditHdl, ::sfx2::SvBaseLink*, EMPTYARG )
{
    return 0;
}

// ============================================================================

static FormulaToken* lcl_convertToToken(ScBaseCell* pCell)
{
    if (!pCell || pCell->HasEmptyData())
    {
        bool bInherited = (pCell && pCell->GetCellType() == CELLTYPE_FORMULA);
        return new ScEmptyCellToken( bInherited, false);
    }

    switch (pCell->GetCellType())
    {
        case CELLTYPE_EDIT:
        {
            String aStr;
            static_cast<ScEditCell*>(pCell)->GetString(aStr);
            return new formula::FormulaStringToken(aStr);
        }
        //break;
        case CELLTYPE_STRING:
        {
            String aStr;
            static_cast<ScStringCell*>(pCell)->GetString(aStr);
            return new formula::FormulaStringToken(aStr);
        }
        //break;
        case CELLTYPE_VALUE:
        {
            double fVal = static_cast<ScValueCell*>(pCell)->GetValue();
            return new formula::FormulaDoubleToken(fVal);
        }
        //break;
        case CELLTYPE_FORMULA:
        {
            ScFormulaCell* pFCell = static_cast<ScFormulaCell*>(pCell);
            USHORT nError = pFCell->GetErrCode();
            if (nError)
                return new FormulaErrorToken( nError);
            else if (pFCell->IsValue())
            {
                double fVal = pFCell->GetValue();
                return new formula::FormulaDoubleToken(fVal);
            }
            else
            {
                String aStr;
                pFCell->GetString(aStr);
                return new formula::FormulaStringToken(aStr);
            }
        }
        //break;
        default:
            DBG_ERROR("attempted to convert an unknown cell type.");
    }

    return NULL;
}

static ScTokenArray* lcl_convertToTokenArray(ScDocument* pSrcDoc, const ScRange& rRange,
                                             vector<ScExternalRefCache::SingleRangeData>& rCacheData)
{
    const ScAddress& s = rRange.aStart;
    const ScAddress& e = rRange.aEnd;

    SCTAB nTab1 = s.Tab(), nTab2 = e.Tab();
    SCCOL nCol1 = s.Col(), nCol2 = e.Col();
    SCROW nRow1 = s.Row(), nRow2 = e.Row();

    if (nTab2 != nTab1)
        // For now, we don't support multi-sheet ranges intentionally because
        // we don't have a way to express them in a single token.  In the
        // future we can introduce a new stack variable type svMatrixList with
        // a new token type that can store a 3D matrix value and convert a 3D
        // range to it.
        return NULL;

    auto_ptr<ScTokenArray> pArray(new ScTokenArray);
    bool bFirstTab = true;
    vector<ScExternalRefCache::SingleRangeData>::iterator
        itrCache = rCacheData.begin(), itrCacheEnd = rCacheData.end();
    for (SCTAB nTab = nTab1; nTab <= nTab2 && itrCache != itrCacheEnd; ++nTab, ++itrCache)
    {
        ScMatrixRef xMat = new ScMatrix(
            static_cast<SCSIZE>(nCol2-nCol1+1),
            static_cast<SCSIZE>(nRow2-nRow1+1));

        for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
        {
            for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
            {
                SCSIZE nC = nCol - nCol1, nR = nRow - nRow1;
                ScBaseCell* pCell;
                pSrcDoc->GetCell(nCol, nRow, nTab, pCell);
                if (!pCell || pCell->HasEmptyData())
                    xMat->PutEmpty(nC, nR);
                else
                {
                    switch (pCell->GetCellType())
                    {
                        case CELLTYPE_EDIT:
                        {
                            String aStr;
                            static_cast<ScEditCell*>(pCell)->GetString(aStr);
                            xMat->PutString(aStr, nC, nR);
                        }
                        break;
                        case CELLTYPE_STRING:
                        {
                            String aStr;
                            static_cast<ScStringCell*>(pCell)->GetString(aStr);
                            xMat->PutString(aStr, nC, nR);
                        }
                        break;
                        case CELLTYPE_VALUE:
                        {
                            double fVal = static_cast<ScValueCell*>(pCell)->GetValue();
                            xMat->PutDouble(fVal, nC, nR);
                        }
                        break;
                        case CELLTYPE_FORMULA:
                        {
                            ScFormulaCell* pFCell = static_cast<ScFormulaCell*>(pCell);
                            USHORT nError = pFCell->GetErrCode();
                            if (nError)
                                xMat->PutDouble( CreateDoubleError( nError), nC, nR);
                            else if (pFCell->IsValue())
                            {
                                double fVal = pFCell->GetValue();
                                xMat->PutDouble(fVal, nC, nR);
                            }
                            else
                            {
                                String aStr;
                                pFCell->GetString(aStr);
                                xMat->PutString(aStr, nC, nR);
                            }
                        }
                        break;
                        default:
                            DBG_ERROR("attempted to convert an unknown cell type.");
                    }
                }
            }
        }
        if (!bFirstTab)
            pArray->AddOpCode(ocSep);

        ScMatrix* pMat2 = xMat;
        ScMatrixToken aToken(pMat2);
        pArray->AddToken(aToken);

        itrCache->mpRangeData = xMat;

        bFirstTab = false;
    }
    return pArray.release();
}

ScExternalRefManager::ScExternalRefManager(ScDocument* pDoc) :
    mpDoc(pDoc),
    bInReferenceMarking(false)
{
    maSrcDocTimer.SetTimeoutHdl( LINK(this, ScExternalRefManager, TimeOutHdl) );
    maSrcDocTimer.SetTimeout(SRCDOC_SCAN_INTERVAL);
}

ScExternalRefManager::~ScExternalRefManager()
{
    clear();
}

String ScExternalRefManager::getCacheTableName(sal_uInt16 nFileId, size_t nTabIndex) const
{
    return maRefCache.getTableName(nFileId, nTabIndex);
}

ScExternalRefCache::TableTypeRef ScExternalRefManager::getCacheTable(sal_uInt16 nFileId, size_t nTabIndex) const
{
    return maRefCache.getCacheTable(nFileId, nTabIndex);
}

ScExternalRefCache::TableTypeRef ScExternalRefManager::getCacheTable(sal_uInt16 nFileId, const String& rTabName, bool bCreateNew, size_t* pnIndex)
{
    return maRefCache.getCacheTable(nFileId, rTabName, bCreateNew, pnIndex);
}

// ============================================================================

ScExternalRefManager::RefCells::TabItem::TabItem(SCTAB nIndex) :
    mnIndex(nIndex)
{
}

ScExternalRefManager::RefCells::TabItem::TabItem(const TabItem& r) :
    mnIndex(r.mnIndex),
    maCols(r.maCols)
{
}

ScExternalRefManager::RefCells::RefCells()
{
}

ScExternalRefManager::RefCells::~RefCells()
{
}

list<ScExternalRefManager::RefCells::TabItemRef>::iterator ScExternalRefManager::RefCells::getTabPos(SCTAB nTab)
{
    list<TabItemRef>::iterator itr = maTables.begin(), itrEnd = maTables.end();
    for (; itr != itrEnd; ++itr)
        if ((*itr)->mnIndex >= nTab)
            return itr;
    // Not found.  return the end position.
    return itrEnd;
}

void ScExternalRefManager::RefCells::insertCell(const ScAddress& rAddr)
{
    SCTAB nTab = rAddr.Tab();
    SCCOL nCol = rAddr.Col();
    SCROW nRow = rAddr.Row();

    // Search by table index.
    list<TabItemRef>::iterator itrTab = getTabPos(nTab);
    TabItemRef xTabRef;
    if (itrTab == maTables.end())
    {
        // All previous tables come before the specificed table.
        xTabRef.reset(new TabItem(nTab));
        maTables.push_back(xTabRef);
    }
    else if ((*itrTab)->mnIndex > nTab)
    {
        // Insert at the current iterator position.
        xTabRef.reset(new TabItem(nTab));
        maTables.insert(itrTab, xTabRef);
    }
    else if ((*itrTab)->mnIndex == nTab)
    {
        // The table found.
        xTabRef = *itrTab;
    }
    ColSet& rCols = xTabRef->maCols;

    // Then by column index.
    ColSet::iterator itrCol = rCols.find(nCol);
    if (itrCol == rCols.end())
    {
        RowSet aRows;
        pair<ColSet::iterator, bool> r = rCols.insert(ColSet::value_type(nCol, aRows));
        if (!r.second)
            // column insertion failed.
            return;
        itrCol = r.first;
    }
    RowSet& rRows = itrCol->second;

    // Finally, insert the row index.
    rRows.insert(nRow);
}

void ScExternalRefManager::RefCells::removeCell(const ScAddress& rAddr)
{
    SCTAB nTab = rAddr.Tab();
    SCCOL nCol = rAddr.Col();
    SCROW nRow = rAddr.Row();

    // Search by table index.
    list<TabItemRef>::iterator itrTab = getTabPos(nTab);
    if (itrTab == maTables.end() || (*itrTab)->mnIndex != nTab)
        // No such table.
        return;

    ColSet& rCols = (*itrTab)->maCols;

    // Then by column index.
    ColSet::iterator itrCol = rCols.find(nCol);
    if (itrCol == rCols.end())
        // No such column
        return;

    RowSet& rRows = itrCol->second;
    rRows.erase(nRow);
}

void ScExternalRefManager::RefCells::moveTable(SCTAB nOldTab, SCTAB nNewTab, bool bCopy)
{
    if (nOldTab == nNewTab)
        // Nothing to do here.
        return;

    list<TabItemRef>::iterator itrOld = getTabPos(nOldTab);
    if (itrOld == maTables.end() || (*itrOld)->mnIndex != nOldTab)
        // No table to move or copy.
        return;

    list<TabItemRef>::iterator itrNew = getTabPos(nNewTab);
    if (bCopy)
    {
        // Simply make a duplicate of the original table, insert it at the
        // new tab position, and increment the table index for all tables
        // that come after that inserted table.

        TabItemRef xNewTab(new TabItem(*(*itrOld)));
        xNewTab->mnIndex = nNewTab;
        maTables.insert(itrNew, xNewTab);
        list<TabItemRef>::iterator itr = itrNew, itrEnd = maTables.end();
        if (itr != itrEnd)  // #i99807# check that itr is not at end already
            for (++itr; itr != itrEnd; ++itr)
                (*itr)->mnIndex += 1;
    }
    else
    {
        if (itrOld == itrNew)
        {
            // No need to move the table.  Just update the table index.
            (*itrOld)->mnIndex = nNewTab;
            return;
        }

        if (nOldTab < nNewTab)
        {
            // Iterate from the old tab position to the new tab position (not
            // inclusive of the old tab itself), and decrement their tab
            // index by one.
            list<TabItemRef>::iterator itr = itrOld;
            for (++itr; itr != itrNew; ++itr)
                (*itr)->mnIndex -= 1;

            // Insert a duplicate of the original table.  This does not
            // invalidate the iterators.
            (*itrOld)->mnIndex = nNewTab - 1;
            if (itrNew == maTables.end())
                maTables.push_back(*itrOld);
            else
                maTables.insert(itrNew, *itrOld);

            // Remove the original table.
            maTables.erase(itrOld);
        }
        else
        {
            // nNewTab < nOldTab

            // Iterate from the new tab position to the one before the old tab
            // position, and increment their tab index by one.
            list<TabItemRef>::iterator itr = itrNew;
            for (++itr; itr != itrOld; ++itr)
                (*itr)->mnIndex += 1;

            (*itrOld)->mnIndex = nNewTab;
            maTables.insert(itrNew, *itrOld);

            // Remove the original table.
            maTables.erase(itrOld);
        }
    }
}

void ScExternalRefManager::RefCells::insertTable(SCTAB nPos)
{
    TabItemRef xNewTab(new TabItem(nPos));
    list<TabItemRef>::iterator itr = getTabPos(nPos);
    if (itr == maTables.end())
        maTables.push_back(xNewTab);
    else
        maTables.insert(itr, xNewTab);
}

void ScExternalRefManager::RefCells::removeTable(SCTAB nPos)
{
    list<TabItemRef>::iterator itr = getTabPos(nPos);
    if (itr == maTables.end())
        // nothing to remove.
        return;

    maTables.erase(itr);
}

void ScExternalRefManager::RefCells::refreshAllCells(ScExternalRefManager& rRefMgr)
{
    // Get ALL the cell positions for re-compilation.
    for (list<TabItemRef>::iterator itrTab = maTables.begin(), itrTabEnd = maTables.end();
          itrTab != itrTabEnd; ++itrTab)
    {
        SCTAB nTab = (*itrTab)->mnIndex;
        ColSet& rCols = (*itrTab)->maCols;
        for (ColSet::iterator itrCol = rCols.begin(), itrColEnd = rCols.end();
              itrCol != itrColEnd; ++itrCol)
        {
            SCCOL nCol = itrCol->first;
            RowSet& rRows = itrCol->second;
            RowSet aNewRows;
            for (RowSet::iterator itrRow = rRows.begin(), itrRowEnd = rRows.end();
                  itrRow != itrRowEnd; ++itrRow)
            {
                SCROW nRow = *itrRow;
                ScAddress aCell(nCol, nRow, nTab);
                if (rRefMgr.compileTokensByCell(aCell))
                    // This cell still contains an external refernce.
                    aNewRows.insert(nRow);
            }
            // Update the rows so that cells with no external references are
            // no longer tracked.
            rRows.swap(aNewRows);
        }
    }
}

// ----------------------------------------------------------------------------

ScExternalRefManager::LinkListener::LinkListener()
{
}

ScExternalRefManager::LinkListener::~LinkListener()
{
}

// ----------------------------------------------------------------------------

void ScExternalRefManager::getAllCachedTableNames(sal_uInt16 nFileId, vector<String>& rTabNames) const
{
    maRefCache.getAllTableNames(nFileId, rTabNames);
}

SCsTAB ScExternalRefManager::getCachedTabSpan( sal_uInt16 nFileId, const String& rStartTabName, const String& rEndTabName ) const
{
    return maRefCache.getTabSpan( nFileId, rStartTabName, rEndTabName);
}

void ScExternalRefManager::getAllCachedNumberFormats(vector<sal_uInt32>& rNumFmts) const
{
    maRefCache.getAllNumberFormats(rNumFmts);
}

bool ScExternalRefManager::hasCacheTable(sal_uInt16 nFileId, const String& rTabName) const
{
    return maRefCache.hasCacheTable(nFileId, rTabName);
}

size_t ScExternalRefManager::getCacheTableCount(sal_uInt16 nFileId) const
{
    return maRefCache.getCacheTableCount(nFileId);
}

sal_uInt16 ScExternalRefManager::getExternalFileCount() const
{
    return static_cast< sal_uInt16 >( maSrcFiles.size() );
}

bool ScExternalRefManager::markUsedByLinkListeners()
{
    bool bAllMarked = false;
    for (LinkListenerMap::const_iterator itr = maLinkListeners.begin();
            itr != maLinkListeners.end() && !bAllMarked; ++itr)
    {
        if (!(*itr).second.empty())
            bAllMarked = maRefCache.setCacheDocReferenced( (*itr).first);
        /* TODO: LinkListeners should remember the table they're listening to. 
         * As is, listening to one table will mark all tables of the document 
         * being referenced. */
    }
    return bAllMarked;
}

bool ScExternalRefManager::setCacheDocReferenced( sal_uInt16 nFileId )
{
    return maRefCache.setCacheDocReferenced( nFileId);
}

bool ScExternalRefManager::setCacheTableReferenced( sal_uInt16 nFileId, const String& rTabName, size_t nSheets )
{
    return maRefCache.setCacheTableReferenced( nFileId, rTabName, nSheets, false);
}

void ScExternalRefManager::setCacheTableReferencedPermanently( sal_uInt16 nFileId, const String& rTabName, size_t nSheets )
{
    if (isInReferenceMarking())
        // Do all maintenance work.
        maRefCache.setCacheTableReferenced( nFileId, rTabName, nSheets, true);
    else
        // Set only the permanent flag.
        maRefCache.setCacheTableReferencedPermanently( nFileId, rTabName, nSheets);
}

void ScExternalRefManager::setAllCacheTableReferencedStati( bool bReferenced )
{
    bInReferenceMarking = !bReferenced;
    maRefCache.setAllCacheTableReferencedStati( bReferenced );
}

void ScExternalRefManager::storeRangeNameTokens(sal_uInt16 nFileId, const String& rName, const ScTokenArray& rArray)
{
    ScExternalRefCache::TokenArrayRef pArray(rArray.Clone());
    maRefCache.setRangeNameTokens(nFileId, rName, pArray);
}

ScExternalRefCache::TokenRef ScExternalRefManager::getSingleRefToken(
    sal_uInt16 nFileId, const String& rTabName, const ScAddress& rCell,
    const ScAddress* pCurPos, SCTAB* pTab, ScExternalRefCache::CellFormat* pFmt)
{
    if (pCurPos)
        insertRefCell(nFileId, *pCurPos);

    maybeLinkExternalFile(nFileId);

    if (pTab)
        *pTab = -1;

    if (pFmt)
        pFmt->mbIsSet = false;

    bool bLoading = mpDoc->IsImportingXML();

    // Check if the given table name and the cell position is cached.
    // #i101304# When loading a file, the saved cache (hidden sheet)
    // is assumed to contain all data for the loaded formulas.
    // No cache entries are created from empty cells in the saved sheet,
    // so they have to be created here (bWriteEmpty parameter).
    // Otherwise, later interpretation of the loaded formulas would
    // load the source document even if the user didn't want to update.
    sal_uInt32 nFmtIndex = 0;
    ScExternalRefCache::TokenRef pToken = maRefCache.getCellData(
        nFileId, rTabName, rCell.Col(), rCell.Row(), bLoading, bLoading, &nFmtIndex);
    if (pToken)
    {
        if (pFmt)
        {
            short nFmtType = mpDoc->GetFormatTable()->GetType(nFmtIndex);
            if (nFmtType != NUMBERFORMAT_UNDEFINED)
            {
                pFmt->mbIsSet = true;
                pFmt->mnIndex = nFmtIndex;
                pFmt->mnType = nFmtType;
            }
        }
        return pToken;
    }

    // reference not cached.  read from the source document.
    ScDocument* pSrcDoc = getSrcDocument(nFileId);
    if (!pSrcDoc)
    {
        // Source document is not reachable.  Try to get data from the cache 
        // once again, but this time treat a non-cached cell as an empty cell
        // as long as the table itself is cached.
        pToken = maRefCache.getCellData(
            nFileId, rTabName, rCell.Col(), rCell.Row(), true, false, &nFmtIndex);
        return pToken;
    }

    ScBaseCell* pCell = NULL;
    SCTAB nTab;
    if (!pSrcDoc->GetTable(rTabName, nTab))
    {
        // specified table name doesn't exist in the source document.
        return ScExternalRefCache::TokenRef();
    }

    if (pTab)
        *pTab = nTab;

    pSrcDoc->GetCell(rCell.Col(), rCell.Row(), nTab, pCell);
    ScExternalRefCache::TokenRef pTok(lcl_convertToToken(pCell));

    pSrcDoc->GetNumberFormat(rCell.Col(), rCell.Row(), nTab, nFmtIndex);
    nFmtIndex = getMappedNumberFormat(nFileId, nFmtIndex, pSrcDoc);
    if (pFmt)
    {
        short nFmtType = mpDoc->GetFormatTable()->GetType(nFmtIndex);
        if (nFmtType != NUMBERFORMAT_UNDEFINED)
        {
            pFmt->mbIsSet = true;
            pFmt->mnIndex = nFmtIndex;
            pFmt->mnType = nFmtType;
        }
    }

    if (!pTok.get())
    {
        // Generate an error for unresolvable cells.
        pTok.reset( new FormulaErrorToken( errNoValue));
    }

    // Now, insert the token into cache table.
    maRefCache.setCellData(nFileId, rTabName, rCell.Row(), rCell.Col(), pTok, nFmtIndex);
    return pTok;
}

ScExternalRefCache::TokenArrayRef ScExternalRefManager::getDoubleRefTokens(sal_uInt16 nFileId, const String& rTabName, const ScRange& rRange, const ScAddress* pCurPos)
{
    if (pCurPos)
        insertRefCell(nFileId, *pCurPos);

    maybeLinkExternalFile(nFileId);

    bool bLoading = mpDoc->IsImportingXML();

    // Check if the given table name and the cell position is cached.
    // #i101304# When loading, put empty cells into cache, see getSingleRefToken.
    ScExternalRefCache::TokenArrayRef p = maRefCache.getCellRangeData(nFileId, rTabName, rRange, bLoading, bLoading);
    if (p.get())
        return p;

    ScDocument* pSrcDoc = getSrcDocument(nFileId);
    if (!pSrcDoc)
    {
        // Source document is not reachable.  Try to get data from the cache 
        // once again, but this time treat non-cached cells as empty cells as
        // long as the table itself is cached.
        return maRefCache.getCellRangeData(nFileId, rTabName, rRange, true, false);
    }

    SCTAB nTab1;
    if (!pSrcDoc->GetTable(rTabName, nTab1))
        // specified table name doesn't exist in the source document.
        return ScExternalRefCache::TokenArrayRef();

    ScRange aRange(rRange);
    SCTAB nTabSpan = aRange.aEnd.Tab() - aRange.aStart.Tab();

    vector<ScExternalRefCache::SingleRangeData> aCacheData;
    aCacheData.reserve(nTabSpan+1);
    aCacheData.push_back(ScExternalRefCache::SingleRangeData());
    aCacheData.back().maTableName = ScGlobal::pCharClass->upper(rTabName);

    for (SCTAB i = 1; i < nTabSpan + 1; ++i)
    {
        String aTabName;
        if (!pSrcDoc->GetName(nTab1 + 1, aTabName))
            // source document doesn't have any table by the specified name.
            break;

        aCacheData.push_back(ScExternalRefCache::SingleRangeData());
        aCacheData.back().maTableName = ScGlobal::pCharClass->upper(aTabName);
    }

    aRange.aStart.SetTab(nTab1);
    aRange.aEnd.SetTab(nTab1 + nTabSpan);

    ScExternalRefCache::TokenArrayRef pArray;
    pArray.reset(lcl_convertToTokenArray(pSrcDoc, aRange, aCacheData));

    if (pArray)
        // Cache these values.
        maRefCache.setCellRangeData(nFileId, rRange, aCacheData, pArray);

    return pArray;
}

ScExternalRefCache::TokenArrayRef ScExternalRefManager::getRangeNameTokens(sal_uInt16 nFileId, const String& rName, const ScAddress* pCurPos)
{
    if (pCurPos)
        insertRefCell(nFileId, *pCurPos);

    maybeLinkExternalFile(nFileId);

    ScExternalRefCache::TokenArrayRef pArray = maRefCache.getRangeNameTokens(nFileId, rName);
    if (pArray.get())
        return pArray;

    ScDocument* pSrcDoc = getSrcDocument(nFileId);
    if (!pSrcDoc)
        return ScExternalRefCache::TokenArrayRef();

    ScRangeName* pExtNames = pSrcDoc->GetRangeName();
    String aUpperName = ScGlobal::pCharClass->upper(rName);
    USHORT n;
    bool bRes = pExtNames->SearchNameUpper(aUpperName, n);
    if (!bRes)
        return ScExternalRefCache::TokenArrayRef();

    ScRangeData* pRangeData = (*pExtNames)[n];
    if (!pRangeData)
        return ScExternalRefCache::TokenArrayRef();

    // Parse all tokens in this external range data, and replace each absolute
    // reference token with an external reference token, and cache them.  Also
    // register the source document with the link manager if it's a new
    // source.

    ScExternalRefCache::TokenArrayRef pNew(new ScTokenArray);

    ScTokenArray* pCode = pRangeData->GetCode();
    for (FormulaToken* pToken = pCode->First(); pToken; pToken = pCode->Next())
    {
        bool bTokenAdded = false;
        switch (pToken->GetType())
        {
            case svSingleRef:
            {
                const ScSingleRefData& rRef = static_cast<ScToken*>(pToken)->GetSingleRef();
                String aTabName;
                pSrcDoc->GetName(rRef.nTab, aTabName);
                ScExternalSingleRefToken aNewToken(nFileId, aTabName, static_cast<ScToken*>(pToken)->GetSingleRef());
                pNew->AddToken(aNewToken);
                bTokenAdded = true;
            }
            break;
            case svDoubleRef:
            {
                const ScSingleRefData& rRef = static_cast<ScToken*>(pToken)->GetSingleRef();
                String aTabName;
                pSrcDoc->GetName(rRef.nTab, aTabName);
                ScExternalDoubleRefToken aNewToken(nFileId, aTabName, static_cast<ScToken*>(pToken)->GetDoubleRef());
                pNew->AddToken(aNewToken);
                bTokenAdded = true;
            }
            break;
            default:
                ;   // nothing
        }

        if (!bTokenAdded)
            pNew->AddToken(*pToken);
    }

    // Make sure to pass the correctly-cased range name here.
    maRefCache.setRangeNameTokens(nFileId, pRangeData->GetName(), pNew);
    return pNew;
}

void ScExternalRefManager::refreshAllRefCells(sal_uInt16 nFileId)
{
    RefCellMap::iterator itrFile = maRefCells.find(nFileId);
    if (itrFile == maRefCells.end())
        return;

    RefCells& rRefCells = itrFile->second;
    rRefCells.refreshAllCells(*this);

    ScViewData* pViewData = ScDocShell::GetViewData();
    if (!pViewData)
        return;

    ScTabViewShell* pVShell = pViewData->GetViewShell();
    if (!pVShell)
        return;

    // Repainting the grid also repaints the texts, but is there a better way
    // to refresh texts?
    pVShell->Invalidate(FID_REPAINT);
    pVShell->PaintGrid();
}

void ScExternalRefManager::insertRefCell(sal_uInt16 nFileId, const ScAddress& rCell)
{
    RefCellMap::iterator itr = maRefCells.find(nFileId);
    if (itr == maRefCells.end())
    {
        RefCells aRefCells;
        pair<RefCellMap::iterator, bool> r = maRefCells.insert(
            RefCellMap::value_type(nFileId, aRefCells));
        if (!r.second)
            // insertion failed.
            return;

        itr = r.first;
    }
    itr->second.insertCell(rCell);
}

ScDocument* ScExternalRefManager::getSrcDocument(sal_uInt16 nFileId)
{
    if (!mpDoc->IsExecuteLinkEnabled())
        return NULL;

    DocShellMap::iterator itrEnd = maDocShells.end();
    DocShellMap::iterator itr = maDocShells.find(nFileId);

    if (itr != itrEnd)
    {
        SfxObjectShell* p = itr->second.maShell;
        itr->second.maLastAccess = Time();
        return static_cast<ScDocShell*>(p)->GetDocument();
    }

    const String* pFile = getExternalFileName(nFileId);
    if (!pFile)
        // no file name associated with this ID.
        return NULL;

    String aFilter;
    SrcShell aSrcDoc;
    aSrcDoc.maShell = loadSrcDocument(nFileId, aFilter);
    if (!aSrcDoc.maShell.Is())
    {
        // source document could not be loaded.
        return NULL;
    }

    if (maDocShells.empty())
    {
        // If this is the first source document insertion, start up the timer.
        maSrcDocTimer.Start();
    }

    maDocShells.insert(DocShellMap::value_type(nFileId, aSrcDoc));
    SfxObjectShell* p = aSrcDoc.maShell;
    ScDocument* pSrcDoc = static_cast<ScDocShell*>(p)->GetDocument();

    SCTAB nTabCount = pSrcDoc->GetTableCount();
    if (!maRefCache.isDocInitialized(nFileId) && nTabCount)
    {
        // Populate the cache with all table names in the source document.
        vector<String> aTabNames;
        aTabNames.reserve(nTabCount);
        for (SCTAB i = 0; i < nTabCount; ++i)
        {
            String aName;
            pSrcDoc->GetName(i, aName);
            aTabNames.push_back(aName);
        }
        maRefCache.initializeDoc(nFileId, aTabNames);
    }
    return pSrcDoc;
}

SfxObjectShellRef ScExternalRefManager::loadSrcDocument(sal_uInt16 nFileId, String& rFilter)
{
    const SrcFileData* pFileData = getExternalFileData(nFileId);
    if (!pFileData)
        return NULL;

    // Always load the document by using the path created from the relative
    // path.  If the referenced document is not there, simply exit.  The
    // original file name should be used only when the relative path is not
    // given.
    String aFile = pFileData->maFileName;
    maybeCreateRealFileName(nFileId);
    if (pFileData->maRealFileName.Len())
        aFile = pFileData->maRealFileName;
    
    if (!isFileLoadable(aFile))
        return NULL;

    String aOptions;
    ScDocumentLoader::GetFilterName(aFile, rFilter, aOptions, true, false);
    const SfxFilter* pFilter = ScDocShell::Factory().GetFilterContainer()->GetFilter4FilterName(rFilter);

    if (!pFileData->maRelativeName.Len())
    {
        // Generate a relative file path.
        INetURLObject aBaseURL(getOwnDocumentName());
        aBaseURL.insertName(OUString::createFromAscii("content.xml"));

        String aStr = URIHelper::simpleNormalizedMakeRelative(
            aBaseURL.GetMainURL(INetURLObject::NO_DECODE), aFile);

        setRelativeFileName(nFileId, aStr);
    }

    // Update the filter data now that we are loading it again.
    setFilterData(nFileId, rFilter, aOptions);

    SfxItemSet* pSet = new SfxAllItemSet(SFX_APP()->GetPool());
    if (aOptions.Len())
        pSet->Put(SfxStringItem(SID_FILE_FILTEROPTIONS, aOptions));

    auto_ptr<SfxMedium> pMedium(new SfxMedium(aFile, STREAM_STD_READ, false, pFilter, pSet));
    if (pMedium->GetError() != ERRCODE_NONE)
        return NULL;

    pMedium->UseInteractionHandler(false);

    ScDocShell* pNewShell = new ScDocShell(SFX_CREATE_MODE_INTERNAL);
    SfxObjectShellRef aRef = pNewShell;

    // increment the recursive link count of the source document.
    ScExtDocOptions* pExtOpt = mpDoc->GetExtDocOptions();
    sal_uInt32 nLinkCount = pExtOpt ? pExtOpt->GetDocSettings().mnLinkCnt : 0;
    ScDocument* pSrcDoc = pNewShell->GetDocument();
    ScExtDocOptions* pExtOptNew = pSrcDoc->GetExtDocOptions();
    if (!pExtOptNew)
    {
        pExtOptNew = new ScExtDocOptions;
        pSrcDoc->SetExtDocOptions(pExtOptNew);
    }
    pExtOptNew->GetDocSettings().mnLinkCnt = nLinkCount + 1;

    pNewShell->DoLoad(pMedium.release());
    return aRef;
}

bool ScExternalRefManager::isFileLoadable(const String& rFile) const
{
    if (!rFile.Len())
        return false;

    if (isOwnDocument(rFile))
        return false;

    if (utl::UCBContentHelper::IsFolder(rFile))
        return false;

    return utl::UCBContentHelper::Exists(rFile);
}

void ScExternalRefManager::maybeLinkExternalFile(sal_uInt16 nFileId)
{
    if (maLinkedDocs.count(nFileId))
        // file alerady linked, or the link has been broken.
        return;

    // Source document not linked yet.  Link it now.
    const String* pFileName = getExternalFileName(nFileId);
    if (!pFileName)
        return;

    String aFilter, aOptions;
    ScDocumentLoader::GetFilterName(*pFileName, aFilter, aOptions, true, false);
    SvxLinkManager* pLinkMgr = mpDoc->GetLinkManager();
    ScExternalRefLink* pLink = new ScExternalRefLink(mpDoc, nFileId, aFilter);
    DBG_ASSERT(pFileName, "ScExternalRefManager::insertExternalFileLink: file name pointer is NULL");
    pLinkMgr->InsertFileLink(*pLink, OBJECT_CLIENT_FILE, *pFileName, &aFilter);

    pLink->SetDoReferesh(false);
    pLink->Update();
    pLink->SetDoReferesh(true);

    maLinkedDocs.insert(LinkedDocMap::value_type(nFileId, true));
}

void ScExternalRefManager::SrcFileData::maybeCreateRealFileName(const String& rOwnDocName)
{
    if (!maRelativeName.Len())
        // No relative path given.  Nothing to do.
        return;

    if (maRealFileName.Len())
        // Real file name already created.  Nothing to do.
        return;

    // Formulate the absolute file path from the relative path.
    const String& rRelPath = maRelativeName;
    INetURLObject aBaseURL(rOwnDocName);
    aBaseURL.insertName(OUString::createFromAscii("content.xml"));
    bool bWasAbs = false;
    maRealFileName = aBaseURL.smartRel2Abs(rRelPath, bWasAbs).GetMainURL(INetURLObject::NO_DECODE);
}

void ScExternalRefManager::maybeCreateRealFileName(sal_uInt16 nFileId)
{
    if (nFileId >= maSrcFiles.size())
        return;

    maSrcFiles[nFileId].maybeCreateRealFileName(getOwnDocumentName());
}

bool ScExternalRefManager::compileTokensByCell(const ScAddress& rCell)
{
    ScBaseCell* pCell;
    mpDoc->GetCell(rCell.Col(), rCell.Row(), rCell.Tab(), pCell);

    if (!pCell || pCell->GetCellType() != CELLTYPE_FORMULA)
        return false;

    ScFormulaCell* pFC = static_cast<ScFormulaCell*>(pCell);

    // Check to make sure the cell really contains ocExternalRef.
    // External names, external cell and range references all have a
    // ocExternalRef token.
    const ScTokenArray* pCode = pFC->GetCode();
    if (!pCode->HasOpCode( ocExternalRef))
        return false;

    ScTokenArray* pArray = pFC->GetCode();
    if (pArray)
        // Clear the error code, or a cell with error won't get re-compiled.
        pArray->SetCodeError(0);

    pFC->SetCompile(true);
    pFC->CompileTokenArray();
    pFC->SetDirty();

    return true;
}

const String& ScExternalRefManager::getOwnDocumentName() const
{
    SfxObjectShell* pShell = mpDoc->GetDocumentShell();
    if (!pShell)
        // This should not happen!
        return EMPTY_STRING;

    SfxMedium* pMed = pShell->GetMedium();
    if (!pMed)
        return EMPTY_STRING;

    return pMed->GetName();
}

bool ScExternalRefManager::isOwnDocument(const String& rFile) const
{
    return getOwnDocumentName().Equals(rFile);
}

void ScExternalRefManager::convertToAbsName(String& rFile) const
{
    SfxObjectShell* pDocShell = mpDoc->GetDocumentShell();
    rFile = ScGlobal::GetAbsDocName(rFile, pDocShell);
}

sal_uInt16 ScExternalRefManager::getExternalFileId(const String& rFile)
{
    vector<SrcFileData>::const_iterator itrBeg = maSrcFiles.begin(), itrEnd = maSrcFiles.end();
    vector<SrcFileData>::const_iterator itr = find_if(itrBeg, itrEnd, FindSrcFileByName(rFile));
    if (itr != itrEnd)
    {
        size_t nId = distance(itrBeg, itr);
        return static_cast<sal_uInt16>(nId);
    }

    SrcFileData aData;
    aData.maFileName = rFile;
    maSrcFiles.push_back(aData);
    return static_cast<sal_uInt16>(maSrcFiles.size() - 1);
}

const String* ScExternalRefManager::getExternalFileName(sal_uInt16 nFileId, bool bForceOriginal)
{
    if (nFileId >= maSrcFiles.size())
        return NULL;

    if (bForceOriginal)
        return &maSrcFiles[nFileId].maFileName;

    maybeCreateRealFileName(nFileId);

    if (maSrcFiles[nFileId].maRealFileName.Len())
        return &maSrcFiles[nFileId].maRealFileName;
    else
        return &maSrcFiles[nFileId].maFileName;
}

bool ScExternalRefManager::hasExternalFile(sal_uInt16 nFileId) const
{
    return nFileId < maSrcFiles.size();
}

bool ScExternalRefManager::hasExternalFile(const String& rFile) const
{
    vector<SrcFileData>::const_iterator itrBeg = maSrcFiles.begin(), itrEnd = maSrcFiles.end();
    vector<SrcFileData>::const_iterator itr = find_if(itrBeg, itrEnd, FindSrcFileByName(rFile));
    return itr != itrEnd;
}

const ScExternalRefManager::SrcFileData* ScExternalRefManager::getExternalFileData(sal_uInt16 nFileId) const
{
    if (nFileId >= maSrcFiles.size())
        return NULL;

    return &maSrcFiles[nFileId];
}

const String* ScExternalRefManager::getRealTableName(sal_uInt16 nFileId, const String& rTabName) const
{
    return maRefCache.getRealTableName(nFileId, rTabName);
}

const String* ScExternalRefManager::getRealRangeName(sal_uInt16 nFileId, const String& rRangeName) const
{
    return maRefCache.getRealRangeName(nFileId, rRangeName);
}

template<typename MapContainer>
void lcl_removeByFileId(sal_uInt16 nFileId, MapContainer& rMap)
{
    typename MapContainer::iterator itr = rMap.find(nFileId);
    if (itr != rMap.end())
        rMap.erase(itr);
}

void ScExternalRefManager::refreshNames(sal_uInt16 nFileId)
{
    maRefCache.clearCache(nFileId);
    lcl_removeByFileId(nFileId, maDocShells);

    if (maDocShells.empty())
        maSrcDocTimer.Stop();

    // Update all cells containing names from this source document.
    refreshAllRefCells(nFileId);

    notifyAllLinkListeners(nFileId, LINK_MODIFIED);
}

void ScExternalRefManager::breakLink(sal_uInt16 nFileId)
{
    lcl_removeByFileId(nFileId, maDocShells);

    if (maDocShells.empty())
        maSrcDocTimer.Stop();

    LinkedDocMap::iterator itr = maLinkedDocs.find(nFileId);
    if (itr != maLinkedDocs.end())
        itr->second = false;

    notifyAllLinkListeners(nFileId, LINK_BROKEN);
}

void ScExternalRefManager::switchSrcFile(sal_uInt16 nFileId, const String& rNewFile, const String& rNewFilter)
{
    maSrcFiles[nFileId].maFileName = rNewFile;
    maSrcFiles[nFileId].maRelativeName.Erase();
    maSrcFiles[nFileId].maRealFileName.Erase();
    if (!maSrcFiles[nFileId].maFilterName.Equals(rNewFilter))
    {
        // Filter type has changed.
        maSrcFiles[nFileId].maFilterName = rNewFilter;
        maSrcFiles[nFileId].maFilterOptions.Erase();
    }
    refreshNames(nFileId);
}

void ScExternalRefManager::setRelativeFileName(sal_uInt16 nFileId, const String& rRelUrl)
{
    if (nFileId >= maSrcFiles.size())
        return;
    maSrcFiles[nFileId].maRelativeName = rRelUrl;
}

void ScExternalRefManager::setFilterData(sal_uInt16 nFileId, const String& rFilterName, const String& rOptions)
{
    if (nFileId >= maSrcFiles.size())
        return;
    maSrcFiles[nFileId].maFilterName = rFilterName;
    maSrcFiles[nFileId].maFilterOptions = rOptions;
}

void ScExternalRefManager::clear()
{
    DocShellMap::iterator itrEnd = maDocShells.end();
    for (DocShellMap::iterator itr = maDocShells.begin(); itr != itrEnd; ++itr)
        itr->second.maShell->DoClose();

    maDocShells.clear();
    maSrcDocTimer.Stop();
}

bool ScExternalRefManager::hasExternalData() const
{
    return !maSrcFiles.empty();
}

void ScExternalRefManager::resetSrcFileData(const String& rBaseFileUrl)
{
    for (vector<SrcFileData>::iterator itr = maSrcFiles.begin(), itrEnd = maSrcFiles.end();
          itr != itrEnd; ++itr)
    {
        // Re-generate relative file name from the absolute file name.
        String aAbsName = itr->maRealFileName;
        if (!aAbsName.Len())
            aAbsName = itr->maFileName;

        itr->maRelativeName = URIHelper::simpleNormalizedMakeRelative(
            rBaseFileUrl, aAbsName);
    }
}

void ScExternalRefManager::updateRefCell(const ScAddress& rOldPos, const ScAddress& rNewPos, bool bCopy)
{
    for (RefCellMap::iterator itr = maRefCells.begin(), itrEnd = maRefCells.end(); itr != itrEnd; ++itr)
    {
        if (!bCopy)
            itr->second.removeCell(rOldPos);
        itr->second.insertCell(rNewPos);
    }
}

void ScExternalRefManager::updateRefMoveTable(SCTAB nOldTab, SCTAB nNewTab, bool bCopy)
{
    for (RefCellMap::iterator itr = maRefCells.begin(), itrEnd = maRefCells.end(); itr != itrEnd; ++itr)
        itr->second.moveTable(nOldTab, nNewTab, bCopy);
}

void ScExternalRefManager::updateRefInsertTable(SCTAB nPos)
{
    for (RefCellMap::iterator itr = maRefCells.begin(), itrEnd = maRefCells.end(); itr != itrEnd; ++itr)
        itr->second.insertTable(nPos);
}

void ScExternalRefManager::updateRefDeleteTable(SCTAB nPos)
{
    for (RefCellMap::iterator itr = maRefCells.begin(), itrEnd = maRefCells.end(); itr != itrEnd; ++itr)
        itr->second.removeTable(nPos);
}

void ScExternalRefManager::addLinkListener(sal_uInt16 nFileId, LinkListener* pListener)
{
    LinkListenerMap::iterator itr = maLinkListeners.find(nFileId);
    if (itr == maLinkListeners.end())
    {
        pair<LinkListenerMap::iterator, bool> r = maLinkListeners.insert(
            LinkListenerMap::value_type(nFileId, LinkListeners()));
        if (!r.second)
        {
            DBG_ERROR("insertion of new link listener list failed");
            return;
        }

        itr = r.first;
    }

    LinkListeners& rList = itr->second;
    rList.insert(pListener);
}

void ScExternalRefManager::removeLinkListener(sal_uInt16 nFileId, LinkListener* pListener)
{
    LinkListenerMap::iterator itr = maLinkListeners.find(nFileId);
    if (itr == maLinkListeners.end())
        // no listeners for a specified file.
        return;

    LinkListeners& rList = itr->second;
    rList.erase(pListener);

    if (rList.empty())
        // No more listeners for this file.  Remove its entry.
        maLinkListeners.erase(itr);
}

void ScExternalRefManager::removeLinkListener(LinkListener* pListener)
{
    LinkListenerMap::iterator itr = maLinkListeners.begin(), itrEnd = maLinkListeners.end();
    for (; itr != itrEnd; ++itr)
        itr->second.erase(pListener);
}

void ScExternalRefManager::notifyAllLinkListeners(sal_uInt16 nFileId, LinkUpdateType eType)
{
    LinkListenerMap::iterator itr = maLinkListeners.find(nFileId);
    if (itr == maLinkListeners.end())
        // no listeners for a specified file.
        return;

    LinkListeners& rList = itr->second;
    for_each(rList.begin(), rList.end(), NotifyLinkListener(nFileId, eType));
}

void ScExternalRefManager::purgeStaleSrcDocument(sal_Int32 nTimeOut)
{
    DocShellMap aNewDocShells;
    DocShellMap::iterator itr = maDocShells.begin(), itrEnd = maDocShells.end();
    for (; itr != itrEnd; ++itr)
    {
        // in 100th of a second.
        sal_Int32 nSinceLastAccess = (Time() - itr->second.maLastAccess).GetTime();
        if (nSinceLastAccess < nTimeOut)
            aNewDocShells.insert(*itr);
    }
    maDocShells.swap(aNewDocShells);

    if (maDocShells.empty())
        maSrcDocTimer.Stop();
}

sal_uInt32 ScExternalRefManager::getMappedNumberFormat(sal_uInt16 nFileId, sal_uInt32 nNumFmt, ScDocument* pSrcDoc)
{
    NumFmtMap::iterator itr = maNumFormatMap.find(nFileId);
    if (itr == maNumFormatMap.end())
    {
        // Number formatter map is not initialized for this external document.
        pair<NumFmtMap::iterator, bool> r = maNumFormatMap.insert(
            NumFmtMap::value_type(nFileId, SvNumberFormatterMergeMap()));

        if (!r.second)
            // insertion failed.
            return nNumFmt;

        itr = r.first;
        mpDoc->GetFormatTable()->MergeFormatter( *pSrcDoc->GetFormatTable());
        SvNumberFormatterMergeMap aMap = mpDoc->GetFormatTable()->ConvertMergeTableToMap();
        itr->second.swap(aMap);
    }
    const SvNumberFormatterMergeMap& rMap = itr->second;
    SvNumberFormatterMergeMap::const_iterator itrNumFmt = rMap.find(nNumFmt);
    if (itrNumFmt != rMap.end())
        // mapped value found.
        return itrNumFmt->second;

    return nNumFmt;
}

IMPL_LINK(ScExternalRefManager, TimeOutHdl, AutoTimer*, pTimer)
{
    if (pTimer == &maSrcDocTimer)
        purgeStaleSrcDocument(SRCDOC_LIFE_SPAN);

    return 0;
}

