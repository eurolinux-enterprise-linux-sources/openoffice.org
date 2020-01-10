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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE3D_TEXTUREPRIMITIVE3D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE3D_TEXTUREPRIMITIVE3D_HXX

#include <drawinglayer/primitive3d/groupprimitive3d.hxx>
#include <drawinglayer/attribute/fillattribute.hxx>
#include <drawinglayer/attribute/fillbitmapattribute.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class TexturePrimitive3D : public GroupPrimitive3D
		{
		private:
			basegfx::B2DVector							maTextureSize;

			// bitfield
			// flag if texture shall be modulated with white interpolated color
			unsigned									mbModulate : 1;

			// flag if texture shall be filtered
			unsigned									mbFilter : 1;

		public:
			TexturePrimitive3D(
				const Primitive3DSequence& rChildren, 
				const basegfx::B2DVector& rTextureSize, 
				bool bModulate, 
				bool bFilter);

			// get data
			const basegfx::B2DVector& getTextureSize() const { return maTextureSize; }
			bool getModulate() const { return mbModulate; }
			bool getFilter() const { return mbFilter; }

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class UnifiedAlphaTexturePrimitive3D : public TexturePrimitive3D
		{
		private:
			double										mfTransparence;

		protected:
			// local decomposition.
			virtual Primitive3DSequence createLocalDecomposition(const geometry::ViewInformation3D& rViewInformation) const;

		public:
			UnifiedAlphaTexturePrimitive3D(
				double fTransparence, 
				const Primitive3DSequence& rChildren);

			// get data
			double getTransparence() const { return mfTransparence; }

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive3DIDBlock()
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class GradientTexturePrimitive3D : public TexturePrimitive3D
		{
		private:
			attribute::FillGradientAttribute		maGradient;

		protected:
			// local decomposition.
			virtual Primitive3DSequence createLocalDecomposition(const geometry::ViewInformation3D& rViewInformation) const;

		public:
			GradientTexturePrimitive3D(
				const attribute::FillGradientAttribute& rGradient, 
				const Primitive3DSequence& rChildren, 
				const basegfx::B2DVector& rTextureSize, 
				bool bModulate, 
				bool bFilter);

			// get data
			const attribute::FillGradientAttribute& getGradient() const { return maGradient; }

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive3DIDBlock()
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class BitmapTexturePrimitive3D : public TexturePrimitive3D
		{
		private:
			attribute::FillBitmapAttribute		maBitmap;

		protected:
			// local decomposition.
			virtual Primitive3DSequence createLocalDecomposition(const geometry::ViewInformation3D& rViewInformation) const;

		public:
			BitmapTexturePrimitive3D(
				const attribute::FillBitmapAttribute& rBitmap, 
				const Primitive3DSequence& rChildren, 
				const basegfx::B2DVector& rTextureSize, 
				bool bModulate, 
				bool bFilter);

			// get data
			const attribute::FillBitmapAttribute& getBitmap() const { return maBitmap; }

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive3DIDBlock()
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class AlphaTexturePrimitive3D : public GradientTexturePrimitive3D
		{
		public:
			AlphaTexturePrimitive3D(
				const attribute::FillGradientAttribute& rGradient, 
				const Primitive3DSequence& rChildren, 
				const basegfx::B2DVector& rTextureSize);

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive3DIDBlock()
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE3D_TEXTUREPRIMITIVE3D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
