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


#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/chart2/data/XDataReceiver.hpp>
#include <com/sun/star/chart/ChartDataRowSource.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/embed/Aspects.hpp>

#include <svx/svditer.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdundo.hxx>
#include <sfx2/app.hxx>
#include <svtools/moduleoptions.hxx>
#include <sot/clsids.hxx>
#include <toolkit/helper/vclunohelper.hxx>

#include "chartuno.hxx"
#include "miscuno.hxx"
#include "docsh.hxx"
#include "drwlayer.hxx"
#include "undodat.hxx"
#include "chartarr.hxx"
#include "chartlis.hxx"
#include "unoguard.hxx"
#include "chart2uno.hxx"

using namespace com::sun::star;

//------------------------------------------------------------------------

SC_SIMPLE_SERVICE_INFO( ScChartObj, "ScChartObj", "com.sun.star.table.TableChart" )
SC_SIMPLE_SERVICE_INFO( ScChartsObj, "ScChartsObj", "com.sun.star.table.TableCharts" )

//------------------------------------------------------------------------

SdrOle2Obj* lcl_FindChartObj( ScDocShell* pDocShell, SCTAB nTab, const String& rName )
{
	if (pDocShell)
	{
		ScDocument* pDoc = pDocShell->GetDocument();
		ScDrawLayer* pDrawLayer = pDoc->GetDrawLayer();
		if (pDrawLayer)
		{
			SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
			DBG_ASSERT(pPage, "Page nicht gefunden");
			if (pPage)
			{
				SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
				SdrObject* pObject = aIter.Next();
				while (pObject)
				{
					if ( pObject->GetObjIdentifier() == OBJ_OLE2 && pDoc->IsChart(pObject) )
					{
                        uno::Reference < embed::XEmbeddedObject > xObj = ((SdrOle2Obj*)pObject)->GetObjRef();
                        if ( xObj.is() )
						{
                            String aObjName = pDocShell->GetEmbeddedObjectContainer().GetEmbeddedObjectName( xObj );
                            if ( aObjName == rName )
                                return (SdrOle2Obj*)pObject;
						}
					}
					pObject = aIter.Next();
				}
			}
		}
	}
	return NULL;
}

//------------------------------------------------------------------------

ScChartsObj::ScChartsObj(ScDocShell* pDocSh, SCTAB nT) :
	pDocShell( pDocSh ),
	nTab( nT )
{
	pDocShell->GetDocument()->AddUnoObject(*this);
}

ScChartsObj::~ScChartsObj()
{
	if (pDocShell)
		pDocShell->GetDocument()->RemoveUnoObject(*this);
}

void ScChartsObj::Notify( SfxBroadcaster&, const SfxHint& rHint )
{
	//!	Referenz-Update

	if ( rHint.ISA( SfxSimpleHint ) &&
			((const SfxSimpleHint&)rHint).GetId() == SFX_HINT_DYING )
	{
		pDocShell = NULL;		// ungueltig geworden
	}
}

ScChartObj* ScChartsObj::GetObjectByIndex_Impl(long nIndex) const
{
	String aName;
	if ( pDocShell )
	{
		ScDocument* pDoc = pDocShell->GetDocument();
		ScDrawLayer* pDrawLayer = pDoc->GetDrawLayer();
		if (pDrawLayer)
		{
			SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
			DBG_ASSERT(pPage, "Page nicht gefunden");
			if (pPage)
			{
				long nPos = 0;
				SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
				SdrObject* pObject = aIter.Next();
				while (pObject)
				{
					if ( pObject->GetObjIdentifier() == OBJ_OLE2 && pDoc->IsChart(pObject) )
					{
						if ( nPos == nIndex )
						{
                            uno::Reference < embed::XEmbeddedObject > xObj = ((SdrOle2Obj*)pObject)->GetObjRef();
                            if ( xObj.is() )
                                aName = pDocShell->GetEmbeddedObjectContainer().GetEmbeddedObjectName( xObj );
							break;	// nicht weitersuchen
						}
						++nPos;
					}
					pObject = aIter.Next();
				}
			}
		}
	}

	if (aName.Len())
		return new ScChartObj( pDocShell, nTab, aName );
	return NULL;
}

