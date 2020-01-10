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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_FILLBITMAPPRIMITIVE2D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_FILLBITMAPPRIMITIVE2D_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <drawinglayer/attribute/fillbitmapattribute.hxx>

//////////////////////////////////////////////////////////////////////////////
// FillbitmapPrimitive2D class

namespace drawinglayer
{
	namespace primitive2d
	{
		class FillBitmapPrimitive2D : public BasePrimitive2D
		{
		private:
			basegfx::B2DHomMatrix						maTransformation;
			attribute::FillBitmapAttribute				maFillBitmap;

		protected:
			// local decomposition.
			virtual Primitive2DSequence createLocalDecomposition(const geometry::ViewInformation2D& rViewInformation) const;

		public:
			FillBitmapPrimitive2D(
				const basegfx::B2DHomMatrix& rTransformation, 
				const attribute::FillBitmapAttribute& rFillBitmap);

			// get data
			const basegfx::B2DHomMatrix& getTransformation() const { return maTransformation; }
			const attribute::FillBitmapAttribute& getFillBitmap() const { return maFillBitmap; }

			// compare operator
			virtual bool operator==( const BasePrimitive2D& rPrimitive ) const;

			// get range
			virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const;

			// provide unique ID
			DeclPrimitrive2DIDBlock()
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_FILLBITMAPPRIMITIVE2D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
