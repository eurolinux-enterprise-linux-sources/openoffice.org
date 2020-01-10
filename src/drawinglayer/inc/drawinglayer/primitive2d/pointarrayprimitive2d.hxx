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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POINTARRAYPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POINTARRAYPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <basegfx/color/bcolor.hxx>

//////////////////////////////////////////////////////////////////////////////
// MarkerPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PointArrayPrimitive2D : public BasePrimitive2D
		{
		private:
			std::vector< basegfx::B2DPoint >				maPositions;
			basegfx::BColor									maRGBColor;

			// #i96669# add simple range buffering for this primitive
			basegfx::B2DRange						maB2DRange;

		public:
			PointArrayPrimitive2D(
				const std::vector< basegfx::B2DPoint >& rPositions,
				const basegfx::BColor& rRGBColor);

			// get data
			const std::vector< basegfx::B2DPoint >& getPositions() const { return maPositions; }
			const basegfx::BColor& getRGBColor() const { return maRGBColor; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// get range
			virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POINTARRAYPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
