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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE2D_PRIMITIVE2DTOOLS_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE2D_PRIMITIVE2DTOOLS_HXX

#include <drawinglayer/primitive2d/baseprimitive2d.hxx>

//////////////////////////////////////////////////////////////////////////////
// tooling class for BasePrimitive2D baseed classes which are view-dependent
// regarding the size of a discrete unit. The implementation of get2DDecomposition
// guards the buffered local decomposition and ensures that a createLocalDecomposition
// implementation may use an up-to-date DiscreteUnit accessible using getDiscreteUnit()

namespace drawinglayer
{
	namespace primitive2d
	{
		class DiscreteMetricDependentPrimitive2D : public BasePrimitive2D
		{
		private:
			// the last used fDiscreteUnit definitions for decomposition. Since this
			// is checked and updated from get2DDecomposition() it will be current and
			// usable in createLocalDecomposition()
			double									mfDiscreteUnit;

		public:
			DiscreteMetricDependentPrimitive2D()
			:	BasePrimitive2D(),
				mfDiscreteUnit(0.0)
			{
			}

			// data access
			double getDiscreteUnit() const { return mfDiscreteUnit; }

			// get local decomposition. Overloaded since this decomposition is view-dependent
			virtual Primitive2DSequence get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const;
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// tooling class for BasePrimitive2D baseed classes which are view-dependent
// regarding the viewport. The implementation of get2DDecomposition
// guards the buffered local decomposition and ensures that a createLocalDecomposition
// implementation may use an up-to-date Viewport accessible using getViewport()

namespace drawinglayer
{
	namespace primitive2d
	{
		class ViewportDependentPrimitive2D : public BasePrimitive2D
		{
		private:
			// the last used Viewport definition for decomposition. Since this
			// is checked and updated from get2DDecomposition() it will be current and
			// usable in createLocalDecomposition()
            basegfx::B2DRange                       maViewport;

		public:
			ViewportDependentPrimitive2D()
			:	BasePrimitive2D(),
				maViewport()
			{
			}

			// data access
			const basegfx::B2DRange& getViewport() const { return maViewport; }

			// get local decomposition. Overloaded since this decomposition is view-dependent
			virtual Primitive2DSequence get2DDecomposition(const geometry::ViewInformation2D& rViewInformation) const;
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE2D_PRIMITIVE2DTOOLS_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
