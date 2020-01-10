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
#include "precompiled_drawinglayer.hxx"

#include <drawinglayer/primitive2d/fillgradientprimitive2d.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <drawinglayer/texture/texture.hxx>
#include <drawinglayer/primitive2d/polypolygonprimitive2d.hxx>
#include <basegfx/tools/canvastools.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>

//////////////////////////////////////////////////////////////////////////////

using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		Primitive2DSequence FillGradientPrimitive2D::createLocalDecomposition(const geometry::ViewInformation2D& /*rViewInformation*/) const
		{
			const attribute::GradientStyle aGradientStyle(maFillGradient.getStyle());
			::std::vector< basegfx::B2DHomMatrix > aMatrices;
			::std::vector< basegfx::BColor > aColors;
			basegfx::B2DPolygon aUnitPolygon;
			sal_uInt32 nSteps(maFillGradient.getSteps());

			if(attribute::GRADIENTSTYLE_RADIAL == aGradientStyle || attribute::GRADIENTSTYLE_ELLIPTICAL == aGradientStyle)
			{
				const basegfx::B2DPoint aCircleCenter(0.5, 0.5);
				aUnitPolygon = basegfx::tools::createPolygonFromEllipse(aCircleCenter, 0.5, 0.5);
			}
			else
			{
				aUnitPolygon = basegfx::tools::createPolygonFromRect(basegfx::B2DRange(0.0, 0.0, 1.0, 1.0));
			}

			// make sure steps is not too high/low
			const basegfx::BColor aStart(maFillGradient.getStartColor());
			const basegfx::BColor aEnd(maFillGradient.getEndColor());
			const sal_uInt32 nMaxSteps(sal_uInt32((aStart.getMaximumDistance(aEnd) * 127.5) + 0.5));

			if(nSteps == 0L)
			{
				nSteps = nMaxSteps;
			}

			if(nSteps < 2L)
			{
				nSteps = 2L;
			}

			if(nSteps > nMaxSteps)
			{
				nSteps = nMaxSteps;
			}

			switch(aGradientStyle)
			{
				case attribute::GRADIENTSTYLE_LINEAR:
				{
					texture::GeoTexSvxGradientLinear aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), -maFillGradient.getAngle());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
				case attribute::GRADIENTSTYLE_AXIAL:
				{
					texture::GeoTexSvxGradientAxial aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), -maFillGradient.getAngle());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
				case attribute::GRADIENTSTYLE_RADIAL:
				{
					texture::GeoTexSvxGradientRadial aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), maFillGradient.getOffsetX(), maFillGradient.getOffsetY());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
				case attribute::GRADIENTSTYLE_ELLIPTICAL:
				{
					texture::GeoTexSvxGradientElliptical aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), maFillGradient.getOffsetX(), maFillGradient.getOffsetY(), -maFillGradient.getAngle());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
				case attribute::GRADIENTSTYLE_SQUARE:
				{
					texture::GeoTexSvxGradientSquare aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), maFillGradient.getOffsetX(), maFillGradient.getOffsetY(), -maFillGradient.getAngle());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
				case attribute::GRADIENTSTYLE_RECT:
				{
					texture::GeoTexSvxGradientRect aGradient(getObjectRange(), aStart, aEnd, nSteps, maFillGradient.getBorder(), maFillGradient.getOffsetX(), maFillGradient.getOffsetY(), -maFillGradient.getAngle());
					aGradient.appendTransformations(aMatrices);
					aGradient.appendColors(aColors);
					break;
				}
			}

			// prepare return value
			Primitive2DSequence aRetval(aColors.size() ? aMatrices.size() + 1L : aMatrices.size());

			// create solid fill with start color
			if(aColors.size())
			{
				// create primitive
				const Primitive2DReference xRef(new PolyPolygonColorPrimitive2D(basegfx::B2DPolyPolygon(basegfx::tools::createPolygonFromRect(getObjectRange())), aColors[0L]));
				aRetval[0L] = xRef;
			}

			// create solid fill steps
			for(sal_uInt32 a(0L); a < aMatrices.size(); a++)
			{
				// create part polygon
				basegfx::B2DPolygon aNewPoly(aUnitPolygon);
				aNewPoly.transform(aMatrices[a]);

				// create solid fill
				const Primitive2DReference xRef(new PolyPolygonColorPrimitive2D(basegfx::B2DPolyPolygon(aNewPoly), aColors[a + 1L]));
				aRetval[a + 1L] = xRef;
			}

			return aRetval;
		}

		FillGradientPrimitive2D::FillGradientPrimitive2D(
			const basegfx::B2DRange& rObjectRange, 
			const attribute::FillGradientAttribute& rFillGradient)
		:	BasePrimitive2D(),
			maObjectRange(rObjectRange),
			maFillGradient(rFillGradient)
		{
		}

		bool FillGradientPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(BasePrimitive2D::operator==(rPrimitive))
			{
				const FillGradientPrimitive2D& rCompare = (FillGradientPrimitive2D&)rPrimitive;

				return (getObjectRange() == rCompare.getObjectRange() 
					&& maFillGradient == rCompare.maFillGradient);
			}

			return false;
		}

		basegfx::B2DRange FillGradientPrimitive2D::getB2DRange(const geometry::ViewInformation2D& /*rViewInformation*/) const
		{
			// return ObjectRange
			return getObjectRange();
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(FillGradientPrimitive2D, PRIMITIVE2D_ID_FILLGRADIENTPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
