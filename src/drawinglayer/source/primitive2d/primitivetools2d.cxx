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

#include <drawinglayer/primitive2d/primitivetools2d.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		Primitive2DSequence DiscreteMetricDependentPrimitive2D::get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const
		{
			::osl::MutexGuard aGuard( m_aMutex );

			// get the current DiscreteUnit
			const double fDiscreteUnit((rViewInformation.getInverseObjectToViewTransformation() * basegfx::B2DVector(1.0, 0.0)).getLength());

			if(getLocalDecomposition().hasElements() && !basegfx::fTools::equal(fDiscreteUnit, getDiscreteUnit()))
			{
				// conditions of last local decomposition have changed, delete
				const_cast< DiscreteMetricDependentPrimitive2D* >(this)->setLocalDecomposition(Primitive2DSequence());
			}

			if(!getLocalDecomposition().hasElements())
			{
				// remember new valid DiscreteUnit
				const_cast< DiscreteMetricDependentPrimitive2D* >(this)->mfDiscreteUnit = fDiscreteUnit;
			}

			// call base implementation
			return BasePrimitive2D::get2DDecomposition(rViewInformation);
		}
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		Primitive2DSequence ViewportDependentPrimitive2D::get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const
		{
			::osl::MutexGuard aGuard( m_aMutex );

			// get the current Viewport
			const basegfx::B2DRange& rViewport = rViewInformation.getViewport();

			if(getLocalDecomposition().hasElements() && !rViewport.equal(getViewport()))
			{
				// conditions of last local decomposition have changed, delete
				const_cast< ViewportDependentPrimitive2D* >(this)->setLocalDecomposition(Primitive2DSequence());
			}

			if(!getLocalDecomposition().hasElements())
			{
				// remember new valid DiscreteUnit
				const_cast< ViewportDependentPrimitive2D* >(this)->maViewport = rViewport;
			}

			// call base implementation
			return BasePrimitive2D::get2DDecomposition(rViewInformation);
		}
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof
