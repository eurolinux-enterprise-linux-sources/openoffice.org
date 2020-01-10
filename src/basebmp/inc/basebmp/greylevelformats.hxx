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

#ifndef INCLUDED_BASEBMP_GREYLEVELFORMATS_HXX
#define INCLUDED_BASEBMP_GREYLEVELFORMATS_HXX

#include <basebmp/color.hxx>
#include <basebmp/colortraits.hxx>
#include <basebmp/accessor.hxx>
#include <basebmp/pixeliterator.hxx>
#include <basebmp/packedpixeliterator.hxx>
#include <basebmp/pixelformatadapters.hxx>
#include <basebmp/metafunctions.hxx>

#include <vigra/numerictraits.hxx>
#include <vigra/metaprogramming.hxx>

#include <functional>

namespace basebmp
{

template< typename PixelType, 
          typename ColorType,
          int      UsedRange > struct GreylevelGetter : 
        public std::unary_function<PixelType, ColorType> 
{
    ColorType operator()( PixelType const& c ) const
    { 
        return ColorTraits<ColorType>::fromGreyscale(
            vigra::NumericTraits<PixelType>::toPromote(c) *
            vigra::NumericTraits<PixelType>::maxConst / UsedRange );
    }
};

template< typename PixelType, 
          typename ColorType,
          int      UsedRange > struct GreylevelSetter : 
    public std::unary_function<ColorType, PixelType> 
{
    PixelType operator()( ColorType const& c ) const
    {
        return vigra::NumericTraits<PixelType>::toPromote(
            ColorTraits<ColorType>::toGreyscale(c)) *  
            UsedRange / 
            vigra::NumericTraits<PixelType>::maxConst;
    }
};

//-----------------------------------------------------------------------------

template< class Iterator,
          class Accessor,
          int   UsedRange > struct PixelFormatTraitsTemplate_Greylevel
{
    typedef typename Iterator::value_type       pixel_type;

    typedef GreylevelGetter<pixel_type,
                            Color,
                            UsedRange>          getter_type;
    typedef GreylevelSetter<pixel_type,
                            Color,
                            UsedRange>          setter_type;

    typedef Iterator                            iterator_type;
    typedef Accessor                            raw_accessor_type;
    typedef AccessorSelector<
        getter_type,
        setter_type >                           accessor_selector;
};

template< int BitsPerPixel,
          bool MsbFirst > struct PixelFormatTraitsTemplate_PackedGreylevel :
    public PixelFormatTraitsTemplate_Greylevel<    
               PackedPixelIterator< sal_uInt8,
                                    BitsPerPixel,
                                    true >,
               NonStandardAccessor< sal_uInt8 >,
               (1UL << BitsPerPixel)-1 >
{};

//-----------------------------------------------------------------------------

// 1bpp MSB
typedef PixelFormatTraitsTemplate_PackedGreylevel<1, true> PixelFormatTraits_GREY1_MSB;

// 1bpp LSB
typedef PixelFormatTraitsTemplate_PackedGreylevel<1, false> PixelFormatTraits_GREY1_LSB;
BASEBMP_SPECIALIZE_ACCESSORTRAITS(PixelFormatTraits_GREY1_MSB::getter_type,
                                  PixelFormatTraits_GREY1_MSB::setter_type);


// 4bpp MSB
typedef PixelFormatTraitsTemplate_PackedGreylevel<4, true> PixelFormatTraits_GREY4_MSB;

// 4bpp LSB
typedef PixelFormatTraitsTemplate_PackedGreylevel<4, false> PixelFormatTraits_GREY4_LSB;
BASEBMP_SPECIALIZE_ACCESSORTRAITS(PixelFormatTraits_GREY4_MSB::getter_type,
                                  PixelFormatTraits_GREY4_MSB::setter_type);

// 8bpp
typedef PixelFormatTraitsTemplate_Greylevel<    
    PixelIterator< sal_uInt8 >,
    StandardAccessor< sal_uInt8 >,
    255 >                                                   PixelFormatTraits_GREY8;
BASEBMP_SPECIALIZE_ACCESSORTRAITS(PixelFormatTraits_GREY8::getter_type,
                                  PixelFormatTraits_GREY8::setter_type);

} // namespace basebmp

#endif /* INCLUDED_BASEBMP_GREYLEVELFORMATS_HXX */