ScChartObj* ScChartsObj::GetObjectByName_Impl(const rtl::OUString& aName) const
{
	String aNameString(aName);
	if ( lcl_FindChartObj( pDocShell, nTab, aNameString ) )
		return new ScChartObj( pDocShell, nTab, aNameString );
	return NULL;
}

// XTableCharts

void SAL_CALL ScChartsObj::addNewByName( const rtl::OUString& aName,
										const awt::Rectangle& aRect,
										const uno::Sequence<table::CellRangeAddress>& aRanges,
										sal_Bool bColumnHeaders, sal_Bool bRowHeaders )
									throw(::com::sun::star::uno::RuntimeException)
{
	ScUnoGuard aGuard;
	if (!pDocShell)
		return;

	ScDocument* pDoc = pDocShell->GetDocument();
	ScDrawLayer* pModel = pDocShell->MakeDrawLayer();
	SdrPage* pPage = pModel->GetPage(static_cast<sal_uInt16>(nTab));
	DBG_ASSERT(pPage,"addChart: keine Page");
	if (!pPage || !pDoc)
		return;

	//	chart can't be inserted if any ole object with that name exists on any table
	//	(empty string: generate valid name)

	String aNameString(aName);
	SCTAB nDummy;
	if ( aNameString.Len() && pModel->GetNamedObject( aNameString, OBJ_OLE2, nDummy ) )
	{
		//	object exists - only RuntimeException is specified
		throw uno::RuntimeException();
	}

	ScRangeList* pList = new ScRangeList;
	USHORT nRangeCount = (USHORT)aRanges.getLength();
	if (nRangeCount)
	{
		const table::CellRangeAddress* pAry = aRanges.getConstArray();
		for (USHORT i=0; i<nRangeCount; i++)
		{
			ScRange aRange( static_cast<SCCOL>(pAry[i].StartColumn), pAry[i].StartRow, pAry[i].Sheet,
							static_cast<SCCOL>(pAry[i].EndColumn),   pAry[i].EndRow,   pAry[i].Sheet );
			pList->Append( aRange );
		}
	}
	ScRangeListRef xNewRanges( pList );

    uno::Reference < embed::XEmbeddedObject > xObj;
    ::rtl::OUString aTmp( aNameString );
	if ( SvtModuleOptions().IsChart() )
        xObj = pDocShell->GetEmbeddedObjectContainer().CreateEmbeddedObject( SvGlobalName( SO3_SCH_CLASSID ).GetByteSequence(), aTmp );
    if ( xObj.is() )
	{
            String aObjName = aNameString;       // wirklich eingefuegter Name...

			//	Rechteck anpassen
			//!	Fehler/Exception, wenn leer/ungueltig ???
			Point aRectPos( aRect.X, aRect.Y );
			if (aRectPos.X() < 0) aRectPos.X() = 0;
			if (aRectPos.Y() < 0) aRectPos.Y() = 0;
			Size aRectSize( aRect.Width, aRect.Height );
			if (aRectSize.Width() <= 0) aRectSize.Width() = 5000;	// Default-Groesse
			if (aRectSize.Height() <= 0) aRectSize.Height() = 5000;
			Rectangle aInsRect( aRectPos, aRectSize );

            sal_Int64 nAspect(embed::Aspects::MSOLE_CONTENT);
            MapUnit aMapUnit(VCLUnoHelper::UnoEmbed2VCLMapUnit( xObj->getMapUnit( nAspect ) ));
			Size aSize(aInsRect.GetSize());
            aSize = Window::LogicToLogic( aSize, MapMode( MAP_100TH_MM ), MapMode( aMapUnit ) );
            awt::Size aSz;
            aSz.Width = aSize.Width();
            aSz.Height = aSize.Height();

            // Calc -> DataProvider
            uno::Reference< chart2::data::XDataProvider > xDataProvider = new
                ScChart2DataProvider( pDoc );
            // Chart -> DataReceiver
            uno::Reference< chart2::data::XDataReceiver > xReceiver;
            uno::Reference< embed::XComponentSupplier > xCompSupp( xObj, uno::UNO_QUERY );
            if( xCompSupp.is())
                xReceiver.set( xCompSupp->getComponent(), uno::UNO_QUERY );
            if( xReceiver.is())
            {
                String sRangeStr;
                xNewRanges->Format(sRangeStr, SCR_ABS_3D, pDoc);

                // connect
                if( sRangeStr.Len() )
                    xReceiver->attachDataProvider( xDataProvider );
                else
                    sRangeStr = String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM( "all" ) );

                uno::Reference< util::XNumberFormatsSupplier > xNumberFormatsSupplier( pDocShell->GetModel(), uno::UNO_QUERY );
                xReceiver->attachNumberFormatsSupplier( xNumberFormatsSupplier );

                // set arguments
                uno::Sequence< beans::PropertyValue > aArgs( 4 );
                aArgs[0] = beans::PropertyValue(
                    ::rtl::OUString::createFromAscii("CellRangeRepresentation"), -1,
                    uno::makeAny( ::rtl::OUString( sRangeStr )), beans::PropertyState_DIRECT_VALUE );
                aArgs[1] = beans::PropertyValue(
                    ::rtl::OUString::createFromAscii("HasCategories"), -1,
                    uno::makeAny( bRowHeaders ), beans::PropertyState_DIRECT_VALUE );
                aArgs[2] = beans::PropertyValue(
                    ::rtl::OUString::createFromAscii("FirstCellAsLabel"), -1,
                    uno::makeAny( bColumnHeaders ), beans::PropertyState_DIRECT_VALUE );
                aArgs[3] = beans::PropertyValue(
                    ::rtl::OUString::createFromAscii("DataRowSource"), -1,
                    uno::makeAny( chart::ChartDataRowSource_COLUMNS ), beans::PropertyState_DIRECT_VALUE );
                xReceiver->setArguments( aArgs );
            }

			ScChartListener* pChartListener =
				new ScChartListener( aObjName, pDoc, xNewRanges );
			pDoc->GetChartListenerCollection()->Insert( pChartListener );
			pChartListener->StartListeningTo();

            SdrOle2Obj* pObj = new SdrOle2Obj( ::svt::EmbeddedObjectRef( xObj, embed::Aspects::MSOLE_CONTENT ), aObjName, aInsRect );

            // set VisArea
            if( xObj.is())
                xObj->setVisualAreaSize( nAspect, aSz );

			pPage->InsertObject( pObj );
			pModel->AddUndo( new SdrUndoInsertObj( *pObj ) );		//! Undo-Kommentar?

			// Dies veranlaesst Chart zum sofortigen Update
			//SvData aEmpty;
			//aIPObj->SendDataChanged( aEmpty );
	}
}

