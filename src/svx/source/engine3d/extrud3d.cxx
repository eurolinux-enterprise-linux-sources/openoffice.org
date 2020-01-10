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

#include "svdstr.hrc"
#include "svdglob.hxx"
#include <svx/svdpage.hxx>
#include "globl3d.hxx"
#include <svx/extrud3d.hxx>
#include <svx/scene3d.hxx>

#include <svx/svxids.hrc>
#include <svx/xpoly.hxx>
#include <svx/svdopath.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svx3ditems.hxx>
#include <svx/sdr/properties/e3dextrudeproperties.hxx>
#include <svx/sdr/contact/viewcontactofe3dextrude.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b3dpolygontools.hxx>
#include <basegfx/polygon/b3dpolypolygontools.hxx>

//////////////////////////////////////////////////////////////////////////////
// #110094# DrawContact section

sdr::contact::ViewContact* E3dExtrudeObj::CreateObjectSpecificViewContact()
{
	return new sdr::contact::ViewContactOfE3dExtrude(*this);
}

//////////////////////////////////////////////////////////////////////////////

sdr::properties::BaseProperties* E3dExtrudeObj::CreateObjectSpecificProperties()
{
	return new sdr::properties::E3dExtrudeProperties(*this);
}

//////////////////////////////////////////////////////////////////////////////

TYPEINIT1(E3dExtrudeObj, E3dCompoundObject);

/*************************************************************************
|*
|* Konstruktor, erzeugt zwei Deckelflaechen-PolyPolygone und (PointCount-1)
|* Seitenflaechen-Rechtecke aus dem uebergebenen PolyPolygon
|*
\************************************************************************/

E3dExtrudeObj::E3dExtrudeObj(E3dDefaultAttributes& rDefault, const basegfx::B2DPolyPolygon& rPP, double fDepth)
:	E3dCompoundObject(rDefault),
	maExtrudePolygon(rPP)
{
	// since the old class PolyPolygon3D did mirror the given PolyPolygons in Y, do the same here
	basegfx::B2DHomMatrix aMirrorY;
	aMirrorY.scale(1.0, -1.0);
	maExtrudePolygon.transform(aMirrorY);

	// Defaults setzen
	SetDefaultAttributes(rDefault);

	// set extrude depth
	GetProperties().SetObjectItemDirect(Svx3DDepthItem((sal_uInt32)(fDepth + 0.5)));
}

E3dExtrudeObj::E3dExtrudeObj()
:	E3dCompoundObject()
{
	// Defaults setzen
	E3dDefaultAttributes aDefault;
	SetDefaultAttributes(aDefault);
}

void E3dExtrudeObj::SetDefaultAttributes(E3dDefaultAttributes& rDefault)
{
	GetProperties().SetObjectItemDirect(Svx3DSmoothNormalsItem(rDefault.GetDefaultExtrudeSmoothed()));
	GetProperties().SetObjectItemDirect(Svx3DSmoothLidsItem(rDefault.GetDefaultExtrudeSmoothFrontBack()));
	GetProperties().SetObjectItemDirect(Svx3DCharacterModeItem(rDefault.GetDefaultExtrudeCharacterMode()));
	GetProperties().SetObjectItemDirect(Svx3DCloseFrontItem(rDefault.GetDefaultExtrudeCloseFront()));
	GetProperties().SetObjectItemDirect(Svx3DCloseBackItem(rDefault.GetDefaultExtrudeCloseBack()));

	// Bei extrudes defaultmaessig StdTexture in X und Y
	GetProperties().SetObjectItemDirect(Svx3DTextureProjectionXItem(1));
	GetProperties().SetObjectItemDirect(Svx3DTextureProjectionYItem(1));
}

/*************************************************************************
|*
|* Identifier zurueckgeben
|*
\************************************************************************/

UINT16 E3dExtrudeObj::GetObjIdentifier() const
{
	return E3D_EXTRUDEOBJ_ID;
}

/*************************************************************************
|*
|* Zuweisungsoperator
|*
\************************************************************************/

