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
#include <com/sun/star/beans/XProperty.hpp>
#include <com/sun/star/awt/FontWeight.hpp>
#include <com/sun/star/awt/FontUnderline.hpp>
#include <com/sun/star/awt/FontStrikeout.hpp>
#include <com/sun/star/awt/FontSlant.hpp>
#include <com/sun/star/text/XSimpleText.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XColumnRowRange.hpp>
#include <ooo/vba/excel/XlColorIndex.hpp>
#include <ooo/vba/excel/XlUnderlineStyle.hpp>
#include <svtools/itemset.hxx>
#include "vbafont.hxx"
#include "scitems.hxx"
#include "cellsuno.hxx"

using namespace ::ooo::vba;
using namespace ::com::sun::star;

// use local constants there is no need to expose these constants
// externally. Looking at the Format->Character dialog it seem that
// these may infact be even be calculated. Leave hardcoded for now
// #FIXEME #TBD investigate the code for dialog mentioned above

// The font baseline is not specified.
const short NORMAL = 0;

// specifies a superscripted.
const short SUPERSCRIPT = 33;

// specifies a subscripted.
const short SUBSCRIPT = -33;

// specifies a hight of superscripted font
 const sal_Int8 SUPERSCRIPTHEIGHT = 58;

// specifies a hight of subscripted font
const sal_Int8 SUBSCRIPTHEIGHT = 58;

// specifies a hight of normal font
const short NORMALHEIGHT = 100;

ScVbaFont::ScVbaFont( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext, const ScVbaPalette& dPalette, uno::Reference< beans::XPropertySet > xPropertySet, ScCellRangeObj* pRangeObj  ) throw ( uno::RuntimeException ) : ScVbaFont_BASE( xParent, xContext ), mxFont( xPropertySet, css::uno::UNO_QUERY_THROW ), mPalette( dPalette ),  mpRangeObj( pRangeObj )
{
}

SfxItemSet*  
ScVbaFont::GetDataSet()
{
    SfxItemSet* pDataSet = ScVbaCellRangeAccess::GetDataSet( mpRangeObj );
    return pDataSet;
}

ScVbaFont::~ScVbaFont()
{
}


uno::Reference< beans::XPropertySet > lcl_TextProperties( uno::Reference< table::XCell >& xIf ) throw ( uno::RuntimeException )
{
	uno::Reference< text::XTextRange > xTxtRange( xIf, uno::UNO_QUERY_THROW );
	uno::Reference< text::XSimpleText > xTxt( xTxtRange->getText(), uno::UNO_QUERY_THROW ) ;
	uno::Reference< beans::XPropertySet > xProps( xTxt->createTextCursor(), uno::UNO_QUERY_THROW );
	return xProps;
}
void SAL_CALL
ScVbaFont::setSuperscript( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	// #FIXEME create some sort of generic get/set code where 
	// you can pass a functor
	// get/set - Super/sub script code is exactly the same
	// except for the call applied at each cell position
        uno::Reference< table::XCell> xCell( mxFont, uno::UNO_QUERY );
        uno::Reference< table::XCellRange > xCellRange( mxFont, uno::UNO_QUERY );
	if ( !xCell.is() )
	{
		uno::Reference< table::XColumnRowRange > xColumnRowRange(xCellRange, uno::UNO_QUERY_THROW );
		sal_Int32 nCols = xColumnRowRange->getColumns()->getCount();
		sal_Int32 nRows = xColumnRowRange->getRows()->getCount();
		for ( sal_Int32 col = 0; col < nCols; ++col )
		{
			for ( sal_Int32 row = 0; row < nRows; ++row )
			{
				uno::Reference< beans::XPropertySet > xProps( xCellRange->getCellByPosition( col, row ) , uno::UNO_QUERY_THROW );
				ScVbaFont aFont( getParent(), mxContext, mPalette, xProps );
				aFont.setSuperscript( aValue );
			}
		}
		return;

	}
        xCell.set( xCellRange->getCellByPosition( 0,0 ) );

	uno::Reference< beans::XPropertySet > xProps = lcl_TextProperties( xCell );
	sal_Bool bValue = sal_False;
	aValue >>= bValue;
	sal_Int16 nValue = NORMAL;
	sal_Int8 nValue2 = NORMALHEIGHT;

        if( bValue )
	{
		nValue = SUPERSCRIPT;
	        nValue2 = SUPERSCRIPTHEIGHT;
	}
	xProps->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapement" ) ), ( uno::Any )nValue );
 	xProps->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapementHeight" ) ), ( uno::Any )nValue2 );
}