void SAL_CALL ScChartsObj::removeByName( const rtl::OUString& aName )
											throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	String aNameString(aName);
	SdrOle2Obj* pObj = lcl_FindChartObj( pDocShell, nTab, aNameString );
	if (pObj)
	{
		ScDocument* pDoc = pDocShell->GetDocument();
		ScDrawLayer* pModel = pDoc->GetDrawLayer();		// ist nicht 0
		SdrPage* pPage = pModel->GetPage(static_cast<sal_uInt16>(nTab));    // ist nicht 0

		pModel->AddUndo( new SdrUndoRemoveObj( *pObj ) );	//!	Undo-Kommentar?
		pPage->RemoveObject( pObj->GetOrdNum() );

		//!	Notify etc.???
	}
}

// XEnumerationAccess

uno::Reference<container::XEnumeration> SAL_CALL ScChartsObj::createEnumeration()
													throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
    return new ScIndexEnumeration(this, rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.table.TableChartsEnumeration")));
}

// XIndexAccess

sal_Int32 SAL_CALL ScChartsObj::getCount() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	INT32 nCount = 0;
	if ( pDocShell )
	{
		ScDocument* pDoc = pDocShell->GetDocument();
		ScDrawLayer* pDrawLayer = pDoc->GetDrawLayer();
		if (pDrawLayer)
		{
			SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
			DBG_ASSERT(pPage, "Page nicht gefunden");
			if (pPage)
			{
				SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
				SdrObject* pObject = aIter.Next();
				while (pObject)
				{
					if ( pObject->GetObjIdentifier() == OBJ_OLE2 && pDoc->IsChart(pObject) )
						++nCount;
					pObject = aIter.Next();
				}
			}
		}
	}
	return nCount;
}

