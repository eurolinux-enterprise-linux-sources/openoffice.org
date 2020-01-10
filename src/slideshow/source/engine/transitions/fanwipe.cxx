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
#include "transitiontools.hxx"
#include "clockwipe.hxx"
#include "fanwipe.hxx"


namespace slideshow {
namespace internal {

::basegfx::B2DPolyPolygon FanWipe::operator () ( double t )
{
    ::basegfx::B2DPolyPolygon res;
    ::basegfx::B2DPolygon poly(
        ClockWipe::calcCenteredClock(
            t / ((m_center && m_single) ? 2.0 : 4.0) ) );
    
    res.append( poly );
    // flip on y-axis:
    ::basegfx::B2DHomMatrix aTransform;
    aTransform.scale( -1.0, 1.0 );
    poly.transform( aTransform );
    poly.flip();
    res.append( poly );
    aTransform.identity();
    
    if (m_center) {
        aTransform.scale( 0.5, 0.5 );
        aTransform.translate( 0.5, 0.5 );
        res.transform( aTransform );
        
        if (! m_single)
            res.append( flipOnXAxis(res) );
    }
    else {
        OSL_ASSERT( ! m_fanIn );
        aTransform.scale( 0.5, 1.0 );
        aTransform.translate( 0.5, 1.0 );
        res.transform( aTransform );
    }
    return res;
}

}
}

