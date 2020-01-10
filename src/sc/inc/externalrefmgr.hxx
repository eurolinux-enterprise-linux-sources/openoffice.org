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

#ifndef SC_EXTERNALREFMGR_HXX
#define SC_EXTERNALREFMGR_HXX

#include "global.hxx"
#include "address.hxx"
#include "sfx2/objsh.hxx"
#include "sfx2/lnkbase.hxx"
#include "tools/time.hxx"
#include "vcl/timer.hxx"
#include "svtools/zforlist.hxx"
#include "scmatrix.hxx"

#include <hash_map>
#include <hash_set>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>
#include <formula/ExternalReferenceHelper.hxx>

class ScDocument;
namespace formula
{
    class FormulaToken;
}
class ScToken;
class ScMatrix;
class ScTokenArray;
class String;
class SfxObjectShellRef;
class Window;

class ScExternalRefCache;

class ScExternalRefLink : public ::sfx2::SvBaseLink
{
public:
    ScExternalRefLink(ScDocument* pDoc, sal_uInt16 nFileId, const String& rFilter);
    virtual ~ScExternalRefLink();

    virtual void Closed();
    virtual void DataChanged(const String& rMimeType, const ::com::sun::star::uno::Any & rValue);
    virtual void Edit(Window* pParent, const Link& rEndEditHdl);

    void SetDoReferesh(bool b);

private:
    ScExternalRefLink(); // disabled
    ScExternalRefLink(const ScExternalRefLink&); // disabled

    DECL_LINK( ExternalRefEndEditHdl, ::sfx2::SvBaseLink* );

    sal_uInt16  mnFileId;
    String      maFilterName;
    ScDocument* mpDoc;
    bool        mbDoRefresh;
};

// ============================================================================

/**
 * Cache table for external reference data.
 */
class ScExternalRefCache
{
public:
    typedef ::boost::shared_ptr< formula::FormulaToken>     TokenRef;
    typedef ::boost::shared_ptr<ScTokenArray>               TokenArrayRef;

    struct TableName
    {
        String maUpperName;
        String maRealName;

        explicit TableName(const String& rUppper, const String& rReal);
    };

    struct CellFormat
    {
        bool        mbIsSet;
        short       mnType;
        sal_uInt32  mnIndex;

        explicit CellFormat();
    };

private:
    /** individual cell within cached external ref table. */
    struct Cell
    {
        TokenRef    mxToken;
        sal_uInt32  mnFmtIndex;
    };
    typedef ::std::hash_map<SCCOL, Cell>            RowDataType;
    typedef ::std::hash_map<SCROW, RowDataType>     RowsDataType;

public:
    // SUNWS needs a forward declared friend, otherwise types and members
    // of the outer class are not accessible.
    class Table;
    friend class ScExternalRefCache::Table;

    class Table
    {
    public:

        enum ReferencedFlag
        {
            UNREFERENCED,
            REFERENCED_MARKED,      // marked as referenced during store to file
            REFERENCED_PERMANENT    // permanently marked, e.g. from within interpreter
        };

        Table();
        ~Table();

        SC_DLLPUBLIC void setCell(SCCOL nCol, SCROW nRow, TokenRef pToken, sal_uInt32 nFmtIndex = 0);
        TokenRef getCell(SCCOL nCol, SCROW nRow, sal_uInt32* pnFmtIndex = NULL) const;
        bool hasRow( SCROW nRow ) const;
        /** Set/clear referenced status flag only if current status is not 
            REFERENCED_PERMANENT. */
        void setReferenced( bool bReferenced );
        /// Unconditionally set the reference status flag.
        void setReferencedFlag( ReferencedFlag eFlag );
        ReferencedFlag getReferencedFlag() const;
        bool isReferenced() const;
        /// Obtain a sorted vector of rows.
        void getAllRows(::std::vector<SCROW>& rRows) const;
        /// Obtain a sorted vector of columns.
        void getAllCols(SCROW nRow, ::std::vector<SCCOL>& rCols) const;
        void getAllNumberFormats(::std::vector<sal_uInt32>& rNumFmts) const;

    private:
        RowsDataType   maRows;
        ReferencedFlag meReferenced;
    };

    typedef ::boost::shared_ptr<Table>      TableTypeRef;
    typedef ::std::hash_map<String, size_t, ScStringHashCode>   TableNameIndexMap;

