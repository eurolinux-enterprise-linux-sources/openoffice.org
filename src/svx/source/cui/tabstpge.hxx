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
#ifndef _SVX_TABSTPGE_HXX
#define _SVX_TABSTPGE_HXX

// include ---------------------------------------------------------------

#include <vcl/group.hxx>
#include <vcl/edit.hxx>
#include <vcl/field.hxx>
#include <vcl/fixed.hxx>
#include <sfx2/tabdlg.hxx>

#include <svx/tstpitem.hxx>
#include "flagsdef.hxx"
// forward ---------------------------------------------------------------

class TabWin_Impl;

// define ----------------------------------------------------------------

// Bitfelder f"ur DisableControls()
//CHINA001 #define TABTYPE_LEFT		0x0001
//CHINA001 #define TABTYPE_RIGHT		0x0002
//CHINA001 #define TABTYPE_CENTER		0x0004
//CHINA001 #define TABTYPE_DEZIMAL		0x0008
//CHINA001 #define TABTYPE_ALL			0x000F
//CHINA001 
//CHINA001 #define TABFILL_NONE		0x0010
//CHINA001 #define TABFILL_POINT		0x0020
//CHINA001 #define TABFILL_DASHLINE		0x0040
//CHINA001 #define TABFILL_SOLIDLINE	0x0080
//CHINA001 #define TABFILL_SPECIAL		0x0100
//CHINA001 #define TABFILL_ALL			0x01F0

// class SvxTabulatorTabPage ---------------------------------------------
/*
	{k:\svx\prototyp\dialog\tabstop.bmp}

	[Beschreibung]
	In dieser TabPage werden Tabulatoren verwaltet.

	[Items]
	<SvxTabStopItem><SID_ATTR_TABSTOP>
	<SfxUInt16Item><SID_ATTR_TABSTOP_DEFAULTS>
	<SfxUInt16Item><SID_ATTR_TABSTOP_POS>
	<SfxInt32Item><SID_ATTR_TABSTOP_OFFSET>
*/

class SvxTabulatorTabPage : public SfxTabPage
{
	using TabPage::DeactivatePage;

public:
	~SvxTabulatorTabPage();

	static SfxTabPage* 	Create( Window* pParent, const SfxItemSet& rSet );
	static USHORT*		GetRanges();

	virtual BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual void 		Reset( const SfxItemSet& rSet );

	void				DisableControls( const USHORT nFlag );

protected:
	virtual int			DeactivatePage( SfxItemSet* pSet = 0 );

private:
	SvxTabulatorTabPage( Window* pParent, const SfxItemSet& rSet );

	// Tabulatoren und Positionen
	MetricBox		aTabBox;
    FixedLine       aTabLabel;
    FixedLine       aTabLabelVert;

	// TabType
	RadioButton		aLeftTab;
	RadioButton		aRightTab;
	RadioButton		aCenterTab;
	RadioButton		aDezTab;

	TabWin_Impl*	pLeftWin;
	TabWin_Impl*	pRightWin;
	TabWin_Impl*	pCenterWin;
	TabWin_Impl*	pDezWin;

	FixedText		aDezCharLabel;
	Edit			aDezChar;
    FixedLine       aTabTypeLabel;

	// Fuellzeichen
	RadioButton		aNoFillChar;
	RadioButton		aFillPoints;
	RadioButton		aFillDashLine ;
	RadioButton		aFillSolidLine;
	RadioButton		aFillSpecial;
	Edit			aFillChar;
    FixedLine       aFillLabel;

	// Buttons
	PushButton		aNewBtn;
	PushButton		aDelAllBtn;
	PushButton		aDelBtn;

	// lokale Variablen, interne Funktionen
	SvxTabStop     	aAktTab;
	SvxTabStopItem	aNewTabs;
	long			nDefDist;
	FieldUnit		eDefUnit;
	BOOL			bCheck;

#ifdef _SVX_TABSTPGE_CXX
	void 			InitTabPos_Impl( USHORT nPos = 0 );
	void 			SetFillAndTabType_Impl();

	// Handler
	DECL_LINK( NewHdl_Impl, Button* );
	DECL_LINK( DelHdl_Impl, Button* );
	DECL_LINK( DelAllHdl_Impl, Button* );

	DECL_LINK( FillTypeCheckHdl_Impl, RadioButton* );
	DECL_LINK( TabTypeCheckHdl_Impl, RadioButton* );

	DECL_LINK( SelectHdl_Impl, MetricBox* );
	DECL_LINK( ModifyHdl_Impl, MetricBox* );
	DECL_LINK( GetFillCharHdl_Impl, Edit* );
	DECL_LINK( GetDezCharHdl_Impl, Edit* );
#endif
	virtual void 			PageCreated(SfxAllItemSet aSet); // add CHINA001
};

#endif // #ifndef _SVX_TABSTPGE_HXX


