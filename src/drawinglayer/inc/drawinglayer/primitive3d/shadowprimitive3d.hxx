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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SHADOWPRIMITIVE3D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SHADOWPRIMITIVE3D_HXX

#include <drawinglayer/primitive3d/groupprimitive3d.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/color/bcolor.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive3d
	{
		class ShadowPrimitive3D : public GroupPrimitive3D
		{
		protected:
			basegfx::B2DHomMatrix					maShadowTransform;
			basegfx::BColor							maShadowColor;
			double									mfShadowTransparence;

			// bitfield
			unsigned								mbShadow3D : 1;

		public:
			ShadowPrimitive3D(
				const basegfx::B2DHomMatrix& rShadowTransform, 
				const basegfx::BColor& rShadowColor, 
				double fShadowTransparence, 
				bool bShadow3D, 
				const Primitive3DSequence& rChildren);

			// get data
			const basegfx::B2DHomMatrix& getShadowTransform() const { return maShadowTransform; }
			const basegfx::BColor& getShadowColor() const { return maShadowColor; }
			double getShadowTransparence() const { return mfShadowTransparence; }
			bool getShadow3D() const { return mbShadow3D; }

			// compare operator
			virtual bool operator==(const BasePrimitive3D& rPrimitive) const;

			// provide unique ID
			DeclPrimitrive3DIDBlock()
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE3D_SHADOWPRIMITIVE3D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
