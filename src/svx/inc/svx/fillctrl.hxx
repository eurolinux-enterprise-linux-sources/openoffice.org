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

#ifndef _FILLCTRL_HXX
#define _FILLCTRL_HXX

#include <svtools/lstner.hxx>
#include <sfx2/tbxctrl.hxx>
#include "svx/svxdllapi.h"

class XFillStyleItem;
class XFillColorItem;
class XFillGradientItem;
class XFillHatchItem;
class XFillBitmapItem;
class FillControl;
class SvxFillTypeBox;
class SvxFillAttrBox;
class ListBox;

/*************************************************************************
|*
|* Klassen fuer Flaechenattribute (Controls und Controller)
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxFillToolBoxControl: public SfxToolBoxControl
{
private:
	XFillStyleItem*		pStyleItem;
	XFillColorItem*		pColorItem;
	XFillGradientItem*	pGradientItem;
	XFillHatchItem*		pHatchItem;
	XFillBitmapItem*	pBitmapItem;

	FillControl*		pFillControl;
	SvxFillTypeBox*		pFillTypeLB;

	BOOL				bUpdate;
    BOOL                bIgnoreStatusUpdate;
	USHORT				eLastXFS;

public:
	SFX_DECL_TOOLBOX_CONTROL();

	SvxFillToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
	~SvxFillToolBoxControl();

	virtual void		StateChanged( USHORT nSID, SfxItemState eState,
									  const SfxPoolItem* pState );
	void 		        Update( const SfxPoolItem* pState );
	virtual Window*		CreateItemWindow( Window *pParent );
    void                IgnoreStatusUpdate( sal_Bool bSet );
};

//========================================================================

class FillControl : public Window
{
private:
	friend class SvxFillToolBoxControl;

	SvxFillTypeBox*	pLbFillType;
    Size            aLogicalFillSize;
    Size            aLogicalAttrSize;
    Timer           aDelayTimer;

//#if 0 // _SOLAR__PRIVATE
	DECL_LINK( DelayHdl, Timer * );
	DECL_LINK( SelectFillTypeHdl, ListBox * );
	DECL_LINK( SelectFillAttrHdl, ListBox * );
//#endif
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );
public:
	FillControl( Window* pParent, WinBits nStyle = 0 );
	~FillControl();

	virtual void Resize();
};

#endif		// _FILLCTRL_HXX

