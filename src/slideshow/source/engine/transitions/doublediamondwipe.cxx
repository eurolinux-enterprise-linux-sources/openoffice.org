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

#include <canvas/debug.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include "doublediamondwipe.hxx"


namespace slideshow {
namespace internal {

::basegfx::B2DPolyPolygon DoubleDiamondWipe::operator () ( double t )
{
    // outer:
    const double a = ::basegfx::pruneScaleValue( 0.25 + (t * 0.75) );
    ::basegfx::B2DPolygon poly;
    poly.append( ::basegfx::B2DPoint( 0.5 + a, 0.5 ) );
    poly.append( ::basegfx::B2DPoint( 0.5, 0.5 - a ) );
    poly.append( ::basegfx::B2DPoint( 0.5 - a, 0.5 ) );
    poly.append( ::basegfx::B2DPoint( 0.5, 0.5 + a ) );
    poly.setClosed(true);
    ::basegfx::B2DPolyPolygon res(poly);

    // inner (reverse order to clip):
    const double b = ::basegfx::pruneScaleValue( (1.0 - t) * 0.25 );
    poly.clear();
    poly.append( ::basegfx::B2DPoint( 0.5 + b, 0.5 ) );
    poly.append( ::basegfx::B2DPoint( 0.5, 0.5 + b ) );
    poly.append( ::basegfx::B2DPoint( 0.5 - b, 0.5 ) );
    poly.append( ::basegfx::B2DPoint( 0.5, 0.5 - b ) );
    poly.setClosed(true);
    res.append(poly);
    
    return res;
}

}
}