    ScExternalRefCache();
    ~ScExternalRefCache();

    const String* getRealTableName(sal_uInt16 nFileId, const String& rTabName) const;
    const String* getRealRangeName(sal_uInt16 nFileId, const String& rRangeName) const;

    /**
     * Get a cached cell data at specified cell location.
     *
     * @param nFileId file ID of an external document
     * @param rTabName sheet name
     * @param nCol
     * @param nRow
     *
     * @return pointer to the token instance in the cache. 
     */
    ScExternalRefCache::TokenRef getCellData(
        sal_uInt16 nFileId, const String& rTabName, SCCOL nCol, SCROW nRow,
        bool bEmptyCellOnNull, bool bWriteEmpty, sal_uInt32* pnFmtIndex);

    /**
     * Get a cached cell range data.
     *
     * @return a new token array instance.  Note that <i>the caller must
     *         manage the life cycle of the returned instance</i>, which is
     *         guaranteed if the TokenArrayRef is properly used..
     */
    ScExternalRefCache::TokenArrayRef getCellRangeData(
        sal_uInt16 nFileId, const String& rTabName, const ScRange& rRange, bool bEmptyCellOnNull, bool bWriteEmpty);

    ScExternalRefCache::TokenArrayRef getRangeNameTokens(sal_uInt16 nFileId, const String& rName);
    void setRangeNameTokens(sal_uInt16 nFileId, const String& rName, TokenArrayRef pArray);

    void setCellData(sal_uInt16 nFileId, const String& rTabName, SCROW nRow, SCCOL nCol, TokenRef pToken, sal_uInt32 nFmtIndex);

    struct SingleRangeData
    {
        /** This name must be in upper-case. */
        String      maTableName;
        ScMatrixRef mpRangeData;
    };
    void setCellRangeData(sal_uInt16 nFileId, const ScRange& rRange, const ::std::vector<SingleRangeData>& rData,
                          TokenArrayRef pArray);

    bool isDocInitialized(sal_uInt16 nFileId);
    void initializeDoc(sal_uInt16 nFileId, const ::std::vector<String>& rTabNames);
    String getTableName(sal_uInt16 nFileId, size_t nCacheId) const;
    void getAllTableNames(sal_uInt16 nFileId, ::std::vector<String>& rTabNames) const;
    SCsTAB getTabSpan( sal_uInt16 nFileId, const String& rStartTabName, const String& rEndTabName ) const;
    void getAllNumberFormats(::std::vector<sal_uInt32>& rNumFmts) const;
    bool hasCacheTable(sal_uInt16 nFileId, const String& rTabName) const;
    size_t getCacheTableCount(sal_uInt16 nFileId) const;

    /**
     * Set all tables of a document as referenced, used only during 
     * store-to-file.
     * @returns <TRUE/> if ALL tables of ALL documents are marked.
     */
    bool setCacheDocReferenced( sal_uInt16 nFileId );

    /**
     * Set a table as referenced, used only during store-to-file.
     * @returns <TRUE/> if ALL tables of ALL documents are marked.
     */
    bool setCacheTableReferenced( sal_uInt16 nFileId, const String& rTabName, size_t nSheets, bool bPermanent );
    void setAllCacheTableReferencedStati( bool bReferenced );
    bool areAllCacheTablesReferenced() const;

    /**
     * Set a table as permanently referenced, to be called if not in 
     * mark-during-store-to-file cycle.
     */
    void setCacheTableReferencedPermanently( sal_uInt16 nFileId, const String& rTabName, size_t nSheets );

private:
    struct ReferencedStatus
    {
        struct DocReferenced
        {
            ::std::vector<bool> maTables;
            bool                mbAllTablesReferenced;
            // Initially, documents have no tables but all referenced.
            DocReferenced() : mbAllTablesReferenced(true) {}
        };
        typedef ::std::vector<DocReferenced> DocReferencedVec;

        DocReferencedVec maDocs;
        bool             mbAllReferenced;

                    ReferencedStatus();
        explicit    ReferencedStatus( size_t nDocs );
        void        reset( size_t nDocs );
        void        checkAllDocs();

    } maReferenced;
    void addCacheTableToReferenced( sal_uInt16 nFileId, size_t nIndex );
    void addCacheDocToReferenced( sal_uInt16 nFileId );
public:

