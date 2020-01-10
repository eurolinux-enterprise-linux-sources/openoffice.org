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
#ifndef _SVX_DLG_CTRL_HXX
#define _SVX_DLG_CTRL_HXX

// include ---------------------------------------------------------------

#include <svtools/ctrlbox.hxx>
#include <sfx2/tabdlg.hxx>
#include "svx/svxdllapi.h"
#include <svx/rectenum.hxx>
#include <vcl/graph.hxx>
#ifndef _XTABLE_HXX
class XBitmapEntry;
class XBitmapList;
class XColorEntry;
class XColorTable;
class XDash;
class XDashEntry;
class XDashList;
class XGradient;
class XGradientEntry;
class XGradientList;
class XHatch;
class XHatchEntry;
class XHatchList;
class XLineEndEntry;
class XLineEndList;
class XFillAttrSetItem;
#endif

class XOBitmap;
class XOutdevItemPool;

namespace com { namespace sun { namespace star { namespace awt {
	struct Point;
} } } }

/*************************************************************************
|*
|* Von SfxTabPage abgeleitet, um vom Control ueber virtuelle Methode
|* benachrichtigt werden zu koennen.
|*
\************************************************************************/
class SvxTabPage : public SfxTabPage
{

public:
	SvxTabPage( Window* pParent, ResId Id, const SfxItemSet& rInAttrs  ) :
		SfxTabPage( pParent, Id, rInAttrs ) {}

	virtual void PointChanged( Window* pWindow, RECT_POINT eRP ) = 0;
};

/*************************************************************************
|*
|*	Control zur Darstellung und Auswahl der Eckpunkte (und Mittelpunkt)
|*	eines Objekts
|*
\************************************************************************/
typedef UINT16 CTL_STATE;
#define CS_NOHORZ	1		// no horizontal input information is used
#define CS_NOVERT	2		// no vertikal input information is used

class SvxRectCtlAccessibleContext;

class SVX_DLLPUBLIC SvxRectCtl : public Control
{
private:
	SVX_DLLPRIVATE void				InitSettings( BOOL bForeground, BOOL bBackground );
	SVX_DLLPRIVATE void				InitRectBitmap( void );
	SVX_DLLPRIVATE Bitmap&          GetRectBitmap( void );
    SVX_DLLPRIVATE void             Resize_Impl();

protected:
	SvxRectCtlAccessibleContext*	pAccContext;
	USHORT							nBorderWidth;
	USHORT							nRadius;
	Size							aSize;
	Point							aPtLT, aPtMT, aPtRT;
	Point							aPtLM, aPtMM, aPtRM;
	Point							aPtLB, aPtMB, aPtRB;
	Point							aPtNew;
	RECT_POINT						eRP, eDefRP;
	CTL_STYLE						eCS;
	Bitmap*							pBitmap;
	CTL_STATE						m_nState;

	// #103516# Added a possibility to completely disable this control
	sal_Bool						mbCompleteDisable;

	RECT_POINT			GetRPFromPoint( Point ) const;
	Point				GetPointFromRP( RECT_POINT ) const;
	void				SetFocusRect( const Rectangle* pRect = NULL );		// pRect == NULL -> calculate rectangle in method
	Point				SetActualRPWithoutInvalidate( RECT_POINT eNewRP );	// returns the last point

	virtual void		GetFocus();
	virtual void		LoseFocus();

	Point				GetApproxLogPtFromPixPt( const Point& rRoughPixelPoint ) const;
public:
	SvxRectCtl( Window* pParent, const ResId& rResId, RECT_POINT eRpt = RP_MM,
				USHORT nBorder = 200, USHORT nCircle = 80, CTL_STYLE eStyle = CS_RECT );
	virtual ~SvxRectCtl();

	virtual void 		Paint( const Rectangle& rRect );
	virtual void 		MouseButtonDown( const MouseEvent& rMEvt );
    virtual void        KeyInput( const KeyEvent& rKeyEvt );
	virtual void		StateChanged( StateChangedType nStateChange );
	virtual void		DataChanged( const DataChangedEvent& rDCEvt );
    virtual void        Resize();

