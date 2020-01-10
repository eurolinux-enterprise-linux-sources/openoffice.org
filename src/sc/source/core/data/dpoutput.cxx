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
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include "scitems.hxx"
#include <svx/algitem.hxx>
#include <svx/boxitem.hxx>
#include <svx/brshitem.hxx>
#include <svx/wghtitem.hxx>
#include <unotools/transliterationwrapper.hxx>

#include "dpoutput.hxx"
#include "dptabsrc.hxx"
#include "dpcachetable.hxx"
#include "document.hxx"
#include "patattr.hxx"
#include "docpool.hxx"
#include "markdata.hxx"
#include "attrib.hxx"
#include "formula/errorcodes.hxx"		// errNoValue
#include "miscuno.hxx"
#include "globstr.hrc"
#include "stlpool.hxx"
#include "stlsheet.hxx"
#include "collect.hxx"
#include "scresid.hxx"
#include "unonames.hxx"
#include "sc.hrc"

#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>
#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/sheet/DataPilotTableHeaderData.hpp>
#include <com/sun/star/sheet/DataPilotTablePositionData.hpp>
#include <com/sun/star/sheet/DataPilotTablePositionType.hpp>
#include <com/sun/star/sheet/DataPilotTableResultData.hpp>
#include <com/sun/star/sheet/DataResultFlags.hpp>
#include <com/sun/star/sheet/GeneralFunction.hpp>
#include <com/sun/star/sheet/MemberResultFlags.hpp>
#include <com/sun/star/sheet/TableFilterField.hpp>
#include <com/sun/star/sheet/XDataPilotMemberResults.hpp>
#include <com/sun/star/sheet/XDataPilotResults.hpp>
#include <com/sun/star/sheet/XHierarchiesSupplier.hpp>
#include <com/sun/star/sheet/XLevelsSupplier.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>

#include <vector>

using namespace com::sun::star;
using ::std::vector;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::sheet::DataPilotTablePositionData;
using ::com::sun::star::sheet::DataPilotTableResultData;
using ::com::sun::star::uno::makeAny;
using ::com::sun::star::uno::Any;
using ::rtl::OUString;

// -----------------------------------------------------------------------

//!	move to a header file
//! use names from unonames.hxx?
#define DP_PROP_FUNCTION            "Function"
#define DP_PROP_ORIENTATION			"Orientation"
#define DP_PROP_POSITION			"Position"
#define DP_PROP_USEDHIERARCHY		"UsedHierarchy"
#define DP_PROP_DATADESCR			"DataDescription"
#define DP_PROP_ISDATALAYOUT		"IsDataLayoutDimension"
#define DP_PROP_NUMBERFORMAT		"NumberFormat"
#define DP_PROP_FILTER				"Filter"
#define DP_PROP_COLUMNGRAND         "ColumnGrand"
#define DP_PROP_ROWGRAND            "RowGrand"
#define DP_PROP_SUBTOTALS           "SubTotals"

// -----------------------------------------------------------------------

//!	dynamic!!!
#define SC_DPOUT_MAXLEVELS	256


struct ScDPOutLevelData
{
	long								nDim;
	long								nHier;
	long								nLevel;
	long								nDimPos;
	uno::Sequence<sheet::MemberResult>	aResult;
	String								aCaption;

	ScDPOutLevelData() { nDim = nHier = nLevel = nDimPos = -1; }

	BOOL operator<(const ScDPOutLevelData& r) const
		{ return nDimPos<r.nDimPos || ( nDimPos==r.nDimPos && nHier<r.nHier ) ||
			( nDimPos==r.nDimPos && nHier==r.nHier && nLevel<r.nLevel ); }

	void Swap(ScDPOutLevelData& r)
//!		{ ScDPOutLevelData aTemp = r; r = *this; *this = aTemp; }
		{ ScDPOutLevelData aTemp; aTemp = r; r = *this; *this = aTemp; }

	//!	bug (73840) in uno::Sequence - copy and then assign doesn't work!
};

// -----------------------------------------------------------------------

void lcl_SetStyleById( ScDocument* pDoc, SCTAB nTab,
					SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
					USHORT nStrId )
{
	if ( nCol1 > nCol2 || nRow1 > nRow2 )
	{
		DBG_ERROR("SetStyleById: invalid range");
		return;
	}

	String aStyleName = ScGlobal::GetRscString( nStrId );
	ScStyleSheetPool* pStlPool = pDoc->GetStyleSheetPool();
	ScStyleSheet* pStyle = (ScStyleSheet*) pStlPool->Find( aStyleName, SFX_STYLE_FAMILY_PARA );
	if (!pStyle)
	{
		//	create new style (was in ScPivot::SetStyle)

		pStyle = (ScStyleSheet*) &pStlPool->Make( aStyleName, SFX_STYLE_FAMILY_PARA,
													SFXSTYLEBIT_USERDEF );
		pStyle->SetParent( ScGlobal::GetRscString(STR_STYLENAME_STANDARD) );
		SfxItemSet& rSet = pStyle->GetItemSet();
		if ( nStrId==STR_PIVOT_STYLE_RESULT || nStrId==STR_PIVOT_STYLE_TITLE )
            rSet.Put( SvxWeightItem( WEIGHT_BOLD, ATTR_FONT_WEIGHT ) );
		if ( nStrId==STR_PIVOT_STYLE_CATEGORY || nStrId==STR_PIVOT_STYLE_TITLE )
            rSet.Put( SvxHorJustifyItem( SVX_HOR_JUSTIFY_LEFT, ATTR_HOR_JUSTIFY ) );
	}

	pDoc->ApplyStyleAreaTab( nCol1, nRow1, nCol2, nRow2, nTab, *pStyle );
}

void lcl_SetFrame( ScDocument* pDoc, SCTAB nTab,
					SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
					USHORT nWidth )
{
	SvxBorderLine aLine;
	aLine.SetOutWidth(nWidth);
    SvxBoxItem aBox( ATTR_BORDER );
	aBox.SetLine(&aLine, BOX_LINE_LEFT);
	aBox.SetLine(&aLine, BOX_LINE_TOP);
	aBox.SetLine(&aLine, BOX_LINE_RIGHT);
	aBox.SetLine(&aLine, BOX_LINE_BOTTOM);
    SvxBoxInfoItem aBoxInfo( ATTR_BORDER_INNER );
	aBoxInfo.SetValid(VALID_HORI,FALSE);
	aBoxInfo.SetValid(VALID_VERT,FALSE);
	aBoxInfo.SetValid(VALID_DISTANCE,FALSE);

	pDoc->ApplyFrameAreaTab( ScRange( nCol1, nRow1, nTab, nCol2, nRow2, nTab ), &aBox, &aBoxInfo );
}

// -----------------------------------------------------------------------

void lcl_FillNumberFormats( UINT32*& rFormats, long& rCount,
							const uno::Reference<sheet::XDataPilotMemberResults>& xLevRes,
							const uno::Reference<container::XIndexAccess>& xDims )
{
	if ( rFormats )
		return;							// already set

	//	xLevRes is from the data layout dimension
	//!	use result sequence from ScDPOutLevelData!

	uno::Sequence<sheet::MemberResult> aResult = xLevRes->getResults();

	long nSize = aResult.getLength();
	if (nSize)
	{
		//	get names/formats for all data dimensions
		//!	merge this with the loop to collect ScDPOutLevelData?

		String aDataNames[SC_DPOUT_MAXLEVELS];
		UINT32 nDataFormats[SC_DPOUT_MAXLEVELS];
		long nDataCount = 0;
		BOOL bAnySet = FALSE;

		long nDimCount = xDims->getCount();
		for (long nDim=0; nDim<nDimCount; nDim++)
		{
			uno::Reference<uno::XInterface> xDim =
					ScUnoHelpFunctions::AnyToInterface( xDims->getByIndex(nDim) );
			uno::Reference<beans::XPropertySet> xDimProp( xDim, uno::UNO_QUERY );
			uno::Reference<container::XNamed> xDimName( xDim, uno::UNO_QUERY );
			if ( xDimProp.is() && xDimName.is() )
			{
				sheet::DataPilotFieldOrientation eDimOrient =
					(sheet::DataPilotFieldOrientation) ScUnoHelpFunctions::GetEnumProperty(
						xDimProp, rtl::OUString::createFromAscii(DP_PROP_ORIENTATION),
						sheet::DataPilotFieldOrientation_HIDDEN );
				if ( eDimOrient == sheet::DataPilotFieldOrientation_DATA )
				{
					aDataNames[nDataCount] = String( xDimName->getName() );
					long nFormat = ScUnoHelpFunctions::GetLongProperty(
											xDimProp,
											rtl::OUString::createFromAscii(DP_PROP_NUMBERFORMAT) );
					nDataFormats[nDataCount] = nFormat;
					if ( nFormat != 0 )
						bAnySet = TRUE;
					++nDataCount;
				}
			}
		}

		if ( bAnySet )		// forget everything if all formats are 0 (or no data dimensions)
		{
			const sheet::MemberResult* pArray = aResult.getConstArray();

			String aName;
			UINT32* pNumFmt = new UINT32[nSize];
			if (nDataCount == 1)
			{
				//	only one data dimension -> use its numberformat everywhere
				long nFormat = nDataFormats[0];
				for (long nPos=0; nPos<nSize; nPos++)
					pNumFmt[nPos] = nFormat;
			}
			else
			{
				for (long nPos=0; nPos<nSize; nPos++)
				{
					//	if CONTINUE bit is set, keep previous name
					//!	keep number format instead!
					if ( !(pArray[nPos].Flags & sheet::MemberResultFlags::CONTINUE) )
						aName = String( pArray[nPos].Name );

					UINT32 nFormat = 0;
					for (long i=0; i<nDataCount; i++)
						if (aName == aDataNames[i])			//!	search more efficiently?
						{
							nFormat = nDataFormats[i];
							break;
						}
					pNumFmt[nPos] = nFormat;
				}
			}

			rFormats = pNumFmt;
			rCount = nSize;
		}
	}
}

