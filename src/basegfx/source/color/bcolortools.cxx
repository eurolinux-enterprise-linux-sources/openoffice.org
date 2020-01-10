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
#include "precompiled_basegfx.hxx"

#include <basegfx/numeric/ftools.hxx>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/color/bcolortools.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace basegfx { namespace tools
{
    BColor rgb2hsl(const BColor& rRGBColor)
    {
        const double r=rRGBColor.getRed(), g=rRGBColor.getGreen(), b=rRGBColor.getBlue();
        const double minVal = ::std::min( ::std::min( r, g ), b );
        const double maxVal = ::std::max( ::std::max( r, g ), b );
        const double d = maxVal - minVal;

        double h=0, s=0, l=0;

        l = (maxVal + minVal) / 2.0;
    
        if( ::basegfx::fTools::equalZero(d) )
        {
            s = h = 0; // hue undefined (achromatic case)
        }
        else
        {
            s = l > 0.5 ? d/(2.0-maxVal-minVal) :
                d/(maxVal + minVal);

            if( r == maxVal )
                h = (g - b)/d;
            else if( g == maxVal )
                h = 2.0 + (b - r)/d;
            else
                h = 4.0 + (r - h)/d;

            h *= 60.0;

            if( h < 0.0 )
                h += 360.0;
        }

        return BColor(h,s,l);
    }

    static inline double hsl2rgbHelper( double nValue1, double nValue2, double nHue )
    {
        // clamp hue to [0,360]
        nHue = fmod( nHue, 360.0 );

        // cope with wrap-arounds
        if( nHue < 0.0 )
            nHue += 360.0;

        if( nHue < 60.0 )
            return nValue1 + (nValue2 - nValue1)*nHue/60.0;
        else if( nHue < 180.0 )
            return nValue2;
        else if( nHue < 240.0 )
            return nValue1 + (nValue2 - nValue1)*(240.0 - nHue)/60.0;
        else 
            return nValue1;
    }

    BColor hsl2rgb(const BColor& rHSLColor)
    {
        const double h=rHSLColor.getRed(), s=rHSLColor.getGreen(), l=rHSLColor.getBlue();

        if( fTools::equalZero(s) )
            return BColor(l, l, l ); // achromatic case

        const double nVal1( l <= 0.5 ? l*(1.0 + s) : l + s - l*s );
        const double nVal2( 2.0*l - nVal1 );

        return BColor(
            hsl2rgbHelper(nVal2,
                          nVal1, 
                          h + 120.0),
            hsl2rgbHelper(nVal2, 
                          nVal1,
                          h),
            hsl2rgbHelper(nVal2,
                          nVal1,
                          h - 120.0) );
    }
    
    BColor rgb2hsv(const BColor& rRGBColor)
    {
        const double r=rRGBColor.getRed(), g=rRGBColor.getGreen(), b=rRGBColor.getBlue();
        const double maxVal = std::max(std::max(r,g),b);
        const double minVal = std::min(std::min(r,g),b);
        const double delta = maxVal-minVal;

        double h=0, s=0, v=0;

        v = maxVal;
        if( fTools::equalZero(v) )
            s = 0;
        else
            s = delta / v;

        if( !fTools::equalZero(s) )
        {
            if( maxVal == r )
            {
                h = (g - b) / delta;
            }
            else if( maxVal == g )
            {
                h = 2.0 + (b - r) / delta;
            }
            else
            {
                h = 4.0 + (r - g) / delta;
            }

            h *= 60.0;

            if( h < 0 )
                h += 360;
        }

        return BColor(h,s,v);
    }

    BColor hsv2rgb(const BColor& rHSVColor)
    {
        double h=rHSVColor.getRed();
        const double s=rHSVColor.getGreen(), v=rHSVColor.getBlue();

        if( fTools::equalZero(s) )
        {
            // achromatic case: no hue.
            return BColor(v,v,v);
        }
        else
        {
            if( fTools::equal(h,360) )
                h = 0; // 360 degrees is equivalent to 0 degrees

            h /= 60.0;
			const sal_Int32 intval = static_cast< sal_Int32 >( h );
            const double f = h - intval;
            const double p = v*(1.0-s);
            const double q = v*(1.0-(s*f));
            const double t = v*(1.0-(s*(1.0-f)));

            /* which hue area? */
            switch( intval )
            {
                case 0:
                    return BColor(v,t,p);

                case 1:
                    return BColor(q,v,p);

                case 2:
                    return BColor(p,v,t);

                case 3:
                    return BColor(p,q,v);

                case 4:
                    return BColor(t,p,v);

                case 5:
                    return BColor(v,p,q);

                default:
                    // hue overflow
                    return BColor();
            }
        }
    }

    BColor rgb2yiq(const BColor& rRGBColor)
    {
        // from Foley, van Dam, Computer Graphics
        const double r=rRGBColor.getRed(), g=rRGBColor.getGreen(), b=rRGBColor.getBlue();
        return BColor(
            0.299*r + 0.587*g + 0.114*b,
            0.596*r - 0.274*g - 0.322*b,
            0.211*r - 0.522*g + 0.311*b);
    }

    BColor yiq2rgb(const BColor& rYIQColor)
    {
        // from Foley, van Dam, Computer Graphics
        const double y=rYIQColor.getRed(), i=rYIQColor.getGreen(), q=rYIQColor.getBlue();
        return BColor(
            y + 0.956*i + 0.623*q,
            y - 0.272*i - 0.648*q,
            y - 1.105*i + 1.705*q );
    }

    BColor ciexyz2rgb( const BColor& rXYZColor )
    {
        // from Poynton color faq, and SMPTE RP 177-1993, Derivation
        // of Basic Television Color Equations
        const double x=rXYZColor.getRed(), y=rXYZColor.getGreen(), z=rXYZColor.getBlue();
        return BColor(
            3.240479*x - 1.53715*y  - 0.498535*z,
            -0.969256*x + 1.875991*y + 0.041556*z,
            0.055648*x - 0.204043*y + 1.057311*z );
    }

    BColor rgb2ciexyz( const BColor& rRGBColor )
    {
        // from Poynton color faq, and SMPTE RP 177-1993, Derivation
        // of Basic Television Color Equations
        const double r=rRGBColor.getRed(), g=rRGBColor.getGreen(), b=rRGBColor.getBlue();
        return BColor(
            0.412453*r + 0.35758*g  + 0.180423*b,
            0.212671*r + 0.71516*g  + 0.072169*b,
            0.019334*r + 0.119193*g + 0.950227*b);
    }

    BColor rgb2ypbpr(const BColor& rRGBColor)
    {
        const double r=rRGBColor.getRed(), g=rRGBColor.getGreen(), b=rRGBColor.getBlue();
        return BColor(
             0.299*r    + 0.587*g    + 0.114*b,
            -0.168736*r - 0.331264*g + 0.5*b,
             0.5*r      - 0.418688*g - 0.081312*b);
    }

    BColor ypbpr2rgb(const BColor& rYPbPrColor)
    {
        const double y=rYPbPrColor.getRed(), pb=rYPbPrColor.getGreen(), pr=rYPbPrColor.getBlue();
        return BColor(
            1.*y +       0.*pb +    1.402*pr,
            1.*y - 0.344136*pb - 0.714136*pr,
            1.*y +    1.772*pb +       0.*pr);
    }

} } // end of namespace basegfx

//////////////////////////////////////////////////////////////////////////////
// eof
