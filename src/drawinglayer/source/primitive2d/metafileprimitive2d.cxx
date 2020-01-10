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

#include <drawinglayer/primitive2d/metafileprimitive2d.hxx>
#include <basegfx/tools/canvastools.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>

//////////////////////////////////////////////////////////////////////////////

using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		MetafilePrimitive2D::MetafilePrimitive2D(
			const basegfx::B2DHomMatrix& rMetaFileTransform, 
			const GDIMetaFile& rMetaFile)
		:	BasePrimitive2D(),
			maMetaFileTransform(rMetaFileTransform),
			maMetaFile(rMetaFile)
		{
		}

		bool MetafilePrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
		{
			if(BasePrimitive2D::operator==(rPrimitive))
			{
				const MetafilePrimitive2D& rCompare = (MetafilePrimitive2D&)rPrimitive;

				return (getTransform() == rCompare.getTransform() 
					&& getMetaFile() == rCompare.getMetaFile());
			}

			return false;
		}

		basegfx::B2DRange MetafilePrimitive2D::getB2DRange(const geometry::ViewInformation2D& /*rViewInformation*/) const
		{
			basegfx::B2DRange aRetval(0.0, 0.0, 1.0, 1.0);
			aRetval.transform(getTransform());
			return aRetval;
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(MetafilePrimitive2D, PRIMITIVE2D_ID_METAFILEPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
