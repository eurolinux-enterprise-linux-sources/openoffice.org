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
#include "precompiled_slideshow.hxx"

#include "transitiontools.hxx"
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>


namespace slideshow {
namespace internal {

// TODO(Q2): Move this to basegfx
::basegfx::B2DPolygon createUnitRect()
{
    return ::basegfx::tools::createPolygonFromRect(
        ::basegfx::B2DRectangle(0.0,0.0,
                                1.0,1.0 ) );
}    

::basegfx::B2DPolyPolygon flipOnYAxis(
    ::basegfx::B2DPolyPolygon const & polypoly )
{
    ::basegfx::B2DPolyPolygon res(polypoly);
    ::basegfx::B2DHomMatrix aTransform;
    aTransform.scale( -1.0, 1.0 );
    aTransform.translate( 1.0, 0.0 );
    res.transform( aTransform );
    res.flip();
    return res;
}

::basegfx::B2DPolyPolygon flipOnXAxis(
    ::basegfx::B2DPolyPolygon const & polypoly )
{
    ::basegfx::B2DPolyPolygon res(polypoly);
    ::basegfx::B2DHomMatrix aTransform;
    aTransform.scale( 1.0, -1.0 );
    aTransform.translate( 0.0, 1.0 );
    res.transform( aTransform );
    res.flip();
    return res;
}

}
}
