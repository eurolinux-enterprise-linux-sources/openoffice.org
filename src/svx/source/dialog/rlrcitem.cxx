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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"

// INCLUDE ---------------------------------------------------------------
#include <svtools/rectitem.hxx>






#include <svx/dialogs.hrc>

#include <svx/ruler.hxx>
#include <svx/lrspitem.hxx>
#include <svx/ulspitem.hxx>
#include <svx/tstpitem.hxx>
#include "protitem.hxx"
#include "rlrcitem.hxx"
#include "rulritem.hxx"
#include <svtools/eitem.hxx>

// class SvxRulerItem ----------------------------------------------------

SvxRulerItem::SvxRulerItem(USHORT _nId, SvxRuler &rRul, SfxBindings &rBindings)
: SfxControllerItem(_nId, rBindings),
  rRuler(rRul)
{
}

// -----------------------------------------------------------------------

void SvxRulerItem::StateChanged( USHORT nSID, SfxItemState eState,
								 const SfxPoolItem* pState)
{
	// SFX_ITEM_DONTCARE => pState == -1 => PTR_CAST buff
	if ( eState != SFX_ITEM_AVAILABLE )
		pState = 0;

	switch(nSID)
	{
		// Linker / rechter Seitenrand
		case SID_RULER_LR_MIN_MAX:
		{
			const SfxRectangleItem *pItem = PTR_CAST(SfxRectangleItem, pState);
			rRuler.UpdateFrameMinMax(pItem);
			break;
		}
        case SID_ATTR_LONG_LRSPACE:
		{
			const SvxLongLRSpaceItem *pItem = PTR_CAST(SvxLongLRSpaceItem, pState);
			DBG_ASSERT(pState? 0 != pItem: TRUE, "SvxLRSpaceItem erwartet");
			rRuler.UpdateFrame(pItem);
			break;
		}
        case SID_ATTR_LONG_ULSPACE:
		{
			const SvxLongULSpaceItem *pItem = PTR_CAST(SvxLongULSpaceItem, pState);
			DBG_ASSERT(pState? 0 != pItem: TRUE, "SvxULSpaceItem erwartet");
			rRuler.UpdateFrame(pItem);
			break;
		}
        case SID_ATTR_TABSTOP_VERTICAL:
        case SID_ATTR_TABSTOP:
		{
			const SvxTabStopItem *pItem = PTR_CAST(SvxTabStopItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxTabStopItem erwartet");
			rRuler.Update(pItem);
			break;
		}
        case SID_ATTR_PARA_LRSPACE_VERTICAL:
        case SID_ATTR_PARA_LRSPACE:
		{
			const SvxLRSpaceItem *pItem = PTR_CAST(SvxLRSpaceItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxLRSpaceItem erwartet");
			rRuler.UpdatePara(pItem);
			break;
		}
        case SID_RULER_BORDERS_VERTICAL:
        case SID_RULER_BORDERS:
		case SID_RULER_ROWS:
		case SID_RULER_ROWS_VERTICAL:
		{
			const SvxColumnItem *pItem = PTR_CAST(SvxColumnItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxColumnItem erwartet");
#ifdef DBG_UTIL
			if(pItem)
			{
				if(pItem->IsConsistent())
                    rRuler.Update(pItem, nSID);
				else
					DBG_ERROR("Spaltenitem corrupted");
			}
			else
                rRuler.Update(pItem, nSID);
#else
            rRuler.Update(pItem, nSID);
#endif
			break;
		}
		case SID_RULER_PAGE_POS:
		{	// Position Seite, Seitenbreite
			const SvxPagePosSizeItem *pItem = PTR_CAST(SvxPagePosSizeItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxPagePosSizeItem erwartet");
			rRuler.Update(pItem);
			break;
		}
		case SID_RULER_OBJECT:
		{	// Object-Selektion
			const SvxObjectItem *pItem = PTR_CAST(SvxObjectItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxObjectItem erwartet");
			rRuler.Update(pItem);
			break;
		}
		case SID_RULER_PROTECT:
		{
			const SvxProtectItem *pItem = PTR_CAST(SvxProtectItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxProtectItem erwartet");
			rRuler.Update(pItem);
			break;
		}
        case SID_RULER_BORDER_DISTANCE:
        {
            const SvxLRSpaceItem *pItem = PTR_CAST(SvxLRSpaceItem, pState);
			DBG_ASSERT(pState?  0 != pItem: TRUE, "SvxLRSpaceItem erwartet");
            rRuler.UpdateParaBorder(pItem);
        }
        break;
        case SID_RULER_TEXT_RIGHT_TO_LEFT :
        {
            const SfxBoolItem *pItem = PTR_CAST(SfxBoolItem, pState);
            DBG_ASSERT(pState?  0 != pItem: TRUE, "SfxBoolItem erwartet");
            rRuler.UpdateTextRTL(pItem);
        }
        break;
	}
}