UINT32 lcl_GetFirstNumberFormat( const uno::Reference<container::XIndexAccess>& xDims )
{
    long nDimCount = xDims->getCount();
    for (long nDim=0; nDim<nDimCount; nDim++)
    {
        uno::Reference<uno::XInterface> xDim =
                ScUnoHelpFunctions::AnyToInterface( xDims->getByIndex(nDim) );
        uno::Reference<beans::XPropertySet> xDimProp( xDim, uno::UNO_QUERY );
        if ( xDimProp.is() )
        {
            sheet::DataPilotFieldOrientation eDimOrient =
                (sheet::DataPilotFieldOrientation) ScUnoHelpFunctions::GetEnumProperty(
                    xDimProp, rtl::OUString::createFromAscii(DP_PROP_ORIENTATION),
                    sheet::DataPilotFieldOrientation_HIDDEN );
            if ( eDimOrient == sheet::DataPilotFieldOrientation_DATA )
            {
                long nFormat = ScUnoHelpFunctions::GetLongProperty(
                                        xDimProp,
                                        rtl::OUString::createFromAscii(DP_PROP_NUMBERFORMAT) );

                return nFormat;     // use format from first found data dimension
            }
        }
    }

    return 0;       // none found
}

void lcl_SortFields( ScDPOutLevelData* pFields, long nFieldCount )
{
	for (long i=0; i+1<nFieldCount; i++)
	{
		for (long j=0; j+i+1<nFieldCount; j++)
			if ( pFields[j+1] < pFields[j] )
				pFields[j].Swap( pFields[j+1] );
	}
}

BOOL lcl_MemberEmpty( const uno::Sequence<sheet::MemberResult>& rSeq )
{
	//	used to skip levels that have no members

	long nLen = rSeq.getLength();
	const sheet::MemberResult* pArray = rSeq.getConstArray();
	for (long i=0; i<nLen; i++)
		if (pArray[i].Flags & sheet::MemberResultFlags::HASMEMBER)
			return FALSE;

	return TRUE;	// no member data -> empty
}

uno::Sequence<sheet::MemberResult> lcl_GetSelectedPageAsResult( const uno::Reference<beans::XPropertySet>& xDimProp )
{
	uno::Sequence<sheet::MemberResult> aRet;
	if ( xDimProp.is() )
	{
		try
		{
			//! merge with ScDPDimension::setPropertyValue?

			uno::Any aValue = xDimProp->getPropertyValue( rtl::OUString::createFromAscii(DP_PROP_FILTER) );

			uno::Sequence<sheet::TableFilterField> aSeq;
			if (aValue >>= aSeq)
			{
				if ( aSeq.getLength() == 1 )
				{
					const sheet::TableFilterField& rField = aSeq[0];
					if ( rField.Field == 0 && rField.Operator == sheet::FilterOperator_EQUAL && !rField.IsNumeric )
					{
						rtl::OUString aSelectedPage( rField.StringValue );
						//!	different name/caption string?
						sheet::MemberResult aResult( aSelectedPage, aSelectedPage, 0 );
						aRet = uno::Sequence<sheet::MemberResult>( &aResult, 1 );
					}
				}
				// else return empty sequence
			}
		}
		catch ( uno::Exception& )
		{
			// recent addition - allow source to not handle it (no error)
		}
	}
	return aRet;
}

ScDPOutput::ScDPOutput( ScDocument* pD, const uno::Reference<sheet::XDimensionsSupplier>& xSrc,
								const ScAddress& rPos, BOOL bFilter ) :
	pDoc( pD ),
	xSource( xSrc ),
	aStartPos( rPos ),
	bDoFilter( bFilter ),
	bResultsError( FALSE ),
	pColNumFmt( NULL ),
	pRowNumFmt( NULL ),
	nColFmtCount( 0 ),
	nRowFmtCount( 0 ),
    nSingleNumFmt( 0 ),
	bSizesValid( FALSE ),
	bSizeOverflow( FALSE )
{
	nTabStartCol = nMemberStartCol = nDataStartCol = nTabEndCol = 0;
	nTabStartRow = nMemberStartRow = nDataStartRow = nTabEndRow = 0;

	pColFields	= new ScDPOutLevelData[SC_DPOUT_MAXLEVELS];
	pRowFields	= new ScDPOutLevelData[SC_DPOUT_MAXLEVELS];
	pPageFields	= new ScDPOutLevelData[SC_DPOUT_MAXLEVELS];
	nColFieldCount = 0;
	nRowFieldCount = 0;
	nPageFieldCount = 0;

	uno::Reference<sheet::XDataPilotResults> xResult( xSource, uno::UNO_QUERY );
	if ( xSource.is() && xResult.is() )
	{
		//	get dimension results:

		uno::Reference<container::XIndexAccess> xDims =
				new ScNameToIndexAccess( xSource->getDimensions() );
		long nDimCount = xDims->getCount();
		for (long nDim=0; nDim<nDimCount; nDim++)
		{
			uno::Reference<uno::XInterface> xDim =
					ScUnoHelpFunctions::AnyToInterface( xDims->getByIndex(nDim) );
			uno::Reference<beans::XPropertySet> xDimProp( xDim, uno::UNO_QUERY );
			uno::Reference<sheet::XHierarchiesSupplier> xDimSupp( xDim, uno::UNO_QUERY );
			if ( xDimProp.is() && xDimSupp.is() )
			{
				sheet::DataPilotFieldOrientation eDimOrient =
					(sheet::DataPilotFieldOrientation) ScUnoHelpFunctions::GetEnumProperty(
						xDimProp, rtl::OUString::createFromAscii(DP_PROP_ORIENTATION),
						sheet::DataPilotFieldOrientation_HIDDEN );
				long nDimPos = ScUnoHelpFunctions::GetLongProperty( xDimProp,
						rtl::OUString::createFromAscii(DP_PROP_POSITION) );
				BOOL bIsDataLayout = ScUnoHelpFunctions::GetBoolProperty(
												xDimProp,
												rtl::OUString::createFromAscii(DP_PROP_ISDATALAYOUT) );

				if ( eDimOrient != sheet::DataPilotFieldOrientation_HIDDEN )
				{
					uno::Reference<container::XIndexAccess> xHiers =
							new ScNameToIndexAccess( xDimSupp->getHierarchies() );
					long nHierarchy = ScUnoHelpFunctions::GetLongProperty(
											xDimProp,
											rtl::OUString::createFromAscii(DP_PROP_USEDHIERARCHY) );
					if ( nHierarchy >= xHiers->getCount() )
						nHierarchy = 0;

					uno::Reference<uno::XInterface> xHier =
							ScUnoHelpFunctions::AnyToInterface(
												xHiers->getByIndex(nHierarchy) );
					uno::Reference<sheet::XLevelsSupplier> xHierSupp( xHier, uno::UNO_QUERY );
					if ( xHierSupp.is() )
					{
						uno::Reference<container::XIndexAccess> xLevels =
								new ScNameToIndexAccess( xHierSupp->getLevels() );
						long nLevCount = xLevels->getCount();
						for (long nLev=0; nLev<nLevCount; nLev++)
						{
							uno::Reference<uno::XInterface> xLevel =
										ScUnoHelpFunctions::AnyToInterface(
															xLevels->getByIndex(nLev) );
							uno::Reference<container::XNamed> xLevNam( xLevel, uno::UNO_QUERY );
							uno::Reference<sheet::XDataPilotMemberResults> xLevRes(
									xLevel, uno::UNO_QUERY );
							if ( xLevNam.is() && xLevRes.is() )
							{
								String aCaption = String(xLevNam->getName());	//! Caption...
								switch ( eDimOrient )
								{
									case sheet::DataPilotFieldOrientation_COLUMN:
										pColFields[nColFieldCount].nDim    = nDim;
										pColFields[nColFieldCount].nHier   = nHierarchy;
										pColFields[nColFieldCount].nLevel  = nLev;
										pColFields[nColFieldCount].nDimPos = nDimPos;
										pColFields[nColFieldCount].aResult = xLevRes->getResults();
										pColFields[nColFieldCount].aCaption= aCaption;
										if (!lcl_MemberEmpty(pColFields[nColFieldCount].aResult))
											++nColFieldCount;
										break;
									case sheet::DataPilotFieldOrientation_ROW:
										pRowFields[nRowFieldCount].nDim    = nDim;
										pRowFields[nRowFieldCount].nHier   = nHierarchy;
										pRowFields[nRowFieldCount].nLevel  = nLev;
										pRowFields[nRowFieldCount].nDimPos = nDimPos;
										pRowFields[nRowFieldCount].aResult = xLevRes->getResults();
										pRowFields[nRowFieldCount].aCaption= aCaption;
										if (!lcl_MemberEmpty(pRowFields[nRowFieldCount].aResult))
											++nRowFieldCount;
										break;
									case sheet::DataPilotFieldOrientation_PAGE:
										pPageFields[nPageFieldCount].nDim    = nDim;
										pPageFields[nPageFieldCount].nHier   = nHierarchy;
										pPageFields[nPageFieldCount].nLevel  = nLev;
										pPageFields[nPageFieldCount].nDimPos = nDimPos;
										pPageFields[nPageFieldCount].aResult = lcl_GetSelectedPageAsResult(xDimProp);
										pPageFields[nPageFieldCount].aCaption= aCaption;
										// no check on results for page fields
										++nPageFieldCount;
										break;
                                    default:
                                    {
                                        // added to avoid warnings
                                    }
								}

								// get number formats from data dimensions
								if ( bIsDataLayout )
								{
									DBG_ASSERT( nLevCount == 1, "data layout: multiple levels?" );
									if ( eDimOrient == sheet::DataPilotFieldOrientation_COLUMN )
										lcl_FillNumberFormats( pColNumFmt, nColFmtCount, xLevRes, xDims );
									else if ( eDimOrient == sheet::DataPilotFieldOrientation_ROW )
										lcl_FillNumberFormats( pRowNumFmt, nRowFmtCount, xLevRes, xDims );
								}
							}
						}
					}
				}
				else if ( bIsDataLayout )
				{
				    // data layout dimension is hidden (allowed if there is only one data dimension)
				    // -> use the number format from the first data dimension for all results

				    nSingleNumFmt = lcl_GetFirstNumberFormat( xDims );
				}
			}
		}
		lcl_SortFields( pColFields, nColFieldCount );
		lcl_SortFields( pRowFields, nRowFieldCount );
		lcl_SortFields( pPageFields, nPageFieldCount );

		//	get data results:

		try
		{
			aData = xResult->getResults();
		}
		catch (uno::RuntimeException&)
		{
			bResultsError = TRUE;
		}
	}

	// get "DataDescription" property (may be missing in external sources)

	uno::Reference<beans::XPropertySet> xSrcProp( xSource, uno::UNO_QUERY );
	if ( xSrcProp.is() )
	{
		try
		{
			uno::Any aAny = xSrcProp->getPropertyValue(
					rtl::OUString::createFromAscii(DP_PROP_DATADESCR) );
			rtl::OUString aUStr;
			aAny >>= aUStr;
			aDataDescription = String( aUStr );
		}
		catch(uno::Exception&)
		{
		}
	}
}

