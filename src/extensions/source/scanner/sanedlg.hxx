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
#ifndef _SVT_SANEDLG_HXX
#define _SVT_SANEDLG_HXX

#include <vcl/dialog.hxx>
#include <tools/config.hxx>
#include <vcl/lstbox.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <vcl/fixed.hxx>
#include <vcl/group.hxx>
#include <vcl/field.hxx>
#include <vcl/edit.hxx>
#include <svtools/svtreebx.hxx>

#include <sane.hxx>

class SaneDlg : public ModalDialog
{
private:
	enum DragDirection { TopLeft, Top, TopRight, Right, BottomRight, Bottom,
						 BottomLeft, Left };

	Sane&			mrSane;
	Bitmap			maPreviewBitmap;
	Rectangle		maPreviewRect;
	Point			maTopLeft, maBottomRight;
	Point			maMinTopLeft, maMaxBottomRight;
    BOOL			mbDragEnable;
	BOOL			mbIsDragging;
	int				mnDragMode;
	BOOL			mbDragDrawn;
	DragDirection	meDragDirection;

	MapMode			maMapMode;

	Link			maOldLink;
	
	OKButton		maOKButton;
	CancelButton	maCancelButton;
	PushButton		maDeviceInfoButton;
	PushButton		maPreviewButton;
	PushButton		maButtonOption;

	FixedText		maOptionsTxt;
	FixedText		maOptionTitle;
	FixedText		maOptionDescTxt;
	FixedText		maVectorTxt;

	FixedText		maScanLeftTxt;
	MetricField		maLeftField;
	FixedText		maScanTopTxt;
	MetricField		maTopField;
	FixedText		maRightTxt;
	MetricField		maRightField;
	FixedText		maBottomTxt;
	MetricField		maBottomField;

	FixedText		maDeviceBoxTxt;
	ListBox			maDeviceBox;
	FixedText		maReslTxt;
	NumericBox		maReslBox;
	FixedText		maAdvancedTxt;
	CheckBox		maAdvancedBox;

	NumericField	maVectorBox;
	ListBox			maQuantumRangeBox;
	ListBox			maStringRangeBox;

	FixedLine		maPreviewBox;
	FixedLine		maAreaBox;

	CheckBox		maBoolCheckBox;

	Edit			maStringEdit;
	Edit			maNumericEdit;

	SvTreeListBox	maOptionBox;

	int				mnCurrentOption;
	int				mnCurrentElement;
	double*			mpRange;
	double			mfMin, mfMax;

	DECL_LINK( ClickBtnHdl, Button* );
	DECL_LINK( SelectHdl, ListBox* );
	DECL_LINK( ModifyHdl, Edit* );
	DECL_LINK( ReloadSaneOptionsHdl, Sane* );
	DECL_LINK( OptionsBoxSelectHdl, SvTreeListBox* );

	void SaveState();
	BOOL LoadState();

	void InitDevices();
	void InitFields();
	void AcquirePreview();
	void DisableOption();
	void EstablishBoolOption();
	void EstablishStringOption();
	void EstablishStringRange();
	void EstablishQuantumRange();
	void EstablishNumericOption();
	void EstablishButtonOption();

	void DrawRectangles( Point&, Point& );
	void DrawDrag();
	Point GetPixelPos( const Point& );
	Point GetLogicPos( const Point& );
	void UpdateScanArea( BOOL );

	// helper
	BOOL SetAdjustedNumericalValue( const char* pOption, double fValue, int nElement = 0 );
	
	virtual void Paint( const Rectangle& );
	virtual void MouseMove( const MouseEvent& rMEvt );
	virtual void MouseButtonDown( const MouseEvent& rMEvt );
	virtual void MouseButtonUp( const MouseEvent& rMEvt );
public:
	SaneDlg( Window*, Sane& );
	~SaneDlg();

	virtual short Execute();
};


#endif