uno::Any SAL_CALL ScChartsObj::getByIndex( sal_Int32 nIndex )
							throw(lang::IndexOutOfBoundsException,
									lang::WrappedTargetException, uno::RuntimeException)
{
	ScUnoGuard aGuard;
	uno::Reference<table::XTableChart> xChart(GetObjectByIndex_Impl(nIndex));
	if (xChart.is())
        return uno::makeAny(xChart);
	else
		throw lang::IndexOutOfBoundsException();
//    return uno::Any();
}

uno::Type SAL_CALL ScChartsObj::getElementType() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	return getCppuType((uno::Reference<table::XTableChart>*)0);
}

sal_Bool SAL_CALL ScChartsObj::hasElements() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	return getCount() != 0;
}

uno::Any SAL_CALL ScChartsObj::getByName( const rtl::OUString& aName )
			throw(container::NoSuchElementException,
					lang::WrappedTargetException, uno::RuntimeException)
{
	ScUnoGuard aGuard;
	uno::Reference<table::XTableChart> xChart(GetObjectByName_Impl(aName));
	if (xChart.is())
        return uno::makeAny(xChart);
	else
		throw container::NoSuchElementException();
//    return uno::Any();
}

uno::Sequence<rtl::OUString> SAL_CALL ScChartsObj::getElementNames() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	if (pDocShell)
	{
		ScDocument* pDoc = pDocShell->GetDocument();

		long nCount = getCount();
		uno::Sequence<rtl::OUString> aSeq(nCount);
		rtl::OUString* pAry = aSeq.getArray();

		long nPos = 0;
		ScDrawLayer* pDrawLayer = pDoc->GetDrawLayer();
		if (pDrawLayer)
		{
			SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
			DBG_ASSERT(pPage, "Page nicht gefunden");
			if (pPage)
			{
				SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
				SdrObject* pObject = aIter.Next();
				while (pObject)
				{
					if ( pObject->GetObjIdentifier() == OBJ_OLE2 && pDoc->IsChart(pObject) )
					{
						String aName;
                        uno::Reference < embed::XEmbeddedObject > xObj = ((SdrOle2Obj*)pObject)->GetObjRef();
                        if ( xObj.is() )
                            aName = pDocShell->GetEmbeddedObjectContainer().GetEmbeddedObjectName( xObj );

						DBG_ASSERT(nPos<nCount, "huch, verzaehlt?");
						pAry[nPos++] = aName;
					}
					pObject = aIter.Next();
				}
			}
		}
		DBG_ASSERT(nPos==nCount, "nanu, verzaehlt?");

		return aSeq;
	}
	return uno::Sequence<rtl::OUString>(0);
}

sal_Bool SAL_CALL ScChartsObj::hasByName( const rtl::OUString& aName )
										throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	String aNameString(aName);
	return ( lcl_FindChartObj( pDocShell, nTab, aNameString ) != NULL );
}

//------------------------------------------------------------------------

ScChartObj::ScChartObj(ScDocShell* pDocSh, SCTAB nT, const String& rN) :
	pDocShell( pDocSh ),
	nTab( nT ),
	aChartName( rN )
{
	pDocShell->GetDocument()->AddUnoObject(*this);
}

ScChartObj::~ScChartObj()
{
	if (pDocShell)
		pDocShell->GetDocument()->RemoveUnoObject(*this);
}

void ScChartObj::Notify( SfxBroadcaster&, const SfxHint& rHint )
{
	//!	Referenz-Update

	if ( rHint.ISA( SfxSimpleHint ) &&
			((const SfxSimpleHint&)rHint).GetId() == SFX_HINT_DYING )
	{
		pDocShell = NULL;		// ungueltig geworden
	}
}