ScDPOutput::~ScDPOutput()
{
	delete[] pColFields;
	delete[] pRowFields;
	delete[] pPageFields;

	delete[] pColNumFmt;
	delete[] pRowNumFmt;
}

void ScDPOutput::SetPosition( const ScAddress& rPos )
{
	aStartPos = rPos;
	bSizesValid = bSizeOverflow = FALSE;
}

void ScDPOutput::DataCell( SCCOL nCol, SCROW nRow, SCTAB nTab, const sheet::DataResult& rData )
{
	long nFlags = rData.Flags;
	if ( nFlags & sheet::DataResultFlags::ERROR )
	{
		pDoc->SetError( nCol, nRow, nTab, errNoValue );
	}
	else if ( nFlags & sheet::DataResultFlags::HASDATA )
	{
		pDoc->SetValue( nCol, nRow, nTab, rData.Value );

		//	use number formats from source

		DBG_ASSERT( bSizesValid, "DataCell: !bSizesValid" );
		UINT32 nFormat = 0;
		if ( pColNumFmt )
		{
			if ( nCol >= nDataStartCol )
			{
				long nIndex = nCol - nDataStartCol;
				if ( nIndex < nColFmtCount )
					nFormat = pColNumFmt[nIndex];
			}
		}
		else if ( pRowNumFmt )
		{
			if ( nRow >= nDataStartRow )
			{
				long nIndex = nRow - nDataStartRow;
				if ( nIndex < nRowFmtCount )
					nFormat = pRowNumFmt[nIndex];
			}
		}
        else if ( nSingleNumFmt != 0 )
            nFormat = nSingleNumFmt;        // single format is used everywhere
		if ( nFormat != 0 )
			pDoc->ApplyAttr( nCol, nRow, nTab, SfxUInt32Item( ATTR_VALUE_FORMAT, nFormat ) );
	}
	else
	{
		//pDoc->SetString( nCol, nRow, nTab, EMPTY_STRING );
	}

	//	SubTotal formatting is controlled by headers
}

void ScDPOutput::HeaderCell( SCCOL nCol, SCROW nRow, SCTAB nTab,
								const sheet::MemberResult& rData, BOOL bColHeader, long nLevel )
{
	long nFlags = rData.Flags;
	if ( nFlags & sheet::MemberResultFlags::HASMEMBER )
	{
		pDoc->SetString( nCol, nRow, nTab, rData.Caption );
	}
	else
	{
		//pDoc->SetString( nCol, nRow, nTab, EMPTY_STRING );
	}

	if ( nFlags & sheet::MemberResultFlags::SUBTOTAL )
	{
//		SvxWeightItem aItem( WEIGHT_BOLD );		// weight is in the style

		//!	limit frames to horizontal or vertical?
		if (bColHeader)
		{
			lcl_SetFrame( pDoc,nTab, nCol,nMemberStartRow+(SCROW)nLevel, nCol,nTabEndRow, 20 );
			lcl_SetStyleById( pDoc,nTab, nCol,nMemberStartRow+(SCROW)nLevel, nCol,nDataStartRow-1,
									STR_PIVOT_STYLE_TITLE );
			lcl_SetStyleById( pDoc,nTab, nCol,nDataStartRow, nCol,nTabEndRow,
									STR_PIVOT_STYLE_RESULT );
		}
		else
		{
			lcl_SetFrame( pDoc,nTab, nMemberStartCol+(SCCOL)nLevel,nRow, nTabEndCol,nRow, 20 );
			lcl_SetStyleById( pDoc,nTab, nMemberStartCol+(SCCOL)nLevel,nRow, nDataStartCol-1,nRow,
									STR_PIVOT_STYLE_TITLE );
			lcl_SetStyleById( pDoc,nTab, nDataStartCol,nRow, nTabEndCol,nRow,
									STR_PIVOT_STYLE_RESULT );
		}
	}
}

void ScDPOutput::FieldCell( SCCOL nCol, SCROW nRow, SCTAB nTab, const String& rCaption, BOOL bFrame )
{
	pDoc->SetString( nCol, nRow, nTab, rCaption );
	if (bFrame)
		lcl_SetFrame( pDoc,nTab, nCol,nRow, nCol,nRow, 20 );

	//	Button
	pDoc->ApplyAttr( nCol, nRow, nTab, ScMergeFlagAttr(SC_MF_BUTTON) );

	lcl_SetStyleById( pDoc,nTab, nCol,nRow, nCol,nRow, STR_PIVOT_STYLE_FIELDNAME );
}

void lcl_DoFilterButton( ScDocument* pDoc, SCCOL nCol, SCROW nRow, SCTAB nTab )
{
	pDoc->SetString( nCol, nRow, nTab, ScGlobal::GetRscString(STR_CELL_FILTER) );
	pDoc->ApplyAttr( nCol, nRow, nTab, ScMergeFlagAttr(SC_MF_BUTTON) );
}

void ScDPOutput::CalcSizes()
{
	if (!bSizesValid)
	{
		//	get column size of data from first row
		//!	allow different sizes (and clear following areas) ???

		nRowCount = aData.getLength();
		const uno::Sequence<sheet::DataResult>* pRowAry = aData.getConstArray();
		nColCount = nRowCount ? ( pRowAry[0].getLength() ) : 0;
		nHeaderSize = 1;			// one row for field names

		//	calculate output positions and sizes

		long nPageSize = 0;		//! use page fields!
		if ( bDoFilter || nPageFieldCount )
		{
			nPageSize += nPageFieldCount + 1;	// plus one empty row
			if ( bDoFilter )
				++nPageSize;		//	filter button above the page fields
		}

		if ( aStartPos.Col() + nRowFieldCount + nColCount - 1 > MAXCOL ||
			 aStartPos.Row() + nPageSize + nHeaderSize + nColFieldCount + nRowCount > MAXROW )
		{
			bSizeOverflow = TRUE;
		}

		nTabStartCol = aStartPos.Col();
		nTabStartRow = aStartPos.Row() + (SCROW)nPageSize;			// below page fields
		nMemberStartCol = nTabStartCol;
		nMemberStartRow = nTabStartRow + (SCROW) nHeaderSize;
		nDataStartCol = nMemberStartCol + (SCCOL)nRowFieldCount;
		nDataStartRow = nMemberStartRow + (SCROW)nColFieldCount;
		if ( nColCount > 0 )
			nTabEndCol = nDataStartCol + (SCCOL)nColCount - 1;
		else
			nTabEndCol = nDataStartCol;		// single column will remain empty
		// if page fields are involved, include the page selection cells
		if ( nPageFieldCount > 0 && nTabEndCol < nTabStartCol + 1 )
			nTabEndCol = nTabStartCol + 1;
		if ( nRowCount > 0 )
			nTabEndRow = nDataStartRow + (SCROW)nRowCount - 1;
		else
			nTabEndRow = nDataStartRow;		// single row will remain empty
		bSizesValid = TRUE;
	}
}

sal_Int32 ScDPOutput::GetPositionType(const ScAddress& rPos)
{
    using namespace ::com::sun::star::sheet;

    SCCOL nCol = rPos.Col();
    SCROW nRow = rPos.Row();
    SCTAB nTab = rPos.Tab();
    if ( nTab != aStartPos.Tab() )
        return DataPilotTablePositionType::NOT_IN_TABLE;

    CalcSizes();

    // Make sure the cursor is within the table.
    if (nCol < nTabStartCol || nRow < nTabStartRow || nCol > nTabEndCol || nRow > nTabEndRow)
        return DataPilotTablePositionType::NOT_IN_TABLE;

    // test for result data area.
    if (nCol >= nDataStartCol && nCol <= nTabEndCol && nRow >= nDataStartRow && nRow <= nTabEndRow)
        return DataPilotTablePositionType::RESULT;

    bool bInColHeader = (nRow >= nTabStartRow && nRow < nDataStartRow);
    bool bInRowHeader = (nCol >= nTabStartCol && nCol < nDataStartCol);

    if (bInColHeader && bInRowHeader)
        // probably in that ugly little box at the upper-left corner of the table.
        return DataPilotTablePositionType::OTHER;

    if (bInColHeader)
    {
        if (nRow == nTabStartRow)
            // first row in the column header area is always used for column 
            // field buttons.
            return DataPilotTablePositionType::OTHER;

        return DataPilotTablePositionType::COLUMN_HEADER;
    }

    if (bInRowHeader)
        return DataPilotTablePositionType::ROW_HEADER;

    return DataPilotTablePositionType::OTHER;
}