uno::Any SAL_CALL
ScVbaFont::getSuperscript() throw ( uno::RuntimeException )
{
        uno::Reference< table::XCell> xCell( mxFont, uno::UNO_QUERY );
        uno::Reference< table::XCellRange > xCellRange( mxFont, uno::UNO_QUERY );
	if ( !xCell.is() )
	{
		uno::Reference< table::XColumnRowRange > xColumnRowRange(xCellRange, uno::UNO_QUERY_THROW );
		sal_Int32 nCols = xColumnRowRange->getColumns()->getCount();
		sal_Int32 nRows = xColumnRowRange->getRows()->getCount();
		uno::Any aRes;
		for ( sal_Int32 col = 0; col < nCols; ++col )
		{
			for ( sal_Int32 row = 0; row < nRows; ++row )
			{
				uno::Reference< beans::XPropertySet > xProps( xCellRange->getCellByPosition( col, row ), uno::UNO_QUERY_THROW );
				ScVbaFont aFont( getParent(), mxContext, mPalette, xProps );
				if ( !col && !row )
					aRes = aFont.getSuperscript();
				else if ( aRes != aFont.getSuperscript() )
					return aNULL();
			}
		}
		return aRes;

	}
        xCell.set( xCellRange->getCellByPosition( 0,0 ) );
	uno::Reference< beans::XPropertySet > xProps = lcl_TextProperties( xCell );
	short nValue = 0;
	xProps->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapement" ) ) ) >>= nValue;
	return uno::makeAny( ( nValue == SUPERSCRIPT ) );
}

void SAL_CALL
ScVbaFont::setSubscript( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
        uno::Reference< table::XCell> xCell( mxFont, uno::UNO_QUERY );
        uno::Reference< table::XCellRange > xCellRange( mxFont, uno::UNO_QUERY );
	if ( !xCell.is() )
	{
		uno::Reference< table::XColumnRowRange > xColumnRowRange(xCellRange, uno::UNO_QUERY_THROW );
		sal_Int32 nCols = xColumnRowRange->getColumns()->getCount();
		sal_Int32 nRows = xColumnRowRange->getRows()->getCount();
		for ( sal_Int32 col = 0; col < nCols; ++col )
		{
			for ( sal_Int32 row = 0; row < nRows; ++row )
			{
				uno::Reference< beans::XPropertySet > xProps( xCellRange->getCellByPosition( col, row ) , uno::UNO_QUERY_THROW );
				ScVbaFont aFont( getParent(), mxContext, mPalette, xProps );
				aFont.setSubscript( aValue );
			}
		}
		return;

	}
        xCell.set( xCellRange->getCellByPosition( 0,0 ) );
	uno::Reference< beans::XPropertySet > xProps = lcl_TextProperties( xCell );

	sal_Bool bValue = sal_False;
	aValue >>= bValue;
	sal_Int16 nValue = NORMAL;
	sal_Int8 nValue2 = NORMALHEIGHT;

        if( bValue )
	{
		nValue= SUBSCRIPT;
	        nValue2 = SUBSCRIPTHEIGHT;
	}

 	xProps->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapementHeight" ) ), ( uno::Any )nValue2 );
	xProps->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapement" ) ), ( uno::Any )nValue );

}

