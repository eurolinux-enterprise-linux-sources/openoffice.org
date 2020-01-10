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
#include "precompiled_chart2.hxx"

#include "BubbleDataInterpreter.hxx"
#include "DataSeries.hxx"
#include "macros.hxx"
#include "DataSeriesHelper.hxx"
#include "CommonConverters.hxx"
#include "ContainerHelper.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/chart2/data/XDataSink.hpp>
#include <com/sun/star/util/XCloneable.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;
using namespace ::std;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::rtl::OUString;

namespace chart
{

BubbleDataInterpreter::BubbleDataInterpreter(
    const uno::Reference< uno::XComponentContext > & xContext ) :
        DataInterpreter( xContext )
{
}

BubbleDataInterpreter::~BubbleDataInterpreter()
{
}

// ____ XDataInterpreter ____
chart2::InterpretedData SAL_CALL BubbleDataInterpreter::interpretDataSource(
    const Reference< chart2::data::XDataSource >& xSource,
    const Sequence< beans::PropertyValue >& aArguments,
    const Sequence< Reference< XDataSeries > >& aSeriesToReUse )
    throw (uno::RuntimeException)
{
    if( ! xSource.is())
        return InterpretedData();

    Sequence< Reference< data::XLabeledDataSequence > > aData( xSource->getDataSequences() );

    Reference< data::XLabeledDataSequence > xValuesX;
    vector< Reference< data::XLabeledDataSequence > > aYValuesVector;
    vector< Reference< data::XLabeledDataSequence > > aSizeValuesVector;

    Reference< data::XLabeledDataSequence > xCategories;
    bool bHasCategories = HasCategories( aArguments, aData );
    
    Sequence< Reference< data::XLabeledDataSequence > > aUnusedData;

    bool bHasXValues = false;
    sal_Int32 nDataSeqCount = aData.getLength();

    bHasXValues = bHasCategories ? ( (nDataSeqCount-1) > 2 && (nDataSeqCount-1) % 2 != 0 )
                                 :( nDataSeqCount > 2 && nDataSeqCount % 2 != 0 );

    bool bCategoriesUsed = false;  
    bool bNextIsYValues = bHasCategories ? nDataSeqCount>2 : nDataSeqCount>1;
    for( sal_Int32 nDataIdx = 0; nDataIdx < aData.getLength(); ++nDataIdx )
    {
        try
        {
            if( bHasCategories && !bCategoriesUsed )
            {
                xCategories.set( aData[nDataIdx] );
                if( xCategories.is())
                    SetRole( xCategories->getValues(), C2U("categories"));
                bCategoriesUsed = true;
            }
            else if( !xValuesX.is() && bHasXValues )
            {
                xValuesX.set( aData[nDataIdx] );
                if( xValuesX.is())
                    SetRole( xValuesX->getValues(), C2U("values-x"));
            }
            else if( bNextIsYValues )
            {
                aYValuesVector.push_back( aData[nDataIdx] );
                if( aData[nDataIdx].is())
                    SetRole( aData[nDataIdx]->getValues(), C2U("values-y"));
                bNextIsYValues = false;
            }
            else if( !bNextIsYValues )
            {
                aSizeValuesVector.push_back( aData[nDataIdx] );
                if( aData[nDataIdx].is())
                    SetRole( aData[nDataIdx]->getValues(), C2U("values-size"));
                bNextIsYValues = true;
            }
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    // create DataSeries
    sal_Int32 nSeriesIndex = 0;
    vector< Reference< XDataSeries > > aSeriesVec;
    aSeriesVec.reserve( aSizeValuesVector.size());

    Reference< data::XLabeledDataSequence > xClonedXValues = xValuesX;
    Reference< util::XCloneable > xCloneableX( xValuesX, uno::UNO_QUERY );

    for( size_t nCount = 0; nCount < aSizeValuesVector.size(); ++nCount, ++nSeriesIndex )
    {
        sal_Int32 nDataSequenceCount = 2;
        if( xValuesX.is() )
            nDataSequenceCount = 3;
        else if( aYValuesVector.empty() )
            nDataSequenceCount = 1;

        Sequence< Reference< data::XLabeledDataSequence > > aNewData( nDataSequenceCount );
        sal_Int32 nDataIndex = 0;
        if( xValuesX.is() )
        {
            if( nCount > 0 && xCloneableX.is() )
                xClonedXValues.set( xCloneableX->createClone(), uno::UNO_QUERY );
            aNewData[nDataIndex++] = xClonedXValues;
        }
        if( aYValuesVector.size() > nCount )
            aNewData[nDataIndex++] = aYValuesVector[nCount];
        if( aSizeValuesVector.size() > nCount )
            aNewData[nDataIndex++] = aSizeValuesVector[nCount];

        Reference< XDataSeries > xSeries;
        if( nSeriesIndex < aSeriesToReUse.getLength())
            xSeries.set( aSeriesToReUse[nSeriesIndex] );
        else
            xSeries.set( new DataSeries( GetComponentContext() ) );
        OSL_ASSERT( xSeries.is() );
        Reference< data::XDataSink > xSink( xSeries, uno::UNO_QUERY );
        OSL_ASSERT( xSink.is() );
        xSink->setData( aNewData );

        aSeriesVec.push_back( xSeries );
    }

    Sequence< Sequence< Reference< XDataSeries > > > aSeries(1);
    aSeries[0] = ContainerHelper::ContainerToSequence( aSeriesVec );
    return InterpretedData( aSeries, xCategories, aUnusedData );
}

chart2::InterpretedData SAL_CALL BubbleDataInterpreter::reinterpretDataSeries(
    const chart2::InterpretedData& aInterpretedData )
    throw (uno::RuntimeException)
{
    InterpretedData aResult( aInterpretedData );
    vector< Reference< data::XLabeledDataSequence > > aUnused(
        ContainerHelper::SequenceToVector( aInterpretedData.UnusedData ));

    sal_Int32 i=0;
    Sequence< Reference< XDataSeries > > aSeries( FlattenSequence( aInterpretedData.Series ));
    const sal_Int32 nCount = aSeries.getLength();
    for( ; i<nCount; ++i )
    {
        try
        {
            Reference< data::XDataSource > xSeriesSource( aSeries[i], uno::UNO_QUERY_THROW );
            Sequence< Reference< data::XLabeledDataSequence > > aNewSequences;

			Reference< data::XLabeledDataSequence > xValuesSize(
                DataSeriesHelper::getDataSequenceByRole( xSeriesSource, C2U("values-size"), false ));
            Reference< data::XLabeledDataSequence > xValuesY(
                DataSeriesHelper::getDataSequenceByRole( xSeriesSource, C2U("values-y"), false ));
            Reference< data::XLabeledDataSequence > xValuesX(
                DataSeriesHelper::getDataSequenceByRole( xSeriesSource, C2U("values-x"), false ));

            if( ! xValuesX.is() ||
                ! xValuesY.is() ||
				! xValuesSize.is() )
            {
                vector< Reference< data::XLabeledDataSequence > > aValueSeqVec(
                    DataSeriesHelper::getAllDataSequencesByRole(
                        xSeriesSource->getDataSequences(), C2U("values"), true ));
                if( xValuesX.is())
                    aValueSeqVec.erase( find( aValueSeqVec.begin(), aValueSeqVec.end(), xValuesX ));
                if( xValuesY.is())
                    aValueSeqVec.erase( find( aValueSeqVec.begin(), aValueSeqVec.end(), xValuesY ));
				if( xValuesSize.is())
                    aValueSeqVec.erase( find( aValueSeqVec.begin(), aValueSeqVec.end(), xValuesSize ));

                size_t nIndex = 0;

                if( ! xValuesSize.is() &&
                    aValueSeqVec.size() > nIndex )
                {
                    xValuesSize.set( aValueSeqVec[nIndex++] );
                    if( xValuesSize.is())
                        SetRole( xValuesSize->getValues(), C2U("values-size"));
                }

                if( ! xValuesY.is() &&
                    aValueSeqVec.size() > nIndex )
                {
                    xValuesY.set( aValueSeqVec[nIndex++] );
                    if( xValuesY.is())
                        SetRole( xValuesY->getValues(), C2U("values-y"));
                }

                if( ! xValuesX.is() &&
                    aValueSeqVec.size() > nIndex )
                {
                    xValuesX.set( aValueSeqVec[nIndex++] );
                    if( xValuesX.is())
                        SetRole( xValuesY->getValues(), C2U("values-x"));
                }
            }
            if( xValuesSize.is())
            {
				if( xValuesY.is() )
				{
				    if( xValuesX.is() )
					{
					    aNewSequences.realloc(3);
                        aNewSequences[0] = xValuesX;
                        aNewSequences[1] = xValuesY;
						aNewSequences[2] = xValuesSize;
					}
					else
					{
					    aNewSequences.realloc(2);
                        aNewSequences[0] = xValuesY;
                        aNewSequences[1] = xValuesSize;
					}
				}
                else
                {
                    aNewSequences.realloc(1);
                    aNewSequences[0] = xValuesSize;
                }
            }

            Sequence< Reference< data::XLabeledDataSequence > > aSeqs( xSeriesSource->getDataSequences());
            if( aSeqs.getLength() != aNewSequences.getLength() )
            {
                sal_Int32 j=0;
                for( ; j<aSeqs.getLength(); ++j )
                {
                    if( aSeqs[j] != xValuesY &&
                        aSeqs[j] != xValuesX &&
						aSeqs[j] != xValuesSize )
                        aUnused.push_back( aSeqs[j] );
                }
                Reference< data::XDataSink > xSink( xSeriesSource, uno::UNO_QUERY_THROW );
                xSink->setData( aNewSequences );
                aResult.UnusedData = ContainerHelper::ContainerToSequence( aUnused );
            }
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return aResult;
}

sal_Bool SAL_CALL BubbleDataInterpreter::isDataCompatible(
    const chart2::InterpretedData& aInterpretedData )
    throw (uno::RuntimeException)
{
    Sequence< Reference< XDataSeries > > aSeries( FlattenSequence( aInterpretedData.Series ));
    for( sal_Int32 i=0; i<aSeries.getLength(); ++i )
    {
        try
        {
            Reference< data::XDataSource > xSrc( aSeries[i], uno::UNO_QUERY_THROW );
            Sequence< Reference< data::XLabeledDataSequence > > aSeq( xSrc->getDataSequences());
            if( aSeq.getLength() != 3 )
                return sal_False;
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return sal_True;
}

} // namespace chart
