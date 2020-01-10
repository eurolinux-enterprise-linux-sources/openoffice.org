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

#include "dlg_CreationWizard.hxx"
#include "dlg_CreationWizard.hrc"
#include "ResId.hxx"
#include "macros.hxx"
#include "Strings.hrc"
#include "HelpIds.hrc"

#include "tp_ChartType.hxx"
#include "tp_RangeChooser.hxx"
#include "tp_Wizard_TitlesAndObjects.hxx"
#include "tp_Location.hxx"

#include "tp_DataSource.hxx"
#include "ChartTypeTemplateProvider.hxx"
#include "DialogModel.hxx"

//.............................................................................
namespace chart
{
//.............................................................................
using namespace ::com::sun::star;

//#define LOCATION_PAGE 1

#define PATH_FULL   1
#define STATE_FIRST        0
#define STATE_CHARTTYPE    STATE_FIRST
#define STATE_SIMPLE_RANGE 1
#define STATE_DATA_SERIES  2
#define STATE_OBJECTS      3
#define STATE_LOCATION     4

#ifdef LOCATION_PAGE
#define STATE_LAST         STATE_LOCATION
#else
#define STATE_LAST         STATE_OBJECTS
#endif

namespace
{
#ifdef LOCATION_PAGE
    const sal_Int32 nPageCount = 5;
#else
    const sal_Int32 nPageCount = 4;
#endif
}

CreationWizard::CreationWizard( Window* pParent, const uno::Reference< frame::XModel >& xChartModel
                               , const uno::Reference< uno::XComponentContext >& xContext
                               , sal_Int32 nOnePageOnlyIndex )
                : svt::RoadmapWizard( pParent, SchResId(DLG_CHART_WIZARD)
                    , ( nOnePageOnlyIndex >= 0 && nOnePageOnlyIndex < nPageCount )
                        ?  WZB_HELP | WZB_CANCEL | WZB_FINISH
                        :  WZB_HELP | WZB_CANCEL | WZB_PREVIOUS | WZB_NEXT | WZB_FINISH
                  )
                , m_xChartModel(xChartModel,uno::UNO_QUERY)
                , m_xCC( xContext )
                , m_bIsClosable(true)
                , m_nOnePageOnlyIndex(nOnePageOnlyIndex)
                , m_pTemplateProvider(0)
                , m_nFirstState(STATE_FIRST)
                , m_nLastState(STATE_LAST)
                , m_aTimerTriggeredControllerLock( xChartModel )
                , m_bCanTravel( true )
{
    m_apDialogModel.reset( new DialogModel( m_xChartModel, m_xCC ));
    // Do not call FreeResource(), because there are no sub-elements defined in
    // the dialog resource
    ShowButtonFixedLine( TRUE );
    defaultButton( WZB_FINISH );

    if( m_nOnePageOnlyIndex < 0 || m_nOnePageOnlyIndex >= nPageCount )
    {
        m_nOnePageOnlyIndex = -1;
        this->setTitleBase(String(SchResId(STR_DLG_CHART_WIZARD)));
    }
    else
        this->setTitleBase(String());

    declarePath( PATH_FULL
        , STATE_CHARTTYPE
        , STATE_SIMPLE_RANGE
        , STATE_DATA_SERIES
        , STATE_OBJECTS
#ifdef LOCATION_PAGE
        , STATE_LOCATION
#endif
        , WZS_INVALID_STATE
    );
    this->SetRoadmapSmartHelpId( SmartId( HID_SCH_WIZARD_ROADMAP ) );
    this->SetRoadmapInteractive( sal_True );
    Size aAdditionalRoadmapSize( LogicToPixel( Size( 85, 0 ), MAP_APPFONT ) );
    Size aSize( this->GetSizePixel() );
    aSize.Width() += aAdditionalRoadmapSize.Width();
    this->SetSizePixel( aSize );

    uno::Reference< chart2::XChartDocument > xChartDoc( m_xChartModel, uno::UNO_QUERY );
    bool bHasOwnData = (xChartDoc.is() && xChartDoc->hasInternalDataProvider());

    if( bHasOwnData )
    {
        this->enableState( STATE_SIMPLE_RANGE, false );
        this->enableState( STATE_DATA_SERIES, false );
    }

    // Call ActivatePage, to create and activate the first page
    ActivatePage();
}
CreationWizard::~CreationWizard()
{
}

svt::OWizardPage* CreationWizard::createPage(WizardState nState)
{
    svt::OWizardPage* pRet = 0;
    if(m_nOnePageOnlyIndex!=-1 && m_nOnePageOnlyIndex!=nState)
        return pRet;
    bool bDoLiveUpdate = m_nOnePageOnlyIndex == -1;
    switch( nState )
    {
    case STATE_CHARTTYPE:
        {
        m_aTimerTriggeredControllerLock.startTimer();
        ChartTypeTabPage* pChartTypeTabPage = new ChartTypeTabPage(this,m_xChartModel,m_xCC,bDoLiveUpdate);
        pRet  = pChartTypeTabPage;
        m_pTemplateProvider = pChartTypeTabPage;
        if( m_pTemplateProvider &&
            m_apDialogModel.get() )
            m_apDialogModel->setTemplate( m_pTemplateProvider->getCurrentTemplate());
        }
        break;
    case STATE_SIMPLE_RANGE:
        {
        m_aTimerTriggeredControllerLock.startTimer();
        pRet = new RangeChooserTabPage(this,*(m_apDialogModel.get()),m_pTemplateProvider,this);
        }
        break;
    case STATE_DATA_SERIES:
        {
        m_aTimerTriggeredControllerLock.startTimer();
        pRet = new DataSourceTabPage(this,*(m_apDialogModel.get()),m_pTemplateProvider,this);
        }
        break;
    case STATE_OBJECTS:
        {
        pRet  = new TitlesAndObjectsTabPage(this,m_xChartModel,m_xCC);
        m_aTimerTriggeredControllerLock.startTimer();
        }
        break;
#ifdef LOCATION_PAGE
    case STATE_LOCATION:
        {
        m_aTimerTriggeredControllerLock.startTimer();
        pRet  = new LocationTabPage(this,m_xChartModel,m_xCC);
        }
        break;
#endif
    default:
        break;
    }
    if(pRet)
        pRet->SetText(String());//remove title of pages to not get them in the wizard title
    return pRet;
}

sal_Bool CreationWizard::leaveState( WizardState /*_nState*/ )
{
    return m_bCanTravel;
}

svt::WizardTypes::WizardState CreationWizard::determineNextState( WizardState nCurrentState ) const
{
    if( !m_bCanTravel )
        return WZS_INVALID_STATE;
    if( nCurrentState == m_nLastState )
        return WZS_INVALID_STATE;
    svt::WizardTypes::WizardState nNextState = nCurrentState + 1;
    while( !isStateEnabled( nNextState ) && nNextState <= m_nLastState )
        ++nNextState;
    return (nNextState==m_nLastState+1) ? WZS_INVALID_STATE : nNextState;
}
void CreationWizard::enterState(WizardState nState)
{
    m_aTimerTriggeredControllerLock.startTimer();
    enableButtons( WZB_PREVIOUS, bool( nState > m_nFirstState ) );
    enableButtons( WZB_NEXT, bool( nState < m_nLastState ) );
    if( isStateEnabled( nState ))
        svt::RoadmapWizard::enterState(nState);
}

bool CreationWizard::isClosable()
{
    //@todo
	return m_bIsClosable;
}

void CreationWizard::setInvalidPage( TabPage * /* pTabPage */ )
{
    m_bCanTravel = false;
}

void CreationWizard::setValidPage( TabPage * /* pTabPage */ )
{
    m_bCanTravel = true;
}

String CreationWizard::getStateDisplayName( WizardState nState ) const
{
    USHORT nResId = 0;
    switch( nState )
    {
    case STATE_CHARTTYPE:
        nResId = STR_PAGE_CHARTTYPE;
        break;
    case STATE_SIMPLE_RANGE:
        nResId = STR_PAGE_DATA_RANGE;
        break;
    case STATE_DATA_SERIES:
        nResId = STR_OBJECT_DATASERIES_PLURAL;
        break;
    case STATE_OBJECTS:
        nResId = STR_PAGE_CHART_ELEMENTS;
        break;
#ifdef LOCATION_PAGE
    case STATE_LOCATION:
        nResId = STR_PAGE_CHART_LOCATION;
        break;
#endif
    default:
        break;
    }
    return String(SchResId(nResId));
}

//.............................................................................
} //namespace chart
//.............................................................................