uno::Any SAL_CALL
ScVbaFont::getSubscript() throw ( uno::RuntimeException )
{
        uno::Reference< table::XCell> xCell( mxFont, uno::UNO_QUERY );
        uno::Reference< table::XCellRange > xCellRange( mxFont, uno::UNO_QUERY );
	if ( !xCell.is() )
	{
		uno::Reference< table::XColumnRowRange > xColumnRowRange(xCellRange, uno::UNO_QUERY_THROW );
		sal_Int32 nCols = xColumnRowRange->getColumns()->getCount();
		sal_Int32 nRows = xColumnRowRange->getRows()->getCount();
		uno::Any aRes;
		for ( sal_Int32 col = 0; col < nCols; ++col )
		{
			for ( sal_Int32 row = 0; row < nRows; ++row )
			{
				uno::Reference< beans::XPropertySet > xProps( xCellRange->getCellByPosition( col, row ), uno::UNO_QUERY_THROW );
				ScVbaFont aFont( getParent(), mxContext, mPalette, xProps );
				if ( !col && !row )
					aRes = aFont.getSubscript();
				else if ( aRes != aFont.getSubscript() )
					return aNULL();
			}
		}
		return aRes;

	}
        xCell.set( xCellRange->getCellByPosition( 0,0 ) );
	uno::Reference< beans::XPropertySet > xProps = lcl_TextProperties( xCell );

	short nValue = NORMAL;
	xProps->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharEscapement" ) ) ) >>= nValue;
	return uno::makeAny( ( nValue == SUBSCRIPT ) );
}

void SAL_CALL
ScVbaFont::setSize( const uno::Any& aValue ) throw( uno::RuntimeException )
{
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharHeight" ) ), aValue );
}

uno::Any SAL_CALL
ScVbaFont::getSize() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_HEIGHT, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();
        return mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharHeight" ) ) );
}

void SAL_CALL
ScVbaFont::setColorIndex( const uno::Any& _colorindex ) throw( uno::RuntimeException )
{
	sal_Int32 nIndex = 0;
	_colorindex >>= nIndex;
	// #FIXME  xlColorIndexAutomatic & xlColorIndexNone are not really
	// handled properly here

	if ( !nIndex || ( nIndex == excel::XlColorIndex::xlColorIndexAutomatic ) )
		nIndex = 1;  // check defualt ( assume black )
	--nIndex; // OOo indices are zero bases
	uno::Reference< container::XIndexAccess > xIndex = mPalette.getPalette();
	// setColor expects colors in XL RGB values
	// #FIXME this is daft we convert OO RGB val to XL RGB val and
	// then back again to OO RGB value
	setColor( OORGBToXLRGB(xIndex->getByIndex( nIndex )) );
}


uno::Any SAL_CALL
ScVbaFont::getColorIndex() throw ( uno::RuntimeException )
{
	sal_Int32 nColor = 0;
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_COLOR, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();

	// getColor returns Xl ColorValue, need to convert it to OO val
	// as the palette deals with OO RGB values
	// #FIXME this is daft in getColor we convert OO RGB val to XL RGB val
	// and then back again to OO RGB value
	XLRGBToOORGB( getColor() ) >>= nColor;
	uno::Reference< container::XIndexAccess > xIndex = mPalette.getPalette();
	sal_Int32 nElems = xIndex->getCount();
	sal_Int32 nIndex = -1;
	for ( sal_Int32 count=0; count<nElems; ++count )
       	{
		sal_Int32 nPaletteColor = 0;
		xIndex->getByIndex( count ) >>= nPaletteColor;
		if ( nPaletteColor == nColor )
		{
			nIndex = count + 1; // 1 based
			break;
		}
	}
	return uno::makeAny( nIndex );
}

//////////////////////////////////////////////////////////////////////////////////////////
void  SAL_CALL
ScVbaFont::setStandardFontSize( const uno::Any& /*aValue*/ ) throw( uno::RuntimeException )
{
//XXX #TODO# #FIXME#
	//mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharSize" ) ), ( uno::Any )fValue );
	throw uno::RuntimeException(
		rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("setStandardFontSize not supported") ), uno::Reference< uno::XInterface >() );
}


uno::Any SAL_CALL
ScVbaFont::getStandardFontSize() throw ( uno::RuntimeException )
{
//XXX #TODO# #FIXME#
	throw uno::RuntimeException(
		rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("getStandardFontSize not supported") ), uno::Reference< uno::XInterface >() );
	// return uno::Any();
}


void  SAL_CALL
ScVbaFont::setStandardFont( const uno::Any& /*aValue*/ ) throw( uno::RuntimeException )
{
//XXX #TODO# #FIXME#
	throw uno::RuntimeException(
		rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("setStandardFont not supported") ), uno::Reference< uno::XInterface >() );
}


