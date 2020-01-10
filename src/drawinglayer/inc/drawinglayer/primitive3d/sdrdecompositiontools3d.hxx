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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SDRDECOMPOSITIONTOOLS3D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SDRDECOMPOSITIONTOOLS3D_HXX

#include <drawinglayer/primitive3d/baseprimitive3d.hxx>
#include <com/sun/star/drawing/TextureProjectionMode.hpp>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// predefines
namespace basegfx {
	class B3DPolygon;
	class B3DPolyPolygon;
	class B3DHomMatrix;
	class B2DVector;
}

namespace drawinglayer { namespace attribute {
	class SdrLineAttribute;
	class SdrFillAttribute;
	class Sdr3DObjectAttribute;
	class FillGradientAttribute;
	class SdrShadowAttribute;
}}

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		// #i98295#
		basegfx::B3DRange getRangeFrom3DGeometry(::std::vector< basegfx::B3DPolyPolygon >& rFill);
		void applyNormalsKindSphereTo3DGeometry(::std::vector< basegfx::B3DPolyPolygon >& rFill, const basegfx::B3DRange& rRange);
		void applyNormalsKindFlatTo3DGeometry(::std::vector< basegfx::B3DPolyPolygon >& rFill);
		void applyNormalsInvertTo3DGeometry(::std::vector< basegfx::B3DPolyPolygon >& rFill);
		
		// #i98314#
		void applyTextureTo3DGeometry(
			::com::sun::star::drawing::TextureProjectionMode eModeX,
			::com::sun::star::drawing::TextureProjectionMode eModeY,
			::std::vector< basegfx::B3DPolyPolygon >& rFill,
			const basegfx::B3DRange& rRange,
			const basegfx::B2DVector& rTextureSize);

		Primitive3DSequence create3DPolyPolygonLinePrimitives(
			const basegfx::B3DPolyPolygon& rUnitPolyPolygon, 
			const basegfx::B3DHomMatrix& rObjectTransform,
			const attribute::SdrLineAttribute& rLine);

		Primitive3DSequence create3DPolyPolygonFillPrimitives(
			const ::std::vector< basegfx::B3DPolyPolygon >& r3DPolyPolygonVector,
			const basegfx::B3DHomMatrix& rObjectTransform,
			const basegfx::B2DVector& rTextureSize,
			const attribute::Sdr3DObjectAttribute& aSdr3DObjectAttribute,
			const attribute::SdrFillAttribute& rFill,
			const attribute::FillGradientAttribute* pFillGradient = 0L);

		Primitive3DSequence createShadowPrimitive3D(
			const Primitive3DSequence& rSource,
			const attribute::SdrShadowAttribute& rShadow,
			bool bShadow3D);
	} // end of namespace overlay
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //_DRAWINGLAYER_PRIMITIVE3D_SDRDECOMPOSITIONTOOLS3D_HXX

// eof