    ScExternalRefCache::TableTypeRef getCacheTable(sal_uInt16 nFileId, size_t nTabIndex) const;
    ScExternalRefCache::TableTypeRef getCacheTable(sal_uInt16 nFileId, const String& rTabName, bool bCreateNew, size_t* pnIndex);

    void clearCache(sal_uInt16 nFileId);

private:
    struct RangeHash
    {
        size_t operator()(const ScRange& rRange) const
        {
            const ScAddress& s = rRange.aStart;
            const ScAddress& e = rRange.aEnd;
            return s.Tab() + s.Col() + s.Row() + e.Tab() + e.Col() + e.Row();
        }
    };

    typedef ::std::hash_map<String, TokenArrayRef, ScStringHashCode>    RangeNameMap;
    typedef ::std::hash_map<ScRange, TokenArrayRef, RangeHash>          RangeArrayMap;
    typedef ::std::hash_map<String, String, ScStringHashCode>           NamePairMap;

    // SUNWS needs a forward declared friend, otherwise types and members
    // of the outer class are not accessible.
    struct DocItem;
    friend struct ScExternalRefCache::DocItem;

    /** Represents data cached for a single external document. */
    struct DocItem
    {
        /** The raw cache tables. */
        ::std::vector<TableTypeRef> maTables;
        /** Table name list in correct order, in both upper- and real-case. */
        ::std::vector<TableName>    maTableNames;
        /** Table name to index map.  The names must be stored upper-case. */
        TableNameIndexMap           maTableNameIndex;
        /** Range name cache. */
        RangeNameMap                maRangeNames;
        /** Token array cache for cell ranges. */
        RangeArrayMap               maRangeArrays;
        /** Upper- to real-case mapping for range names. */
        NamePairMap                 maRealRangeNameMap;

        bool mbInitFromSource;

        DocItem() : mbInitFromSource(false) {}
    };
    typedef ::std::hash_map<sal_uInt16, DocItem>  DocDataType;
    DocItem* getDocItem(sal_uInt16 nFileId) const;

private:
    mutable DocDataType maDocs;
};

// ============================================================================

class SC_DLLPUBLIC ScExternalRefManager : public formula::ExternalReferenceHelper
{
public:

    // SUNWS needs a forward declared friend, otherwise types and members
    // of the outer class are not accessible.
    class RefCells;
    friend class ScExternalRefManager::RefCells;

    /** 
     *  Collection of cell addresses that contain external references. This
     *  data is used for link updates.
     */
    class RefCells
    {
    public:
        RefCells();
        ~RefCells();

        void insertCell(const ScAddress& rAddr);
        void removeCell(const ScAddress& rAddr);
        void moveTable(SCTAB nOldTab, SCTAB nNewTab, bool bCopy);
        void insertTable(SCTAB nPos);
        void removeTable(SCTAB nPos);
        void refreshAllCells(ScExternalRefManager& rRefMgr);
    private:

        typedef ::std::hash_set<SCROW>              RowSet;
        typedef ::std::hash_map<SCCOL, RowSet>      ColSet;

        // SUNWS needs a forward declared friend, otherwise types and members
        // of the outer class are not accessible.
        struct TabItem;
        friend struct ScExternalRefManager::RefCells::TabItem;

        struct TabItem
        {
            SCTAB       mnIndex;
            ColSet      maCols;
            explicit TabItem(SCTAB nIndex);
            explicit TabItem(const TabItem& r);
        };
        typedef ::boost::shared_ptr<TabItem>        TabItemRef;

        /** 
         * Return the position that points either to the specified table 
         * position or to the position where a new table would be inserted in 
         * case the specified table is not present.
         *  
         * @param nTab index of the desired table 
         */
        ::std::list<TabItemRef>::iterator getTabPos(SCTAB nTab);

        // This list must be sorted by the table index at all times.
        ::std::list<TabItemRef> maTables;
    };

    enum LinkUpdateType { LINK_MODIFIED, LINK_BROKEN };

    /** 
     * Base class for objects that need to listen to link updates.  When a
     * link to a certain external file is updated, the notify() method gets
     * called.
     */
    class LinkListener
    {
    public:
        LinkListener();
        virtual ~LinkListener() = 0;
        virtual void notify(sal_uInt16 nFileId, LinkUpdateType eType) = 0;

        struct Hash
        {
            size_t operator() (const LinkListener* p) const
            {
                return reinterpret_cast<size_t>(p);
            }
        };
    };

private:
    /** Shell instance for a source document. */
    struct SrcShell
    {
        SfxObjectShellRef   maShell;
        Time                maLastAccess;
    };