void ScDPOutput::Output()
{
	long nField;
	SCTAB nTab = aStartPos.Tab();
	const uno::Sequence<sheet::DataResult>* pRowAry = aData.getConstArray();

	//	calculate output positions and sizes

	CalcSizes();
	if ( bSizeOverflow || bResultsError )	// does output area exceed sheet limits?
		return;								// nothing

	//	clear whole (new) output area
	//!	when modifying table, clear old area
	//!	include IDF_OBJECTS ???
	pDoc->DeleteAreaTab( aStartPos.Col(), aStartPos.Row(), nTabEndCol, nTabEndRow, nTab, IDF_ALL );

	if ( bDoFilter )
		lcl_DoFilterButton( pDoc, aStartPos.Col(), aStartPos.Row(), nTab );

	//	output page fields:

	for (nField=0; nField<nPageFieldCount; nField++)
	{
		SCCOL nHdrCol = aStartPos.Col();
		SCROW nHdrRow = aStartPos.Row() + nField + ( bDoFilter ? 1 : 0 );
		// draw without frame for consistency with filter button:
		FieldCell( nHdrCol, nHdrRow, nTab, pPageFields[nField].aCaption, FALSE );
		SCCOL nFldCol = nHdrCol + 1;

		String aPageValue;
		if ( pPageFields[nField].aResult.getLength() == 1 )
			aPageValue = pPageFields[nField].aResult[0].Caption;
		else
			aPageValue = String( ScResId( SCSTR_ALL ) );		//! separate string?

		pDoc->SetString( nFldCol, nHdrRow, nTab, aPageValue );

		lcl_SetFrame( pDoc,nTab, nFldCol,nHdrRow, nFldCol,nHdrRow, 20 );
		pDoc->ApplyAttr( nFldCol, nHdrRow, nTab, ScMergeFlagAttr(SC_MF_AUTO) );
		//!	which style?
	}

	//	data description
	//	(may get overwritten by first row field)

	String aDesc = aDataDescription;
	if ( !aDesc.Len() )
	{
		//!	use default string ("result") ?
	}
	pDoc->SetString( nTabStartCol, nTabStartRow, nTab, aDesc );

	//	set STR_PIVOT_STYLE_INNER for whole data area (subtotals are overwritten)

	if ( nDataStartRow > nTabStartRow )
		lcl_SetStyleById( pDoc, nTab, nTabStartCol, nTabStartRow, nTabEndCol, nDataStartRow-1,
							STR_PIVOT_STYLE_TOP );
	lcl_SetStyleById( pDoc, nTab, nDataStartCol, nDataStartRow, nTabEndCol, nTabEndRow,
						STR_PIVOT_STYLE_INNER );

	//	output column headers:

	for (nField=0; nField<nColFieldCount; nField++)
	{
		SCCOL nHdrCol = nDataStartCol + (SCCOL)nField;				//! check for overflow
		FieldCell( nHdrCol, nTabStartRow, nTab, pColFields[nField].aCaption );

		SCROW nRowPos = nMemberStartRow + (SCROW)nField;				//! check for overflow
		const uno::Sequence<sheet::MemberResult> rSequence = pColFields[nField].aResult;
		const sheet::MemberResult* pArray = rSequence.getConstArray();
		long nThisColCount = rSequence.getLength();
		DBG_ASSERT( nThisColCount == nColCount, "count mismatch" );		//! ???
		for (long nCol=0; nCol<nThisColCount; nCol++)
		{
			SCCOL nColPos = nDataStartCol + (SCCOL)nCol;				//! check for overflow
			HeaderCell( nColPos, nRowPos, nTab, pArray[nCol], TRUE, nField );
			if ( ( pArray[nCol].Flags & sheet::MemberResultFlags::HASMEMBER ) &&
				!( pArray[nCol].Flags & sheet::MemberResultFlags::SUBTOTAL ) )
			{
				if ( nField+1 < nColFieldCount )
				{
					long nEnd = nCol;
					while ( nEnd+1 < nThisColCount && ( pArray[nEnd+1].Flags & sheet::MemberResultFlags::CONTINUE ) )
						++nEnd;
					SCCOL nEndColPos = nDataStartCol + (SCCOL)nEnd;		//! check for overflow
					lcl_SetFrame( pDoc,nTab, nColPos,nRowPos, nEndColPos,nRowPos, 20 );
					lcl_SetFrame( pDoc,nTab, nColPos,nRowPos, nEndColPos,nTabEndRow, 20 );

					lcl_SetStyleById( pDoc, nTab, nColPos,nRowPos, nEndColPos,nDataStartRow-1, STR_PIVOT_STYLE_CATEGORY );
				}
				else
					lcl_SetStyleById( pDoc, nTab, nColPos,nRowPos, nColPos,nDataStartRow-1, STR_PIVOT_STYLE_CATEGORY );
			}
		}
	}

	//	output row headers:

	for (nField=0; nField<nRowFieldCount; nField++)
	{
		SCCOL nHdrCol = nTabStartCol + (SCCOL)nField;					//! check for overflow
		SCROW nHdrRow = nDataStartRow - 1;
		FieldCell( nHdrCol, nHdrRow, nTab, pRowFields[nField].aCaption );

		SCCOL nColPos = nMemberStartCol + (SCCOL)nField;				//! check for overflow
		const uno::Sequence<sheet::MemberResult> rSequence = pRowFields[nField].aResult;
		const sheet::MemberResult* pArray = rSequence.getConstArray();
		long nThisRowCount = rSequence.getLength();
		DBG_ASSERT( nThisRowCount == nRowCount, "count mismatch" );		//! ???
		for (long nRow=0; nRow<nThisRowCount; nRow++)
		{
			SCROW nRowPos = nDataStartRow + (SCROW)nRow;				//! check for overflow
			HeaderCell( nColPos, nRowPos, nTab, pArray[nRow], FALSE, nField );
			if ( ( pArray[nRow].Flags & sheet::MemberResultFlags::HASMEMBER ) &&
				!( pArray[nRow].Flags & sheet::MemberResultFlags::SUBTOTAL ) )
			{
				if ( nField+1 < nRowFieldCount )
				{
					long nEnd = nRow;
					while ( nEnd+1 < nThisRowCount && ( pArray[nEnd+1].Flags & sheet::MemberResultFlags::CONTINUE ) )
						++nEnd;
					SCROW nEndRowPos = nDataStartRow + (SCROW)nEnd;		//! check for overflow
					lcl_SetFrame( pDoc,nTab, nColPos,nRowPos, nColPos,nEndRowPos, 20 );
					lcl_SetFrame( pDoc,nTab, nColPos,nRowPos, nTabEndCol,nEndRowPos, 20 );

					lcl_SetStyleById( pDoc, nTab, nColPos,nRowPos, nDataStartCol-1,nEndRowPos, STR_PIVOT_STYLE_CATEGORY );
				}
				else
					lcl_SetStyleById( pDoc, nTab, nColPos,nRowPos, nDataStartCol-1,nRowPos, STR_PIVOT_STYLE_CATEGORY );
			}
		}
	}

	//	output data results:

	for (long nRow=0; nRow<nRowCount; nRow++)
	{
		SCROW nRowPos = nDataStartRow + (SCROW)nRow;					//! check for overflow
		const sheet::DataResult* pColAry = pRowAry[nRow].getConstArray();
		long nThisColCount = pRowAry[nRow].getLength();
		DBG_ASSERT( nThisColCount == nColCount, "count mismatch" );		//! ???
		for (long nCol=0; nCol<nThisColCount; nCol++)
		{
			SCCOL nColPos = nDataStartCol + (SCCOL)nCol;				//! check for overflow
			DataCell( nColPos, nRowPos, nTab, pColAry[nCol] );
		}
	}

	//	frame around the whole table

	lcl_SetFrame( pDoc,nTab, nDataStartCol,nDataStartRow, nTabEndCol,nTabEndRow, 20 );
	if ( nDataStartCol > nMemberStartCol )
		lcl_SetFrame( pDoc,nTab, nMemberStartCol,nDataStartRow, nDataStartCol-1,nTabEndRow, 20 );
	if ( nDataStartRow > nMemberStartRow )
		lcl_SetFrame( pDoc,nTab, nDataStartCol,nMemberStartRow, nTabEndCol,nDataStartRow-1, 20 );

	lcl_SetFrame( pDoc,nTab, nTabStartCol,nTabStartRow, nTabEndCol,nTabEndRow, 40 );
}

ScRange ScDPOutput::GetOutputRange( sal_Int32 nRegionType )
{
    using namespace ::com::sun::star::sheet;

    CalcSizes();

//  fprintf(stdout, "ScDPOutput::GetOutputRange: aStartPos = (%ld, %d)\n", aStartPos.Row(), aStartPos.Col());fflush(stdout);
//  fprintf(stdout, "ScDPOutput::GetOutputRange: nTabStart (Row = %ld, Col = %ld)\n", nTabStartRow, nTabStartCol);fflush(stdout);
//  fprintf(stdout, "ScDPOutput::GetOutputRange: nMemberStart (Row = %ld, Col = %ld)\n", nMemberStartRow, nMemberStartCol);fflush(stdout);
//  fprintf(stdout, "ScDPOutput::GetOutputRange: nDataStart (Row = %ld, Col = %ld)\n", nDataStartRow, nDataStartCol);fflush(stdout);
//  fprintf(stdout, "ScDPOutput::GetOutputRange: nTabEnd (Row = %ld, Col = %ld)\n", nTabEndRow, nTabStartCol);fflush(stdout);

    SCTAB nTab = aStartPos.Tab();
    switch (nRegionType)
    {
        case DataPilotOutputRangeType::RESULT:
            return ScRange(nDataStartCol, nDataStartRow, nTab, nTabEndCol, nTabEndRow, nTab);
        case DataPilotOutputRangeType::TABLE:
            return ScRange(aStartPos.Col(), nTabStartRow, nTab, nTabEndCol, nTabEndRow, nTab);
        default:
            DBG_ASSERT(nRegionType == DataPilotOutputRangeType::WHOLE, "ScDPOutput::GetOutputRange: unknown region type");
        break;
    }
    return ScRange(aStartPos.Col(), aStartPos.Row(), nTab, nTabEndCol, nTabEndRow, nTab);
}

BOOL ScDPOutput::HasError()
{
	CalcSizes();

	return bSizeOverflow || bResultsError;
}

