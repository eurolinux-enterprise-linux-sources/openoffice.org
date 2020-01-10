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

#ifndef INCLUDED_DRAWINGLAYER_TEXTLAYOUTDEVICE_HXX
#define INCLUDED_DRAWINGLAYER_TEXTLAYOUTDEVICE_HXX

#include <sal/types.h>
#include <tools/solar.h>
#include <tools/poly.hxx>
#include <basegfx/range/b2drange.hxx>
#include <vector>
#include <com/sun/star/lang/Locale.hpp>

//////////////////////////////////////////////////////////////////////////////
// predefines
class VirtualDevice;
class Font;
class String;
class OutputDevice;

namespace drawinglayer { namespace primitive2d {
	class FontAttributes;
}}

namespace basegfx {
    class B2DPolyPolygon;
    typedef ::std::vector< B2DPolyPolygon > B2DPolyPolygonVector;
}

//////////////////////////////////////////////////////////////////////////////
// access to one global impTimedRefDev incarnation in namespace drawinglayer::primitive

namespace drawinglayer
{
	namespace primitive2d
	{
		class TextLayouterDevice
		{
			// internally used VirtualDevice
			VirtualDevice&					mrDevice;

		public:
			TextLayouterDevice();
			~TextLayouterDevice();

			void setFont(const Font& rFont);
			void setFontAttributes(
                const FontAttributes& rFontAttributes, 
                double fFontScaleX, 
                double fFontScaleY, 
                const ::com::sun::star::lang::Locale & rLocale);

			double getTextHeight() const;
            double getOverlineHeight() const;
            double getOverlineOffset() const;
            double getUnderlineHeight() const;
			double getUnderlineOffset() const;
			double getStrikeoutOffset() const;

            double getTextWidth(
				const String& rText,
				xub_StrLen nIndex,
				xub_StrLen nLength) const;

			bool getTextOutlines(
				basegfx::B2DPolyPolygonVector&,
			    const String& rText,
				xub_StrLen nIndex,
				xub_StrLen nLength,
                const ::std::vector< double >& rDXArray);

			basegfx::B2DRange getTextBoundRect(
				const String& rText,
				xub_StrLen nIndex,
				xub_StrLen nLength) const;
		};
	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// helper methods for vcl font handling

namespace drawinglayer
{
	namespace primitive2d
	{
        // Create a VCL-Font based on the definitions in FontAttributes
        // and the given FontScaling. The FontScaling defines the FontHeight
        // (fFontScaleY) and the FontWidth (fFontScaleX). The combination of 
        // both defines FontStretching, where no stretching happens at
        // fFontScaleY == fFontScaleX
		Font getVclFontFromFontAttributes(
            const FontAttributes& rFontAttributes,
            double fFontScaleX,
            double fFontScaleY,
            double fFontRotation,
            const ::com::sun::star::lang::Locale & rLocale);

        // Generate FontAttributes DataSet derived from the given VCL-Font.
        // The FontScaling with fFontScaleY, fFontScaleX relationship (see
        // above) will be set in return parameter o_rSize to allow further 
        // processing
        FontAttributes getFontAttributesFromVclFont(
            basegfx::B2DVector& o_rSize, 
            const Font& rFont, 
            bool bRTL, 
            bool bBiDiStrong);

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //_DRAWINGLAYER_TEXTLAYOUTDEVICE_HXX
