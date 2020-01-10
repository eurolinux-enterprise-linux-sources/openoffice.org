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
#include "precompiled_xmloff.hxx"

#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/io/XSeekableInputStream.hpp>
#include <com/sun/star/drawing/HomogenMatrix.hpp>
#include <com/sun/star/drawing/PolyPolygonShape3D.hpp>
#include <com/sun/star/drawing/ProjectionMode.hpp>
#include <com/sun/star/drawing/ShadeMode.hpp>
#include <com/sun/star/drawing/Direction3D.hpp>
#include <com/sun/star/drawing/Position3D.hpp>
#include <com/sun/star/drawing/CameraGeometry.hpp>
#include <com/sun/star/drawing/DoubleSequence.hpp>

#include <com/sun/star/table/XColumnRowRange.hpp>

#ifndef _XMLOFF_SHAPEEXPORT_HXX
#include <xmloff/shapeexport.hxx>
#endif
#include "sdpropls.hxx"
#include <tools/debug.hxx>
#include <rtl/ustrbuf.hxx>
#include <xmloff/xmlexp.hxx>
#include <xmloff/xmluconv.hxx>
#include "xexptran.hxx"
#include <xmloff/xmltoken.hxx>
#include "EnhancedCustomShapeToken.hxx"
#include <com/sun/star/container/XIdentifierContainer.hpp>
#include <com/sun/star/drawing/ShadeMode.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeParameterType.hpp>
#ifndef _COM_SUN_STAR_DRAWING_ENHANCEDCUSTOMSHAPEPARAMETERPARI_HPP_
#include <com/sun/star/drawing/EnhancedCustomShapeParameterPair.hpp>
#endif
#include <com/sun/star/drawing/EnhancedCustomShapeGluePointType.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeTextFrame.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeSegment.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeSegmentCommand.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeAdjustmentValue.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeTextPathMode.hpp>
#include <com/sun/star/beans/PropertyValues.hpp>
#include <rtl/math.hxx>
#include <tools/string.hxx>
#include <basegfx/vector/b3dvector.hxx>

#include "xmlnmspe.hxx"
#include "XMLBase64Export.hxx"

using ::rtl::OUString;
using ::rtl::OUStringBuffer;

using namespace ::com::sun::star;
using namespace ::com::sun::star::io;
using namespace ::xmloff::token;
using namespace ::xmloff::EnhancedCustomShapeToken;

using ::com::sun::star::embed::XStorage;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::UNO_QUERY_THROW;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;

//////////////////////////////////////////////////////////////////////////////

