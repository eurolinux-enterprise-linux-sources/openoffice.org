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

#ifndef _SVX_GRAFCTRL_HXX
#define _SVX_GRAFCTRL_HXX

#include <svtools/lstner.hxx>
#include <svtools/intitem.hxx>
#include <sfx2/tbxctrl.hxx>
#include "svx/svxdllapi.h"

// ----------------
// - TbxImageItem -
// ----------------

class SVX_DLLPUBLIC TbxImageItem : public SfxUInt16Item
{
public:
							TYPEINFO();
							TbxImageItem( USHORT nWhich = 0, UINT16 nImage = 0 );

	virtual SfxPoolItem*	Clone( SfxItemPool* pPool = 0 ) const;
	virtual int 			operator==( const SfxPoolItem& ) const;
};

// -------------------------------
// - SvxGrafFilterToolBoxControl -
// -------------------------------

class SVX_DLLPUBLIC SvxGrafFilterToolBoxControl : public SfxToolBoxControl
{
public:

								SFX_DECL_TOOLBOX_CONTROL();
								
								SvxGrafFilterToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
								~SvxGrafFilterToolBoxControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

// -------------------------
// - SvxGrafToolBoxControl -
// -------------------------

class SvxGrafToolBoxControl : public SfxToolBoxControl
{
public:

						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
						~SvxGrafToolBoxControl();

	virtual void		StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual Window*		CreateItemWindow( Window *pParent );
};

// ----------------------------
// - SvxGrafRedToolBoxControl -
// ----------------------------

class SVX_DLLPUBLIC SvxGrafRedToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafRedToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// ------------------------------
// - SvxGrafGreenToolBoxControl -
// ------------------------------

class SVX_DLLPUBLIC SvxGrafGreenToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafGreenToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// -----------------------------
// - SvxGrafBlueToolBoxControl -
// -----------------------------

class SVX_DLLPUBLIC SvxGrafBlueToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafBlueToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// ----------------------------------
// - SvxGrafLuminanceToolBoxControl -
// ----------------------------------

class SVX_DLLPUBLIC SvxGrafLuminanceToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafLuminanceToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// ---------------------------------
// - SvxGrafContrastToolBoxControl -
// ---------------------------------

class SVX_DLLPUBLIC SvxGrafContrastToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafContrastToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// ------------------------------
// - SvxGrafGammaToolBoxControl -
// ------------------------------

class SVX_DLLPUBLIC SvxGrafGammaToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafGammaToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// -------------------------------------
// - SvxGrafTransparenceToolBoxControl -
// -------------------------------------

class SVX_DLLPUBLIC SvxGrafTransparenceToolBoxControl : public SvxGrafToolBoxControl
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafTransparenceToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
};

// -----------------------------
// - SvxGrafModeToolBoxControl -
// -----------------------------

class SVX_DLLPUBLIC SvxGrafModeToolBoxControl : public SfxToolBoxControl, public SfxListener
{
public:
						SFX_DECL_TOOLBOX_CONTROL();
						SvxGrafModeToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
						~SvxGrafModeToolBoxControl();

	virtual void		StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual Window*		CreateItemWindow( Window *pParent );
};

// ---------------------
// - SvxGrafAttrHelper -
// ---------------------

class SdrView;
class SfxRequest;

class SVX_DLLPUBLIC SvxGrafAttrHelper
{
public:

	static void		ExecuteGrafAttr( SfxRequest& rReq, SdrView& rView );
	static void		GetGrafAttrState( SfxItemSet& rSet, SdrView& rView );
};

#endif // _SVX_GRAFCTRL_HXX
