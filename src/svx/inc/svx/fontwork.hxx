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
#ifndef _SVX_FONTWORK_HXX
#define _SVX_FONTWORK_HXX

// include ---------------------------------------------------------------

#ifndef _TOOLBOX_HXX //autogen
#include <vcl/toolbox.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#include <svtools/valueset.hxx>
#include <sfx2/dockwin.hxx>
#include <sfx2/ctrlitem.hxx>
#include <svx/xenum.hxx>
#include <svx/dlgctrl.hxx>
#include "svx/svxdllapi.h"

// forward ---------------------------------------------------------------

class SdrView;
class SdrPageView;
class SdrObject;

class XFormTextAdjustItem;
class XFormTextDistanceItem;
class XFormTextStartItem;
class XFormTextMirrorItem;
class XFormTextStdFormItem;
class XFormTextHideFormItem;
class XFormTextOutlineItem;
class XFormTextShadowItem;
class XFormTextShadowColorItem;
class XFormTextShadowXValItem;
class XFormTextShadowYValItem;

/*************************************************************************
|*
|* ControllerItem fuer Fontwork
|*
\************************************************************************/

class SvxFontWorkDialog;

class SvxFontWorkControllerItem : public SfxControllerItem
{
	SvxFontWorkDialog  &rFontWorkDlg;

protected:
	virtual void StateChanged(USHORT nSID, SfxItemState eState,
							  const SfxPoolItem* pState);

public:
	SvxFontWorkControllerItem(USHORT nId, SvxFontWorkDialog&, SfxBindings&);
};

/*************************************************************************
|*
|* Ableitung vom SfxChildWindow als "Behaelter" fuer Fontwork-Dialog
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxFontWorkChildWindow : public SfxChildWindow
{
 public:
	SvxFontWorkChildWindow(Window*, USHORT, SfxBindings*, SfxChildWinInfo*);
	SFX_DECL_CHILDWINDOW(SvxFontWorkChildWindow);
};

/*************************************************************************
|*
|* Floating Window zur Attributierung von Texteffekten
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxFontWorkDialog : public SfxDockingWindow
{
 #define CONTROLLER_COUNT 12

	SvxFontWorkControllerItem* pCtrlItems[CONTROLLER_COUNT];

	ValueSet		aFormSet;

	ToolBox			aTbxStyle;
	ToolBox			aTbxAdjust;

	FixedImage		aFbDistance;
	MetricField		aMtrFldDistance;
	FixedImage		aFbTextStart;
	MetricField		aMtrFldTextStart;

	ToolBox			aTbxShadow;

	FixedImage		aFbShadowX;
	MetricField		aMtrFldShadowX;
	FixedImage		aFbShadowY;
	MetricField		aMtrFldShadowY;

	ColorLB			aShadowColorLB;

	SfxBindings&	rBindings;
	Timer			aInputTimer;
	BOOL			bUserZoomedIn;

	USHORT			nLastStyleTbxId;
	USHORT			nLastAdjustTbxId;
	USHORT			nLastShadowTbxId;
	long			nSaveShadowX;
	long			nSaveShadowY;
	long			nSaveShadowAngle;
	long			nSaveShadowSize;

	ImageList		maImageList;
	ImageList		maImageListH;

	const XColorTable* pColorTable;

#ifdef _SVX_FONTWORK_CXX
 friend class SvxFontWorkChildWindow;
 friend class SvxFontWorkControllerItem;

	DECL_LINK( SelectStyleHdl_Impl, void * );
	DECL_LINK( SelectAdjustHdl_Impl, void * );
	DECL_LINK( SelectShadowHdl_Impl, void * );

	DECL_LINK( ModifyInputHdl_Impl, void * );
	DECL_LINK( InputTimoutHdl_Impl, void * );

	DECL_LINK( FormSelectHdl_Impl, void * );
	DECL_LINK( ColorSelectHdl_Impl, void * );

	void SetStyle_Impl(const XFormTextStyleItem*);
	void SetAdjust_Impl(const XFormTextAdjustItem*);
	void SetDistance_Impl(const XFormTextDistanceItem*);
	void SetStart_Impl(const XFormTextStartItem*);
	void SetMirror_Impl(const XFormTextMirrorItem*);
	void SetStdForm_Impl(const XFormTextStdFormItem*);
	void SetShowForm_Impl(const XFormTextHideFormItem*);
	void SetOutline_Impl(const XFormTextOutlineItem*);
	void SetShadow_Impl(const XFormTextShadowItem*,
						BOOL bRestoreValues = FALSE);
	void SetShadowColor_Impl(const XFormTextShadowColorItem*);
	void SetShadowXVal_Impl(const XFormTextShadowXValItem*);
	void SetShadowYVal_Impl(const XFormTextShadowYValItem*);
#endif

	virtual void DataChanged( const DataChangedEvent& rDCEvt );
	void ApplyImageList();

 protected:
	virtual void	Zoom();
	virtual SfxChildAlignment CheckAlignment( SfxChildAlignment eActAlign,
											  SfxChildAlignment eAlign );

 public:
	SvxFontWorkDialog(	SfxBindings *pBindinx,
						SfxChildWindow *pCW,
						Window* pParent,
						const ResId& rResId );
	~SvxFontWorkDialog();

	void SetColorTable(const XColorTable* pTable);
	void SetActive(BOOL bActivate = TRUE);

	void CreateStdFormObj(SdrView& rView, SdrPageView& rPV,
						  const SfxItemSet& rAttr, SdrObject& rOldObj,
						  XFormTextStdForm eForm);
};

#endif		// _SVX_FONTWORK_HXX

