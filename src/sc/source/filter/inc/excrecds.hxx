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

#ifndef SC_EXCRECDS_HXX
#define SC_EXCRECDS_HXX

#include <tools/solar.h>
#include <svtools/zforlist.hxx>
#include <tools/string.hxx>
#include <vcl/vclenum.hxx>
#include <tools/color.hxx>


#include <vector>
#include "olinetab.hxx"
#include "filter.hxx"
#include "rangelst.hxx"
#include "xerecord.hxx"
#include "xeroot.hxx"
#include "xeformula.hxx"
#include "xestring.hxx"
#include "root.hxx"
#include "excdefs.hxx"
#include "cell.hxx"

//------------------------------------------------------------------ Forwards -

class SvxBorderLine;

class SvStream;
class Font;
class List;
class ScPatternAttr;
class ScTokenArray;
class ScRangeData;
class ScDBData;
class ScEditCell;
class SfxItemSet;
class EditTextObject;
class ScPageHFItem;
class ScProgress;

class ExcTable;

//----------------------------------------------------------- class ExcRecord -

class ExcRecord : public XclExpRecord
{
public:
	virtual void			Save( XclExpStream& rStrm );

	virtual UINT16			GetNum() const = 0;
    virtual sal_Size        GetLen() const = 0;

protected:
	virtual void			SaveCont( XclExpStream& rStrm );

private:
    /** Writes the body of the record. */
    virtual void            WriteBody( XclExpStream& rStrm );
};


//--------------------------------------------------------- class ExcEmptyRec -

class ExcEmptyRec : public ExcRecord
{
private:
protected:
public:
	virtual void			Save( XclExpStream& rStrm );
	virtual UINT16			GetNum() const;
    virtual sal_Size        GetLen() const;
};


//------------------------------------------------------- class ExcRecordList -

class ExcRecordList : protected List, public ExcEmptyRec
{
private:
protected:
public:
	virtual					~ExcRecordList();

    using                   List::Count;

	inline ExcRecord*		First( void )				{ return ( ExcRecord* ) List::First(); }
	inline ExcRecord*		Next( void )				{ return ( ExcRecord* ) List::Next(); }

	inline void				Append( ExcRecord* pNew )	{ if( pNew ) List::Insert( pNew, LIST_APPEND ); }
	inline const ExcRecord*	Get( UINT32 nNum ) const	{ return ( ExcRecord* ) List::GetObject( nNum ); }

	virtual void			Save( XclExpStream& rStrm );
};


//--------------------------------------------------------- class ExcDummyRec -

class ExcDummyRec : public ExcRecord
{
protected:
public:
	virtual void			Save( XclExpStream& rStrm );
	virtual UINT16			GetNum() const;
	virtual	const BYTE*		GetData() const = 0;	// byte data must contain header and body
};


//------------------------------------------------------- class ExcBoolRecord -
// stores BOOL as 16bit val ( 0x0000 | 0x0001 )

class ExcBoolRecord : public ExcRecord
{
private:
	virtual void			SaveCont( XclExpStream& rStrm );

protected:
	BOOL					bVal;

	inline					ExcBoolRecord() : bVal( FALSE ) {}

public:
	inline					ExcBoolRecord( const BOOL bDefault ) : bVal( bDefault )	{}

    virtual sal_Size        GetLen( void ) const;
};


//--------------------------------------------------------- class ExcBof_Base -

class ExcBof_Base : public ExcRecord
{
private:
protected:
	UINT16					nDocType;
	UINT16					nVers;
	UINT16					nRupBuild;
	UINT16					nRupYear;
public:
							ExcBof_Base( void );
};


//-------------------------------------------------------------- class ExcBof -
// Header Record fuer WORKSHEETS

class ExcBof : public ExcBof_Base
{
private:
	virtual void			SaveCont( XclExpStream& rStrm );
public:
							ExcBof( void );

	virtual UINT16			GetNum( void ) const;
    virtual sal_Size        GetLen( void ) const;
};


//------------------------------------------------------------- class ExcBofW -
// Header Record fuer WORKBOOKS

class ExcBofW : public ExcBof_Base
{
private:
	virtual void			SaveCont( XclExpStream& rStrm );
public:
							ExcBofW( void );

	virtual UINT16			GetNum( void ) const;
    virtual sal_Size        GetLen( void ) const;
};


//-------------------------------------------------------------- class ExcEof -

