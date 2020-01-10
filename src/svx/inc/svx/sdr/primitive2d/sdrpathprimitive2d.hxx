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

#ifndef INCLUDED_SDR_PRIMITIVE2D_SDRPATHPRIMITIVE2D_HXX
#define INCLUDED_SDR_PRIMITIVE2D_SDRPATHPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <drawinglayer/attribute/sdrattribute.hxx>
#include <svx/sdr/attribute/sdrallattribute.hxx>

//////////////////////////////////////////////////////////////////////////////
// predefines

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class SdrPathPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DHomMatrix						maTransform;
			attribute::SdrLineFillShadowTextAttribute	maSdrLFSTAttribute;
			basegfx::B2DPolyPolygon						maUnitPolyPolygon;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& aViewInformation) const;

		public:
			SdrPathPrimitive2D(
				const basegfx::B2DHomMatrix& rTransform, 
				const attribute::SdrLineFillShadowTextAttribute& rSdrLFSTAttribute,
				const basegfx::B2DPolyPolygon& rUnitPolyPolygon);

			// data access
			const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
			const attribute::SdrLineFillShadowTextAttribute& getSdrLFSTAttribute() const { return maSdrLFSTAttribute; }
			const basegfx::B2DPolyPolygon& getUnitPolyPolygon() const { return maUnitPolyPolygon; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_SDR_PRIMITIVE2D_SDRPATHPRIMITIVE2D_HXX

// eof