void ScChartObj::GetData_Impl( ScRangeListRef& rRanges, bool& rColHeaders, bool& rRowHeaders ) const
{
	bool bFound = false;
    ScDocument* pDoc = (pDocShell? pDocShell->GetDocument(): 0);
    uno::Reference< embed::XEmbeddedObject > xIPObj;
    if( pDoc )
        xIPObj.set( pDoc->FindOleObjectByName( aChartName ), uno::UNO_QUERY );
    if( xIPObj.is())
    {
        //make sure that the chart is loaded
        svt::EmbeddedObjectRef::TryRunningState( xIPObj );

        uno::Reference< chart2::XChartDocument > xChartDoc( xIPObj->getComponent(), uno::UNO_QUERY );
        uno::Reference< chart2::data::XDataReceiver > xReceiver( xChartDoc, uno::UNO_QUERY );
        if( xReceiver.is() )
        {
            uno::Reference< chart2::data::XDataProvider > xProvider = xChartDoc->getDataProvider();
            if( xProvider.is() )
            {
                uno::Sequence< beans::PropertyValue > aArgs( xProvider->detectArguments( xReceiver->getUsedData() ) );

                rtl::OUString aRanges;
                chart::ChartDataRowSource eDataRowSource = chart::ChartDataRowSource_COLUMNS;
                bool bHasCategories=false;
                bool bFirstCellAsLabel=false;
                const beans::PropertyValue* pPropArray = aArgs.getConstArray();
                long nPropCount = aArgs.getLength();
                for (long i = 0; i < nPropCount; i++)
                {
                    const beans::PropertyValue& rProp = pPropArray[i];
                    String aPropName(rProp.Name);

                    if (aPropName.EqualsAscii( "CellRangeRepresentation" ))
                        rProp.Value >>= aRanges;
                    else if (aPropName.EqualsAscii( "DataRowSource" ))
                        eDataRowSource = (chart::ChartDataRowSource)ScUnoHelpFunctions::GetEnumFromAny( rProp.Value );
                    else if (aPropName.EqualsAscii( "HasCategories" ))
                        bHasCategories = ScUnoHelpFunctions::GetBoolFromAny( rProp.Value );
                    else if (aPropName.EqualsAscii( "FirstCellAsLabel" ))
                        bFirstCellAsLabel = ScUnoHelpFunctions::GetBoolFromAny( rProp.Value );
                }

                if( chart::ChartDataRowSource_COLUMNS == eDataRowSource )
                {
                    rColHeaders=bFirstCellAsLabel;
                    rRowHeaders=bHasCategories;
                }
                else
                {
                    rColHeaders=bHasCategories;
                    rRowHeaders=bFirstCellAsLabel;
                }
                rRanges->Parse( aRanges, pDoc);
            }
            bFound = true;
        }
 	}
	if( !bFound )
	{
		rRanges = 0;
		rColHeaders = false;
		rRowHeaders = false;
	}
}

void ScChartObj::Update_Impl( const ScRangeListRef& rRanges, bool bColHeaders, bool bRowHeaders )
{
	if (pDocShell)
	{
		ScDocument* pDoc = pDocShell->GetDocument();
		BOOL bUndo(pDoc->IsUndoEnabled());

		if (bUndo)
		{
			pDocShell->GetUndoManager()->AddUndoAction(
				new ScUndoChartData( pDocShell, aChartName, rRanges, bColHeaders, bRowHeaders, FALSE ) );
		}
		pDoc->UpdateChartArea( aChartName, rRanges, bColHeaders, bRowHeaders, FALSE );
	}
}

// XTableChart

sal_Bool SAL_CALL ScChartObj::getHasColumnHeaders() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xRanges = new ScRangeList;
	bool bColHeaders, bRowHeaders;
	GetData_Impl( xRanges, bColHeaders, bRowHeaders );
	return bColHeaders;
}

void SAL_CALL ScChartObj::setHasColumnHeaders( sal_Bool bHasColumnHeaders )
												throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xRanges = new ScRangeList;
	bool bOldColHeaders, bOldRowHeaders;
	GetData_Impl( xRanges, bOldColHeaders, bOldRowHeaders );
	if ( bOldColHeaders != (bHasColumnHeaders != sal_False) )
		Update_Impl( xRanges, bHasColumnHeaders, bOldRowHeaders );
}