class ExcEof : public ExcRecord
{
private:
public:
	virtual UINT16			GetNum( void ) const;
    virtual sal_Size        GetLen( void ) const;
};


//--------------------------------------------------------- class ExcDummy_00 -
// INTERFACEHDR to FNGROUPCOUNT (see excrecds.cxx)

class ExcDummy_00 : public ExcDummyRec
{
private:
	static const BYTE		pMyData[];
    static const sal_Size   nMyLen;
public:
    virtual sal_Size        GetLen( void ) const;
	virtual	const BYTE*		GetData( void ) const;
};

// EXC_ID_WINDOWPROTECTION
class XclExpWindowProtection : public	XclExpBoolRecord
{
	public:
		XclExpWindowProtection(bool bValue);

    virtual void            SaveXml( XclExpXmlStream& rStrm );
};

// EXC_ID_PROTECT  Document Protection
class XclExpProtection : public	XclExpBoolRecord
{
	public:
		XclExpProtection(bool bValue);
};

class XclExpPassHash : public XclExpRecord
{
public:
    XclExpPassHash(const ::com::sun::star::uno::Sequence<sal_Int8>& aHash);
    virtual ~XclExpPassHash();

private:
    virtual void    WriteBody(XclExpStream& rStrm);

private:
    sal_uInt16  mnHash;
};


//-------------------------------------------------------- class ExcDummy_04x -
// PASSWORD to BOOKBOOL (see excrecds.cxx), no 1904

class ExcDummy_040 : public ExcDummyRec
{
private:
	static const BYTE		pMyData[];
    static const sal_Size   nMyLen;
public:
    virtual sal_Size        GetLen( void ) const;
	virtual	const BYTE*		GetData( void ) const;
};



class ExcDummy_041 : public ExcDummyRec
{
private:
	static const BYTE		pMyData[];
    static const sal_Size   nMyLen;
public:
    virtual sal_Size        GetLen( void ) const;
	virtual	const BYTE*		GetData( void ) const;
};


//------------------------------------------------------------- class Exc1904 -

class Exc1904 : public ExcBoolRecord
{
public:
							Exc1904( ScDocument& rDoc );
	virtual UINT16			GetNum( void ) const;

    virtual void            SaveXml( XclExpXmlStream& rStrm );
};


//------------------------------------------------------ class ExcBundlesheet -

class ExcBundlesheetBase : public ExcRecord
{
protected:
    sal_Size                nStrPos;
    sal_Size                nOwnPos;    // Position NACH # und Len
	UINT16					nGrbit;
    SCTAB                   nTab;

							ExcBundlesheetBase();

public:
							ExcBundlesheetBase( RootData& rRootData, SCTAB nTab );

    inline void             SetStreamPos( sal_Size nNewStrPos ) { nStrPos = nNewStrPos; }
	void					UpdateStreamPos( XclExpStream& rStrm );

	virtual UINT16			GetNum() const;
};



class ExcBundlesheet : public ExcBundlesheetBase
{
private:
	ByteString				aName;

	virtual void			SaveCont( XclExpStream& rStrm );

public:
							ExcBundlesheet( RootData& rRootData, SCTAB nTab );
    virtual sal_Size        GetLen() const;
};

//--------------------------------------------------------- class ExcDummy_02 -
// sheet dummies: CALCMODE to SETUP

class ExcDummy_02a : public ExcDummyRec
{
private:
	static const BYTE		pMyData[];
    static const sal_Size   nMyLen;
public:
    virtual sal_Size        GetLen( void ) const;
	virtual	const BYTE*		GetData( void ) const;
};


// ----------------------------------------------------------------------------

/** This record contains the Windows country IDs for the UI and document language. */
class XclExpCountry : public XclExpRecord
{
public:
    explicit                    XclExpCountry( const XclExpRoot& rRoot );

private:
    sal_uInt16                  mnUICountry;        /// The UI country ID.
    sal_uInt16                  mnDocCountry;       /// The document country ID.

    /** Writes the body of the COUNTRY record. */
    virtual void                WriteBody( XclExpStream& rStrm );
};


// XclExpWsbool ===============================================================

class XclExpWsbool : public XclExpUInt16Record
{
public:
    explicit                    XclExpWsbool( bool bFitToPages, SCTAB nScTab = -1, XclExpFilterManager* pManager = NULL );

    virtual void                SaveXml( XclExpXmlStream& rStrm );
private:
    SCTAB                       mnScTab;
    XclExpFilterManager*        mpManager;
};


