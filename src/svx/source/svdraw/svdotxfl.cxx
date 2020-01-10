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
#include <svx/eeitem.hxx>

#include <svx/svdfield.hxx>
#include <svx/svdotext.hxx>

static BOOL bInit = FALSE;

// Do not remove this, it is still used in src536a!
void SdrRegisterFieldClasses()
{
	if ( !bInit )
	{
		SvxFieldItem::GetClassManager().SV_CLASS_REGISTER(SdrMeasureField);
		SvxFieldItem::GetClassManager().SV_CLASS_REGISTER(SvxHeaderField);
		SvxFieldItem::GetClassManager().SV_CLASS_REGISTER(SvxFooterField);
		SvxFieldItem::GetClassManager().SV_CLASS_REGISTER(SvxDateTimeField);
		bInit = TRUE;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////// */

FASTBOOL SdrTextObj::CalcFieldValue(const SvxFieldItem& /*rField*/, USHORT /*nPara*/, USHORT /*nPos*/,
	FASTBOOL /*bEdit*/,	Color*& /*rpTxtColor*/, Color*& /*rpFldColor*/, XubString& /*rRet*/) const
{
	return FALSE;
}

