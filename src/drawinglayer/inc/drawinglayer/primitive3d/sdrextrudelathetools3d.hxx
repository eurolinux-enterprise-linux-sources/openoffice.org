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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SDREXTRUDELATHETOOLS3D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SDREXTRUDELATHETOOLS3D_HXX

#include <basegfx/polygon/b3dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// predefines

namespace drawinglayer { namespace geometry	{
	class ViewInformation3D;
}}

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		// Slice3D types
		enum SliceType3D
		{
			SLICETYPE3D_REGULAR,		// normal geoemtry Slice3D
			SLICETYPE3D_FRONTCAP,		// front cap
			SLICETYPE3D_BACKCAP			// back cap
		};

		// class to hold one Slice3D
		class Slice3D
		{
		protected:
			basegfx::B3DPolyPolygon					maPolyPolygon;
			SliceType3D								maSliceType;

		public:
			Slice3D(
				const basegfx::B2DPolyPolygon& rPolyPolygon, 
				const basegfx::B3DHomMatrix& aTransform, 
				SliceType3D aSliceType = SLICETYPE3D_REGULAR)
			:	maPolyPolygon(basegfx::tools::createB3DPolyPolygonFromB2DPolyPolygon(rPolyPolygon)),
				maSliceType(aSliceType)
			{
				maPolyPolygon.transform(aTransform);
			}

			Slice3D(
				const basegfx::B3DPolyPolygon& rPolyPolygon, 
				SliceType3D aSliceType = SLICETYPE3D_REGULAR)
			:	maPolyPolygon(rPolyPolygon),
				maSliceType(aSliceType)
			{
			}

			// data access
			const basegfx::B3DPolyPolygon& getB3DPolyPolygon() const { return maPolyPolygon; }
			SliceType3D getSliceType() const { return maSliceType; }
		};

		// typedef for a group of Slice3Ds
		typedef ::std::vector< Slice3D > Slice3DVector;

		// helpers for creation
		void createLatheSlices(
			Slice3DVector& rSliceVector, 
			const basegfx::B2DPolyPolygon& rSource, 
			double fBackScale, 
			double fDiagonal, 
			double fRotation, 
			sal_uInt32 nSteps, 
			bool bCharacterMode, 
			bool bCloseFront, 
			bool bCloseBack);
		
		void createExtrudeSlices(
			Slice3DVector& rSliceVector, 
			const basegfx::B2DPolyPolygon& rSource, 
			double fBackScale, 
			double fDiagonal, 
			double fDepth, 
			bool bCharacterMode, 
			bool bCloseFront, 
			bool bCloseBack);

		// helpers for geometry extraction
		basegfx::B3DPolyPolygon extractHorizontalLinesFromSlice(const Slice3DVector& rSliceVector, bool bCloseHorLines);
		basegfx::B3DPolyPolygon extractVerticalLinesFromSlice(const Slice3DVector& rSliceVector);

		void extractPlanesFromSlice(
			::std::vector< basegfx::B3DPolyPolygon >& rFill, 
			const Slice3DVector& rSliceVector,
			bool bCreateNormals, 
			bool bSmoothHorizontalNormals, 
			bool bSmoothNormals, 
			bool bSmoothLids, 
			bool bClosed,
			double fSmoothNormalsMix, 
			double fSmoothLidsMix, 
			bool bCreateTextureCoordinates, 
			const basegfx::B2DHomMatrix& rTexTransform);

		void createReducedOutlines(
			const geometry::ViewInformation3D& rViewInformation, 
			const basegfx::B3DHomMatrix& rObjectTransform,
			const basegfx::B3DPolygon& rLoopA, 
			const basegfx::B3DPolygon& rLoopB, 
			basegfx::B3DPolyPolygon& rTarget);

	} // end of namespace overlay
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //_DRAWINGLAYER_PRIMITIVE3D_SDREXTRUDELATHETOOLS3D_HXX

// eof