long ScDPOutput::GetHeaderRows()
{
	return nPageFieldCount + ( bDoFilter ? 1 : 0 );
}

void ScDPOutput::GetMemberResultNames( ScStrCollection& rNames, long nDimension )
{
    //  Return the list of all member names in a dimension's MemberResults.
    //  Only the dimension has to be compared because this is only used with table data,
    //  where each dimension occurs only once.

    uno::Sequence<sheet::MemberResult> aMemberResults;
    bool bFound = false;
    long nField;

    // look in column fields

    for (nField=0; nField<nColFieldCount && !bFound; nField++)
        if ( pColFields[nField].nDim == nDimension )
        {
            aMemberResults = pColFields[nField].aResult;
            bFound = true;
        }

    // look in row fields

    for (nField=0; nField<nRowFieldCount && !bFound; nField++)
        if ( pRowFields[nField].nDim == nDimension )
        {
            aMemberResults = pRowFields[nField].aResult;
            bFound = true;
        }

    // collect the member names

    if ( bFound )
    {
        const sheet::MemberResult* pArray = aMemberResults.getConstArray();
        long nResultCount = aMemberResults.getLength();

        for (long nItem=0; nItem<nResultCount; nItem++)
        {
            if ( pArray[nItem].Flags & sheet::MemberResultFlags::HASMEMBER )
            {
                StrData* pNew = new StrData( pArray[nItem].Name );
                if ( !rNames.Insert( pNew ) )
                    delete pNew;
            }
        }
    }
}


void ScDPOutput::GetPositionData(const ScAddress& rPos, DataPilotTablePositionData& rPosData)
{
    using namespace ::com::sun::star::sheet;

	SCCOL nCol = rPos.Col();
	SCROW nRow = rPos.Row();
	SCTAB nTab = rPos.Tab();
	if ( nTab != aStartPos.Tab() )
		return;										// wrong sheet

	//	calculate output positions and sizes

	CalcSizes();

    rPosData.PositionType = GetPositionType(rPos);
    switch (rPosData.PositionType)
    {
        case DataPilotTablePositionType::RESULT:
        {
            vector<DataPilotFieldFilter> aFilters;
            GetDataResultPositionData(aFilters, rPos);
            sal_Int32 nSize = aFilters.size();
    
            DataPilotTableResultData aResData;
            aResData.FieldFilters.realloc(nSize);
            for (sal_Int32 i = 0; i < nSize; ++i)
                aResData.FieldFilters[i] = aFilters[i];
    
            aResData.DataFieldIndex = 0;
            Reference<beans::XPropertySet> xPropSet(xSource, UNO_QUERY);
            if (xPropSet.is())
            {
                sal_Int32 nDataFieldCount = 0;
                Any any = xPropSet->getPropertyValue(rtl::OUString::createFromAscii("DataFieldCount"));
                if ((any >>= nDataFieldCount) && nDataFieldCount > 0)
                    aResData.DataFieldIndex = (nRow - nDataStartRow) % nDataFieldCount;
            }

            // Copy appropriate DataResult object from the cached sheet::DataResult table.
            if (aData.getLength() > nRow - nDataStartRow && 
                aData[nRow-nDataStartRow].getLength() > nCol-nDataStartCol)
                aResData.Result = aData[nRow-nDataStartRow][nCol-nDataStartCol];
    
            rPosData.PositionData = makeAny(aResData);
            return;
        }
        case DataPilotTablePositionType::COLUMN_HEADER:
        {
            long nField = nRow - nTabStartRow - 1; // 1st line is used for the buttons
            if (nField < 0)
                break;

            const uno::Sequence<sheet::MemberResult> rSequence = pColFields[nField].aResult;
            if (rSequence.getLength() == 0)
                break;
            const sheet::MemberResult* pArray = rSequence.getConstArray();

            long nItem = nCol - nDataStartCol;
            //  get origin of "continue" fields
            while (nItem > 0 && ( pArray[nItem].Flags & sheet::MemberResultFlags::CONTINUE) )
                --nItem;

            if (nItem < 0)
                break;

            DataPilotTableHeaderData aHeaderData;
            aHeaderData.MemberName = OUString(pArray[nItem].Name);
            aHeaderData.Flags = pArray[nItem].Flags;
            aHeaderData.Dimension = static_cast<sal_Int32>(pColFields[nField].nDim);
            aHeaderData.Hierarchy = static_cast<sal_Int32>(pColFields[nField].nHier);
            aHeaderData.Level     = static_cast<sal_Int32>(pColFields[nField].nLevel);

            rPosData.PositionData = makeAny(aHeaderData);
            return;
        }
        case DataPilotTablePositionType::ROW_HEADER:
        {
            long nField = nCol - nTabStartCol;
            if (nField < 0)
                break;

            const uno::Sequence<sheet::MemberResult> rSequence = pRowFields[nField].aResult;
            if (rSequence.getLength() == 0)
                break;
            const sheet::MemberResult* pArray = rSequence.getConstArray();

            long nItem = nRow - nDataStartRow;
            //	get origin of "continue" fields
            while ( nItem > 0 && (pArray[nItem].Flags & sheet::MemberResultFlags::CONTINUE) )
                --nItem;

            if (nItem < 0)
                break;

            DataPilotTableHeaderData aHeaderData;
            aHeaderData.MemberName = OUString(pArray[nItem].Name);
            aHeaderData.Flags = pArray[nItem].Flags;
            aHeaderData.Dimension = static_cast<sal_Int32>(pRowFields[nField].nDim);
            aHeaderData.Hierarchy = static_cast<sal_Int32>(pRowFields[nField].nHier);
            aHeaderData.Level     = static_cast<sal_Int32>(pRowFields[nField].nLevel);

            rPosData.PositionData = makeAny(aHeaderData);
            return;
        }
    }
}

bool ScDPOutput::GetDataResultPositionData(vector<sheet::DataPilotFieldFilter>& rFilters, const ScAddress& rPos)
{
    // Check to make sure there is at least one data field.
    Reference<beans::XPropertySet> xPropSet(xSource, UNO_QUERY);
    if (!xPropSet.is())
        return false;

    sal_Int32 nDataFieldCount = 0;
    Any any = xPropSet->getPropertyValue(rtl::OUString::createFromAscii("DataFieldCount"));
    if (!(any >>= nDataFieldCount) || nDataFieldCount == 0)
        // No data field is present in this datapilot table.
        return false;

    bool bColGrand = bool();
    any = xPropSet->getPropertyValue(rtl::OUString::createFromAscii(SC_UNO_COLGRAND));
    if (!(any >>= bColGrand))
        return false;

    bool bRowGrand = bool();
    any = xPropSet->getPropertyValue(rtl::OUString::createFromAscii(SC_UNO_ROWGRAND));
    if (!(any >>= bRowGrand))
        return false;

    SCCOL nCol = rPos.Col();
    SCROW nRow = rPos.Row();
    SCTAB nTab = rPos.Tab();
    if ( nTab != aStartPos.Tab() )
        return false;                                     // wrong sheet

    CalcSizes();

    // test for data area.
    if (nCol < nDataStartCol || nCol > nTabEndCol || nRow < nDataStartRow || nRow > nTabEndRow)
    {
        // Cell is outside the data field area.
        return false;
    }

    bool bFilterByCol = !(bColGrand && (nCol == nTabEndCol));
    bool bFilterByRow = !(bRowGrand && (nRow == nTabEndRow));

    // column fields
    for (SCCOL nColField = 0; nColField < nColFieldCount && bFilterByCol; ++nColField)
    {
        sheet::DataPilotFieldFilter filter;
        filter.FieldName = pColFields[nColField].aCaption;

        const uno::Sequence<sheet::MemberResult> rSequence = pColFields[nColField].aResult;
        const sheet::MemberResult* pArray = rSequence.getConstArray();

        DBG_ASSERT(nDataStartCol + rSequence.getLength() - 1 == nTabEndCol, "ScDPOutput::GetDataFieldCellData: error in geometric assumption");

        long nItem = nCol - nDataStartCol;
                //	get origin of "continue" fields
        while ( nItem > 0 && (pArray[nItem].Flags & sheet::MemberResultFlags::CONTINUE) )
            --nItem;

        filter.MatchValue = pArray[nItem].Name;
        rFilters.push_back(filter);
    }

    // row fields
    for (SCROW nRowField = 0; nRowField < nRowFieldCount && bFilterByRow; ++nRowField)
    {
        sheet::DataPilotFieldFilter filter;
        filter.FieldName = pRowFields[nRowField].aCaption;

        const uno::Sequence<sheet::MemberResult> rSequence = pRowFields[nRowField].aResult;
        const sheet::MemberResult* pArray = rSequence.getConstArray();

        DBG_ASSERT(nDataStartRow + rSequence.getLength() - 1 == nTabEndRow, "ScDPOutput::GetDataFieldCellData: error in geometric assumption");

        long nItem = nRow - nDataStartRow;
			//	get origin of "continue" fields
        while ( nItem > 0 && (pArray[nItem].Flags & sheet::MemberResultFlags::CONTINUE) )
            --nItem;

        filter.MatchValue = pArray[nItem].Name;
        rFilters.push_back(filter);
    }

    return true;
}

//
//  helper functions for ScDPOutput::GetPivotData
//

bool lcl_IsNamedDataField( const ScDPGetPivotDataField& rTarget, const String& rSourceName, const String& rGivenName )
{
    // match one of the names, ignoring case
    return ScGlobal::GetpTransliteration()->isEqual( rTarget.maFieldName, rSourceName ) ||
           ScGlobal::GetpTransliteration()->isEqual( rTarget.maFieldName, rGivenName );
}

bool lcl_IsNamedCategoryField( const ScDPGetPivotDataField& rFilter, const ScDPOutLevelData& rField )
{
    //! name from source instead of caption?
    return ScGlobal::GetpTransliteration()->isEqual( rFilter.maFieldName, rField.aCaption );
}

