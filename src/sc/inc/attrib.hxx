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

#ifndef SC_SCATTR_HXX
#define SC_SCATTR_HXX

#include <svtools/poolitem.hxx>
#include <svtools/intitem.hxx>
#include <svtools/eitem.hxx>
#include "scdllapi.h"
#include "global.hxx"
#include "address.hxx"

//------------------------------------------------------------------------

										// Flags fuer durch Merge verdeckte Zellen
										// und Control fuer Auto-Filter
#define SC_MF_HOR				1
#define SC_MF_VER				2
#define SC_MF_AUTO				4
#define SC_MF_BUTTON			8
#define SC_MF_SCENARIO			16

#define SC_MF_ALL				31


class EditTextObject;
class SvxBorderLine;

BOOL SC_DLLPUBLIC ScHasPriority( const SvxBorderLine* pThis, const SvxBorderLine* pOther );

//------------------------------------------------------------------------

class SC_DLLPUBLIC ScMergeAttr: public SfxPoolItem
{
	SCsCOL      nColMerge;
	SCsROW      nRowMerge;
public:
				TYPEINFO();
				ScMergeAttr();
				ScMergeAttr( SCsCOL nCol, SCsROW nRow = 0);
				ScMergeAttr( const ScMergeAttr& );
				~ScMergeAttr();

	virtual String          	GetValueText() const;

	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create( SvStream& rStream, USHORT nVer ) const;

			SCsCOL          GetColMerge() const {return nColMerge; }
			SCsROW          GetRowMerge() const {return nRowMerge; }

			BOOL			IsMerged() const { return nColMerge>1 || nRowMerge>1; }

	inline  ScMergeAttr& operator=(const ScMergeAttr& rMerge)
			{
				nColMerge = rMerge.nColMerge;
				nRowMerge = rMerge.nRowMerge;
				return *this;
			}
};

//------------------------------------------------------------------------

class SC_DLLPUBLIC ScMergeFlagAttr: public SfxInt16Item
{
public:
			ScMergeFlagAttr();
			ScMergeFlagAttr(INT16 nFlags);
			~ScMergeFlagAttr();

	BOOL	IsHorOverlapped() const		{ return ( GetValue() & SC_MF_HOR ) != 0;  }
	BOOL	IsVerOverlapped() const		{ return ( GetValue() & SC_MF_VER ) != 0;  }
	BOOL	IsOverlapped() const		{ return ( GetValue() & ( SC_MF_HOR | SC_MF_VER ) ) != 0; }

	BOOL	HasAutoFilter() const		{ return ( GetValue() & SC_MF_AUTO ) != 0; }
	BOOL	HasButton() const			{ return ( GetValue() & SC_MF_BUTTON ) != 0; }

	BOOL	IsScenario() const			{ return ( GetValue() & SC_MF_SCENARIO ) != 0; }
};

//------------------------------------------------------------------------
class SC_DLLPUBLIC ScProtectionAttr: public SfxPoolItem
{
	BOOL        bProtection;    // Zelle schuetzen
	BOOL        bHideFormula;   // Formel nicht Anzeigen
	BOOL        bHideCell;      // Zelle nicht Anzeigen
	BOOL        bHidePrint;     // Zelle nicht Ausdrucken
public:
							TYPEINFO();
							ScProtectionAttr();
							ScProtectionAttr(   BOOL bProtect,
												BOOL bHFormula = FALSE,
												BOOL bHCell = FALSE,
												BOOL bHPrint = FALSE);
							ScProtectionAttr( const ScProtectionAttr& );
							~ScProtectionAttr();

	virtual String          	GetValueText() const;
	virtual SfxItemPresentation GetPresentation(
									SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
									String& rText,
                                    const IntlWrapper* pIntl = 0 ) const;

	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create( SvStream& rStream, USHORT nVer ) const;

	virtual	BOOL			QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	BOOL			PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

			BOOL            GetProtection() const { return bProtection; }
			BOOL            SetProtection( BOOL bProtect);
			BOOL            GetHideFormula() const { return bHideFormula; }
			BOOL            SetHideFormula( BOOL bHFormula);
			BOOL            GetHideCell() const { return bHideCell; }
			BOOL            SetHideCell( BOOL bHCell);
			BOOL            GetHidePrint() const { return bHidePrint; }
			BOOL            SetHidePrint( BOOL bHPrint);
	inline  ScProtectionAttr& operator=(const ScProtectionAttr& rProtection)
			{
				bProtection = rProtection.bProtection;
				bHideFormula = rProtection.bHideFormula;
				bHideCell = rProtection.bHideCell;
				bHidePrint = rProtection.bHidePrint;
				return *this;
			}
};