void E3dExtrudeObj::operator=(const SdrObject& rObj)
{
	// erstmal alle Childs kopieren
	E3dCompoundObject::operator=(rObj);

	// weitere Parameter kopieren
	const E3dExtrudeObj& r3DObj = (const E3dExtrudeObj&)rObj;

	maExtrudePolygon = r3DObj.maExtrudePolygon;
}

/*************************************************************************
|*
|* Lokale Parameter setzen mit Geometrieneuerzeugung
|*
\************************************************************************/

void E3dExtrudeObj::SetExtrudePolygon(const basegfx::B2DPolyPolygon &rNew)
{
	if(maExtrudePolygon != rNew)
	{
		maExtrudePolygon = rNew;
		ActionChanged();
	}
}

/*************************************************************************
|*
|* Get the name of the object (singular)
|*
\************************************************************************/

void E3dExtrudeObj::TakeObjNameSingul(XubString& rName) const
{
	rName=ImpGetResStr(STR_ObjNameSingulExtrude3d);

	String aName( GetName() );
	if(aName.Len())
	{
		rName += sal_Unicode(' ');
		rName += sal_Unicode('\'');
		rName += aName;
		rName += sal_Unicode('\'');
	}
}

/*************************************************************************
|*
|* Get the name of the object (plural)
|*
\************************************************************************/

void E3dExtrudeObj::TakeObjNamePlural(XubString& rName) const
{
	rName=ImpGetResStr(STR_ObjNamePluralExtrude3d);
}

/*************************************************************************
|*
|* Aufbrechen
|*
\************************************************************************/

BOOL E3dExtrudeObj::IsBreakObjPossible()
{
	return TRUE;
}

SdrAttrObj* E3dExtrudeObj::GetBreakObj()
{
	basegfx::B3DPolyPolygon aFrontSide;
	basegfx::B3DPolyPolygon aBackSide;

	if(maExtrudePolygon.count())
	{
		basegfx::B2DPolyPolygon aTemp(maExtrudePolygon);
		aTemp.removeDoublePoints();
		aTemp = basegfx::tools::correctOrientations(aTemp);
		const basegfx::B2VectorOrientation aOrient = basegfx::tools::getOrientation(aTemp.getB2DPolygon(0L));

		if(basegfx::ORIENTATION_POSITIVE == aOrient)
		{
			aTemp.flip();
		}
	
		aFrontSide = basegfx::tools::createB3DPolyPolygonFromB2DPolyPolygon(aTemp);
	}

    if(aFrontSide.count())
    {
        aBackSide = aFrontSide;

	    if(GetExtrudeDepth())
	    {
		    basegfx::B3DHomMatrix aTransform;

            if(100 != GetPercentBackScale()) 
		    {
                // scale polygon from center
                const double fScaleFactor(GetPercentBackScale() / 100.0);
	            const basegfx::B3DRange aPolyPolyRange(basegfx::tools::getRange(aBackSide));
	            const basegfx::B3DPoint aCenter(aPolyPolyRange.getCenter());

	            aTransform.translate(-aCenter.getX(), -aCenter.getY(), -aCenter.getZ());
	            aTransform.scale(fScaleFactor, fScaleFactor, fScaleFactor);
	            aTransform.translate(aCenter.getX(), aCenter.getY(), aCenter.getZ());
		    }

            // translate by extrude depth
		    aTransform.translate(0.0, 0.0, (double)GetExtrudeDepth());
		    
            aBackSide.transform(aTransform);
	    }
    }

    if(aBackSide.count())
    {
	// create PathObj
	    basegfx::B2DPolyPolygon aPoly = TransformToScreenCoor(aBackSide);
		SdrPathObj* pPathObj = new SdrPathObj(OBJ_PLIN, aPoly);

		if(pPathObj)
		{
			SfxItemSet aSet(GetObjectItemSet());
			aSet.Put(XLineStyleItem(XLINE_SOLID));
			pPathObj->SetMergedItemSet(aSet);
		}

		return pPathObj;
	}

    return 0;
}

// eof