    typedef ::std::hash_map<sal_uInt16, SrcShell>           DocShellMap;
    typedef ::std::hash_map<sal_uInt16, bool>               LinkedDocMap;

    typedef ::std::hash_map<sal_uInt16, RefCells>           RefCellMap;
    typedef ::std::hash_map<sal_uInt16, SvNumberFormatterMergeMap> NumFmtMap;


    typedef ::std::hash_set<LinkListener*, LinkListener::Hash>  LinkListeners;
    typedef ::std::hash_map<sal_uInt16, LinkListeners>          LinkListenerMap;

public:
    /** Source document meta-data container. */
    struct SrcFileData
    {
        String maFileName;      /// original file name as loaded from the file.
        String maRealFileName;  /// file name created from the relative name.
        String maRelativeName;
        String maFilterName;
        String maFilterOptions;

        void maybeCreateRealFileName(const String& rOwnDocName);
    };

public:
    explicit ScExternalRefManager(ScDocument* pDoc);
    virtual ~ScExternalRefManager();

    virtual String getCacheTableName(sal_uInt16 nFileId, size_t nTabIndex) const;

    /**
     * Get a cache table instance for specified table and table index.  Unlike
     * the other method that takes a table name, this method does not create a
     * new table when a table is not available for specified index.
     *
     * @param nFileId file ID
     * @param nTabIndex cache table index
     *
     * @return shared_ptr to the cache table instance
     */
    ScExternalRefCache::TableTypeRef getCacheTable(sal_uInt16 nFileId, size_t nTabIndex) const;

    /**
     * Get a cache table instance for specified file and table name.  If the
     * table instance is not already present, it'll instantiate a new one and
     * append it to the end of the table array.  <I>It's important to be
     * aware of this fact especially for multi-table ranges for which
     * table orders are critical.</I>
     *
     * Excel filter calls this method to populate the cache table from the
     * XCT/CRN records.
     *
     * @param nFileId file ID
     * @param rTabName table name 
     * @param bCreateNew if true, create a new table instance if it's not 
     *                   already present.  If false, it returns NULL if the
     *                   specified table's cache doesn't exist.
     * @param pnIndex if non-NULL pointer is passed, it stores the internal 
     *                index of a cache table instance.
     *
     * @return shared_ptr to the cache table instance
     */
    ScExternalRefCache::TableTypeRef getCacheTable(sal_uInt16 nFileId, const String& rTabName, bool bCreateNew, size_t* pnIndex = 0);
    void getAllCachedTableNames(sal_uInt16 nFileId, ::std::vector<String>& rTabNames) const;

    /**
     * Get the span (distance+sign(distance)) of two sheets of a specified 
     * file.
     *
     * @param nFileId file ID
     * @param rStartTabName name of first sheet (sheet1)
     * @param rEndTabName name of second sheet (sheet2)
     *
     * @return span
     *         1 if sheet2 == sheet1
     *      >  1 if sheet2 > sheet1
     *      < -1 if sheet2 < sheet1
     *        -1 if nFileId or rStartTabName not found
     *         0 if rEndTabName not found
     */
    SCsTAB getCachedTabSpan( sal_uInt16 nFileId, const String& rStartTabName, const String& rEndTabName ) const;

    /** 
     * Get all unique number format indices that are used in the cache tables. 
     * The retrieved indices are sorted in ascending order.
     *
     * @param rNumFmts (reference) all unique number format indices.
     */
    void getAllCachedNumberFormats(::std::vector<sal_uInt32>& rNumFmts) const;

    bool hasCacheTable(sal_uInt16 nFileId, const String& rTabName) const;
    size_t getCacheTableCount(sal_uInt16 nFileId) const;
    sal_uInt16 getExternalFileCount() const;

    /**
     * Mark all tables as referenced that are used by any LinkListener, used 
     * only during store-to-file.
     * @returns <TRUE/> if ALL tables of ALL external documents are marked.
     */
    bool markUsedByLinkListeners();

    /**
     * Set all tables of a document as referenced, used only during 
     * store-to-file.
     * @returns <TRUE/> if ALL tables of ALL external documents are marked.
     */
    bool setCacheDocReferenced( sal_uInt16 nFileId );

