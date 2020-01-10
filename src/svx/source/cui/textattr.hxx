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
#ifndef _SVX_TEXTATTR_HXX
#define _SVX_TEXTATTR_HXX

// include ---------------------------------------------------------------

#include <svx/dlgctrl.hxx>

#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif

#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#include <sfx2/basedlgs.hxx>

class SdrView;

/*************************************************************************
|*
|* Dialog zum "Andern von TextAttributen
|*
\************************************************************************/

class SvxTextAttrPage : public SvxTabPage
{
private:
	FixedLine			aFlText;
	TriStateBox         aTsbAutoGrowWidth;
	TriStateBox         aTsbAutoGrowHeight;
	TriStateBox			aTsbFitToSize;
	TriStateBox			aTsbContour;
    TriStateBox         aTsbWordWrapText;
    TriStateBox         aTsbAutoGrowSize;


	FixedLine			aFlDistance;
	FixedText			aFtLeft;
	MetricField			aMtrFldLeft;
	FixedText			aFtRight;
	MetricField			aMtrFldRight;
	FixedText			aFtTop;
	MetricField			aMtrFldTop;
	FixedText			aFtBottom;
	MetricField			aMtrFldBottom;

	FixedLine			aFlSeparator;

	FixedLine			aFlPosition;
	SvxRectCtl			aCtlPosition;
	TriStateBox			aTsbFullWidth;

	const SfxItemSet&	rOutAttrs;
	const SdrView*		pView;

	BOOL				bAutoGrowSizeEnabled;
	BOOL				bContourEnabled;
    BOOL                bAutoGrowWidthEnabled;
    BOOL                bAutoGrowHeightEnabled;
    BOOL                bWordWrapTextEnabled;
    BOOL                bFitToSizeEnabled;


#ifdef _SVX_TEXTATTR_CXX
	DECL_LINK( ClickFullWidthHdl_Impl, void * );
	DECL_LINK( ClickHdl_Impl, void * );
#endif

    /** Return whether the text direction is from left to right (</TRUE>) or
        top to bottom (</FALSE>).
    */
    bool IsTextDirectionLeftToRight (void) const;

public:

	SvxTextAttrPage( Window* pWindow, const SfxItemSet& rInAttrs );
	~SvxTextAttrPage();

	static SfxTabPage* 	Create( Window*, const SfxItemSet& );
	static  USHORT*	    GetRanges();

	virtual BOOL 		FillItemSet( SfxItemSet& );
	virtual void 		Reset( const SfxItemSet & );

	virtual void 		PointChanged( Window* pWindow, RECT_POINT eRP );

	void 		 Construct();
	void		 SetView( const SdrView* pSdrView ) { pView = pSdrView; }
	virtual void 			PageCreated(SfxAllItemSet aSet); // add CHINA001
};

/*************************************************************************
|*
|* Von SfxSingleTabDialog abgeleitet, um vom Control "uber virtuelle Methode
|* benachrichtigt werden zu k"onnen.
|*
\************************************************************************/

//CHINA001 class SvxTextAttrDialog : public SfxSingleTabDialog
//CHINA001 {
//CHINA001 public:
//CHINA001 SvxTextAttrDialog( Window* pParent, const SfxItemSet& rAttr,
//CHINA001 const SdrView* pView );
//CHINA001 ~SvxTextAttrDialog();
//CHINA001 };


#endif // _SVX_TEXTATTR_HXX