void ExportParameter( rtl::OUStringBuffer& rStrBuffer, const com::sun::star::drawing::EnhancedCustomShapeParameter& rParameter )
{
	if ( rStrBuffer.getLength() )
		rStrBuffer.append( (sal_Unicode)' ' );
	if ( rParameter.Value.getValueTypeClass() == uno::TypeClass_DOUBLE )
	{
		double fNumber = 0.0;
		rParameter.Value >>= fNumber;
		::rtl::math::doubleToUStringBuffer( rStrBuffer, fNumber, rtl_math_StringFormat_Automatic, rtl_math_DecimalPlaces_Max, '.', sal_True );
	}
	else
	{
		sal_Int32 nValue = 0;
		rParameter.Value >>= nValue;

		switch( rParameter.Type )
		{
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::EQUATION :
			{
				rStrBuffer.append( (sal_Unicode)'?' );
				rStrBuffer.append( (sal_Unicode)'f' );
				rStrBuffer.append( rtl::OUString::valueOf( nValue ) );
			}
			break;

			case com::sun::star::drawing::EnhancedCustomShapeParameterType::ADJUSTMENT :
			{
				rStrBuffer.append( (sal_Unicode)'$' );
				rStrBuffer.append( rtl::OUString::valueOf( nValue ) );
			}
			break;
			
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::BOTTOM :
				rStrBuffer.append( GetXMLToken( XML_BOTTOM ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::RIGHT :
				rStrBuffer.append( GetXMLToken( XML_RIGHT ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::TOP :
				rStrBuffer.append( GetXMLToken( XML_TOP ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::LEFT :
				rStrBuffer.append( GetXMLToken( XML_LEFT ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::XSTRETCH :
				rStrBuffer.append( GetXMLToken( XML_XSTRETCH ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::YSTRETCH :
				rStrBuffer.append( GetXMLToken( XML_YSTRETCH ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::HASSTROKE :
				rStrBuffer.append( GetXMLToken( XML_HASSTROKE ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::HASFILL :
				rStrBuffer.append( GetXMLToken( XML_HASFILL ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::WIDTH :
				rStrBuffer.append( GetXMLToken( XML_WIDTH ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::HEIGHT :
				rStrBuffer.append( GetXMLToken( XML_HEIGHT ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::LOGWIDTH :
				rStrBuffer.append( GetXMLToken( XML_LOGWIDTH ) ); break;
			case com::sun::star::drawing::EnhancedCustomShapeParameterType::LOGHEIGHT :
				rStrBuffer.append( GetXMLToken( XML_LOGHEIGHT ) ); break;
			default :
				rStrBuffer.append( rtl::OUString::valueOf( nValue ) );
		}
	}
}

void ImpExportEquations( SvXMLExport& rExport, const uno::Sequence< rtl::OUString >& rEquations )
{
	sal_Int32 i;
	for ( i = 0; i < rEquations.getLength(); i++ )
	{
		rtl::OUString aStr( String( 'f' ) );
		aStr += rtl::OUString::valueOf( i );
		rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_NAME, aStr );

		aStr = rEquations[ i ];
		sal_Int32 nIndex = 0;
		do
		{
			nIndex = aStr.indexOf( (sal_Unicode)'?', nIndex );
			if ( nIndex != -1 )
			{
				rtl::OUString aNew( aStr.copy( 0, nIndex + 1 ) );
				aNew += String( 'f' );
				aNew += aStr.copy( nIndex + 1, ( aStr.getLength() - nIndex ) - 1 );
				aStr = aNew;
				nIndex++;
			}
		} while( nIndex != -1 );
		rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_FORMULA, aStr );
		SvXMLElementExport aOBJ( rExport, XML_NAMESPACE_DRAW, XML_EQUATION, sal_True, sal_True );
	}
}

void ImpExportHandles( SvXMLExport& rExport, const uno::Sequence< beans::PropertyValues >& rHandles )
{
	sal_uInt32 i, j, nElements = rHandles.getLength();
	if ( nElements )
	{
		rtl::OUString		aStr;
		rtl::OUStringBuffer aStrBuffer;

		for ( i = 0; i < nElements; i++ )
		{
			sal_Bool bPosition = sal_False;
			const uno::Sequence< beans::PropertyValue >& rPropSeq = rHandles[ i ];
			for ( j = 0; j < (sal_uInt32)rPropSeq.getLength(); j++ )
			{
				const beans::PropertyValue& rPropVal = rPropSeq[ j ];
				switch( EASGet( rPropVal.Name ) )
				{
					case EAS_Position :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameterPair aPosition;
						if ( rPropVal.Value >>= aPosition )
						{
							ExportParameter( aStrBuffer, aPosition.First );
							ExportParameter( aStrBuffer, aPosition.Second );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_POSITION, aStr );
							bPosition = sal_True;
						}
					}
					break;
					case EAS_MirroredX :
					{
						sal_Bool bMirroredX = sal_Bool();
						if ( rPropVal.Value >>= bMirroredX )
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_MIRROR_HORIZONTAL,
								bMirroredX ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
					}
					break;
					case EAS_MirroredY :
					{
						sal_Bool bMirroredY = sal_Bool();
						if ( rPropVal.Value >>= bMirroredY )
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_MIRROR_VERTICAL,
								bMirroredY ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
					}
					break;
					case EAS_Switched :
					{
						sal_Bool bSwitched = sal_Bool();
						if ( rPropVal.Value >>= bSwitched )
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_SWITCHED,
								bSwitched ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
					}
					break;
					case EAS_Polar :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameterPair aPolar;
						if ( rPropVal.Value >>= aPolar )
						{
							ExportParameter( aStrBuffer, aPolar.First );
							ExportParameter( aStrBuffer, aPolar.Second );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_POLAR, aStr );
						}
					}
					break;
					case EAS_RadiusRangeMinimum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aRadiusRangeMinimum;
						if ( rPropVal.Value >>= aRadiusRangeMinimum )
						{
							ExportParameter( aStrBuffer, aRadiusRangeMinimum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RADIUS_RANGE_MINIMUM, aStr );
						}
					}
					break;
					case EAS_RadiusRangeMaximum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aRadiusRangeMaximum;
						if ( rPropVal.Value >>= aRadiusRangeMaximum )
						{
							ExportParameter( aStrBuffer, aRadiusRangeMaximum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RADIUS_RANGE_MAXIMUM, aStr );
						}
					}
					break;
					case EAS_RangeXMinimum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aXRangeMinimum;
						if ( rPropVal.Value >>= aXRangeMinimum )
						{
							ExportParameter( aStrBuffer, aXRangeMinimum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RANGE_X_MINIMUM, aStr );
						}
					}
					break;
					case EAS_RangeXMaximum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aXRangeMaximum;
						if ( rPropVal.Value >>= aXRangeMaximum )
						{
							ExportParameter( aStrBuffer, aXRangeMaximum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RANGE_X_MAXIMUM, aStr );
						}
					}
					break;
					case EAS_RangeYMinimum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aYRangeMinimum;
						if ( rPropVal.Value >>= aYRangeMinimum )
						{
							ExportParameter( aStrBuffer, aYRangeMinimum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RANGE_Y_MINIMUM, aStr );
						}
					}
					break;
					case EAS_RangeYMaximum :
					{
						com::sun::star::drawing::EnhancedCustomShapeParameter aYRangeMaximum;
						if ( rPropVal.Value >>= aYRangeMaximum )
						{
							ExportParameter( aStrBuffer, aYRangeMaximum );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_HANDLE_RANGE_Y_MAXIMUM, aStr );
						}
					}
					break;
					default:
						break;
				}
			}
			if ( bPosition )
				SvXMLElementExport aOBJ( rExport, XML_NAMESPACE_DRAW, XML_HANDLE, sal_True, sal_True );
			else
				rExport.ClearAttrList();
		}
	}
}

void ImpExportEnhancedPath( SvXMLExport& rExport,
	const uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeParameterPair >& rCoordinates,
		const uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeSegment >& rSegments )
{

	rtl::OUString		aStr;
	rtl::OUStringBuffer aStrBuffer;

	sal_Int32 i, j, k, l;

	sal_Int32 nCoords = rCoordinates.getLength();
	sal_Int32 nSegments = rSegments.getLength();
	sal_Bool bSimpleSegments = nSegments == 0;
	if ( bSimpleSegments )
		nSegments = 4;
	for ( j = i = 0; j < nSegments; j++ )
	{
		com::sun::star::drawing::EnhancedCustomShapeSegment aSegment;
		if ( bSimpleSegments )
		{
			// if there are not enough segments we will default them
			switch( j )
			{
				case 0 :
				{
					aSegment.Count = 1;
					aSegment.Command = com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::MOVETO;
				}
				break;
				case 1 :
				{
					aSegment.Count = (sal_Int16)Min( nCoords - 1, (sal_Int32)32767 );
					aSegment.Command = com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::LINETO;
				}
				break;
				case 2 :
				{
					aSegment.Count = 1;
					aSegment.Command = com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::CLOSESUBPATH;
				}
				break;
				case 3 :
				{
					aSegment.Count = 1;
					aSegment.Command = com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ENDSUBPATH;
				}
				break;
			}
		}
		else
			aSegment = rSegments[ j ];

		if ( aStrBuffer.getLength() )
			aStrBuffer.append( (sal_Unicode)' ' );

		sal_Int32 nParameter = 0;
		switch( aSegment.Command )
		{
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::CLOSESUBPATH :
				aStrBuffer.append( (sal_Unicode)'Z' ); break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ENDSUBPATH :
				aStrBuffer.append( (sal_Unicode)'N' ); break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::NOFILL :
				aStrBuffer.append( (sal_Unicode)'F' ); break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::NOSTROKE :
				aStrBuffer.append( (sal_Unicode)'S' ); break;

			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::MOVETO :
				aStrBuffer.append( (sal_Unicode)'M' ); nParameter = 1; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::LINETO :
				aStrBuffer.append( (sal_Unicode)'L' ); nParameter = 1; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::CURVETO :
				aStrBuffer.append( (sal_Unicode)'C' ); nParameter = 3; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ANGLEELLIPSETO :
				aStrBuffer.append( (sal_Unicode)'T' ); nParameter = 3; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ANGLEELLIPSE :
				aStrBuffer.append( (sal_Unicode)'U' ); nParameter = 3; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ARCTO :
				aStrBuffer.append( (sal_Unicode)'A' ); nParameter = 4; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ARC :
				aStrBuffer.append( (sal_Unicode)'B' ); nParameter = 4; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::CLOCKWISEARCTO :
				aStrBuffer.append( (sal_Unicode)'W' ); nParameter = 4; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::CLOCKWISEARC :
				aStrBuffer.append( (sal_Unicode)'V' ); nParameter = 4; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ELLIPTICALQUADRANTX :
				aStrBuffer.append( (sal_Unicode)'X' ); nParameter = 1; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::ELLIPTICALQUADRANTY :
				aStrBuffer.append( (sal_Unicode)'Y' ); nParameter = 1; break;
			case com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::QUADRATICCURVETO :
				aStrBuffer.append( (sal_Unicode)'Q' ); nParameter = 2; break;

			default : // ups, seems to be something wrong
			{
				aSegment.Count = 1;
				aSegment.Command = com::sun::star::drawing::EnhancedCustomShapeSegmentCommand::LINETO;
			}
			break;
		}
		if ( nParameter )
		{
			for ( k = 0; k < aSegment.Count; k++ )
			{
				if ( ( i + nParameter ) <= nCoords )
				{
					for ( l = 0; l < nParameter; l++ )
					{
						ExportParameter( aStrBuffer, rCoordinates[ i ].First );
						ExportParameter( aStrBuffer, rCoordinates[ i++ ].Second );
					}
				}
				else
				{
					j = nSegments;	// error -> exiting
					break;
				}
			}
		}
	}
	aStr = aStrBuffer.makeStringAndClear();
	rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_ENHANCED_PATH, aStr );
}

void ImpExportEnhancedGeometry( SvXMLExport& rExport, const uno::Reference< beans::XPropertySet >& xPropSet )
{
	sal_Bool bEquations = sal_False;
	uno::Sequence< rtl::OUString > aEquations;

	sal_Bool bHandles = sal_False;
	uno::Sequence< beans::PropertyValues > aHandles;

	sal_Bool bCoordinates = sal_False;
	uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeSegment > aSegments;
	uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeParameterPair > aCoordinates;

	uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeAdjustmentValue > aAdjustmentValues;

	rtl::OUString		aStr;
	rtl::OUStringBuffer aStrBuffer;
	SvXMLUnitConverter& rUnitConverter = rExport.GetMM100UnitConverter();

	uno::Reference< beans::XPropertySetInfo > xPropSetInfo( xPropSet->getPropertySetInfo() );

	// geometry
	const rtl::OUString	sCustomShapeGeometry( RTL_CONSTASCII_USTRINGPARAM( "CustomShapeGeometry" ) );
	if ( xPropSetInfo.is() && xPropSetInfo->hasPropertyByName( sCustomShapeGeometry ) )
	{
		uno::Any aGeoPropSet( xPropSet->getPropertyValue( sCustomShapeGeometry ) );
		uno::Sequence< beans::PropertyValue > aGeoPropSeq;

		if ( aGeoPropSet >>= aGeoPropSeq )
		{
			const rtl::OUString	sCustomShapeType( RTL_CONSTASCII_USTRINGPARAM( "NonPrimitive" ) );
			rtl::OUString aCustomShapeType( sCustomShapeType );

			sal_Int32 j, nGeoPropCount = aGeoPropSeq.getLength();
			for ( j = 0; j < nGeoPropCount; j++ )
			{
				const beans::PropertyValue& rGeoProp = aGeoPropSeq[ j ];
				switch( EASGet( rGeoProp.Name ) )
				{
					case EAS_Type :
					{
						rGeoProp.Value >>= aCustomShapeType;
					}
					break;
					case EAS_MirroredX :
					{
						sal_Bool bMirroredX = sal_Bool();
						if ( rGeoProp.Value >>= bMirroredX )
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_MIRROR_HORIZONTAL,
								bMirroredX ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
					}
					break;
					case EAS_MirroredY :
					{
						sal_Bool bMirroredY = sal_Bool();
						if ( rGeoProp.Value >>= bMirroredY )
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_MIRROR_VERTICAL,
								bMirroredY ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
					}
					break;
					case EAS_ViewBox :
					{
						awt::Rectangle aRect;
						if ( rGeoProp.Value >>= aRect )
						{
							SdXMLImExViewBox aViewBox( aRect.X, aRect.Y, aRect.Width, aRect.Height );
							rExport.AddAttribute( XML_NAMESPACE_SVG, XML_VIEWBOX, aViewBox.GetExportString() );
						}
					}
					break;
					case EAS_TextRotateAngle :
					{
						double fTextRotateAngle = 0;
						if ( rGeoProp.Value >>= fTextRotateAngle )
						{
							rUnitConverter.convertDouble( aStrBuffer, fTextRotateAngle );
							aStr = aStrBuffer.makeStringAndClear();
							rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_ROTATE_ANGLE, aStr );
						}
					}
					break;
					case EAS_Extrusion :
					{
						uno::Sequence< beans::PropertyValue > aExtrusionPropSeq;
						if ( rGeoProp.Value >>= aExtrusionPropSeq )
						{
							sal_Int32 i, nCount = aExtrusionPropSeq.getLength();
							for ( i = 0; i < nCount; i++ )
							{
								const beans::PropertyValue& rProp = aExtrusionPropSeq[ i ];
								switch( EASGet( rProp.Name ) )
								{
									case EAS_Extrusion :
									{
										sal_Bool bExtrusionOn = sal_Bool();
										if ( rProp.Value >>= bExtrusionOn )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION,
												bExtrusionOn ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_Brightness :
									{
										double fExtrusionBrightness = 0;
										if ( rProp.Value >>= fExtrusionBrightness )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionBrightness, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_BRIGHTNESS, aStr );
										}
									}
									break;
									case EAS_Depth :
									{
										com::sun::star::drawing::EnhancedCustomShapeParameterPair aDepthParaPair;
										if ( rProp.Value >>= aDepthParaPair )
										{
											double fDepth = 0;
											if ( aDepthParaPair.First.Value >>= fDepth )
											{
												rExport.GetMM100UnitConverter().convertDouble( aStrBuffer, fDepth, sal_True );
												ExportParameter( aStrBuffer, aDepthParaPair.Second );
												aStr = aStrBuffer.makeStringAndClear();
												rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_DEPTH, aStr );
											}
										}
									}
									break;
									case EAS_Diffusion :
									{
										double fExtrusionDiffusion = 0;
										if ( rProp.Value >>= fExtrusionDiffusion )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionDiffusion, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_DIFFUSION, aStr );
										}
									}
									break;
									case EAS_NumberOfLineSegments :
									{
										sal_Int32 nExtrusionNumberOfLineSegments = 0;
										if ( rProp.Value >>= nExtrusionNumberOfLineSegments )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_NUMBER_OF_LINE_SEGMENTS, rtl::OUString::valueOf( nExtrusionNumberOfLineSegments ) );
									}
									break;
									case EAS_LightFace :
									{
										sal_Bool bExtrusionLightFace = sal_Bool();
										if ( rProp.Value >>= bExtrusionLightFace )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_LIGHT_FACE,
												bExtrusionLightFace ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_FirstLightHarsh :
									{
										sal_Bool bExtrusionFirstLightHarsh = sal_Bool();
										if ( rProp.Value >>= bExtrusionFirstLightHarsh )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_FIRST_LIGHT_HARSH,
												bExtrusionFirstLightHarsh ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_SecondLightHarsh :
									{
										sal_Bool bExtrusionSecondLightHarsh = sal_Bool();
										if ( rProp.Value >>= bExtrusionSecondLightHarsh )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SECOND_LIGHT_HARSH,
												bExtrusionSecondLightHarsh ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_FirstLightLevel :
									{
										double fExtrusionFirstLightLevel = 0;
										if ( rProp.Value >>= fExtrusionFirstLightLevel )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionFirstLightLevel, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_FIRST_LIGHT_LEVEL, aStr );
										}
									}
									break;
									case EAS_SecondLightLevel :
									{
										double fExtrusionSecondLightLevel = 0;
										if ( rProp.Value >>= fExtrusionSecondLightLevel )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionSecondLightLevel, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SECOND_LIGHT_LEVEL, aStr );
										}
									}
									break;
									case EAS_FirstLightDirection :
									{
										drawing::Direction3D aExtrusionFirstLightDirection;
										if ( rProp.Value >>= aExtrusionFirstLightDirection )
										{
											::basegfx::B3DVector aVec3D( aExtrusionFirstLightDirection.DirectionX, aExtrusionFirstLightDirection.DirectionY,
												aExtrusionFirstLightDirection.DirectionZ );
											rUnitConverter.convertB3DVector( aStrBuffer, aVec3D );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_FIRST_LIGHT_DIRECTION, aStr );
										}
									}
									break;
									case EAS_SecondLightDirection :
									{
										drawing::Direction3D aExtrusionSecondLightDirection;
										if ( rProp.Value >>= aExtrusionSecondLightDirection )
										{
											::basegfx::B3DVector aVec3D( aExtrusionSecondLightDirection.DirectionX, aExtrusionSecondLightDirection.DirectionY,
												aExtrusionSecondLightDirection.DirectionZ );
											rUnitConverter.convertB3DVector( aStrBuffer, aVec3D );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SECOND_LIGHT_DIRECTION, aStr );
										}
									}
									break;
									case EAS_Metal :
									{
										sal_Bool bExtrusionMetal = sal_Bool();
										if ( rProp.Value >>= bExtrusionMetal )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_METAL,
												bExtrusionMetal ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_ShadeMode :
									{
										// shadeMode
										drawing::ShadeMode eShadeMode;
										if( rProp.Value >>= eShadeMode )
										{
											if( eShadeMode == drawing::ShadeMode_FLAT )
												aStr = GetXMLToken( XML_FLAT );
											else if( eShadeMode == drawing::ShadeMode_PHONG )
												aStr = GetXMLToken( XML_PHONG );
											else if( eShadeMode == drawing::ShadeMode_SMOOTH )
												aStr = GetXMLToken( XML_GOURAUD );
											else
												aStr = GetXMLToken( XML_DRAFT );
										}
										else
										{
											// ShadeMode enum not there, write default
											aStr = GetXMLToken( XML_FLAT);
										}
										rExport.AddAttribute( XML_NAMESPACE_DR3D, XML_SHADE_MODE, aStr );
									}
									break;
									case EAS_RotateAngle :
									{
										com::sun::star::drawing::EnhancedCustomShapeParameterPair aRotateAngleParaPair;
										if ( rProp.Value >>= aRotateAngleParaPair )
										{
											ExportParameter( aStrBuffer, aRotateAngleParaPair.First );
											ExportParameter( aStrBuffer, aRotateAngleParaPair.Second );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_ROTATION_ANGLE, aStr );
										}
									}
									break;
									case EAS_RotationCenter :
									{
										drawing::Direction3D aExtrusionRotationCenter;
										if ( rProp.Value >>= aExtrusionRotationCenter )
										{
											::basegfx::B3DVector aVec3D( aExtrusionRotationCenter.DirectionX, aExtrusionRotationCenter.DirectionY,
												aExtrusionRotationCenter.DirectionZ );
											rUnitConverter.convertB3DVector( aStrBuffer, aVec3D );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_ROTATION_CENTER, aStr );
										}
									}
									break;
									case EAS_Shininess :
									{
										double fExtrusionShininess = 0;
										if ( rProp.Value >>= fExtrusionShininess )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionShininess, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SHININESS, aStr );
										}
									}
									break;
									case EAS_Skew :
									{
										com::sun::star::drawing::EnhancedCustomShapeParameterPair aSkewParaPair;
										if ( rProp.Value >>= aSkewParaPair )
										{
											ExportParameter( aStrBuffer, aSkewParaPair.First );
											ExportParameter( aStrBuffer, aSkewParaPair.Second );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SKEW, aStr );
										}
									}
									break;
									case EAS_Specularity :
									{
										double fExtrusionSpecularity = 0;
										if ( rProp.Value >>= fExtrusionSpecularity )
										{
											rUnitConverter.convertDouble( aStrBuffer, fExtrusionSpecularity, sal_False, MAP_RELATIVE, MAP_RELATIVE );
											aStrBuffer.append( (sal_Unicode)'%' );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_SPECULARITY, aStr );
										}
									}
									break;
									case EAS_ProjectionMode :
									{
										drawing::ProjectionMode eProjectionMode;
										if ( rProp.Value >>= eProjectionMode )
											rExport.AddAttribute( XML_NAMESPACE_DR3D, XML_PROJECTION,
												eProjectionMode == drawing::ProjectionMode_PARALLEL ? GetXMLToken( XML_PARALLEL ) : GetXMLToken( XML_PERSPECTIVE ) );
									}
									break;
									case EAS_ViewPoint :
									{
										drawing::Position3D aExtrusionViewPoint;
										if ( rProp.Value >>= aExtrusionViewPoint )
										{
											rUnitConverter.convertPosition3D( aStrBuffer, aExtrusionViewPoint );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_VIEWPOINT, aStr );
										}
									}
									break;
									case EAS_Origin :
									{
										com::sun::star::drawing::EnhancedCustomShapeParameterPair aOriginParaPair;
										if ( rProp.Value >>= aOriginParaPair )
										{
											ExportParameter( aStrBuffer, aOriginParaPair.First );
											ExportParameter( aStrBuffer, aOriginParaPair.Second );
											aStr = aStrBuffer.makeStringAndClear();
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_ORIGIN, aStr );
										}
									}
									break;
									case EAS_Color :
									{
										sal_Bool bExtrusionColor = sal_Bool();
										if ( rProp.Value >>= bExtrusionColor )
										{
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_COLOR,
												bExtrusionColor ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
										}
									}
									break;
									default:
										break;
								}
							}
						}
					}
					break;
					case EAS_TextPath :
					{
						uno::Sequence< beans::PropertyValue > aTextPathPropSeq;
						if ( rGeoProp.Value >>= aTextPathPropSeq )
						{
							sal_Int32 i, nCount = aTextPathPropSeq.getLength();
							for ( i = 0; i < nCount; i++ )
							{
								const beans::PropertyValue& rProp = aTextPathPropSeq[ i ];
								switch( EASGet( rProp.Name ) )
								{
									case EAS_TextPath :
									{
										sal_Bool bTextPathOn = sal_Bool();
										if ( rProp.Value >>= bTextPathOn )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_PATH,
												bTextPathOn ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_TextPathMode :
									{
										com::sun::star::drawing::EnhancedCustomShapeTextPathMode eTextPathMode;
										if ( rProp.Value >>= eTextPathMode )
										{
											switch ( eTextPathMode )
											{
												case com::sun::star::drawing::EnhancedCustomShapeTextPathMode_NORMAL: aStr = GetXMLToken( XML_NORMAL ); break;
												case com::sun::star::drawing::EnhancedCustomShapeTextPathMode_PATH	: aStr = GetXMLToken( XML_PATH );   break;
												case com::sun::star::drawing::EnhancedCustomShapeTextPathMode_SHAPE : aStr = GetXMLToken( XML_SHAPE );  break;
												default:
													break;
											}
											if ( aStr.getLength() )
												rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_PATH_MODE, aStr );
										}
									}
									break;
									case EAS_ScaleX :
									{
										sal_Bool bScaleX = sal_Bool();
										if ( rProp.Value >>= bScaleX )
										{
											aStr = bScaleX ? GetXMLToken( XML_SHAPE ) : GetXMLToken( XML_PATH );
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_PATH_SCALE, aStr );
										}
									}
									break;
									case EAS_SameLetterHeights :
									{
										sal_Bool bSameLetterHeights = sal_Bool();
										if ( rProp.Value >>= bSameLetterHeights )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_PATH_SAME_LETTER_HEIGHTS,
												bSameLetterHeights ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									default:
										break;
								}
							}
						}
					}
					break;
					case EAS_Path :
					{
						uno::Sequence< beans::PropertyValue > aPathPropSeq;
						if ( rGeoProp.Value >>= aPathPropSeq )
						{
							sal_Int32 i, nCount = aPathPropSeq.getLength();
							for ( i = 0; i < nCount; i++ )
							{
								const beans::PropertyValue& rProp = aPathPropSeq[ i ];
								switch( EASGet( rProp.Name ) )
								{
									case EAS_ExtrusionAllowed :
									{
										sal_Bool bExtrusionAllowed = sal_Bool();
										if ( rProp.Value >>= bExtrusionAllowed )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_EXTRUSION_ALLOWED,
												bExtrusionAllowed ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_ConcentricGradientFillAllowed :
									{
										sal_Bool bConcentricGradientFillAllowed = sal_Bool();
										if ( rProp.Value >>= bConcentricGradientFillAllowed )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_CONCENTRIC_GRADIENT_FILL_ALLOWED,
												bConcentricGradientFillAllowed ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_TextPathAllowed  :
									{
										sal_Bool bTextPathAllowed = sal_Bool();
										if ( rProp.Value >>= bTextPathAllowed )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_PATH_ALLOWED,
												bTextPathAllowed ? GetXMLToken( XML_TRUE ) : GetXMLToken( XML_FALSE ) );
									}
									break;
									case EAS_GluePoints :
									{
										com::sun::star::uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeParameterPair> aGluePoints;
										if ( rProp.Value >>= aGluePoints )
										{
											sal_Int32 k, nElements = aGluePoints.getLength();
											if ( nElements )
											{
												for( k = 0; k < nElements; k++ )
												{
													ExportParameter( aStrBuffer, aGluePoints[ k ].First );
													ExportParameter( aStrBuffer, aGluePoints[ k ].Second );
												}
												aStr = aStrBuffer.makeStringAndClear();
											}
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_GLUE_POINTS, aStr );
										}
									}
									break;
									case EAS_GluePointType :
									{
										sal_Int16 nGluePointType = sal_Int16();
										if ( rProp.Value >>= nGluePointType )
										{
											switch ( nGluePointType )
											{
												case com::sun::star::drawing::EnhancedCustomShapeGluePointType::NONE     : aStr = GetXMLToken( XML_NONE );    break;
												case com::sun::star::drawing::EnhancedCustomShapeGluePointType::SEGMENTS : aStr = GetXMLToken( XML_SEGMENTS ); break;
												case com::sun::star::drawing::EnhancedCustomShapeGluePointType::RECT     : aStr = GetXMLToken( XML_RECTANGLE ); break;
											}
											if ( aStr.getLength() )
												rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_GLUE_POINT_TYPE, aStr );
										}
									}
									break;
									case EAS_Coordinates :
									{
										bCoordinates = ( rProp.Value >>= aCoordinates );
									}
									break;
									case EAS_Segments :
									{
										rProp.Value >>= aSegments;
									}
									break;
									case EAS_StretchX :
									{
										sal_Int32 nStretchPoint = 0;
										if ( rProp.Value >>= nStretchPoint )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_PATH_STRETCHPOINT_X, rtl::OUString::valueOf( nStretchPoint ) );
									}
									break;
									case EAS_StretchY :
									{
										sal_Int32 nStretchPoint = 0;
										if ( rProp.Value >>= nStretchPoint )
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_PATH_STRETCHPOINT_Y, rtl::OUString::valueOf( nStretchPoint ) );
									}
									break;
									case EAS_TextFrames :
									{
										com::sun::star::uno::Sequence< com::sun::star::drawing::EnhancedCustomShapeTextFrame > aPathTextFrames;
										if ( rProp.Value >>= aPathTextFrames )
										{
											if ( (sal_uInt16)aPathTextFrames.getLength() )
											{
												sal_uInt16 k, nElements = (sal_uInt16)aPathTextFrames.getLength();
												for ( k = 0; k < nElements; k++ )
												{
													ExportParameter( aStrBuffer, aPathTextFrames[ k ].TopLeft.First );
													ExportParameter( aStrBuffer, aPathTextFrames[ k ].TopLeft.Second );
													ExportParameter( aStrBuffer, aPathTextFrames[ k ].BottomRight.First );
													ExportParameter( aStrBuffer, aPathTextFrames[ k ].BottomRight.Second );
												}
												aStr = aStrBuffer.makeStringAndClear();
											}
											rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TEXT_AREAS, aStr );
										}
									}
									break;
									default:
										break;
								}
							}
						}
					}
					break;
					case EAS_Equations :
					{
						bEquations = ( rGeoProp.Value >>= aEquations );
					}
					break;
					case EAS_Handles :
					{
						bHandles = ( rGeoProp.Value >>= aHandles );
					}
					break;
					case EAS_AdjustmentValues :
					{
						rGeoProp.Value >>= aAdjustmentValues;
					}
					break;
					default:
						break;
				}
			}	// for
			rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_TYPE, aCustomShapeType );

			// adjustments
			sal_Int32 nAdjustmentValues = aAdjustmentValues.getLength();
			if ( nAdjustmentValues )
			{
				sal_Int32 i, nValue = 0;
				for ( i = 0; i < nAdjustmentValues; i++ )
				{
					if ( i )
						aStrBuffer.append( (sal_Unicode)' ' );

					const com::sun::star::drawing::EnhancedCustomShapeAdjustmentValue& rAdj = aAdjustmentValues[ i ];
					if ( rAdj.State == beans::PropertyState_DIRECT_VALUE )
					{
						if ( rAdj.Value.getValueTypeClass() == uno::TypeClass_DOUBLE )
						{
							double fValue;
							rAdj.Value >>= fValue;
							rUnitConverter.convertDouble( aStrBuffer, fValue );
						}
						else
						{
							rAdj.Value >>= nValue;
							rUnitConverter.convertNumber( aStrBuffer, nValue );
						}
					}
					else
						rUnitConverter.convertNumber( aStrBuffer, 0 );			// this should not be, but better than setting nothing
				}
				aStr = aStrBuffer.makeStringAndClear();
				rExport.AddAttribute( XML_NAMESPACE_DRAW, XML_MODIFIERS, aStr );
			}
			if ( bCoordinates )
				ImpExportEnhancedPath( rExport, aCoordinates, aSegments );
		}
	}
	SvXMLElementExport aOBJ( rExport, XML_NAMESPACE_DRAW, XML_ENHANCED_GEOMETRY, sal_True, sal_True );
	if ( bEquations )
		ImpExportEquations( rExport, aEquations );
	if ( bHandles )
		ImpExportHandles( rExport, aHandles );
}