//----------------------------------------------------------------------------
// ScRangeItem: verwaltet einen Tabellenbereich

#define SCR_INVALID		0x01
#define SCR_ALLTABS		0x02
#define SCR_TONEWTAB	0x04

class ScRangeItem : public SfxPoolItem
{
public:
			TYPEINFO();

			inline	ScRangeItem( const USHORT nWhich );
			inline	ScRangeItem( const USHORT   nWhich,
								 const ScRange& rRange,
								 const USHORT 	nNewFlags = 0 );
			inline	ScRangeItem( const ScRangeItem& rCpy );

	inline ScRangeItem& operator=( const ScRangeItem &rCpy );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual int 				operator==( const SfxPoolItem& ) const;
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
												 SfxMapUnit eCoreMetric,
												 SfxMapUnit ePresMetric,
												 String &rText,
                                                 const IntlWrapper* pIntl = 0 ) const;
	virtual SfxPoolItem*		Clone( SfxItemPool *pPool = 0 ) const;

	const ScRange&	GetRange() const 				{ return aRange;  }
	void			SetRange( const ScRange& rNew )	{ aRange = rNew; }

	USHORT			GetFlags() const 				{ return nFlags;  }
	void			SetFlags( USHORT nNew )	 		{ nFlags = nNew; }

private:
	ScRange aRange;
	USHORT	nFlags;
};

inline ScRangeItem::ScRangeItem( const USHORT nWhichP )
    :   SfxPoolItem( nWhichP ), nFlags( SCR_INVALID ) // == ungueltige Area
{
}

inline ScRangeItem::ScRangeItem( const USHORT   nWhichP,
								 const ScRange& rRange,
								 const USHORT	nNew )
    : SfxPoolItem( nWhichP ), aRange( rRange ), nFlags( nNew )
{
}

inline ScRangeItem::ScRangeItem( const ScRangeItem& rCpy )
	: SfxPoolItem( rCpy.Which() ), aRange( rCpy.aRange ), nFlags( rCpy.nFlags )
{}

inline ScRangeItem& ScRangeItem::operator=( const ScRangeItem &rCpy )
{
	aRange = rCpy.aRange;
	return *this;
}

//----------------------------------------------------------------------------
// ScTableListItem: verwaltet eine Liste von Tabellen
//----------------------------------------------------------------------------
class ScTableListItem : public SfxPoolItem
{
public:
	TYPEINFO();

	inline	ScTableListItem( const USHORT nWhich );
			ScTableListItem( const ScTableListItem& rCpy );
//UNUSED2008-05  ScTableListItem( const USHORT nWhich, const List& rList );
			~ScTableListItem();

	ScTableListItem& operator=( const ScTableListItem &rCpy );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual int 				operator==( const SfxPoolItem& ) const;
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
												 SfxMapUnit eCoreMetric,
												 SfxMapUnit ePresMetric,
												 String &rText,
                                                 const IntlWrapper* pIntl = 0 ) const;
	virtual SfxPoolItem*		Clone( SfxItemPool *pPool = 0 ) const;

//UNUSED2009-05 BOOL	GetTableList( List& aList ) const;
//UNUSED2009-05 void	SetTableList( const List& aList );

public:
	USHORT  nCount;
	SCTAB*  pTabArr;
};

inline ScTableListItem::ScTableListItem( const USHORT nWhichP )
    : SfxPoolItem(nWhichP), nCount(0), pTabArr(NULL)
{}

//----------------------------------------------------------------------------
// Seitenformat-Item: Kopf-/Fusszeileninhalte

#define SC_HF_LEFTAREA   1
#define SC_HF_CENTERAREA 2
#define SC_HF_RIGHTAREA  3

class SC_DLLPUBLIC ScPageHFItem : public SfxPoolItem
{
	EditTextObject* pLeftArea;
	EditTextObject* pCenterArea;
	EditTextObject* pRightArea;

public:
				TYPEINFO();
				ScPageHFItem( USHORT nWhich );
				ScPageHFItem( const ScPageHFItem& rItem );
				~ScPageHFItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	virtual SfxPoolItem*    Create( SvStream& rStream, USHORT nVer ) const;