    /**
     * Set a table as referenced, used only during store-to-file.
     * @returns <TRUE/> if ALL tables of ALL external documents are marked.
     */
    bool setCacheTableReferenced( sal_uInt16 nFileId, const String& rTabName, size_t nSheets );
    void setAllCacheTableReferencedStati( bool bReferenced );

    /**
     * Set a table as permanently referenced, to be called if not in 
     * mark-during-store-to-file cycle.
     */
    void setCacheTableReferencedPermanently( sal_uInt16 nFileId, const String& rTabName, size_t nSheets );

    /**
     * @returns <TRUE/> if setAllCacheTableReferencedStati(false) was called, 
     * <FALSE/> if setAllCacheTableReferencedStati(true) was called.
     */
    bool isInReferenceMarking() const   { return bInReferenceMarking; }

    void storeRangeNameTokens(sal_uInt16 nFileId, const String& rName, const ScTokenArray& rArray);

    ScExternalRefCache::TokenRef getSingleRefToken(
        sal_uInt16 nFileId, const String& rTabName, const ScAddress& rCell, 
        const ScAddress* pCurPos, SCTAB* pTab, ScExternalRefCache::CellFormat* pFmt = NULL);

    /**
     * Get an array of tokens that consist of the specified external cell
     * range.
     *
     * @param nFileId file ID for an external document
     * @param rTabName referenced sheet name
     * @param rRange referenced cell range
     * @param pCurPos current cursor position to keep track of cells that
     *                reference an external data.
     *
     * @return shared_ptr to a token array instance.  <i>The caller must not
     *         delete the instance returned by this method.</i>
     */
    ScExternalRefCache::TokenArrayRef getDoubleRefTokens(sal_uInt16 nFileId, const String& rTabName, const ScRange& rRange, const ScAddress* pCurPos);

    /**
     * Get an array of tokens corresponding with a specified name in a
     * specified file.
     *
     * @param pCurPos currnet cell address where this name token is used.
     *                This is purely to keep track of all cells containing
     *                external names for refreshing purposes.  If this is
     *                NULL, then the cell will not be added to the list.
     *
     * @return shared_ptr to array of tokens composing the name
     */
    ScExternalRefCache::TokenArrayRef getRangeNameTokens(sal_uInt16 nFileId, const String& rName, const ScAddress* pCurPos = NULL);

    const String& getOwnDocumentName() const;
    bool isOwnDocument(const String& rFile) const;

    /**
     * Takes a flat file name, and convert it to an absolute URL path.  An
     * absolute URL path begines with 'file:///.
     *
     * @param rFile file name to convert
     */
    void convertToAbsName(String& rFile) const;
    sal_uInt16 getExternalFileId(const String& rFile);

    /** 
     * It returns a pointer to the name of the URI associated with a given 
     * external file ID.  In case the original document has moved, it returns 
     * an URI adjusted for the relocation. 
     *
     * @param nFileId file ID for an external document
     * @param bForceOriginal If true, it always returns the original document 
     *                       URI even if the referring document has relocated.
     *                       If false, it returns an URI adjusted for
     *                       relocated document.
     * 
     * @return const String* external document URI.
     */
    const String* getExternalFileName(sal_uInt16 nFileId, bool bForceOriginal = false);
    bool hasExternalFile(sal_uInt16 nFileId) const;
    bool hasExternalFile(const String& rFile) const;
    const SrcFileData* getExternalFileData(sal_uInt16 nFileId) const;

    const String* getRealTableName(sal_uInt16 nFileId, const String& rTabName) const;
    const String* getRealRangeName(sal_uInt16 nFileId, const String& rRangeName) const;
    void refreshNames(sal_uInt16 nFileId);
    void breakLink(sal_uInt16 nFileId);
    void switchSrcFile(sal_uInt16 nFileId, const String& rNewFile, const String& rNewFilter);

    /** 
     * Set a relative file path for the specified file ID.  Note that the 
     * caller must ensure that the passed URL is a valid relative URL. 
     *
     * @param nFileId file ID for an external document
     * @param rRelUrl relative URL
     */
    void setRelativeFileName(sal_uInt16 nFileId, const String& rRelUrl);

    /**
     * Set the filter name and options if any for a given source document.
     * These values get reset when the source document ever gets reloaded.
     *
     * @param nFileId
     * @param rFilterName
     * @param rOptions
     */
    void setFilterData(sal_uInt16 nFileId, const String& rFilterName, const String& rOptions);