	void				Reset();
	RECT_POINT			GetActualRP() const;
	void				SetActualRP( RECT_POINT eNewRP );

	void				SetState( CTL_STATE nState );

	UINT8				GetNumOfChilds( void ) const;	// returns number of usable radio buttons

	Rectangle			CalculateFocusRectangle( void ) const;
	Rectangle			CalculateFocusRectangle( RECT_POINT eRectPoint ) const;

    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > CreateAccessible();

	RECT_POINT			GetApproxRPFromPixPt( const ::com::sun::star::awt::Point& rPixelPoint ) const;

	// #103516# Added a possibility to completely disable this control
	sal_Bool IsCompletelyDisabled() const { return mbCompleteDisable; }
	void DoCompletelyDisable(sal_Bool bNew);
};

/*************************************************************************
|*
|*	Control zur Darstellung und Auswahl des Winkels der Eckpunkte
|*	eines Objekts
|*
\************************************************************************/
class SvxAngleCtl : public SvxRectCtl
{
private:
	void	Initialize();

protected:
	Font	aFont;
	Size	aFontSize;
	BOOL	bPositive;

public:
			SvxAngleCtl( Window* pParent, const ResId& rResId );
			SvxAngleCtl( Window* pParent, const ResId& rResId, Size aSize );

	void	ChangeMetric()
				{ bPositive = !bPositive; }
	virtual void Paint( const Rectangle& rRect );
};

/*************************************************************************
|*
|*	Preview-Control zur Darstellung von Bitmaps
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxBitmapCtl
{
protected:
	Size			aSize;
	USHORT			nLines;
	Color			aPixelColor, aBackgroundColor;
	const USHORT*	pBmpArray;

public:
			SvxBitmapCtl( Window* pParent, const Size& rSize );
			~SvxBitmapCtl();

	XOBitmap	GetXBitmap();

	void	SetBmpArray( const USHORT* pPixel ) { pBmpArray = pPixel; }
	void	SetLines( USHORT nLns ) { nLines = nLns; }
	void	SetPixelColor( Color aColor ) { aPixelColor = aColor; }
	void	SetBackgroundColor( Color aColor ) { aBackgroundColor = aColor; }
};

/*************************************************************************
|*
|*	Control zum Editieren von Bitmaps
|*
\************************************************************************/
class SVX_DLLPUBLIC SvxPixelCtl : public Control
{
private:
    using OutputDevice::SetLineColor;

protected:
	USHORT		nLines, nSquares;
	Color		aPixelColor;
	Color		aBackgroundColor;
	Color		aLineColor;
	Size		aRectSize;
	USHORT* 	pPixel;
	BOOL		bPaintable;

	void	ChangePixel( USHORT nPixel );

public:
			SvxPixelCtl( Window* pParent, const ResId& rResId,
						USHORT nNumber = 8 );
			~SvxPixelCtl();

	virtual void Paint( const Rectangle& rRect );
	virtual void MouseButtonDown( const MouseEvent& rMEvt );

	void	SetXBitmap( const XOBitmap& rXOBitmap );

	void	SetPixelColor( const Color& rCol ) { aPixelColor = rCol; }
	void	SetBackgroundColor( const Color& rCol ) { aBackgroundColor = rCol; }
	void	SetLineColor( const Color& rCol ) { aLineColor = rCol; }

	USHORT	GetLineCount() const { return nLines; }
	Color	GetPixelColor() const { return aPixelColor; }
	Color	GetBackgroundColor() const { return aBackgroundColor; }

	USHORT	GetBitmapPixel( const USHORT nPixelNumber );
	USHORT* GetBitmapPixelPtr() { return pPixel; }

	void	SetPaintable( BOOL bTmp ) { bPaintable = bTmp; }
	void	Reset();
};

/*************************************************************************
|*
|* ColorLB kann mit Farben und Namen gefuellt werden
|*
\************************************************************************/
class SVX_DLLPUBLIC ColorLB : public ColorListBox
{

public:
		 ColorLB( Window* pParent, ResId Id ) : ColorListBox( pParent, Id ) {}
		 ColorLB( Window* pParent, WinBits aWB ) : ColorListBox( pParent, aWB ) {}