uno::Any SAL_CALL
ScVbaFont::getStandardFont() throw ( uno::RuntimeException )
{
//XXX #TODO# #FIXME#
	throw uno::RuntimeException(
		rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("getStandardFont not supported") ), uno::Reference< uno::XInterface >() );
	// return uno::Any();
}

void SAL_CALL
ScVbaFont::setFontStyle( const uno::Any& aValue ) throw( uno::RuntimeException )
{
    sal_Bool bBold = sal_False;
    sal_Bool bItalic = sal_False;

    rtl::OUString aStyles;
    aValue >>= aStyles;

    std::vector< rtl::OUString > aTokens;
    sal_Int32 nIndex = 0;
    do
    {
        rtl::OUString aToken = aStyles.getToken( 0, ' ', nIndex );
        aTokens.push_back( aToken );
    }while( nIndex >= 0 );
    
    std::vector< rtl::OUString >::iterator it;
    for( it = aTokens.begin(); it != aTokens.end(); ++it )
    {
        if( (*it).equalsIgnoreAsciiCaseAscii( "Bold" ) )
            bBold = sal_True;

        if( (*it).equalsIgnoreAsciiCaseAscii( "Italic" ) )
            bItalic = sal_True;
    }

    setBold( uno::makeAny( bBold ) );
    setItalic( uno::makeAny( bItalic ) );
}


uno::Any SAL_CALL
ScVbaFont::getFontStyle() throw ( uno::RuntimeException )
{
    rtl::OUStringBuffer aStyles;
    sal_Bool bValue = sal_False;
    getBold() >>= bValue;
    if( bValue )
        aStyles.appendAscii("Bold");
    
    getItalic() >>= bValue;
    if( bValue )
    {
        if( aStyles.getLength() )
            aStyles.appendAscii(" ");
        aStyles.appendAscii("Italic");
    }
    return uno::makeAny( aStyles.makeStringAndClear() );
}

void SAL_CALL
ScVbaFont::setBold( const uno::Any& aValue ) throw( uno::RuntimeException )
{
	sal_Bool bValue = sal_False;
	aValue >>= bValue;
	double fBoldValue = awt::FontWeight::NORMAL;
	if( bValue )
		fBoldValue = awt::FontWeight::BOLD;
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharWeight" ) ), ( uno::Any )fBoldValue );

}

uno::Any SAL_CALL
ScVbaFont::getBold() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_WEIGHT, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();

	double fValue = 0.0;
	mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharWeight" ) ) ) >>= fValue;
	return uno::makeAny( fValue == awt::FontWeight::BOLD );
}

void SAL_CALL
ScVbaFont::setUnderline( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	// default
	sal_Int32 nValue = excel::XlUnderlineStyle::xlUnderlineStyleNone;
	aValue >>= nValue;
	switch ( nValue )
	{
// NOTE:: #TODO #FIMXE
// xlUnderlineStyleDoubleAccounting & xlUnderlineStyleSingleAccounting
// don't seem to be supported in Openoffice.
// The import filter converts them to single or double underlines as appropriate
// So, here at the moment we are similarly silently converting
// xlUnderlineStyleSingleAccounting to xlUnderlineStyleSingle.

		case excel::XlUnderlineStyle::xlUnderlineStyleNone:
			nValue = awt::FontUnderline::NONE;
			break;
		case excel::XlUnderlineStyle::xlUnderlineStyleSingle:
		case excel::XlUnderlineStyle::xlUnderlineStyleSingleAccounting:
			nValue = awt::FontUnderline::SINGLE;
			break;
		case excel::XlUnderlineStyle::xlUnderlineStyleDouble:
		case excel::XlUnderlineStyle::xlUnderlineStyleDoubleAccounting:
			nValue = awt::FontUnderline::DOUBLE;
			break;
		default:
			throw uno::RuntimeException( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("Unknown value for Underline")), uno::Reference< uno::XInterface >() );
	}

	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharUnderline" ) ), ( uno::Any )nValue );

}

