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
#ifndef _SYLAYITM_HXX
#define _SYLAYITM_HXX

#include <svx/svddef.hxx>
#include <svtools/stritem.hxx>
#include <svtools/intitem.hxx>
#include <svx/svdtypes.hxx>

//------------------------------
// class SdrLayerIdItem
//------------------------------
class SdrLayerIdItem: public SfxUInt16Item {
public:
	SdrLayerIdItem(USHORT nId=0): SfxUInt16Item(SDRATTR_LAYERID,nId)  {}
	SdrLayerIdItem(SvStream& rIn): SfxUInt16Item(SDRATTR_LAYERID,rIn) {}
	SdrLayerID GetValue() const { return (SdrLayerID)SfxUInt16Item::GetValue(); }
};

//------------------------------
// class SdrLayerNameItem
//------------------------------

class SdrLayerNameItem: public SfxStringItem {
public:
	SdrLayerNameItem()                   : SfxStringItem() { SetWhich(SDRATTR_LAYERNAME); }
	SdrLayerNameItem(const String& rStr) : SfxStringItem(SDRATTR_LAYERNAME,rStr) {}
	SdrLayerNameItem(SvStream& rIn)      : SfxStringItem(SDRATTR_LAYERNAME,rIn)  {}
};

#endif