    void clear();

    bool hasExternalData() const;

    /**
     * Re-generates relative names for all stored source files.  This is
     * necessary when exporting to an ods document, to ensure that all source
     * files have their respective relative names for xlink:href export. 
     *  
     * @param rBaseFileUrl Absolute URL of the content.xml fragment of the 
     *                     document being exported.
     */
    void resetSrcFileData(const String& rBaseFileUrl);

    /** 
     * Update a single referencing cell position.
     *
     * @param rOldPos old position
     * @param rNewPos new position
     */
    void updateRefCell(const ScAddress& rOldPos, const ScAddress& rNewPos, bool bCopy);

    /** 
     * Update referencing cells affected by sheet movement.
     *
     * @param nOldTab old sheet position
     * @param nNewTab new sheet position
     * @param bCopy whether this is a sheet move (false) or sheet copy (true)
     */
    void updateRefMoveTable(SCTAB nOldTab, SCTAB nNewTab, bool bCopy);

    /** 
     * Update referencing cells affected by sheet insertion.
     *
     * @param nPos sheet insertion position.  All sheets to the right 
     *             including the one at the insertion poistion shift to the
     *             right by one.
     */
    void updateRefInsertTable(SCTAB nPos);

    void updateRefDeleteTable(SCTAB nPos);

    /** 
     * Register a new link listener to a specified external document.  Note 
     * that the caller is responsible for managing the life cycle of the 
     * listener object.
     */
    void addLinkListener(sal_uInt16 nFileId, LinkListener* pListener);

    /** 
     * Remove an existing link listener.  Note that removing a listener
     * pointer here does not delete the listener object instance.
     */
    void removeLinkListener(sal_uInt16 nFileId, LinkListener* pListener);

    void removeLinkListener(LinkListener* pListener);

    /** 
     * Notify all listeners that are listening to a specified external 
     * document. 
     *
     * @param nFileId file ID for an external document.
     */
    void notifyAllLinkListeners(sal_uInt16 nFileId, LinkUpdateType eType);

private:
    ScExternalRefManager();
    ScExternalRefManager(const ScExternalRefManager&);

    void refreshAllRefCells(sal_uInt16 nFileId);

    void insertRefCell(sal_uInt16 nFileId, const ScAddress& rCell);

    ScDocument* getSrcDocument(sal_uInt16 nFileId);
    SfxObjectShellRef loadSrcDocument(sal_uInt16 nFileId, String& rFilter);
    bool isFileLoadable(const String& rFile) const;

    void maybeLinkExternalFile(sal_uInt16 nFileId);

    /** 
     * Try to create a "real" file name from the relative path.  The original
     * file name may not point to the real document when the referencing and 
     * referenced documents have been moved. 
     *  
     * For the real file name to be created, the relative name should not be 
     * empty before calling this method, or the real file name will not be 
     * created. 
     *
     * @param nFileId file ID for an external document
     */
    void maybeCreateRealFileName(sal_uInt16 nFileId);

    bool compileTokensByCell(const ScAddress& rCell);

    /**
     * Purge those source document instances that have not been accessed for
     * the specified duration.
     *
     * @param nTimeOut time out value in 100th of a second
     */
    void purgeStaleSrcDocument(sal_Int32 nTimeOut);

    sal_uInt32 getMappedNumberFormat(sal_uInt16 nFileId, sal_uInt32 nNumFmt, ScDocument* pSrcDoc);

private:
    /** cache of referenced ranges and names from source documents. */
    ScExternalRefCache maRefCache;

    ScDocument* mpDoc;

    /**
     * Source document cache.  This stores the original source document shell
     * instances.  They get purged after a certain period of time.
     */
    DocShellMap maDocShells;

    /** list of source documents that are managed by the link manager. */
    LinkedDocMap maLinkedDocs;

    /**
     * List of referencing cells that may contain external names.  There is
     * one list per source document.
     */
    RefCellMap maRefCells;

    LinkListenerMap maLinkListeners;

    NumFmtMap maNumFormatMap;

    /** original source file index. */
    ::std::vector<SrcFileData> maSrcFiles;

    /** Status whether in reference marking state. See isInReferenceMarking(). */
    bool bInReferenceMarking;

    AutoTimer maSrcDocTimer;
    DECL_LINK(TimeOutHdl, AutoTimer*);
};


#endif
