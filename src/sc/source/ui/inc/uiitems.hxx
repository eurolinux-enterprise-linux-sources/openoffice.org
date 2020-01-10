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

#ifndef SC_UIITEMS_HXX
#define SC_UIITEMS_HXX

#include "scdllapi.h"
#include "conditio.hxx"
#include "sortparam.hxx"
#include "paramisc.hxx"
#include <svtools/poolitem.hxx>

class ScEditEngineDefaulter;
class EditTextObject;
class ScViewData;
class ScDPSaveData;

// ---------------------------------------------------------------------------

//  Items

class ScInputStatusItem : public SfxPoolItem
{
	ScAddress           aCursorPos;
	ScAddress           aStartPos;
	ScAddress           aEndPos;
	String              aString;
	EditTextObject*		pEditData;

public:
							TYPEINFO();
//UNUSED2008-05     		ScInputStatusItem( USHORT nWhich,
//UNUSED2008-05     						   SCTAB nTab,
//UNUSED2008-05     						   SCCOL nCol, SCROW nRow,
//UNUSED2008-05     						   SCCOL nStartCol, SCROW nStartRow,
//UNUSED2008-05     						   SCCOL nEndCol,   SCROW nSEndRow,
//UNUSED2008-05     						   const String& rString,
//UNUSED2008-05     						   const EditTextObject* pData );

							ScInputStatusItem( USHORT nWhich,
											   const ScAddress& rCurPos,
											   const ScAddress& rStartPos,
											   const ScAddress& rEndPos,
											   const String& rString,
											   const EditTextObject* pData );
							ScInputStatusItem( const ScInputStatusItem& rItem );
							~ScInputStatusItem();

	virtual String          GetValueText() const;

	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	const ScAddress&        GetPos() const		{ return aCursorPos; }
	const ScAddress&        GetStartPos() const { return aStartPos; }
	const ScAddress&        GetEndPos() const	{ return aEndPos; }
	SCTAB                   GetTab() const      { return aCursorPos.Tab(); }
	SCCOL                   GetCol() const      { return aCursorPos.Col(); }
	SCROW                   GetRow() const      { return aCursorPos.Row(); }
	SCCOL                   GetStartCol() const { return aStartPos.Col(); }
	SCROW                   GetStartRow() const { return aStartPos.Row(); }
	SCCOL                   GetEndCol() const	{ return aEndPos.Col(); }
	SCROW                   GetEndRow() const	{ return aEndPos.Row(); }

	const String&           GetString() const   { return aString; }
	const EditTextObject*	GetEditData() const	{ return pEditData; }
};


#define SC_TAB_INSERTED		1
#define SC_TAB_DELETED		2
#define SC_TAB_MOVED		3
#define SC_TAB_COPIED		4
#define SC_TAB_HIDDEN		5

class ScTablesHint : public SfxHint
{
	USHORT nId;
	SCTAB nTab1;
	SCTAB nTab2;

public:
					TYPEINFO();
					ScTablesHint(USHORT nNewId, SCTAB nTable1, SCTAB nTable2=0);
					~ScTablesHint();

	USHORT			GetId() const			{ return nId; }
	SCTAB			GetTab1() const			{ return nTab1; }
	SCTAB			GetTab2() const			{ return nTab2; }
};

class ScEditViewHint : public SfxHint
{
	ScEditEngineDefaulter*	pEditEngine;
	ScAddress       			aCursorPos;

public:
					TYPEINFO();
					ScEditViewHint( ScEditEngineDefaulter* pEngine, const ScAddress& rCurPos );
					~ScEditViewHint();

	SCCOL           GetCol() const      { return aCursorPos.Col(); }
	SCROW           GetRow() const      { return aCursorPos.Row(); }
	SCTAB           GetTab() const      { return aCursorPos.Tab(); }
	ScEditEngineDefaulter*	GetEngine() const   { return pEditEngine; }

private:
	ScEditViewHint(); // disabled
};

class ScIndexHint : public SfxHint
{
	USHORT nId;
	USHORT nIndex;

public:
					TYPEINFO();
					ScIndexHint(USHORT nNewId, USHORT nIdx);
					~ScIndexHint();