	virtual	BOOL			QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	BOOL			PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	const EditTextObject* GetLeftArea() const		{ return pLeftArea; }
	const EditTextObject* GetCenterArea() const		{ return pCenterArea; }
	const EditTextObject* GetRightArea() const		{ return pRightArea; }

	void SetLeftArea( const EditTextObject& rNew );
	void SetCenterArea( const EditTextObject& rNew );
	void SetRightArea( const EditTextObject& rNew );

	//Set mit Uebereignung der Pointer, nArea siehe defines oben
	void SetArea( EditTextObject *pNew, int nArea );
};


//----------------------------------------------------------------------------
// Seitenformat-Item: Kopf-/Fusszeileninhalte

class SC_DLLPUBLIC ScViewObjectModeItem: public SfxEnumItem
{
public:
				TYPEINFO();

				ScViewObjectModeItem( USHORT nWhich );
				ScViewObjectModeItem( USHORT nWhich, ScVObjMode eMode );
				~ScViewObjectModeItem();

	virtual USHORT				GetValueCount() const;
	virtual String				GetValueText( USHORT nVal ) const;
	virtual SfxPoolItem*		Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*		Create(SvStream &, USHORT) const;
	virtual USHORT				GetVersion( USHORT nFileVersion ) const;
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
												 SfxMapUnit eCoreMetric,
												 SfxMapUnit ePresMetric,
												 String& rText,
                                                 const IntlWrapper* pIntl = 0 ) const;
};

//----------------------------------------------------------------------------
//

class ScDoubleItem : public SfxPoolItem
{
public:
				TYPEINFO();
				ScDoubleItem( USHORT nWhich, double nVal=0 );
				ScDoubleItem( const ScDoubleItem& rItem );
				~ScDoubleItem();

	virtual String          GetValueText() const;
	virtual int             operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	virtual SfxPoolItem*    Create( SvStream& rStream, USHORT nVer ) const;

	double GetValue() const		{ return nValue; }

	void SetValue( const double nVal ) { nValue = nVal;}

private:
	double	nValue;
};


// ============================================================================

/** Member ID for "page scale to width" value in QueryValue() and PutValue(). */
const BYTE SC_MID_PAGE_SCALETO_WIDTH    = 1;
/** Member ID for "page scale to height" value in QueryValue() and PutValue(). */
const BYTE SC_MID_PAGE_SCALETO_HEIGHT   = 2;


/** Contains the "scale to width/height" attribute in page styles. */
class SC_DLLPUBLIC ScPageScaleToItem : public SfxPoolItem
{
public:
                                TYPEINFO();

    /** Default c'tor sets the width and height to 0. */
    explicit                    ScPageScaleToItem();
    explicit                    ScPageScaleToItem( sal_uInt16 nWidth, sal_uInt16 nHeight );

    virtual                     ~ScPageScaleToItem();

    virtual ScPageScaleToItem*  Clone( SfxItemPool* = 0 ) const;

    virtual int                 operator==( const SfxPoolItem& rCmp ) const;

    inline sal_uInt16           GetWidth() const { return mnWidth; }
    inline sal_uInt16           GetHeight() const { return mnHeight; }
    inline bool                 IsValid() const { return mnWidth || mnHeight; }

    inline void                 SetWidth( sal_uInt16 nWidth ) { mnWidth = nWidth; }
    inline void                 SetHeight( sal_uInt16 nHeight ) { mnHeight = nHeight; }
    inline void                 Set( sal_uInt16 nWidth, sal_uInt16 nHeight )
                                    { mnWidth = nWidth; mnHeight = nHeight; }
    inline void                 SetInvalid() { mnWidth = mnHeight = 0; }

    virtual SfxItemPresentation GetPresentation(
                                    SfxItemPresentation ePresentation,
                                    SfxMapUnit, SfxMapUnit,
                                    XubString& rText,
                                    const IntlWrapper* = 0 ) const;

    virtual BOOL                QueryValue( ::com::sun::star::uno::Any& rAny, BYTE nMemberId = 0 ) const;
    virtual BOOL                PutValue( const ::com::sun::star::uno::Any& rAny, BYTE nMemberId = 0 );

private:
    sal_uInt16                  mnWidth;
    sal_uInt16                  mnHeight;
};

// ============================================================================

#endif

