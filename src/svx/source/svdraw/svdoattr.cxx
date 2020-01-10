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

#include <svx/svdoattr.hxx>
#include <svx/xpool.hxx>
#include "svditext.hxx"
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdattr.hxx>
#include <svx/svdattrx.hxx>
#include <svx/svdpool.hxx>
#include <svx/svdotext.hxx>
#include <svx/svdocapt.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdorect.hxx>
#include <svx/svdocirc.hxx>
#include <svx/svdomeas.hxx>
#include <svtools/smplhint.hxx>
#include <svtools/itemiter.hxx>
#include <svx/xenum.hxx>
#include <svx/xlineit0.hxx>
#include <svx/xlnstwit.hxx>
#include <svx/xlnedwit.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xflbmtit.hxx>
#include <svx/xtextit0.hxx>
#include <svx/xflbstit.hxx>
#include <svx/xflbtoxy.hxx>
#include <svx/xftshit.hxx>


#include <svx/colritem.hxx>
#include "fontitem.hxx"
#include <svx/fhgtitem.hxx>

//#include <svx/charscaleitem.hxx>
#include <svx/xlnstcit.hxx>
#include <svx/xlnwtit.hxx>
#include <svtools/style.hxx>
#include <svtools/style.hxx>
#include <svtools/whiter.hxx>
#include <svx/xlnclit.hxx>
#include <svx/xflclit.hxx>
#include <svx/xlntrit.hxx>
#include <svx/xfltrit.hxx>
#include <svx/xlnedcit.hxx>
#include <svx/adjitem.hxx>
#include <svx/xflbckit.hxx>
#include <svx/xtable.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/xlndsit.hxx>
#include <svx/xlnedit.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflftrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xlnstit.hxx>
#include <svx/sdr/properties/attributeproperties.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include "xlinjoit.hxx"
#include <svdoimp.hxx>

//////////////////////////////////////////////////////////////////////////////

sdr::properties::BaseProperties* SdrAttrObj::CreateObjectSpecificProperties()
{
	return new sdr::properties::AttributeProperties(*this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TYPEINIT1(SdrAttrObj,SdrObject);

SdrAttrObj::SdrAttrObj()
{
}

SdrAttrObj::~SdrAttrObj()
{
}

const Rectangle& SdrAttrObj::GetSnapRect() const
{
	if(bSnapRectDirty)
	{
		((SdrAttrObj*)this)->RecalcSnapRect();
		((SdrAttrObj*)this)->bSnapRectDirty = false;
	}

	return maSnapRect;
}

void SdrAttrObj::SetModel(SdrModel* pNewModel)
{
	SdrModel* pOldModel = pModel;

	// test for correct pool in ItemSet; move to new pool if necessary
	if(pNewModel && GetObjectItemPool() && GetObjectItemPool() != &pNewModel->GetItemPool())
	{
		MigrateItemPool(GetObjectItemPool(), &pNewModel->GetItemPool(), pNewModel);
	}

	// call parent
	SdrObject::SetModel(pNewModel);

	// modify properties
	GetProperties().SetModel(pOldModel, pNewModel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// syntactical sugar for ItemSet accesses

void __EXPORT SdrAttrObj::Notify(SfxBroadcaster& /*rBC*/, const SfxHint& rHint)
{
	SfxSimpleHint *pSimple = PTR_CAST(SfxSimpleHint, &rHint);
	BOOL bDataChg(pSimple && SFX_HINT_DATACHANGED == pSimple->GetId());

	if(bDataChg)
	{
		Rectangle aBoundRect = GetLastBoundRect();
		SetBoundRectDirty();
		SetRectsDirty(sal_True);

		// This may have lead to object change
		SetChanged();
		BroadcastObjectChange();
		SendUserCall(SDRUSERCALL_CHGATTR, aBoundRect);
	}
}

sal_Int32 SdrAttrObj::ImpGetLineWdt() const
{
	sal_Int32 nRetval(0);

	if(XLINE_NONE != ((XLineStyleItem&)(GetObjectItem(XATTR_LINESTYLE))).GetValue())
	{
		nRetval = ((XLineWidthItem&)(GetObjectItem(XATTR_LINEWIDTH))).GetValue();
	}

	return nRetval;
}

BOOL SdrAttrObj::HasFill() const
{
	return bClosedObj && ((XFillStyleItem&)(GetProperties().GetObjectItemSet().Get(XATTR_FILLSTYLE))).GetValue()!=XFILL_NONE;
}

BOOL SdrAttrObj::HasLine() const
{
	return ((XLineStyleItem&)(GetProperties().GetObjectItemSet().Get(XATTR_LINESTYLE))).GetValue()!=XLINE_NONE;
}

// eof