bool lcl_IsCondition( const sheet::MemberResult& rResultEntry, const ScDPGetPivotDataField& rFilter )
{
    //! handle numeric conditions?
    return ScGlobal::GetpTransliteration()->isEqual( rResultEntry.Name, rFilter.maValStr );
}

bool lcl_CheckPageField( const ScDPOutLevelData& rField,
                        const std::vector< ScDPGetPivotDataField >& rFilters,
                        std::vector< BOOL >& rFilterUsed )
{
    for (SCSIZE nFilterPos = 0; nFilterPos < rFilters.size(); ++nFilterPos)
    {
        if ( lcl_IsNamedCategoryField( rFilters[nFilterPos], rField ) )
        {
            rFilterUsed[nFilterPos] = TRUE;

            // page field result is empty or the selection as single entry (see lcl_GetSelectedPageAsResult)
            if ( rField.aResult.getLength() == 1 &&
                 lcl_IsCondition( rField.aResult[0], rFilters[nFilterPos] ) )
            {
                return true;        // condition matches page selection
            }
            else
            {
                return false;       // no page selection or different entry
            }
        }
    }

    return true;    // valid if the page field doesn't have a filter
}

uno::Sequence<sheet::GeneralFunction> lcl_GetSubTotals(
        const uno::Reference<sheet::XDimensionsSupplier>& xSource, const ScDPOutLevelData& rField )
{
    uno::Sequence<sheet::GeneralFunction> aSubTotals;

    uno::Reference<sheet::XHierarchiesSupplier> xHierSupp;
    uno::Reference<container::XNameAccess> xDimsName = xSource->getDimensions();
    uno::Reference<container::XIndexAccess> xIntDims = new ScNameToIndexAccess( xDimsName );
    sal_Int32 nIntCount = xIntDims->getCount();
    if ( rField.nDim < nIntCount )
    {
        uno::Reference<uno::XInterface> xIntDim = ScUnoHelpFunctions::AnyToInterface(
                                    xIntDims->getByIndex( rField.nDim ) );
        xHierSupp = uno::Reference<sheet::XHierarchiesSupplier>( xIntDim, uno::UNO_QUERY );
    }
    DBG_ASSERT( xHierSupp.is(), "dimension not found" );

    sal_Int32 nHierCount = 0;
    uno::Reference<container::XIndexAccess> xHiers;
    if ( xHierSupp.is() )
    {
        uno::Reference<container::XNameAccess> xHiersName = xHierSupp->getHierarchies();
        xHiers = new ScNameToIndexAccess( xHiersName );
        nHierCount = xHiers->getCount();
    }
    uno::Reference<uno::XInterface> xHier;
    if ( rField.nHier < nHierCount )
        xHier = ScUnoHelpFunctions::AnyToInterface( xHiers->getByIndex( rField.nHier ) );
    DBG_ASSERT( xHier.is(), "hierarchy not found" );

    sal_Int32 nLevCount = 0;
    uno::Reference<container::XIndexAccess> xLevels;
    uno::Reference<sheet::XLevelsSupplier> xLevSupp( xHier, uno::UNO_QUERY );
    if ( xLevSupp.is() )
    {
        uno::Reference<container::XNameAccess> xLevsName = xLevSupp->getLevels();
        xLevels = new ScNameToIndexAccess( xLevsName );
        nLevCount = xLevels->getCount();
    }
    uno::Reference<uno::XInterface> xLevel;
    if ( rField.nLevel < nLevCount )
        xLevel = ScUnoHelpFunctions::AnyToInterface( xLevels->getByIndex( rField.nLevel ) );
    DBG_ASSERT( xLevel.is(), "level not found" );

    uno::Reference<beans::XPropertySet> xLevelProp( xLevel, uno::UNO_QUERY );
    if ( xLevelProp.is() )
    {
        try
        {
            uno::Any aValue = xLevelProp->getPropertyValue( rtl::OUString::createFromAscii(DP_PROP_SUBTOTALS) );
            aValue >>= aSubTotals;
        }
        catch(uno::Exception&)
        {
        }
    }

    return aSubTotals;
}

void lcl_FilterInclude( std::vector< BOOL >& rResult, std::vector< sal_Int32 >& rSubtotal,
                        const ScDPOutLevelData& rField,
                        const std::vector< ScDPGetPivotDataField >& rFilters,
                        std::vector< BOOL >& rFilterUsed,
                        bool& rBeforeDataLayout,
                        sal_Int32 nGrandTotals, sal_Int32 nDataLayoutIndex,
                        const std::vector<String>& rDataNames, const std::vector<String>& rGivenNames,
                        const ScDPGetPivotDataField& rTarget, const uno::Reference<sheet::XDimensionsSupplier>& xSource )
{
    // returns true if a filter was given for the field

    DBG_ASSERT( rFilters.size() == rFilterUsed.size(), "wrong size" );

    const bool bIsDataLayout = ( rField.nDim == nDataLayoutIndex );
    if (bIsDataLayout)
        rBeforeDataLayout = false;

    bool bHasFilter = false;
    ScDPGetPivotDataField aFilter;
    if ( !bIsDataLayout )          // selection of data field is handled separately
    {
        for (SCSIZE nFilterPos = 0; nFilterPos < rFilters.size() && !bHasFilter; ++nFilterPos)
        {
            if ( lcl_IsNamedCategoryField( rFilters[nFilterPos], rField ) )
            {
                aFilter = rFilters[nFilterPos];
                rFilterUsed[nFilterPos] = TRUE;
                bHasFilter = true;
            }
        }
    }

    bool bHasFunc = bHasFilter && aFilter.meFunction != sheet::GeneralFunction_NONE;

    uno::Sequence<sheet::GeneralFunction> aSubTotals;
    if ( !bIsDataLayout )
        aSubTotals = lcl_GetSubTotals( xSource, rField );
    bool bManualSub = ( aSubTotals.getLength() > 0 && aSubTotals[0] != sheet::GeneralFunction_AUTO );

    const uno::Sequence<sheet::MemberResult>& rSequence = rField.aResult;
    const sheet::MemberResult* pArray = rSequence.getConstArray();
    sal_Int32 nSize = rSequence.getLength();

    DBG_ASSERT( (sal_Int32)rResult.size() == nSize, "Number of fields do not match result count" );

    sal_Int32 nContCount = 0;
    sal_Int32 nSubTotalCount = 0;
    sheet::MemberResult aPrevious;
    for( sal_Int32 j=0; j < nSize; j++ )
    {
        sheet::MemberResult aResultEntry = pArray[j];
        if ( aResultEntry.Flags & sheet::MemberResultFlags::CONTINUE )
        {
            aResultEntry = aPrevious;
            ++nContCount;
        }
        else if ( ( aResultEntry.Flags & sheet::MemberResultFlags::SUBTOTAL ) == 0 )
        {
            // count the CONTINUE entries before a SUBTOTAL
            nContCount = 0;
        }

        if ( j >= nSize - nGrandTotals )
        {
            // mark as subtotal for the preceding data
            if ( ( aResultEntry.Flags & sheet::MemberResultFlags::SUBTOTAL ) != 0 )
            {
                rSubtotal[j] = nSize - nGrandTotals;

                if ( rResult[j] && nGrandTotals > 1 )
                {
                    // grand total is always automatic
                    sal_Int32 nDataPos = j - ( nSize - nGrandTotals );
                    DBG_ASSERT( nDataPos < (sal_Int32)rDataNames.size(), "wrong data count" );
                    String aSourceName( rDataNames[nDataPos] );     // vector contains source names
                    String aGivenName( rGivenNames[nDataPos] );

                    rResult[j] = lcl_IsNamedDataField( rTarget, aSourceName, aGivenName );
                }
            }

            // treat "grand total" columns/rows as empty description, as if they were marked
            // in a previous field

            DBG_ASSERT( ( aResultEntry.Flags &
                            ( sheet::MemberResultFlags::HASMEMBER | sheet::MemberResultFlags::SUBTOTAL ) ) == 0 ||
                        ( aResultEntry.Flags &
                            ( sheet::MemberResultFlags::HASMEMBER | sheet::MemberResultFlags::SUBTOTAL ) ) ==
                                ( sheet::MemberResultFlags::HASMEMBER | sheet::MemberResultFlags::SUBTOTAL ),
                        "non-subtotal member found in grand total result" );
            aResultEntry.Flags = 0;
        }

        // mark subtotals (not grand total) for preceding data (assume CONTINUE is set)
        if ( ( aResultEntry.Flags & sheet::MemberResultFlags::SUBTOTAL ) != 0 )
        {
            rSubtotal[j] = nContCount + 1 + nSubTotalCount;

            if ( rResult[j] )
            {
                if ( bManualSub )
                {
                    if ( rBeforeDataLayout )
                    {
                        // manual subtotals and several data fields

                        sal_Int32 nDataCount = rDataNames.size();
                        sal_Int32 nFuncPos = nSubTotalCount / nDataCount;       // outer order: subtotal functions
                        sal_Int32 nDataPos = nSubTotalCount % nDataCount;       // inner order: data fields

                        String aSourceName( rDataNames[nDataPos] );             // vector contains source names
                        String aGivenName( rGivenNames[nDataPos] );

                        DBG_ASSERT( nFuncPos < aSubTotals.getLength(), "wrong subtotal count" );
                        rResult[j] = lcl_IsNamedDataField( rTarget, aSourceName, aGivenName ) &&
                                     aSubTotals[nFuncPos] == aFilter.meFunction;
                    }
                    else
                    {
                        // manual subtotals for a single data field

                        DBG_ASSERT( nSubTotalCount < aSubTotals.getLength(), "wrong subtotal count" );
                        rResult[j] = ( aSubTotals[nSubTotalCount] == aFilter.meFunction );
                    }
                }
                else    // automatic subtotals
                {
                    if ( rBeforeDataLayout )
                    {
                        DBG_ASSERT( nSubTotalCount < (sal_Int32)rDataNames.size(), "wrong data count" );
                        String aSourceName( rDataNames[nSubTotalCount] );       // vector contains source names
                        String aGivenName( rGivenNames[nSubTotalCount] );

                        rResult[j] = lcl_IsNamedDataField( rTarget, aSourceName, aGivenName );
                    }

                    // if a function was specified, automatic subtotals never match
                    if ( bHasFunc )
                        rResult[j] = FALSE;
                }
            }

            ++nSubTotalCount;
        }
        else
            nSubTotalCount = 0;

        if( rResult[j] )
        {
            if ( bIsDataLayout )
            {
                if ( ( aResultEntry.Flags & sheet::MemberResultFlags::HASMEMBER ) != 0 )
                {
                    // Asterisks are added in ScDPSaveData::WriteToSource to create unique names.
                    //! preserve original name there?
                    String aSourceName( aResultEntry.Name );
                    aSourceName.EraseTrailingChars( '*' );

                    String aGivenName( aResultEntry.Caption );  //! Should use a stored name when available
                    aGivenName.EraseLeadingChars( '\'' );

                    rResult[j] = lcl_IsNamedDataField( rTarget, aSourceName, aGivenName );
                }
            }
            else if ( bHasFilter )
            {
                // name must match (simple value or subtotal)
                rResult[j] = ( ( aResultEntry.Flags & sheet::MemberResultFlags::HASMEMBER ) != 0 ) &&
                             lcl_IsCondition( aResultEntry, aFilter );

                // if a function was specified, simple (non-subtotal) values never match
                if ( bHasFunc && nSubTotalCount == 0 )
                    rResult[j] = FALSE;
            }
            // if no condition is given, keep the columns/rows included
        }
        aPrevious = aResultEntry;
    }
}

