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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYGONPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYGONPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <drawinglayer/attribute/lineattribute.hxx>
#include <drawinglayer/attribute/strokeattribute.hxx>
#include <drawinglayer/attribute/linestartendattribute.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>

//////////////////////////////////////////////////////////////////////////////
// PolygonHairlinePrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolygonHairlinePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolygon						maPolygon;
			basegfx::BColor							maBColor;

		public:
			PolygonHairlinePrimitive2D(const basegfx::B2DPolygon& rPolygon, const basegfx::BColor& rBColor);

			// get data
			const basegfx::B2DPolygon& getB2DPolygon() const { return maPolygon; }
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
// PolygonMarkerPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolygonMarkerPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolygon						maPolygon;
			basegfx::BColor							maRGBColorA;
			basegfx::BColor							maRGBColorB;
			double									mfDiscreteDashLength;

			// decomposition is view-dependent, remember last InverseObjectToViewTransformation
			basegfx::B2DHomMatrix					maLastInverseObjectToViewTransformation;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolygonMarkerPrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
				const basegfx::BColor& rRGBColorA,
				const basegfx::BColor& rRGBColorB,
				double fDiscreteDashLength);

			// get data
			const basegfx::B2DPolygon& getB2DPolygon() const { return maPolygon; }
			const basegfx::BColor& getRGBColorA() const { return maRGBColorA; }
			const basegfx::BColor& getRGBColorB() const { return maRGBColorB; }
			double getDiscreteDashLength() const { return mfDiscreteDashLength; }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// get range
			virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const;

			// get local decomposition. Overloaded since this decomposition is view-dependent
			virtual Primitive2DSequence get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// PolygonStrokePrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolygonStrokePrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DPolygon						maPolygon;
			attribute::LineAttribute				maLineAttribute;
			attribute::StrokeAttribute				maStrokeAttribute;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolygonStrokePrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
                const attribute::LineAttribute& rLineAttribute,
				const attribute::StrokeAttribute& rStrokeAttribute);

			PolygonStrokePrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
                const attribute::LineAttribute& rLineAttribute);

			// get data
			basegfx::B2DPolygon getB2DPolygon() const { return maPolygon; }
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
// PolygonWavePrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolygonWavePrimitive2D : public PolygonStrokePrimitive2D
		{
		private:
			double									mfWaveWidth;
			double									mfWaveHeight;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolygonWavePrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
                const attribute::LineAttribute& rLineAttribute,
				const attribute::StrokeAttribute& rStrokeAttribute,
				double fWaveWidth,
				double fWaveHeight);

			PolygonWavePrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
                const attribute::LineAttribute& rLineAttribute,
				double fWaveWidth,
				double fWaveHeight);

			// get data
			double getWaveWidth() const { return mfWaveWidth; }
			double getWaveHeight() const { return mfWaveHeight; }

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
// PolygonStrokeArrowPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class PolygonStrokeArrowPrimitive2D : public PolygonStrokePrimitive2D
		{
		private:
			attribute::LineStartEndAttribute				maStart;
			attribute::LineStartEndAttribute				maEnd;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			PolygonStrokeArrowPrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
                const attribute::LineAttribute& rLineAttribute,
				const attribute::StrokeAttribute& rStrokeAttribute, 
				const attribute::LineStartEndAttribute& rStart, 
				const attribute::LineStartEndAttribute& rEnd);

			PolygonStrokeArrowPrimitive2D(
				const basegfx::B2DPolygon& rPolygon, 
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

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_POLYGONPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