void XMLShapeExport::ImpExportCustomShape(
	const uno::Reference< drawing::XShape >& xShape,
	XmlShapeType, sal_Int32 nFeatures, com::sun::star::awt::Point* pRefPoint )
{
	const uno::Reference< beans::XPropertySet > xPropSet(xShape, uno::UNO_QUERY);
	if ( xPropSet.is() )
	{
		rtl::OUString aStr;
		uno::Reference< beans::XPropertySetInfo > xPropSetInfo( xPropSet->getPropertySetInfo() );

		// Transformation
		ImpExportNewTrans( xPropSet, nFeatures, pRefPoint );

		if ( xPropSetInfo.is() )
		{
			const rtl::OUString	sCustomShapeEngine( RTL_CONSTASCII_USTRINGPARAM( "CustomShapeEngine" ) );
			if ( xPropSetInfo->hasPropertyByName( sCustomShapeEngine ) )
			{
				uno::Any aEngine( xPropSet->getPropertyValue( sCustomShapeEngine ) );
				if ( ( aEngine >>= aStr ) && aStr.getLength() )
					mrExport.AddAttribute( XML_NAMESPACE_DRAW, XML_ENGINE, aStr );
			}
			const rtl::OUString	sCustomShapeData( RTL_CONSTASCII_USTRINGPARAM( "CustomShapeData" ) );
			if ( xPropSetInfo->hasPropertyByName( sCustomShapeData ) )
			{
				uno::Any aData( xPropSet->getPropertyValue( sCustomShapeData ) );
				if ( ( aData >>= aStr ) && aStr.getLength() )
					mrExport.AddAttribute( XML_NAMESPACE_DRAW, XML_DATA, aStr );
			}
		}
		sal_Bool bCreateNewline( (nFeatures & SEF_EXPORT_NO_WS) == 0 ); // #86116#/#92210#
		SvXMLElementExport aOBJ( mrExport, XML_NAMESPACE_DRAW, XML_CUSTOM_SHAPE, bCreateNewline, sal_True );
		ImpExportDescription( xShape ); // #i68101#
		ImpExportEvents( xShape );
		ImpExportGluePoints( xShape );
		ImpExportText( xShape );
		ImpExportEnhancedGeometry( mrExport, xPropSet );
	}
}