void lcl_StripSubTotals( std::vector< BOOL >& rResult, const std::vector< sal_Int32 >& rSubtotal )
{
    sal_Int32 nSize = rResult.size();
    DBG_ASSERT( (sal_Int32)rSubtotal.size() == nSize, "sizes don't match" );

    for (sal_Int32 nPos=0; nPos<nSize; nPos++)
        if ( rResult[nPos] && rSubtotal[nPos] )
        {
            // if a subtotal is included, clear the result flag for the columns/rows that the subtotal includes
            sal_Int32 nStart = nPos - rSubtotal[nPos];
            DBG_ASSERT( nStart >= 0, "invalid subtotal count" );

            for (sal_Int32 nPrev = nStart; nPrev < nPos; nPrev++)
                rResult[nPrev] = FALSE;
        }
}

String lcl_GetDataFieldName( const String& rSourceName, sheet::GeneralFunction eFunc )
{
    USHORT nStrId = 0;
    switch ( eFunc )
    {
        case sheet::GeneralFunction_SUM:        nStrId = STR_FUN_TEXT_SUM;      break;
        case sheet::GeneralFunction_COUNT:
        case sheet::GeneralFunction_COUNTNUMS:  nStrId = STR_FUN_TEXT_COUNT;    break;
        case sheet::GeneralFunction_AVERAGE:    nStrId = STR_FUN_TEXT_AVG;      break;
        case sheet::GeneralFunction_MAX:        nStrId = STR_FUN_TEXT_MAX;      break;
        case sheet::GeneralFunction_MIN:        nStrId = STR_FUN_TEXT_MIN;      break;
        case sheet::GeneralFunction_PRODUCT:    nStrId = STR_FUN_TEXT_PRODUCT;  break;
        case sheet::GeneralFunction_STDEV:
        case sheet::GeneralFunction_STDEVP:     nStrId = STR_FUN_TEXT_STDDEV;   break;
        case sheet::GeneralFunction_VAR:
        case sheet::GeneralFunction_VARP:       nStrId = STR_FUN_TEXT_VAR;      break;
        case sheet::GeneralFunction_NONE:
        case sheet::GeneralFunction_AUTO:
        default:
        {
            DBG_ERRORFILE("wrong function");
        }
    }
    if ( !nStrId )
        return String();

    String aRet( ScGlobal::GetRscString( nStrId ) );
    aRet.AppendAscii(RTL_CONSTASCII_STRINGPARAM( " - " ));
    aRet.Append( rSourceName );
    return aRet;
}

// static
void ScDPOutput::GetDataDimensionNames( String& rSourceName, String& rGivenName,
                                        const uno::Reference<uno::XInterface>& xDim )
{
    uno::Reference<beans::XPropertySet> xDimProp( xDim, uno::UNO_QUERY );
    uno::Reference<container::XNamed> xDimName( xDim, uno::UNO_QUERY );
    if ( xDimProp.is() && xDimName.is() )
    {
        // Asterisks are added in ScDPSaveData::WriteToSource to create unique names.
        //! preserve original name there?
        rSourceName = xDimName->getName();
        rSourceName.EraseTrailingChars( '*' );

        // Generate "given name" the same way as in dptabres.
        //! Should use a stored name when available

        sheet::GeneralFunction eFunc = (sheet::GeneralFunction)ScUnoHelpFunctions::GetEnumProperty(
                                xDimProp, rtl::OUString::createFromAscii(DP_PROP_FUNCTION),
                                sheet::GeneralFunction_NONE );
        rGivenName = lcl_GetDataFieldName( rSourceName, eFunc );
    }
}

void lcl_GetTableVars( sal_Int32& rGrandTotalCols, sal_Int32& rGrandTotalRows, sal_Int32& rDataLayoutIndex,
                             std::vector<String>& rDataNames, std::vector<String>& rGivenNames,
                             sheet::DataPilotFieldOrientation& rDataOrient,
                             const uno::Reference<sheet::XDimensionsSupplier>& xSource )
{
    rDataLayoutIndex = -1;  // invalid
    rGrandTotalCols = 0;
    rGrandTotalRows = 0;
    rDataOrient = sheet::DataPilotFieldOrientation_HIDDEN;

    uno::Reference<beans::XPropertySet> xSrcProp( xSource, uno::UNO_QUERY );
    BOOL bColGrand = ScUnoHelpFunctions::GetBoolProperty( xSrcProp,
                                         rtl::OUString::createFromAscii(DP_PROP_COLUMNGRAND) );
    if ( bColGrand )
        rGrandTotalCols = 1;    // default if data layout not in columns

    BOOL bRowGrand = ScUnoHelpFunctions::GetBoolProperty( xSrcProp,
                                         rtl::OUString::createFromAscii(DP_PROP_ROWGRAND) );
    if ( bRowGrand )
        rGrandTotalRows = 1;    // default if data layout not in rows

    if ( xSource.is() )
    {
        // find index and orientation of "data layout" dimension, count data dimensions

        sal_Int32 nDataCount = 0;

        uno::Reference<container::XIndexAccess> xDims = new ScNameToIndexAccess( xSource->getDimensions() );
        long nDimCount = xDims->getCount();
        for (long nDim=0; nDim<nDimCount; nDim++)
        {
            uno::Reference<uno::XInterface> xDim =
                    ScUnoHelpFunctions::AnyToInterface( xDims->getByIndex(nDim) );
            uno::Reference<beans::XPropertySet> xDimProp( xDim, uno::UNO_QUERY );
            if ( xDimProp.is() )
            {
                sheet::DataPilotFieldOrientation eDimOrient =
                    (sheet::DataPilotFieldOrientation) ScUnoHelpFunctions::GetEnumProperty(
                        xDimProp, rtl::OUString::createFromAscii(DP_PROP_ORIENTATION),
                        sheet::DataPilotFieldOrientation_HIDDEN );
                if ( ScUnoHelpFunctions::GetBoolProperty( xDimProp,
                                         rtl::OUString::createFromAscii(DP_PROP_ISDATALAYOUT) ) )
                {
                    rDataLayoutIndex = nDim;
                    rDataOrient = eDimOrient;
                }
                if ( eDimOrient == sheet::DataPilotFieldOrientation_DATA )
                {
                    String aSourceName;
                    String aGivenName;
                    ScDPOutput::GetDataDimensionNames( aSourceName, aGivenName, xDim );
                    rDataNames.push_back( aSourceName );
                    rGivenNames.push_back( aGivenName );

                    ++nDataCount;
                }
            }
        }

        if ( ( rDataOrient == sheet::DataPilotFieldOrientation_COLUMN ) && bColGrand )
            rGrandTotalCols = nDataCount;
        else if ( ( rDataOrient == sheet::DataPilotFieldOrientation_ROW ) && bRowGrand )
            rGrandTotalRows = nDataCount;
    }
}

