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
#ifndef _SVX_BACKGRND_HXX
#define _SVX_BACKGRND_HXX

// include ---------------------------------------------------------------

#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif
#include <svtools/stdctrl.hxx>
#ifndef _GRAPH_HXX //autogen
#include <vcl/graph.hxx>
#endif
#include <svtools/valueset.hxx>
#include <svx/dlgctrl.hxx>

// class SvxBackgroundTabPage --------------------------------------------
/*
{k:\svx\prototyp\dialog\backgrnd.bmp}
	[Beschreibung]
	Mit dieser TabPage kann eine Brush (z.B. fuer die Hintergrundfarbe eines
	Rahmens) eingestellt werden.
	[Items]
	<SvxBrushItem>:     <SID_ATTR_BRUSH>;
*/

//------------------------------------------------------------------------
// forwards:

class BackgroundPreviewImpl;
class SvxOpenGraphicDialog;
struct SvxBackgroundTable_Impl;
struct SvxBackgroundPara_Impl;
struct SvxBackgroundPage_Impl;
class SvxBrushItem;
//------------------------------------------------------------------------

class SvxBackgroundTabPage : public SvxTabPage
{
	using TabPage::DeactivatePage;
public:
	static SfxTabPage*  Create( Window* pParent, const SfxItemSet& rAttrSet );
	static USHORT*      GetRanges();

	virtual BOOL        FillItemSet( SfxItemSet& rSet );
	virtual void        Reset( const SfxItemSet& rSet );
	virtual void        FillUserData();
	virtual void        PointChanged( Window* pWindow, RECT_POINT eRP );

	void                ShowSelector(); // Umschalt-ListBox aktivieren
	void                ShowTblControl(); // fuer den Writer (Zellen/Zeilen/Tabelle)
	void                ShowParaControl(BOOL bCharOnly = FALSE); // fuer den Writer (Absatz/Zeichen)
    void                EnableTransparency(BOOL bColor, BOOL bGraphic);
	virtual void		PageCreated (SfxAllItemSet aSet); //add CHINA001
protected:
	virtual int         DeactivatePage( SfxItemSet* pSet = 0 );

private:
	SvxBackgroundTabPage( Window* pParent, const SfxItemSet& rCoreSet );
	~SvxBackgroundTabPage();

	// Hintergrundfarbe ------------------------------------
	Control					aBorderWin;
	ValueSet                aBackgroundColorSet;
    FixedLine               aBackgroundColorBox;
	BackgroundPreviewImpl*  pPreviewWin1;
    //color transparency
    FixedText               aColTransFT;
    MetricField             aColTransMF;
    // Hintergrund-Bitmap ----------------------------------
	PushButton              aBtnBrowse;
	CheckBox                aBtnLink;
	CheckBox                aBtnPreview;
	FixedInfo               aFtFile;
    FixedLine               aGbFile;
	RadioButton             aBtnPosition;
	RadioButton             aBtnArea;
	RadioButton             aBtnTile;
	SvxRectCtl              aWndPosition;
    FixedLine               aGbPosition;
    //transparency of graphics
    FixedLine               aGraphTransFL;
    MetricField             aGraphTransMF;

    BackgroundPreviewImpl*  pPreviewWin2;
    // Selektor --------------------------------------------
	FixedText               aSelectTxt;
	ListBox                 aLbSelect;
	const String            aStrBrowse;
	const String            aStrUnlinked;

	// DDListBox fuer Writer -------------------------------

	FixedText               aTblDesc;
	ListBox                 aTblLBox;
	ListBox					aParaLBox;

	//------------------------------------------------------
	Color		aBgdColor;
	USHORT		nHtmlMode;
	BOOL        bAllowShowSelector	: 1;
	BOOL        bIsGraphicValid		: 1;
	BOOL		bLinkOnly			: 1;
    BOOL        bResized            : 1;
    BOOL        bColTransparency    : 1;
    BOOL        bGraphTransparency  : 1;
    Graphic     aBgdGraphic;
	String      aBgdGraphicPath;
	String      aBgdGraphicFilter;

	SvxBackgroundPage_Impl* pPageImpl;
	SvxOpenGraphicDialog* pImportDlg;

	// Items fuer Sw-Tabelle muessen gesammelt werden
	SvxBackgroundTable_Impl*	pTableBck_Impl;
	// auch fuer die Absatzvorlage
	SvxBackgroundPara_Impl*	pParaBck_Impl;

#ifdef _SVX_BACKGRND_CXX
	void                FillColorValueSets_Impl();
	void                ShowColorUI_Impl();
	void                ShowBitmapUI_Impl();
	BOOL                LoadLinkedGraphic_Impl();
	void                RaiseLoadError_Impl();
	void                SetGraphicPosition_Impl( SvxGraphicPosition ePos );
	SvxGraphicPosition  GetGraphicPosition_Impl();
	void				FillControls_Impl(const SvxBrushItem& rBgdAttr,
											const String& rUserData);
	BOOL 				FillItemSetWithWallpaperItem( SfxItemSet& rCoreSet, USHORT nSlot);
	void				ResetFromWallpaperItem( const SfxItemSet& rSet );

	DECL_LINK( LoadTimerHdl_Impl, Timer* );
	DECL_LINK( SelectHdl_Impl, ListBox* );
	DECL_LINK( BrowseHdl_Impl, PushButton* );
	DECL_LINK( RadioClickHdl_Impl, RadioButton* );
	DECL_LINK( FileClickHdl_Impl, CheckBox* );
	DECL_LINK( BackgroundColorHdl_Impl, ValueSet* );
	DECL_LINK( TblDestinationHdl_Impl, ListBox* );
	DECL_LINK( ParaDestinationHdl_Impl, ListBox* );
#endif
};

#endif // #ifndef _SVX_BACKGRND_HXX


