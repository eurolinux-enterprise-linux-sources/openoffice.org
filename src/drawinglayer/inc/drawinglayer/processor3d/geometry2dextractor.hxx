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

#ifndef INCLUDED_DRAWINGLAYER_PROCESSOR3D_GEOMETRY2DEXTRACTOR_HXX
#define INCLUDED_DRAWINGLAYER_PROCESSOR3D_GEOMETRY2DEXTRACTOR_HXX

#include <drawinglayer/processor3d/baseprocessor3d.hxx>
#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/matrix/b3dhommatrix.hxx>
#include <basegfx/color/bcolormodifier.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace processor3d
	{
		class Geometry2DExtractingProcessor : public BaseProcessor3D
		{
		private:
			// result holding vector (2D)
			primitive2d::Primitive2DSequence				maPrimitive2DSequence;

			// object transformation for scene for 2d definition
			basegfx::B2DHomMatrix							maObjectTransformation;

			// the modifiedColorPrimitive stack
			basegfx::BColorModifierStack					maBColorModifierStack;

			// as tooling, the process() implementation takes over API handling and calls this
			// virtual render method when the primitive implementation is BasePrimitive3D-based.
			virtual void processBasePrimitive3D(const primitive3d::BasePrimitive3D& rCandidate);

		public:
			Geometry2DExtractingProcessor(
				const geometry::ViewInformation3D& rViewInformation,
				const basegfx::B2DHomMatrix& rObjectTransformation);

			// data access
			const primitive2d::Primitive2DSequence& getPrimitive2DSequence() const { return maPrimitive2DSequence; }
			const basegfx::B2DHomMatrix& getObjectTransformation() const { return maObjectTransformation; }
			const basegfx::BColorModifierStack& getBColorModifierStack() const { return maBColorModifierStack; }
		};
	} // end of namespace processor3d
} // end of namespace drawinglayer

#endif //INCLUDED_DRAWINGLAYER_PROCESSOR3D_GEOMETRY2DEXTRACTOR_HXX

// eof