// Returns TRUE on success and stores the result in rTarget
// Returns FALSE if rFilters or rTarget describes something that is not visible
BOOL ScDPOutput::GetPivotData( ScDPGetPivotDataField& rTarget,
                               const std::vector< ScDPGetPivotDataField >& rFilters )
{
    CalcSizes();

    // need to know about grand total columns/rows:
    sal_Int32 nGrandTotalCols;
    sal_Int32 nGrandTotalRows;
    sal_Int32 nDataLayoutIndex;
    std::vector<String> aDataNames;
    std::vector<String> aGivenNames;
    sheet::DataPilotFieldOrientation eDataOrient;
    lcl_GetTableVars( nGrandTotalCols, nGrandTotalRows, nDataLayoutIndex, aDataNames, aGivenNames, eDataOrient, xSource );

    if ( aDataNames.empty() )
        return FALSE;               // incomplete table without data fields -> no result

    if ( eDataOrient == sheet::DataPilotFieldOrientation_HIDDEN )
    {
        // no data layout field -> single data field -> must match the selected field in rTarget

        DBG_ASSERT( aDataNames.size() == 1, "several data fields but no data layout field" );
        if ( !lcl_IsNamedDataField( rTarget, aDataNames[0], aGivenNames[0] ) )
            return FALSE;
    }

    std::vector< BOOL > aIncludeCol( nColCount, TRUE );
    std::vector< sal_Int32 > aSubtotalCol( nColCount, 0 );
    std::vector< BOOL > aIncludeRow( nRowCount, TRUE );
    std::vector< sal_Int32 > aSubtotalRow( nRowCount, 0 );

    std::vector< BOOL > aFilterUsed( rFilters.size(), FALSE );

    long nField;
    long nCol;
    long nRow;
    bool bBeforeDataLayout;

    // look in column fields

    bBeforeDataLayout = ( eDataOrient == sheet::DataPilotFieldOrientation_COLUMN );
    for (nField=0; nField<nColFieldCount; nField++)
        lcl_FilterInclude( aIncludeCol, aSubtotalCol, pColFields[nField], rFilters, aFilterUsed, bBeforeDataLayout,
                           nGrandTotalCols, nDataLayoutIndex, aDataNames, aGivenNames, rTarget, xSource );

    // look in row fields

    bBeforeDataLayout = ( eDataOrient == sheet::DataPilotFieldOrientation_ROW );
    for (nField=0; nField<nRowFieldCount; nField++)
        lcl_FilterInclude( aIncludeRow, aSubtotalRow, pRowFields[nField], rFilters, aFilterUsed, bBeforeDataLayout,
                           nGrandTotalRows, nDataLayoutIndex, aDataNames, aGivenNames, rTarget, xSource );

    // page fields

    for (nField=0; nField<nPageFieldCount; nField++)
        if ( !lcl_CheckPageField( pPageFields[nField], rFilters, aFilterUsed ) )
            return FALSE;

    // all filter fields must be used
    for (SCSIZE nFilter=0; nFilter<aFilterUsed.size(); nFilter++)
        if (!aFilterUsed[nFilter])
            return FALSE;

    lcl_StripSubTotals( aIncludeCol, aSubtotalCol );
    lcl_StripSubTotals( aIncludeRow, aSubtotalRow );

    long nColPos = 0;
    long nColIncluded = 0;
    for (nCol=0; nCol<nColCount; nCol++)
        if (aIncludeCol[nCol])
        {
            nColPos = nCol;
            ++nColIncluded;
        }

    long nRowPos = 0;
    long nRowIncluded = 0;
    for (nRow=0; nRow<nRowCount; nRow++)
        if (aIncludeRow[nRow])
        {
            nRowPos = nRow;
            ++nRowIncluded;
        }

    if ( nColIncluded != 1 || nRowIncluded != 1 )
        return FALSE;

    const uno::Sequence<sheet::DataResult>& rDataRow = aData[nRowPos];
    if ( nColPos >= rDataRow.getLength() )
        return FALSE;

    const sheet::DataResult& rResult = rDataRow[nColPos];
    if ( rResult.Flags & sheet::DataResultFlags::ERROR )
        return FALSE;                                       //! different error?

    rTarget.mbValIsStr = FALSE;
    rTarget.mnValNum = rResult.Value;

    return TRUE;
}

BOOL ScDPOutput::IsFilterButton( const ScAddress& rPos )
{
	SCCOL nCol = rPos.Col();
	SCROW nRow = rPos.Row();
	SCTAB nTab = rPos.Tab();
	if ( nTab != aStartPos.Tab() || !bDoFilter )
		return FALSE;								// wrong sheet or no button at all

	//	filter button is at top left
	return ( nCol == aStartPos.Col() && nRow == aStartPos.Row() );
}

long ScDPOutput::GetHeaderDim( const ScAddress& rPos, USHORT& rOrient )
{
	SCCOL nCol = rPos.Col();
	SCROW nRow = rPos.Row();
	SCTAB nTab = rPos.Tab();
	if ( nTab != aStartPos.Tab() )
		return -1;										// wrong sheet

	//	calculate output positions and sizes

	CalcSizes();

	//	test for column header

	if ( nRow == nTabStartRow && nCol >= nDataStartCol && nCol < nDataStartCol + nColFieldCount )
	{
		rOrient = sheet::DataPilotFieldOrientation_COLUMN;
		long nField = nCol - nDataStartCol;
		return pColFields[nField].nDim;
	}

	//	test for row header

	if ( nRow+1 == nDataStartRow && nCol >= nTabStartCol && nCol < nTabStartCol + nRowFieldCount )
	{
		rOrient = sheet::DataPilotFieldOrientation_ROW;
		long nField = nCol - nTabStartCol;
		return pRowFields[nField].nDim;
	}

	//	test for page field

	SCROW nPageStartRow = aStartPos.Row() + ( bDoFilter ? 1 : 0 );
	if ( nCol == aStartPos.Col() && nRow >= nPageStartRow && nRow < nPageStartRow + nPageFieldCount )
	{
		rOrient = sheet::DataPilotFieldOrientation_PAGE;
		long nField = nRow - nPageStartRow;
		return pPageFields[nField].nDim;
	}

	//!	single data field (?)

	rOrient = sheet::DataPilotFieldOrientation_HIDDEN;
	return -1;		// invalid
}

BOOL ScDPOutput::GetHeaderDrag( const ScAddress& rPos, BOOL bMouseLeft, BOOL bMouseTop,
								long nDragDim,
								Rectangle& rPosRect, USHORT& rOrient, long& rDimPos )
{
	//	Rectangle instead of ScRange for rPosRect to allow for negative values

	SCCOL nCol = rPos.Col();
	SCROW nRow = rPos.Row();
	SCTAB nTab = rPos.Tab();
	if ( nTab != aStartPos.Tab() )
		return FALSE;										// wrong sheet

	//	calculate output positions and sizes

	CalcSizes();

	//	test for column header

	if ( nCol >= nDataStartCol && nCol <= nTabEndCol &&
			nRow + 1 >= nMemberStartRow && nRow < nMemberStartRow + nColFieldCount )
	{
		long nField = nRow - nMemberStartRow;
		if (nField < 0)
		{
			nField = 0;
			bMouseTop = TRUE;
		}
		//!	find start of dimension

		rPosRect = Rectangle( nDataStartCol, nMemberStartRow + nField,
							  nTabEndCol, nMemberStartRow + nField -1 );

		BOOL bFound = FALSE;			// is this within the same orientation?
		BOOL bBeforeDrag = FALSE;
		BOOL bAfterDrag = FALSE;
		for (long nPos=0; nPos<nColFieldCount && !bFound; nPos++)
		{
			if (pColFields[nPos].nDim == nDragDim)
			{
				bFound = TRUE;
				if ( nField < nPos )
					bBeforeDrag = TRUE;
				else if ( nField > nPos )
					bAfterDrag = TRUE;
			}
		}

		if ( bFound )
		{
			if (!bBeforeDrag)
			{
				++rPosRect.Bottom();
				if (bAfterDrag)
					++rPosRect.Top();
			}
		}
		else
		{
			if ( !bMouseTop )
			{
				++rPosRect.Top();
				++rPosRect.Bottom();
				++nField;
			}
		}

		rOrient = sheet::DataPilotFieldOrientation_COLUMN;
		rDimPos = nField;						//!...
		return TRUE;
	}

	//	test for row header

	//	special case if no row fields
	BOOL bSpecial = ( nRow+1 >= nDataStartRow && nRow <= nTabEndRow &&
						nRowFieldCount == 0 && nCol == nTabStartCol && bMouseLeft );

	if ( bSpecial || ( nRow+1 >= nDataStartRow && nRow <= nTabEndRow &&
						nCol + 1 >= nTabStartCol && nCol < nTabStartCol + nRowFieldCount ) )
	{
		long nField = nCol - nTabStartCol;
		//!	find start of dimension

		rPosRect = Rectangle( nTabStartCol + nField, nDataStartRow - 1,
							  nTabStartCol + nField - 1, nTabEndRow );

		BOOL bFound = FALSE;			// is this within the same orientation?
		BOOL bBeforeDrag = FALSE;
		BOOL bAfterDrag = FALSE;
		for (long nPos=0; nPos<nRowFieldCount && !bFound; nPos++)
		{
			if (pRowFields[nPos].nDim == nDragDim)
			{
				bFound = TRUE;
				if ( nField < nPos )
					bBeforeDrag = TRUE;
				else if ( nField > nPos )
					bAfterDrag = TRUE;
			}
		}

		if ( bFound )
		{
			if (!bBeforeDrag)
			{
				++rPosRect.Right();
				if (bAfterDrag)
					++rPosRect.Left();
			}
		}
		else
		{
			if ( !bMouseLeft )
			{
				++rPosRect.Left();
				++rPosRect.Right();
				++nField;
			}
		}

		rOrient = sheet::DataPilotFieldOrientation_ROW;
		rDimPos = nField;						//!...
		return TRUE;
	}

	//	test for page fields

	SCROW nPageStartRow = aStartPos.Row() + ( bDoFilter ? 1 : 0 );
	if ( nCol >= aStartPos.Col() && nCol <= nTabEndCol &&
			nRow + 1 >= nPageStartRow && nRow < nPageStartRow + nPageFieldCount )
	{
		long nField = nRow - nPageStartRow;
		if (nField < 0)
		{
			nField = 0;
			bMouseTop = TRUE;
		}
		//!	find start of dimension

		rPosRect = Rectangle( aStartPos.Col(), nPageStartRow + nField,
							  nTabEndCol, nPageStartRow + nField - 1 );

		BOOL bFound = FALSE;			// is this within the same orientation?
		BOOL bBeforeDrag = FALSE;
		BOOL bAfterDrag = FALSE;
		for (long nPos=0; nPos<nPageFieldCount && !bFound; nPos++)
		{
			if (pPageFields[nPos].nDim == nDragDim)
			{
				bFound = TRUE;
				if ( nField < nPos )
					bBeforeDrag = TRUE;
				else if ( nField > nPos )
					bAfterDrag = TRUE;
			}
		}

		if ( bFound )
		{
			if (!bBeforeDrag)
			{
				++rPosRect.Bottom();
				if (bAfterDrag)
					++rPosRect.Top();
			}
		}
		else
		{
			if ( !bMouseTop )
			{
				++rPosRect.Top();
				++rPosRect.Bottom();
				++nField;
			}
		}

		rOrient = sheet::DataPilotFieldOrientation_PAGE;
		rDimPos = nField;						//!...
		return TRUE;
	}

	return FALSE;
}