// ============================================================================

class XclExpFiltermode : public XclExpEmptyRecord
{
public:
    explicit            XclExpFiltermode();
};

// ----------------------------------------------------------------------------

class XclExpAutofilterinfo : public XclExpUInt16Record
{
public:
    explicit            XclExpAutofilterinfo( const ScAddress& rStartPos, SCCOL nScCol );

    inline const ScAddress GetStartPos() const { return maStartPos; }
    inline SCCOL        GetColCount() const { return static_cast< SCCOL >( GetValue() ); }

private:
    ScAddress           maStartPos;
};

// ----------------------------------------------------------------------------

class ExcFilterCondition
{
private:
	UINT8					nType;
	UINT8					nOper;
	double					fVal;
    XclExpString*           pText;

protected:
public:
							ExcFilterCondition();
							~ExcFilterCondition();

	inline BOOL				IsEmpty() const 	{ return (nType == EXC_AFTYPE_NOTUSED); }
	inline BOOL				HasEqual() const	{ return (nOper == EXC_AFOPER_EQUAL); }
	ULONG					GetTextBytes() const;

	void					SetCondition( UINT8 nTp, UINT8 nOp, double fV, String* pT );

	void					Save( XclExpStream& rStrm );
	void					SaveXml( XclExpXmlStream& rStrm );
	void					SaveText( XclExpStream& rStrm );
};

// ----------------------------------------------------------------------------

class XclExpAutofilter : public XclExpRecord, protected XclExpRoot
{
private:
	UINT16					nCol;
	UINT16					nFlags;
	ExcFilterCondition		aCond[ 2 ];

	BOOL					AddCondition( ScQueryConnect eConn, UINT8 nType,
								UINT8 nOp, double fVal, String* pText,
								BOOL bSimple = FALSE );

    virtual void            WriteBody( XclExpStream& rStrm );

protected:
public:
                            XclExpAutofilter( const XclExpRoot& rRoot, UINT16 nC );

	inline UINT16			GetCol() const			{ return nCol; }
	inline BOOL				HasCondition() const	{ return !aCond[ 0 ].IsEmpty(); }
    inline BOOL             HasTop10() const        { return ::get_flag( nFlags, EXC_AFFLAG_TOP10 ); }

    BOOL                    AddEntry( const ScQueryEntry& rEntry );

    virtual void            SaveXml( XclExpXmlStream& rStrm );
};

// ----------------------------------------------------------------------------

class ExcAutoFilterRecs : public XclExpRecordBase, protected XclExpRoot
{
public:
    explicit            ExcAutoFilterRecs( const XclExpRoot& rRoot, SCTAB nTab );
    virtual             ~ExcAutoFilterRecs();

    void                AddObjRecs();

    virtual void        Save( XclExpStream& rStrm );
    virtual void        SaveXml( XclExpXmlStream& rStrm );

    bool                HasFilterMode() const;

private:
    XclExpAutofilter*   GetByCol( SCCOL nCol ); // always 0-based
    BOOL                IsFiltered( SCCOL nCol );

private:
    typedef XclExpRecordList< XclExpAutofilter >    XclExpAutofilterList;
    typedef XclExpAutofilterList::RecordRefType     XclExpAutofilterRef;

    XclExpAutofilterList maFilterList;
    XclExpFiltermode*   pFilterMode;
    XclExpAutofilterinfo* pFilterInfo;
    ScRange                 maRef;
};

// ----------------------------------------------------------------------------

/** Sheet filter manager. Contains auto filters or advanced filters from all sheets. */
class XclExpFilterManager : protected XclExpRoot
{
public:
    explicit            XclExpFilterManager( const XclExpRoot& rRoot );

    /** Creates the filter records for the specified sheet.
        @descr  Creates and inserts related built-in NAME records. Therefore this
            function is called from the name buffer itself. */
    void                InitTabFilter( SCTAB nScTab );

    /** Returns a record object containing all filter records for the specified sheet. */
    XclExpRecordRef     CreateRecord( SCTAB nScTab );

    /** Returns whether or not FilterMode is present */
    bool                HasFilterMode( SCTAB nScTab );

private:
    using               XclExpRoot::CreateRecord;

    typedef ScfRef< ExcAutoFilterRecs >             XclExpTabFilterRef;
    typedef ::std::map< SCTAB, XclExpTabFilterRef > XclExpTabFilterMap;

    XclExpTabFilterMap  maFilterMap;
};


#endif