	virtual void Fill( const XColorTable* pTab );

	void Append( XColorEntry* pEntry, Bitmap* pBmp = NULL );
	void Modify( XColorEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL );
};

/*************************************************************************
|*
|* HatchingLB
|*
\************************************************************************/
class SVX_DLLPUBLIC HatchingLB : public ListBox
{

public:
		 HatchingLB( Window* pParent, ResId Id, BOOL bUserDraw = TRUE );
		 HatchingLB( Window* pParent, WinBits aWB, BOOL bUserDraw = TRUE );

	virtual void Fill( const XHatchList* pList );
	virtual void UserDraw( const UserDrawEvent& rUDEvt );

	void	Append( XHatchEntry* pEntry, Bitmap* pBmp = NULL );
	void	Modify( XHatchEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL );
	void	SelectEntryByList( const XHatchList* pList, const String& rStr,
						const XHatch& rXHatch, USHORT nDist = 0 );

private:
	XHatchList*		mpList;
	BOOL			mbUserDraw;
};

/*************************************************************************
|*
|* GradientLB
|*
\************************************************************************/
class SVX_DLLPUBLIC GradientLB : public ListBox
{
public:
	GradientLB( Window* pParent, ResId Id, BOOL bUserDraw = TRUE );
	GradientLB( Window* pParent, WinBits aWB, BOOL bUserDraw = TRUE );

	virtual void Fill( const XGradientList* pList );
	virtual void UserDraw( const UserDrawEvent& rUDEvt );

	void	Append( XGradientEntry* pEntry, Bitmap* pBmp = NULL );
	void	Modify( XGradientEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL );
	void	SelectEntryByList( const XGradientList* pList, const String& rStr,
						const XGradient& rXGradient, USHORT nDist = 0 );

private:
	XGradientList* mpList;
	BOOL			mbUserDraw;
};

/*************************************************************************
|*
|* BitmapLB
|*
\************************************************************************/
class SVX_DLLPUBLIC BitmapLB : public ListBox
{
public:
		 BitmapLB( Window* pParent, ResId Id, BOOL bUserDraw = TRUE );

	virtual void Fill( const XBitmapList* pList );
	virtual void UserDraw( const UserDrawEvent& rUDEvt );

	void	Append( XBitmapEntry* pEntry, Bitmap* pBmp = NULL );
	void	Modify( XBitmapEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL );
	void	SelectEntryByList( const XBitmapList* pList, const String& rStr,
						const Bitmap& rBmp);

private:
	VirtualDevice	aVD;
	Bitmap			aBitmap;

	XBitmapList*	mpList;
	BOOL			mbUserDraw;

	SVX_DLLPRIVATE void SetVirtualDevice();
};

/*************************************************************************
|*
|* FillAttrLB vereint alle Fuellattribute in einer ListBox
|*
\************************************************************************/
class FillAttrLB : public ColorListBox
{
private:
	VirtualDevice	aVD;
	Bitmap			aBitmap;

	void SetVirtualDevice();

public:
		 FillAttrLB( Window* pParent, ResId Id );
		 FillAttrLB( Window* pParent, WinBits aWB );

	virtual void Fill( const XColorTable* pTab );
	virtual void Fill( const XHatchList* pList );
	virtual void Fill( const XGradientList* pList );
	virtual void Fill( const XBitmapList* pList );

	void	SelectEntryByList( const XBitmapList* pList, const String& rStr,
						const Bitmap& rBmp);
};

/*************************************************************************
|*
|* FillTypeLB
|*
\************************************************************************/
class FillTypeLB : public ListBox
{

public:
		 FillTypeLB( Window* pParent, ResId Id ) : ListBox( pParent, Id ) {}
		 FillTypeLB( Window* pParent, WinBits aWB ) : ListBox( pParent, aWB ) {}

	virtual void Fill();
};