	USHORT			GetId() const			{ return nId; }
	USHORT			GetIndex() const		{ return nIndex; }
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Sortierdialog:

class SC_DLLPUBLIC ScSortItem : public SfxPoolItem
{
public:
							TYPEINFO();
							ScSortItem( USHORT				nWhich,
										ScViewData*			ptrViewData,
										const ScSortParam*	pSortData );
							ScSortItem( USHORT				nWhich,
										const ScSortParam*	pSortData );
							ScSortItem( const ScSortItem& rItem );
							~ScSortItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
    virtual sal_Bool        QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberUd ) const;

	ScViewData*			GetViewData () const { return pViewData; }
	const ScSortParam&	GetSortData	() const { return theSortData; }

private:
	ScViewData* 	pViewData;
	ScSortParam		theSortData;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Filterdialog:

class SC_DLLPUBLIC ScQueryItem : public SfxPoolItem
{
public:
							TYPEINFO();
							ScQueryItem( USHORT					nWhich,
										 ScViewData*			ptrViewData,
										 const ScQueryParam*	pQueryData );
							ScQueryItem( USHORT					nWhich,
										 const ScQueryParam*	pQueryData );
							ScQueryItem( const ScQueryItem& rItem );
							~ScQueryItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	ScViewData*			GetViewData () const { return pViewData; }
	const ScQueryParam&	GetQueryData() const { return theQueryData; }

	BOOL		GetAdvancedQuerySource(ScRange& rSource) const;
	void		SetAdvancedQuerySource(const ScRange* pSource);

private:
	ScViewData* 	pViewData;
	ScQueryParam	theQueryData;
	BOOL			bIsAdvanced;
	ScRange			aAdvSource;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Zwischenergebnisdialog:

class SC_DLLPUBLIC ScSubTotalItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScSubTotalItem( USHORT					nWhich,
								ScViewData*				ptrViewData,
								const ScSubTotalParam*	pSubTotalData );
				ScSubTotalItem( USHORT					nWhich,
								const ScSubTotalParam*	pSubTotalData );
				ScSubTotalItem( const ScSubTotalItem&	rItem );
				~ScSubTotalItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
    virtual sal_Bool        QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberUd ) const;

	ScViewData*				GetViewData () const { return pViewData; }
	const ScSubTotalParam&	GetSubTotalData() const { return theSubTotalData; }

private:
	ScViewData* 	pViewData;
	ScSubTotalParam	theSubTotalData;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer die Benutzerlisten-TabPage:

class SC_DLLPUBLIC ScUserListItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScUserListItem( USHORT nWhich );
				ScUserListItem( const ScUserListItem& rItem );
				~ScUserListItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	void		SetUserList ( const ScUserList& rUserList );
	ScUserList*	GetUserList () const { return pUserList; }

private:
	ScUserList*	pUserList;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Pivot-Dialog

class ScPivotItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScPivotItem( USHORT nWhich, const ScDPSaveData* pData,
							 const ScRange* pRange, BOOL bNew );
				ScPivotItem( const ScPivotItem&	rItem );
				~ScPivotItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	const ScDPSaveData& GetData() const			{ return *pSaveData; }
	const ScRange&		GetDestRange() const	{ return aDestRange; }
	BOOL				IsNewSheet() const		{ return bNewSheet; }

private:
	ScDPSaveData*	pSaveData;
	ScRange			aDestRange;
	BOOL			bNewSheet;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Solver-Dialog

class ScSolveItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScSolveItem( USHORT				 nWhich,
							 const ScSolveParam* pParam );
				ScSolveItem( const ScSolveItem&	rItem );
				~ScSolveItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	const ScSolveParam& GetData() const { return theSolveData; }

private:
	ScSolveParam	theSolveData;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Mehrfachoperationen-Dialog

class ScTabOpItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScTabOpItem( USHORT				 nWhich,
							 const ScTabOpParam* pParam );
				ScTabOpItem( const ScTabOpItem&	rItem );
				~ScTabOpItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	const ScTabOpParam& GetData() const { return theTabOpData; }

private:
	ScTabOpParam	theTabOpData;
};

//----------------------------------------------------------------------------
// Parameter-Item fuer den Dialog bedingte Formatierung

class ScCondFrmtItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScCondFrmtItem( USHORT nWhich,
//!								const ScConditionalFormat* pCondFrmt );
								const ScConditionalFormat& rCondFrmt );
				ScCondFrmtItem( const ScCondFrmtItem& rItem );
				~ScCondFrmtItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	const ScConditionalFormat&	GetData() const { return theCondFrmtData; }

private:
	ScConditionalFormat	theCondFrmtData;
};



#endif

