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
#ifndef _SVX_MEASURE_HXX
#define _SVX_MEASURE_HXX

// include ---------------------------------------------------------------

#include <vcl/fixed.hxx>
#include <vcl/group.hxx>
#include <sfx2/basedlgs.hxx>
#ifndef _SVX_DLGCTRL_HXX
#include <svx/dlgctrl.hxx>
#endif
#include "measctrl.hxx"

class SdrView;

/*************************************************************************
|*
|* Dialog zum "Andern von TextAttributen
|*
\************************************************************************/

class SvxMeasurePage : public SvxTabPage
{
private:

	FixedLine			aFlLine;
	FixedText			aFtLineDist;
	MetricField			aMtrFldLineDist;
	FixedText			aFtHelplineOverhang;
	MetricField			aMtrFldHelplineOverhang;
	FixedText			aFtHelplineDist;
	MetricField			aMtrFldHelplineDist;
	FixedText			aFtHelpline1Len;
	MetricField			aMtrFldHelpline1Len;
	FixedText			aFtHelpline2Len;
	MetricField			aMtrFldHelpline2Len;
	TriStateBox 		aTsbBelowRefEdge;
	FixedText			aFtDecimalPlaces;
	MetricField			aMtrFldDecimalPlaces;

	FixedLine			aFlLabel;
    FixedText           aFtPosition;
	SvxRectCtl			aCtlPosition;
	TriStateBox 		aTsbAutoPosV;
	TriStateBox 		aTsbAutoPosH;
	TriStateBox 		aTsbShowUnit;
	ListBox				aLbUnit;
	TriStateBox 		aTsbParallel;

	SvxXMeasurePreview	aCtlPreview;

    FixedLine           aFlVert;

	const SfxItemSet&	rOutAttrs;
	SfxItemSet			aAttrSet;
	const SdrView*		pView;
	SfxMapUnit			eUnit;

	BOOL				bPositionModified;

#ifdef _SVX_MEASURE_CXX
	void 				FillUnitLB();

	DECL_LINK( ClickAutoPosHdl_Impl, void * );
	DECL_LINK( ChangeAttrHdl_Impl, void * );
#endif

public:

	SvxMeasurePage( Window* pWindow, const SfxItemSet& rInAttrs );
	~SvxMeasurePage();

	static SfxTabPage* 	Create( Window*, const SfxItemSet& );
	static  USHORT*	    GetRanges();

	virtual BOOL 		FillItemSet( SfxItemSet& );
	virtual void 		Reset( const SfxItemSet & );

	virtual void 		PointChanged( Window* pWindow, RECT_POINT eRP );

	void 		 Construct();
	void		 SetView( const SdrView* pSdrView ) { pView = pSdrView; }
	virtual void PageCreated (SfxAllItemSet aSet); //add CHINA001 
};

/*************************************************************************
|*
|* Von SfxSingleTabDialog abgeleitet, um vom Control "uber virtuelle Methode
|* benachrichtigt werden zu k"onnen.
|*
\************************************************************************/

class SvxMeasureDialog : public SfxSingleTabDialog
{
public:
	SvxMeasureDialog( Window* pParent, const SfxItemSet& rAttr,
					   const SdrView* pView );
	~SvxMeasureDialog();
};


#endif // _SVX_MEASURE_HXX

