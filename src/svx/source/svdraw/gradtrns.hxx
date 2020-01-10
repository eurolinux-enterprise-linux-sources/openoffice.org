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

#ifndef _GRADTRANS_HXX
#define _GRADTRANS_HXX

#include <svx/xgrad.hxx>
#include <tools/gen.hxx>
#include <basegfx/point/b2dpoint.hxx>

class SdrObject;

class GradTransVector
{
public:
	basegfx::B2DPoint			maPositionA;
	basegfx::B2DPoint			maPositionB;
	Color						aCol1;
	Color						aCol2;
};

class GradTransGradient
{
public:
	XGradient					aGradient;
};

class GradTransformer
{
public:
	GradTransformer() {}

	void GradToVec(GradTransGradient& rG, GradTransVector& rV, 
		const SdrObject* pObj);
	void VecToGrad(GradTransVector& rV, GradTransGradient& rG, 
		GradTransGradient& rGOld, const SdrObject* pObj, sal_Bool bMoveSingle, sal_Bool bMoveFirst);
};

#endif // _GRADTRANS_HXX

