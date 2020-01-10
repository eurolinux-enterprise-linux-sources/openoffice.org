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

#ifndef _E3D_POLYGON3D_HXX
#define _E3D_POLYGON3D_HXX

#include <svx/obj3d.hxx>
#include "svx/svxdllapi.h"

class SVX_DLLPUBLIC E3dPolygonObj : public E3dCompoundObject
{
private:
	// #110094# DrawContact section
	virtual sdr::contact::ViewContact* CreateObjectSpecificViewContact();

	// Parameter
	basegfx::B3DPolyPolygon	aPolyPoly3D;
	basegfx::B3DPolyPolygon	aPolyNormals3D;
	basegfx::B2DPolyPolygon	aPolyTexture2D;
	BOOL			bLineOnly;

	SVX_DLLPRIVATE void CreateDefaultNormals();
	SVX_DLLPRIVATE void CreateDefaultTexture();

public:
	void SetPolyPolygon3D(const basegfx::B3DPolyPolygon& rNewPolyPoly3D);
	void SetPolyNormals3D(const basegfx::B3DPolyPolygon& rNewPolyPoly3D);
	void SetPolyTexture2D(const basegfx::B2DPolyPolygon& rNewPolyPoly2D);

	TYPEINFO();

	E3dPolygonObj(
		E3dDefaultAttributes& rDefault, 
		const basegfx::B3DPolyPolygon& rPolyPoly3D,
		BOOL bLinOnly=FALSE);
	E3dPolygonObj(
		E3dDefaultAttributes& rDefault, 
		const basegfx::B3DPolyPolygon& rPolyPoly3D,
		const basegfx::B3DPolyPolygon& rPolyNormals3D, 
		BOOL bLinOnly=FALSE);
	E3dPolygonObj(
		E3dDefaultAttributes& rDefault, 
		const basegfx::B3DPolyPolygon& rPolyPoly3D,
		const basegfx::B3DPolyPolygon& rPolyNormals3D, 
		const basegfx::B2DPolyPolygon& rPolyTexture2D, 
		BOOL bLinOnly=FALSE);

	E3dPolygonObj();
	virtual ~E3dPolygonObj();

	const basegfx::B3DPolyPolygon& GetPolyPolygon3D() const { return aPolyPoly3D; }
	const basegfx::B3DPolyPolygon& GetPolyNormals3D() const { return aPolyNormals3D; }
	const basegfx::B2DPolyPolygon& GetPolyTexture2D() const { return aPolyTexture2D; }

	virtual UINT16 GetObjIdentifier() const;
	virtual SdrObject* DoConvertToPolyObj(BOOL bBezier) const;

	virtual void operator=(const SdrObject&);

	// LineOnly?
	BOOL GetLineOnly() { return bLineOnly; }
	void SetLineOnly(BOOL bNew);
};

#endif			// _E3D_POLYGON3D_HXX