uno::Any SAL_CALL
ScVbaFont::getUnderline() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_UNDERLINE, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();

	sal_Int32 nValue = awt::FontUnderline::NONE;
	mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharUnderline" ) ) ) >>= nValue;
	switch ( nValue )
	{
		case  awt::FontUnderline::DOUBLE:
			nValue = excel::XlUnderlineStyle::xlUnderlineStyleDouble;
			break;
		case  awt::FontUnderline::SINGLE:
			nValue = excel::XlUnderlineStyle::xlUnderlineStyleSingle;
			break;
		case  awt::FontUnderline::NONE:
			nValue = excel::XlUnderlineStyle::xlUnderlineStyleNone;
			break;
		default:
			throw uno::RuntimeException( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("Unknown value retrieved for Underline") ), uno::Reference< uno::XInterface >() );

	}
	return uno::makeAny( nValue );
}

void SAL_CALL
ScVbaFont::setStrikethrough( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	sal_Bool bValue = sal_False;
	aValue >>= bValue;
	short nValue = awt::FontStrikeout::NONE;
	if( bValue )
		nValue = awt::FontStrikeout::SINGLE;
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharStrikeout" ) ), ( uno::Any )nValue );
}

uno::Any SAL_CALL
ScVbaFont::getStrikethrough() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_CROSSEDOUT, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();
	short nValue = 0;
	mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharStrikeout" ) ) ) >>= nValue;
	return uno::Any( nValue == awt::FontStrikeout::SINGLE );
}

void  SAL_CALL
ScVbaFont::setShadow( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharShadowed" ) ), aValue );
}

uno::Any SAL_CALL
ScVbaFont::getShadow() throw (uno::RuntimeException)
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_SHADOWED, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();
	return mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharShadowed" ) ) );
}

void  SAL_CALL
ScVbaFont::setItalic( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	sal_Bool bValue = sal_False;
	aValue >>= bValue;
	short nValue = awt::FontSlant_NONE;
	if( bValue )
		nValue = awt::FontSlant_ITALIC;
    mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharPosture" ) ), ( uno::Any )nValue );
}

uno::Any SAL_CALL
ScVbaFont::getItalic() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_POSTURE, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();

    awt::FontSlant aFS;
	mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharPosture" ) ) ) >>= aFS;
	return uno::makeAny( aFS == awt::FontSlant_ITALIC );
}

void  SAL_CALL
ScVbaFont::setName( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	rtl::OUString sString;
	aValue >>= sString;
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharFontName" ) ), aValue);
}

uno::Any SAL_CALL
ScVbaFont::getName() throw ( uno::RuntimeException )
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();
	return mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharFontName" ) ) );
}
uno::Any
ScVbaFont::getColor() throw (uno::RuntimeException)
{
	uno::Any aAny;
	aAny = OORGBToXLRGB( mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharColor" ) ) ) );
	return aAny;
}

void
ScVbaFont::setColor( const uno::Any& _color  ) throw (uno::RuntimeException)
{
	mxFont->setPropertyValue(  rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharColor" ) ) , XLRGBToOORGB(_color));
}

void  SAL_CALL
ScVbaFont::setOutlineFont( const uno::Any& aValue ) throw ( uno::RuntimeException )
{
	mxFont->setPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharContoured" ) ), aValue );
}

uno::Any SAL_CALL
ScVbaFont::getOutlineFont() throw (uno::RuntimeException)
{
	if ( GetDataSet() )
		if (  GetDataSet()->GetItemState( ATTR_FONT_CONTOUR, TRUE, NULL) == SFX_ITEM_DONTCARE )
			return aNULL();
	return mxFont->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "CharContoured" ) ) );
}

rtl::OUString&
ScVbaFont::getServiceImplName()
{
	static rtl::OUString sImplName( RTL_CONSTASCII_USTRINGPARAM("ScVbaFont") );
	return sImplName;
}

uno::Sequence< rtl::OUString >
ScVbaFont::getServiceNames()
{
	static uno::Sequence< rtl::OUString > aServiceNames;
	if ( aServiceNames.getLength() == 0 )
	{
		aServiceNames.realloc( 1 );
		aServiceNames[ 0 ] = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("ooo.vba.excel.Font" ) );
	}
	return aServiceNames;
}
