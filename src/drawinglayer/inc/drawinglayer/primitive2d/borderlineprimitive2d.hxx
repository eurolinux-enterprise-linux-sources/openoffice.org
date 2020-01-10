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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_BORDERLINEPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_BORDERLINEPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class BorderLinePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPoint								maStart;
			basegfx::B2DPoint								maEnd;
			double											mfLeftWidth;
			double											mfDistance;
			double											mfRightWidth;
			double											mfExtendInnerStart;
			double											mfExtendInnerEnd;
			double											mfExtendOuterStart;
			double											mfExtendOuterEnd;
			basegfx::BColor									maRGBColor;

			// bitfield
			unsigned										mbCreateInside : 1;
			unsigned										mbCreateOutside : 1;

			// helpers
			double getCorrectedLeftWidth() const 
			{ 
				return basegfx::fTools::equal(1.0, mfLeftWidth) ? 0.0 : mfLeftWidth; 
			}
			
			double getCorrectedDistance() const 
			{ 
				return basegfx::fTools::equal(1.0, mfDistance) ? 0.0 : mfDistance; 
			}

			double getCorrectedRightWidth() const 
			{ 
				return basegfx::fTools::equal(1.0, mfRightWidth) ? 0.0 : mfRightWidth; 
			}
			
			double getWidth() const 
			{ 
				return getCorrectedLeftWidth() + getCorrectedDistance() + getCorrectedRightWidth();
			}

			bool leftIsHairline() const
			{
				return basegfx::fTools::equal(1.0, mfLeftWidth); 
			}

			bool rightIsHairline() const
			{
				return basegfx::fTools::equal(1.0, mfRightWidth); 
			}

			bool isInsideUsed() const
			{
				return !basegfx::fTools::equalZero(mfLeftWidth);
			}

			bool isOutsideUsed() const
			{
				return !basegfx::fTools::equalZero(mfRightWidth);
			}

		protected:
			// create local decomposition
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			BorderLinePrimitive2D(
				const basegfx::B2DPoint& rStart,
				const basegfx::B2DPoint& rEnd,
				double fLeftWidth,
				double fDistance,
				double fRightWidth,
				double fExtendInnerStart,
				double fExtendInnerEnd,
				double fExtendOuterStart,
				double fExtendOuterEnd,
				bool bCreateInside,
				bool bCreateOutside,
				const basegfx::BColor& rRGBColor);

			// get data
			const basegfx::B2DPoint& getStart() const { return maStart; }
			const basegfx::B2DPoint& getEnd() const { return maEnd; }
			double getLeftWidth() const { return mfLeftWidth; }
			double getDistance() const { return mfDistance; }
			double getRightWidth() const { return mfRightWidth; }
			double getExtendInnerStart() const { return mfExtendInnerStart; }
			double getExtendInnerEnd() const { return mfExtendInnerEnd; }
			double getExtendOuterStart() const { return mfExtendOuterStart; }
			double getExtendOuterEnd() const { return mfExtendOuterEnd; }
			bool getCreateInside() const { return mbCreateInside; }
			bool getCreateOutside() const { return mbCreateOutside; }
			const basegfx::BColor& getRGBColor() const { return maRGBColor; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_BORDERLINEPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