sal_Bool SAL_CALL ScChartObj::getHasRowHeaders() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xRanges = new ScRangeList;
	bool bColHeaders, bRowHeaders;
	GetData_Impl( xRanges, bColHeaders, bRowHeaders );
	return bRowHeaders;
}

void SAL_CALL ScChartObj::setHasRowHeaders( sal_Bool bHasRowHeaders )
												throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xRanges = new ScRangeList;
	bool bOldColHeaders, bOldRowHeaders;
	GetData_Impl( xRanges, bOldColHeaders, bOldRowHeaders );
	if ( bOldRowHeaders != (bHasRowHeaders != sal_False) )
		Update_Impl( xRanges, bOldColHeaders, bHasRowHeaders );
}

uno::Sequence<table::CellRangeAddress> SAL_CALL ScChartObj::getRanges() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xRanges = new ScRangeList;
	bool bColHeaders, bRowHeaders;
	GetData_Impl( xRanges, bColHeaders, bRowHeaders );
	if ( xRanges.Is() )
	{
		ULONG nCount = xRanges->Count();

		table::CellRangeAddress aRangeAddress;
		uno::Sequence<table::CellRangeAddress> aSeq(nCount);
		table::CellRangeAddress* pAry = aSeq.getArray();
		for (USHORT i=0; i<nCount; i++)
		{
			ScRange aRange(*xRanges->GetObject(i));

			aRangeAddress.Sheet		  = aRange.aStart.Tab();
			aRangeAddress.StartColumn = aRange.aStart.Col();
			aRangeAddress.StartRow	  = aRange.aStart.Row();
			aRangeAddress.EndColumn	  = aRange.aEnd.Col();
			aRangeAddress.EndRow	  = aRange.aEnd.Row();

			pAry[i] = aRangeAddress;
		}
		return aSeq;
	}

	DBG_ERROR("ScChartObj::getRanges: keine Ranges");
	return uno::Sequence<table::CellRangeAddress>();
}

void SAL_CALL ScChartObj::setRanges( const uno::Sequence<table::CellRangeAddress>& aRanges )
												throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	ScRangeListRef xOldRanges = new ScRangeList;
	bool bColHeaders, bRowHeaders;
	GetData_Impl( xOldRanges, bColHeaders, bRowHeaders );

	ScRangeList* pList = new ScRangeList;
	USHORT nRangeCount = (USHORT)aRanges.getLength();
	if (nRangeCount)
	{
		const table::CellRangeAddress* pAry = aRanges.getConstArray();
		for (USHORT i=0; i<nRangeCount; i++)
		{
			ScRange aRange( static_cast<SCCOL>(pAry[i].StartColumn), pAry[i].StartRow, pAry[i].Sheet,
							static_cast<SCCOL>(pAry[i].EndColumn),   pAry[i].EndRow,   pAry[i].Sheet );
			pList->Append( aRange );
		}
	}
	ScRangeListRef xNewRanges( pList );

	if ( !xOldRanges.Is() || *xOldRanges != *xNewRanges )
		Update_Impl( xNewRanges, bColHeaders, bRowHeaders );
}

// XEmbeddedObjectSupplier

uno::Reference<lang::XComponent> SAL_CALL ScChartObj::getEmbeddedObject() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	SdrOle2Obj* pObject = lcl_FindChartObj( pDocShell, nTab, aChartName );
    if ( pObject && svt::EmbeddedObjectRef::TryRunningState( pObject->GetObjRef() ) )
	{
        //TODO/LATER: is it OK that something is returned for *all* objects, not only own objects?
        return uno::Reference < lang::XComponent > ( pObject->GetObjRef()->getComponent(), uno::UNO_QUERY );
	}

	return NULL;
}

// XNamed

rtl::OUString SAL_CALL ScChartObj::getName() throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	return aChartName;
}

void SAL_CALL ScChartObj::setName( const rtl::OUString& /* aName */ ) throw(uno::RuntimeException)
{
	ScUnoGuard aGuard;
	throw uno::RuntimeException();		// name cannot be changed
}

//------------------------------------------------------------------------