/*************************************************************************
|*
|* LineLB
|*
\************************************************************************/
class SVX_DLLPUBLIC LineLB : public ListBox
{

public:
		 LineLB( Window* pParent, ResId Id ) : ListBox( pParent, Id ) {}
		 LineLB( Window* pParent, WinBits aWB ) : ListBox( pParent, aWB ) {}

	virtual void Fill( const XDashList* pList );

	void Append( XDashEntry* pEntry, Bitmap* pBmp = NULL );
	void Modify( XDashEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL );
	void SelectEntryByList( const XDashList* pList, const String& rStr,
							const XDash& rDash, USHORT nDist = 0 );
};

/*************************************************************************
|*
|* LineEndsLB
|*
\************************************************************************/
class SVX_DLLPUBLIC LineEndLB : public ListBox
{

public:
		 LineEndLB( Window* pParent, ResId Id ) : ListBox( pParent, Id ) {}
		 LineEndLB( Window* pParent, WinBits aWB ) : ListBox( pParent, aWB ) {}

	virtual void Fill( const XLineEndList* pList, BOOL bStart = TRUE );

	void	Append( XLineEndEntry* pEntry, Bitmap* pBmp = NULL,
					BOOL bStart = TRUE );
	void	Modify( XLineEndEntry* pEntry, USHORT nPos, Bitmap* pBmp = NULL,
					BOOL bStart = TRUE );
};

//////////////////////////////////////////////////////////////////////////////

class SdrObject;
class SdrModel;

class SvxPreviewBase : public Control
{
private:
	SdrModel*										mpModel;
    VirtualDevice*                                  mpBufferDevice;

protected:
	void InitSettings(bool bForeground, bool bBackground);

    // prepare buffered paint
    void LocalPrePaint();

    // end and output buffered paint
    void LocalPostPaint();

public:
	SvxPreviewBase( Window* pParent, const ResId& rResId );
	virtual ~SvxPreviewBase();

    // change support
	virtual void StateChanged(StateChangedType nStateChange);
	virtual void DataChanged(const DataChangedEvent& rDCEvt);

    // dada read access
    SdrModel& getModel() const { return *mpModel; }
    OutputDevice& getBufferDevice() const { return *mpBufferDevice; }
};

/*************************************************************************
|*
|* SvxLinePreview
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxXLinePreview : public SvxPreviewBase
{
private:
	SdrObject*										mpLineObjA;
	SdrObject*										mpLineObjB;
	SdrObject*										mpLineObjC;
	
	//#58425# Symbole auf einer Linie (z.B. StarChart)
	Graphic*										mpGraphic;
	sal_Bool										mbWithSymbol;
	Size											maSymbolSize;

public:
	SvxXLinePreview( Window* pParent, const ResId& rResId );
	virtual ~SvxXLinePreview();

	void SetLineAttributes(const SfxItemSet& rItemSet);

	void ShowSymbol( BOOL b ) { mbWithSymbol = b; };
	void SetSymbol( Graphic* p, const Size& s );
	void ResizeSymbol( const Size& s );

	virtual void Paint( const Rectangle& rRect );
};

/*************************************************************************
|*
|* SvxXRectPreview
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxXRectPreview : public SvxPreviewBase
{
private:
	SdrObject*										mpRectangleObject;

public:
	SvxXRectPreview( Window* pParent, const ResId& rResId );
	virtual ~SvxXRectPreview();

	void SetAttributes(const SfxItemSet& rItemSet);

	virtual void	Paint( const Rectangle& rRect );
};

/*************************************************************************
|*
|* SvxXShadowPreview
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxXShadowPreview : public SvxPreviewBase
{
private:
	SdrObject*										mpRectangleObject;
	SdrObject*										mpRectangleShadow;

public:
	SvxXShadowPreview( Window* pParent, const ResId& rResId );
	virtual ~SvxXShadowPreview();

	void SetRectangleAttributes(const SfxItemSet& rItemSet);
	void SetShadowAttributes(const SfxItemSet& rItemSet);
	void SetShadowPosition(const Point& rPos);
	
	virtual void	Paint( const Rectangle& rRect );
};

#endif // _SVX_DLG_CTRL_HXX

