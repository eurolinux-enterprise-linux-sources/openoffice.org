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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYPOLYGONPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYPOLYGONPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <drawinglayer/attribute/fillattribute.hxx>
#include <drawinglayer/attribute/fillbitmapattribute.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <drawinglayer/attribute/lineattribute.hxx>
#include <drawinglayer/attribute/strokeattribute.hxx>
#include <drawinglayer/attribute/linestartendattribute.hxx>

//////////////////////////////////////////////////////////////////////////////
// PolyPolygonHairlinePrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonHairlinePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolyPolygon					maPolyPolygon;
			basegfx::BColor							maBColor;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonHairlinePrimitive2D(const basegfx::B2DPolyPolygon& rPolyPolygon, const basegfx::BColor& rBColor);

			// get data
			basegfx::B2DPolyPolygon getB2DPolyPolygon() const { return maPolyPolygon; }
			const basegfx::BColor& getBColor() const { return maBColor; }

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
// PolyPolygonMarkerPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonMarkerPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolyPolygon					maPolyPolygon;
			basegfx::BColor							maRGBColorA;
			basegfx::BColor							maRGBColorB;
			double									mfDiscreteDashLength;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonMarkerPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::BColor& rRGBColorA,
				const basegfx::BColor& rRGBColorB,
				double fDiscreteDashLength);

			// get data
			basegfx::B2DPolyPolygon getB2DPolyPolygon() const { return maPolyPolygon; }
			const basegfx::BColor& getRGBColorA() const { return maRGBColorA; }
			const basegfx::BColor& getRGBColorB() const { return maRGBColorB; }
			double getDiscreteDashLength() const { return mfDiscreteDashLength; }

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
// PolyPolygonStrokePrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonStrokePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolyPolygon					maPolyPolygon;
			attribute::LineAttribute				maLineAttribute;
			attribute::StrokeAttribute				maStrokeAttribute;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonStrokePrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
    			const attribute::LineAttribute& rLineAttribute,
				const attribute::StrokeAttribute& rStrokeAttribute);

			PolyPolygonStrokePrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
    			const attribute::LineAttribute& rLineAttribute);

			// get data
			basegfx::B2DPolyPolygon getB2DPolyPolygon() const { return maPolyPolygon; }
			const attribute::LineAttribute& getLineAttribute() const { return maLineAttribute; }
			const attribute::StrokeAttribute& getStrokeAttribute() const { return maStrokeAttribute; }

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
// PolyPolygonStrokeArrowPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonStrokeArrowPrimitive2D : public PolyPolygonStrokePrimitive2D
		{
		private:
			attribute::LineStartEndAttribute				maStart;
			attribute::LineStartEndAttribute				maEnd;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonStrokeArrowPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
    			const attribute::LineAttribute& rLineAttribute,
				const attribute::StrokeAttribute& rStrokeAttribute, 
				const attribute::LineStartEndAttribute& rStart, 
				const attribute::LineStartEndAttribute& rEnd);

			PolyPolygonStrokeArrowPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
    			const attribute::LineAttribute& rLineAttribute,
				const attribute::LineStartEndAttribute& rStart, 
				const attribute::LineStartEndAttribute& rEnd);

			// get data
			const attribute::LineStartEndAttribute& getStart() const { return maStart; }
			const attribute::LineStartEndAttribute& getEnd() const { return maEnd; }

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
// PolyPolygonColorPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonColorPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolyPolygon					maPolyPolygon;
			basegfx::BColor							maBColor;

		public:
			PolyPolygonColorPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::BColor& rBColor);

			// get data
			const basegfx::B2DPolyPolygon& getB2DPolyPolygon() const { return maPolyPolygon; }
			const basegfx::BColor& getBColor() const { return maBColor; }

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
// PolyPolygonGradientPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonGradientPrimitive2D : public PolyPolygonColorPrimitive2D
		{
		private:
			attribute::FillGradientAttribute			maFillGradient;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonGradientPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::BColor& rBColor, 
				const attribute::FillGradientAttribute& rFillGradient);

			// get data
			const attribute::FillGradientAttribute& getFillGradient() const { return maFillGradient; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// PolyPolygonHatchPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonHatchPrimitive2D : public PolyPolygonColorPrimitive2D
		{
		private:
			attribute::FillHatchAttribute				maFillHatch;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonHatchPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::BColor& rBColor, 
				const attribute::FillHatchAttribute& rFillHatch);

			// get data
			const attribute::FillHatchAttribute& getFillHatch() const { return maFillHatch; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// PolyPolygonBitmapPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolyPolygonBitmapPrimitive2D : public PolyPolygonColorPrimitive2D
		{
		private:
			attribute::FillBitmapAttribute				maFillBitmap;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolyPolygonBitmapPrimitive2D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::BColor& rBColor, 
				const attribute::FillBitmapAttribute& rFillBitmap);

			// get data
			const attribute::FillBitmapAttribute& getFillBitmap() const { return maFillBitmap; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYPOLYGONPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
