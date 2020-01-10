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
#ifndef _SDTAAITM_HXX
#define _SDTAAITM_HXX

#include <svtools/intitem.hxx>
#include <svx/svddef.hxx>
#include "svx/svxdllapi.h"

class SVX_DLLPUBLIC SdrTextAniAmountItem: public SfxInt16Item {
public:
	TYPEINFO();
	SdrTextAniAmountItem(INT16 nVal=0): SfxInt16Item(SDRATTR_TEXT_ANIAMOUNT,nVal) {}
	SdrTextAniAmountItem(SvStream& rIn): SfxInt16Item(SDRATTR_TEXT_ANIAMOUNT,rIn) {}
	virtual SfxPoolItem* Clone(SfxItemPool* pPool=NULL) const;
	virtual SfxPoolItem* Create(SvStream& rIn, USHORT nVer) const;
	virtual FASTBOOL HasMetrics() const;
	virtual FASTBOOL ScaleMetrics(long nMul, long nDiv);

    virtual SfxItemPresentation GetPresentation(SfxItemPresentation ePres, SfxMapUnit eCoreMetric, SfxMapUnit ePresMetric, String& rText, const IntlWrapper * = 0) const;
};

#endif