void XMLShapeExport::ImpExportTableShape( const uno::Reference< drawing::XShape >& xShape, XmlShapeType /*eShapeType*/, sal_Int32 nFeatures, com::sun::star::awt::Point* pRefPoint )
{
	uno::Reference< beans::XPropertySet > xPropSet(xShape, uno::UNO_QUERY);
	uno::Reference< container::XNamed > xNamed(xShape, uno::UNO_QUERY);

	DBG_ASSERT( xPropSet.is() && xNamed.is(), "xmloff::XMLShapeExport::ImpExportTableShape(), tabe shape is not implementing needed interfaces");
	if(xPropSet.is() && xNamed.is()) try
	{
		// Transformation
		ImpExportNewTrans(xPropSet, nFeatures, pRefPoint);

		sal_Bool bIsEmptyPresObj = sal_False;

		// presentation settings
//		if(eShapeType == XmlShapeTypePresTableShape)
//			bIsEmptyPresObj = ImpExportPresentationAttributes( xPropSet, GetXMLToken(XML_PRESENTATION_TABLE) );

		const bool bCreateNewline( (nFeatures & SEF_EXPORT_NO_WS) == 0 );
		const bool bExportEmbedded(0 != (mrExport.getExportFlags() & EXPORT_EMBEDDED));

		SvXMLElementExport aElement( mrExport, XML_NAMESPACE_DRAW, XML_FRAME, bCreateNewline, sal_True );

		if( !bIsEmptyPresObj )
		{
			uno::Reference< container::XNamed > xTemplate( xPropSet->getPropertyValue( OUString( RTL_CONSTASCII_USTRINGPARAM( "TableTemplate" ) ) ), uno::UNO_QUERY );
			if( xTemplate.is() )
			{
				const OUString sTemplate( xTemplate->getName() );
				if( sTemplate.getLength() )
				{
					mrExport.AddAttribute(XML_NAMESPACE_TABLE, XML_TEMPLATE_NAME, sTemplate );

					for( const XMLPropertyMapEntry* pEntry = &aXMLTableShapeAttributes[0]; pEntry->msApiName; pEntry++ )
					{
					    try
					    {
						    sal_Bool bBool = sal_False;
						    const OUString sAPIPropertyName( OUString(pEntry->msApiName, pEntry->nApiNameLength, RTL_TEXTENCODING_ASCII_US ) );

						    xPropSet->getPropertyValue( sAPIPropertyName ) >>= bBool;
						    if( bBool )
							    mrExport.AddAttribute(pEntry->mnNameSpace, pEntry->meXMLName, XML_TRUE );
					    }
					    catch( uno::Exception& )
					    {
						    DBG_ERROR("XMLShapeExport::ImpExportTableShape(), exception caught!");
					    }
                    }
				}
			}
			uno::Reference< table::XColumnRowRange > xRange( xPropSet->getPropertyValue( msModel ), uno::UNO_QUERY_THROW );
			
			GetShapeTableExport()->exportTable( xRange );

			uno::Reference< graphic::XGraphic > xGraphic( xPropSet->getPropertyValue( OUString( RTL_CONSTASCII_USTRINGPARAM( "ReplacementGraphic" ) ) ), uno::UNO_QUERY );
			if( xGraphic.is() ) try
			{	
				Reference< lang::XMultiServiceFactory > xSM( GetExport().getServiceFactory(), UNO_QUERY_THROW );

				uno::Reference< embed::XStorage > xPictureStorage;
				uno::Reference< embed::XStorage > xStorage;
				uno::Reference< io::XStream > xPictureStream;

				OUString sPictureName;
				if( bExportEmbedded )
				{
					xPictureStream.set( xSM->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.comp.MemoryStream" ) ) ), UNO_QUERY_THROW );					
				}
				else
				{
					xStorage.set( GetExport().GetTargetStorage(), UNO_QUERY_THROW );

					xPictureStorage.set( xStorage->openStorageElement( OUString( RTL_CONSTASCII_USTRINGPARAM( "Pictures" ) ), ::embed::ElementModes::READWRITE ), uno::UNO_QUERY_THROW );
					const OUString sPrefix( RTL_CONSTASCII_USTRINGPARAM("TablePreview") );
					const OUString sSuffix( RTL_CONSTASCII_USTRINGPARAM(".svm") );

					sal_Int32 nIndex = 0;
					do
					{
						sPictureName = sPrefix;
						sPictureName += OUString::valueOf( ++nIndex );
						sPictureName += sSuffix;
					}
					while( xPictureStorage->hasByName( sPictureName ) );

					xPictureStream.set( xPictureStorage->openStreamElement( sPictureName, ::embed::ElementModes::READWRITE ), UNO_QUERY_THROW );
				}

				Reference< graphic::XGraphicProvider > xProvider( xSM->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.graphic.GraphicProvider" ) ) ), UNO_QUERY_THROW );
				Sequence< beans::PropertyValue > aArgs( 2 );
				aArgs[ 0 ].Name = OUString( RTL_CONSTASCII_USTRINGPARAM( "MimeType" ) );
				aArgs[ 0 ].Value <<= OUString( RTL_CONSTASCII_USTRINGPARAM( "image/x-vclgraphic" ) );
				aArgs[ 1 ].Name = OUString( RTL_CONSTASCII_USTRINGPARAM( "OutputStream" ) );
				aArgs[ 1 ].Value <<= xPictureStream->getOutputStream();
				xProvider->storeGraphic( xGraphic, aArgs );

				if( xPictureStorage.is() )
				{
					Reference< embed::XTransactedObject > xTrans( xPictureStorage, UNO_QUERY );
					if( xTrans.is() )
						xTrans->commit();
				}

				if( !bExportEmbedded )
				{
					OUString sURL( RTL_CONSTASCII_USTRINGPARAM( "Pictures/" ) );
					sURL += sPictureName;
					mrExport.AddAttribute(XML_NAMESPACE_XLINK, XML_HREF, sURL );
					mrExport.AddAttribute( XML_NAMESPACE_XLINK, XML_TYPE, XML_SIMPLE );
					mrExport.AddAttribute( XML_NAMESPACE_XLINK, XML_SHOW, XML_EMBED );
					mrExport.AddAttribute( XML_NAMESPACE_XLINK, XML_ACTUATE, XML_ONLOAD );
				}

				SvXMLElementExport aElem( GetExport(), XML_NAMESPACE_DRAW, XML_IMAGE, sal_False, sal_True );

				if( bExportEmbedded )
				{
					Reference< XSeekableInputStream > xSeekable( xPictureStream, UNO_QUERY_THROW );
					xSeekable->seek(0);

					XMLBase64Export aBase64Exp( GetExport() );
					aBase64Exp.exportOfficeBinaryDataElement( Reference < XInputStream >( xPictureStream, UNO_QUERY_THROW ) );
				}
			}
			catch( uno::Exception& )
			{
				DBG_ERROR("xmloff::XMLShapeExport::ImpExportTableShape(), exception caught!");
			}
		}

		ImpExportEvents( xShape );
		ImpExportGluePoints( xShape );
		ImpExportDescription( xShape ); // #i68101#
	}
	catch( uno::Exception& )
	{
		DBG_ERROR( "xmloff::XMLShapeExport::ImpExportTableShape(), exception caught!" );
	}
}
