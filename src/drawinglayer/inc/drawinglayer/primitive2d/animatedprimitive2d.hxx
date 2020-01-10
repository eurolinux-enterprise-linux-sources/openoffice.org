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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_ANIMATEDPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_ANIMATEDPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>

//////////////////////////////////////////////////////////////////////////////
// predefines
namespace drawinglayer { namespace animation {
	class AnimationEntry;
}}

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class AnimatedSwitchPrimitive2D : public GroupPrimitive2D
		{
		private:
			// the animation definition which allows translation of a point in time
			// to an animation state [0.0 .. 1.0]. This member contains a cloned
			// definition and is owned by this implementation
			animation::AnimationEntry*						mpAnimationEntry;

			// the last remembered decompose time, created and used by getDecomposition() for
			// buffering purposes
			double											mfDecomposeViewTime;

			// bitfield
			// flag if this is a text or graphic animation. Necessary since SdrViews need to differentiate
			// between both types if they are on/off
			unsigned										mbIsTextAnimation : 1;

		protected:
			// create local decomposition
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			AnimatedSwitchPrimitive2D(
				const animation::AnimationEntry& rAnimationEntry, 
				const Primitive2DSequence& rChildren,
				bool bIsTextAnimation);
			virtual ~AnimatedSwitchPrimitive2D();

			// get data
			const animation::AnimationEntry& getAnimationEntry() const { return *mpAnimationEntry; }
			bool isTextAnimation() const { return mbIsTextAnimation; }
			bool isGraphicAnimation() const { return !isTextAnimation(); }

			// compare operator
			virtual bool operator==(const BasePrimitive2D& rPrimitive) const;

			// get range
			virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()

			// The getDecomposition is overloaded here since the decompose is dependent of the point in time,
			// so the default implementation is nut useful here, it needs to be handled locally
			virtual Primitive2DSequence get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const;
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class AnimatedBlinkPrimitive2D : public AnimatedSwitchPrimitive2D
		{
		protected:
			// create local decomposition
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			AnimatedBlinkPrimitive2D(
				const animation::AnimationEntry& rAnimationEntry, 
				const Primitive2DSequence& rChildren,
				bool bIsTextAnimation);

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// helper class for AnimatedInterpolatePrimitive2D

namespace drawinglayer
{
	namespace primitive2d
	{
		class BufferedMatrixDecompose
		{
		private:
			// the matrix itself
			basegfx::B2DHomMatrix						maB2DHomMatrix;

			// the decomposition
			basegfx::B2DVector							maScale;
			basegfx::B2DVector							maTranslate;
			double										mfRotate;
			double										mfShearX;

			// flag if already decomposed, used by ensureDecompose()
			bool										mbDecomposed;

		public:
			BufferedMatrixDecompose(const basegfx::B2DHomMatrix& rMatrix);
			void ensureDecompose() const;

			// data access
			const basegfx::B2DHomMatrix& getB2DHomMatrix() const { return maB2DHomMatrix; }
			const basegfx::B2DVector& getScale() const { return maScale; }
			const basegfx::B2DVector& getTranslate() const { return maTranslate; }
			double getRotate() const { return mfRotate; }
			double getShearX() const { return mfShearX; }
		};
	} // end of anonymous namespace
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		class AnimatedInterpolatePrimitive2D : public AnimatedSwitchPrimitive2D
		{
		private:
			// the transformations
			std::vector< BufferedMatrixDecompose >		maMatrixStack;

		protected:
			// create local decomposition
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			AnimatedInterpolatePrimitive2D(
				const std::vector< basegfx::B2DHomMatrix >& rmMatrixStack,
				const animation::AnimationEntry& rAnimationEntry, 
				const Primitive2DSequence& rChildren,
				bool bIsTextAnimation);

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_ANIMATEDPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
