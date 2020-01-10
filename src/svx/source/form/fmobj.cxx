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
#include "precompiled_svx.hxx"
#include <tools/resmgr.hxx>
#include <tools/diagnose_ex.h>
#include "fmobj.hxx"
#include "fmprop.hrc"
#include "fmvwimp.hxx"
#include <svx/editeng.hxx>
#include <svx/svdovirt.hxx>

/** === begin UNO includes === **/
#include <com/sun/star/awt/XDevice.hpp>
#include <com/sun/star/script/XEventAttacherManager.hpp>
#include <com/sun/star/io/XPersistObject.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
/** === end UNO includes === **/
#include <svx/fmmodel.hxx>
#include "fmtools.hxx"
#include <tools/shl.hxx>
#include <svx/dialmgr.hxx>

#include "fmresids.hrc"
#include <svx/fmview.hxx>
#include <svx/fmglob.hxx>

#include "fmpgeimp.hxx"
#include <svx/fmpage.hxx>
#include <comphelper/property.hxx>
#include <comphelper/processfactory.hxx>
#include <toolkit/awt/vclxdevice.hxx>
#include <vcl/svapp.hxx>

using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::container;
using namespace ::svxform;

TYPEINIT1(FmFormObj, SdrUnoObj);
DBG_NAME(FmFormObj);
//------------------------------------------------------------------
FmFormObj::FmFormObj(const ::rtl::OUString& rModelName,sal_Int32 _nType)
		  :SdrUnoObj                ( rModelName    )
		  ,m_nPos                   ( -1            )
          ,m_nType                  ( _nType        )
          ,m_pLastKnownRefDevice    ( NULL          )
{
	DBG_CTOR(FmFormObj, NULL);
}

//------------------------------------------------------------------
FmFormObj::FmFormObj( sal_Int32 _nType )
		  :SdrUnoObj                ( String()  )
		  ,m_nPos                   ( -1        )
          ,m_nType                  ( _nType    )
          ,m_pLastKnownRefDevice    ( NULL      )
{
	DBG_CTOR(FmFormObj, NULL);
}

//------------------------------------------------------------------
FmFormObj::~FmFormObj()
{
	DBG_DTOR(FmFormObj, NULL);

	Reference< XComponent> xHistory(m_xEnvironmentHistory, UNO_QUERY);
	if (xHistory.is())
		xHistory->dispose();

	m_xEnvironmentHistory = NULL;
	m_aEventsHistory.realloc(0);
}

//------------------------------------------------------------------
void FmFormObj::SetObjEnv(const Reference< XIndexContainer > & xForm, const sal_Int32 nIdx,
						  const Sequence< ScriptEventDescriptor >& rEvts)
{
	m_xParent = xForm;
	aEvts     = rEvts;
	m_nPos	  = nIdx;
}

//------------------------------------------------------------------
void FmFormObj::ClearObjEnv()
{
	m_xParent.clear();
	aEvts.realloc( 0 );
	m_nPos = -1;
}

//------------------------------------------------------------------
void FmFormObj::impl_isolateControlModel_nothrow()
{
    try
    {
        Reference< XChild > xControlModel( GetUnoControlModel(), UNO_QUERY );
        if ( xControlModel.is() )
        {
    	    Reference< XIndexContainer> xParent( xControlModel->getParent(), UNO_QUERY );
    	    if ( xParent.is() )
	        {
		        sal_Int32 nPos = getElementPos( xParent.get(), xControlModel );
                xParent->removeByIndex( nPos );
            }
        }
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

//------------------------------------------------------------------
void FmFormObj::SetPage(SdrPage* _pNewPage)
{
    if ( GetPage() == _pNewPage )
    {
		SdrUnoObj::SetPage(_pNewPage);
		return;
    }

    FmFormPage* pOldFormPage = PTR_CAST( FmFormPage, GetPage() );
    if ( pOldFormPage )
        pOldFormPage->GetImpl().formObjectRemoved( *this );

    FmFormPage* pNewFormPage = PTR_CAST( FmFormPage, _pNewPage );
	if ( !pNewFormPage )
	{	// Maybe it makes sense to create an environment history here : if somebody set's our page to NULL, and we have a valid page before,
		// me may want to remember our place within the old page. For this we could create a new m_xEnvironmentHistory to store it.
		// So the next SetPage with a valid new page would restore that environment within the new page.
		// But for the original Bug (#57300#) we don't need that, so I omit it here. Maybe this will be implemented later.
        impl_isolateControlModel_nothrow();
		SdrUnoObj::SetPage(_pNewPage);
        return;
	}

    Reference< XIndexContainer >        xNewPageForms( pNewFormPage->GetForms( true ), UNO_QUERY );
	Reference< XIndexContainer > 	    xNewParent;
	Sequence< ScriptEventDescriptor>	aNewEvents;

	// calc the new parent for my model (within the new page's forms hierarchy)
	// do we have a history ? (from :Clone)
	if ( m_xEnvironmentHistory.is() )
	{
		// the element in m_xEnvironmentHistory which is equivalent to my new parent (which (perhaps) has to be created within _pNewPage->GetForms)
		// is the right-most element in the tree.
	    Reference< XIndexContainer > xRightMostLeaf = m_xEnvironmentHistory;
        try
        {
			while ( xRightMostLeaf->getCount() )
            {
                xRightMostLeaf.set(
                    xRightMostLeaf->getByIndex( xRightMostLeaf->getCount() - 1 ),
                    UNO_QUERY_THROW
                );
            }

		    xNewParent.set( ensureModelEnv( xRightMostLeaf, xNewPageForms ), UNO_QUERY_THROW );

            // we successfully cloned the environment in m_xEnvironmentHistory, so we can use m_aEventsHistory
		    // (which describes the events of our model at the moment m_xEnvironmentHistory was created)
		    aNewEvents = m_aEventsHistory;
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
	}

	if ( !xNewParent.is() )
	{
		// are we a valid part of our current page forms ?
		Reference< XIndexContainer > xOldForms;
        if ( pOldFormPage )
            xOldForms.set( pOldFormPage->GetForms(), UNO_QUERY_THROW );

		if ( xOldForms.is() )
		{
			// search (upward from our model) for xOldForms
			Reference< XChild > xSearch( GetUnoControlModel(), UNO_QUERY );
			while (xSearch.is())
			{
				if ( xSearch == xOldForms )
					break;
				xSearch = Reference< XChild >( xSearch->getParent(), UNO_QUERY );
			}
			if ( xSearch.is() )	// implies xSearch == xOldForms, which means we're a valid part of our current page forms hierarchy
			{
				Reference< XChild >  xMeAsChild( GetUnoControlModel(), UNO_QUERY );
				xNewParent.set( ensureModelEnv( xMeAsChild->getParent(), xNewPageForms ), UNO_QUERY );

				if ( xNewParent.is() )
				{
					try
					{
						// transfer the events from our (model's) parent to the new (model's) parent, too
						Reference< XEventAttacherManager >  xEventManager(xMeAsChild->getParent(), UNO_QUERY);
						Reference< XIndexAccess >  xManagerAsIndex(xEventManager, UNO_QUERY);
						if (xManagerAsIndex.is())
						{
							sal_Int32 nPos = getElementPos(xManagerAsIndex, xMeAsChild);
							if (nPos >= 0)
								aNewEvents = xEventManager->getScriptEvents(nPos);
						}
						else
							aNewEvents = aEvts;
					}
                    catch( const Exception& )
                    {
                    	DBG_UNHANDLED_EXCEPTION();
                    }
				}
			}
		}
	}

	// now set the page
	SdrUnoObj::SetPage(_pNewPage);

	// place my model within the new parent container
	if (xNewParent.is())
	{
		Reference< XFormComponent >  xMeAsFormComp(GetUnoControlModel(), UNO_QUERY);
		if (xMeAsFormComp.is())
		{
			// check if I have another parent (and remove me, if neccessary)
			Reference< XIndexContainer >  xOldParent(xMeAsFormComp->getParent(), UNO_QUERY);
			if (xOldParent.is())
			{
				sal_Int32 nPos = getElementPos(Reference< XIndexAccess > (xOldParent, UNO_QUERY), xMeAsFormComp);
				if (nPos > -1)
					xOldParent->removeByIndex(nPos);
			}
			// and insert into the new container
			xNewParent->insertByIndex(xNewParent->getCount(), makeAny(xMeAsFormComp));

			// transfer the events
			if (aNewEvents.getLength())
			{
				try
				{
					Reference< XEventAttacherManager >  xEventManager(xNewParent, UNO_QUERY);
					Reference< XIndexAccess >  xManagerAsIndex(xEventManager, UNO_QUERY);
					if (xManagerAsIndex.is())
					{
						sal_Int32 nPos = getElementPos(xManagerAsIndex, xMeAsFormComp);
						DBG_ASSERT(nPos >= 0, "FmFormObj::SetPage : inserted but not present ?");
						xEventManager->registerScriptEvents(nPos, aNewEvents);
					}
				}
                catch( const Exception& )
                {
                	DBG_UNHANDLED_EXCEPTION();
                }
				
			}
		}
	}

	// delete my history
	Reference< XComponent> xHistory(m_xEnvironmentHistory, UNO_QUERY);
	if (xHistory.is())
		xHistory->dispose();

	m_xEnvironmentHistory = NULL;
	m_aEventsHistory.realloc(0);

    if ( pNewFormPage )
        pNewFormPage->GetImpl().formObjectInserted( *this );
}

//------------------------------------------------------------------
sal_uInt32 FmFormObj::GetObjInventor()   const
{
	return FmFormInventor;
}

//------------------------------------------------------------------
sal_uInt16 FmFormObj::GetObjIdentifier() const
{
    return OBJ_UNO;
}

//------------------------------------------------------------------
void FmFormObj::clonedFrom(const FmFormObj* _pSource)
{
	DBG_ASSERT(_pSource != NULL, "FmFormObj::clonedFrom : invalid source !");
	Reference< XComponent> xHistory(m_xEnvironmentHistory, UNO_QUERY);
	if (xHistory.is())
		xHistory->dispose();

	m_xEnvironmentHistory = NULL;
	m_aEventsHistory.realloc(0);

	Reference< XChild >  xSourceAsChild(_pSource->GetUnoControlModel(), UNO_QUERY);
	if (!xSourceAsChild.is())
		return;

	Reference< XInterface >  xSourceContainer = xSourceAsChild->getParent();

	m_xEnvironmentHistory = Reference< XIndexContainer >(
		::comphelper::getProcessServiceFactory()->createInstance(::rtl::OUString::createFromAscii("com.sun.star.form.Forms")), 
		UNO_QUERY);
	DBG_ASSERT(m_xEnvironmentHistory.is(), "FmFormObj::clonedFrom : could not create a forms collection !");

	if (m_xEnvironmentHistory.is())
	{
		ensureModelEnv(xSourceContainer, m_xEnvironmentHistory);
		m_aEventsHistory = aEvts;
			// if we we're clone there was a call to operator=, so aEvts are excatly the events we need here ...
	}
}

//------------------------------------------------------------------
SdrObject* FmFormObj::Clone() const
{
	SdrObject* pReturn = SdrUnoObj::Clone();

	FmFormObj* pFormObject = PTR_CAST(FmFormObj, pReturn);
	DBG_ASSERT(pFormObject != NULL, "FmFormObj::Clone : invalid clone !");
	if (pFormObject)
		pFormObject->clonedFrom(this);

	return pReturn;
}

//------------------------------------------------------------------
void FmFormObj::ReformatText()
{
	const FmFormModel* pFormModel = PTR_CAST( FmFormModel, GetModel() );
    OutputDevice* pCurrentRefDevice = pFormModel ? pFormModel->GetRefDevice() : NULL;

    if ( m_pLastKnownRefDevice != pCurrentRefDevice )
    {
        m_pLastKnownRefDevice = pCurrentRefDevice;

        try
        {
            Reference< XPropertySet > xModelProps( GetUnoControlModel(), UNO_QUERY );
            Reference< XPropertySetInfo > xPropertyInfo;
            if ( xModelProps.is() )
                xPropertyInfo = xModelProps->getPropertySetInfo();

            const ::rtl::OUString sRefDevicePropName( RTL_CONSTASCII_USTRINGPARAM( "ReferenceDevice" ) );
            if ( xPropertyInfo.is() && xPropertyInfo->hasPropertyByName( sRefDevicePropName ) )
            {
                VCLXDevice* pUnoRefDevice = new VCLXDevice;
                pUnoRefDevice->SetOutputDevice( m_pLastKnownRefDevice );
                Reference< XDevice > xRefDevice( pUnoRefDevice );
                xModelProps->setPropertyValue( sRefDevicePropName, makeAny( xRefDevice ) );
            }
        }
        catch( const Exception& )
        {
        	OSL_ENSURE( sal_False, "FmFormObj::ReformatText: caught an exception!" );
        }
    }

    SdrUnoObj::ReformatText();
}

//------------------------------------------------------------------
void FmFormObj::operator= (const SdrObject& rObj)
{
	SdrUnoObj::operator= (rObj);

	FmFormObj* pFormObj = PTR_CAST(FmFormObj, &rObj);
	if (pFormObj)
	{
		// liegt das UnoControlModel in einer Eventumgebung,
		// dann koennen noch Events zugeordnet sein
		Reference< XFormComponent >  xContent(pFormObj->xUnoControlModel, UNO_QUERY);
		if (xContent.is())
		{
			Reference< XEventAttacherManager >  xManager(xContent->getParent(), UNO_QUERY);
			Reference< XIndexAccess >  xManagerAsIndex(xManager, UNO_QUERY);
			if (xManagerAsIndex.is())
			{
				sal_Int32 nPos = getElementPos( xManagerAsIndex, xContent );
				if ( nPos >= 0 )
					aEvts = xManager->getScriptEvents( nPos );
			}
		}
		else
			aEvts = pFormObj->aEvts;
	}
}

//------------------------------------------------------------------
Reference< XInterface >  FmFormObj::ensureModelEnv(const Reference< XInterface > & _rSourceContainer, const ::com::sun::star::uno::Reference< ::com::sun::star::container::XIndexContainer >  _rTopLevelDestContainer)
{
	Reference< XInterface >  xTopLevelSouce;
	String sAccessPath = getFormComponentAccessPath(_rSourceContainer, xTopLevelSouce);
	if (!xTopLevelSouce.is())
		// somthing went wrong, maybe _rSourceContainer isn't part of a valid forms hierarchy
		return Reference< XInterface > ();

	Reference< XIndexContainer >  xDestContainer(_rTopLevelDestContainer);
	Reference< XIndexContainer >  xSourceContainer(xTopLevelSouce, UNO_QUERY);
	DBG_ASSERT(xSourceContainer.is(), "FmFormObj::ensureModelEnv : the top level source is invalid !");

	for (xub_StrLen i=0; i<sAccessPath.GetTokenCount('\\'); ++i)
	{
		sal_uInt16 nIndex = (sal_uInt16)sAccessPath.GetToken(i, '\\').ToInt32();

		// get the DSS of the source form (we have to find an aquivalent for)
		DBG_ASSERT(nIndex<xSourceContainer->getCount(), "FmFormObj::ensureModelEnv : invalid access path !");
		Reference< XPropertySet >  xSourceForm;
		xSourceContainer->getByIndex(nIndex) >>= xSourceForm;
		DBG_ASSERT(xSourceForm.is(), "FmFormObj::ensureModelEnv : invalid source form !");

		Any aSrcCursorSource, aSrcCursorSourceType, aSrcDataSource;
		DBG_ASSERT(::comphelper::hasProperty(FM_PROP_COMMAND, xSourceForm) && ::comphelper::hasProperty(FM_PROP_COMMANDTYPE, xSourceForm)
			&& ::comphelper::hasProperty(FM_PROP_DATASOURCE, xSourceForm), "FmFormObj::ensureModelEnv : invalid access path or invalid form (missing props) !");
			// the parent access path should refer to a row set
		try
		{
			aSrcCursorSource		= xSourceForm->getPropertyValue(FM_PROP_COMMAND);
			aSrcCursorSourceType	= xSourceForm->getPropertyValue(FM_PROP_COMMANDTYPE);
			aSrcDataSource			= xSourceForm->getPropertyValue(FM_PROP_DATASOURCE);
		}
		catch(Exception&)
		{
			DBG_ERROR("FmFormObj::ensureModelEnv : could not retrieve a source DSS !");
		}
		

		// calc the number of (source) form siblings with the same DSS
		Reference< XPropertySet >  xCurrentSourceForm, xCurrentDestForm;
		sal_Int16 nCurrentSourceIndex = 0, nCurrentDestIndex = 0;
		while (nCurrentSourceIndex <= nIndex)
		{
			sal_Bool bEqualDSS = sal_False;
			while (!bEqualDSS)	// (we don't have to check nCurrentSourceIndex here : it's bounded by nIndex)
			{
				xSourceContainer->getByIndex(nCurrentSourceIndex) >>= xCurrentSourceForm;
				DBG_ASSERT(xCurrentSourceForm.is(), "FmFormObj::ensureModelEnv : invalid form ancestor (2) !");
				bEqualDSS = sal_False;
				if (::comphelper::hasProperty(FM_PROP_DATASOURCE, xCurrentSourceForm))
				{	// it is a form
					try
					{
						if	(	::comphelper::compare(xCurrentSourceForm->getPropertyValue(FM_PROP_COMMAND), aSrcCursorSource)
							&&	::comphelper::compare(xCurrentSourceForm->getPropertyValue(FM_PROP_COMMANDTYPE), aSrcCursorSourceType)
							&&	::comphelper::compare(xCurrentSourceForm->getPropertyValue(FM_PROP_DATASOURCE), aSrcDataSource)
							)
						{
							bEqualDSS = sal_True;
						}
					}
					catch(Exception&)
					{
						DBG_ERROR("FmFormObj::ensureModelEnv : exception while getting a sibling's DSS !");
					}
					
				}
				++nCurrentSourceIndex;
			}

			DBG_ASSERT(bEqualDSS, "FmFormObj::ensureModelEnv : found no source form !");
			// ??? at least the nIndex-th one should have been found ???

			// now search the next one with the given DSS (within the destination container)
			bEqualDSS = sal_False;
			while (!bEqualDSS && (nCurrentDestIndex < xDestContainer->getCount()))
			{
				xDestContainer->getByIndex(nCurrentDestIndex) >>= xCurrentDestForm;
				DBG_ASSERT(xCurrentDestForm.is(), "FmFormObj::ensureModelEnv : invalid destination form !");
				bEqualDSS = sal_False;
				if (::comphelper::hasProperty(FM_PROP_DATASOURCE, xCurrentDestForm))
				{	// it is a form
					try
					{
						if	(	::comphelper::compare(xCurrentDestForm->getPropertyValue(FM_PROP_COMMAND), aSrcCursorSource)
							&&	::comphelper::compare(xCurrentDestForm->getPropertyValue(FM_PROP_COMMANDTYPE), aSrcCursorSourceType)
							&&	::comphelper::compare(xCurrentDestForm->getPropertyValue(FM_PROP_DATASOURCE), aSrcDataSource)
							)
						{
							bEqualDSS = sal_True;
						}
					}
					catch(Exception&)
					{
						DBG_ERROR("FmFormObj::ensureModelEnv : exception while getting a destination DSS !");
					}
					
				}
				++nCurrentDestIndex;
			}

			if (!bEqualDSS)
			{	// There is at least one more source form with the given DSS than destination forms are.
				// correct this ...
				try
				{
					Reference< XPersistObject >  xSourcePersist(xCurrentSourceForm, UNO_QUERY);
					DBG_ASSERT(xSourcePersist.is(), "FmFormObj::ensureModelEnv : invalid form (no persist object) !");

					// create and insert (into the destination) a clone of the form
					xCurrentDestForm = Reference< XPropertySet > (cloneUsingProperties(xSourcePersist), UNO_QUERY);
					DBG_ASSERT(xCurrentDestForm.is(), "FmFormObj::ensureModelEnv : invalid cloned form !");

					DBG_ASSERT(nCurrentDestIndex == xDestContainer->getCount(), "FmFormObj::ensureModelEnv : something went wrong with the numbers !");
					xDestContainer->insertByIndex(nCurrentDestIndex, makeAny(xCurrentDestForm));

					++nCurrentDestIndex;
						// like nCurrentSourceIndex, nCurrentDestIndex now points 'behind' the form it actally means
				}
				catch(Exception&)
				{
					DBG_ERROR("FmFormObj::ensureModelEnv : something went seriously wrong while creating a new form !");
					// no more options anymore ...
					return Reference< XInterface > ();
				}
				
			}
		}

		// now xCurrentDestForm is a form aequivalent to xSourceForm (which means they have the same DSS and the same number
		// of left siblings with the same DSS, which counts for all their ancestors, too)

		// go down
		xDestContainer = Reference< XIndexContainer > (xCurrentDestForm, UNO_QUERY);
		xSourceContainer = Reference< XIndexContainer > (xSourceForm, UNO_QUERY);
		DBG_ASSERT(xDestContainer.is() && xSourceContainer.is(), "FmFormObj::ensureModelEnv : invalid container !");
	}

	return Reference< XInterface > (xDestContainer, UNO_QUERY);
}

//------------------------------------------------------------------
FmFormObj* FmFormObj::GetFormObject( SdrObject* _pSdrObject )
{
    FmFormObj* pFormObject = dynamic_cast< FmFormObj* >( _pSdrObject );
    if ( !pFormObject )
    {
        SdrVirtObj* pVirtualObject = dynamic_cast< SdrVirtObj* >( _pSdrObject );
        if ( pVirtualObject )
            pFormObject = dynamic_cast< FmFormObj* >( &pVirtualObject->ReferencedObj() );
    }
    return pFormObject;
}

//------------------------------------------------------------------
const FmFormObj* FmFormObj::GetFormObject( const SdrObject* _pSdrObject )
{
    const FmFormObj* pFormObject = dynamic_cast< const FmFormObj* >( _pSdrObject );
    if ( !pFormObject )
    {
        const SdrVirtObj* pVirtualObject = dynamic_cast< const SdrVirtObj* >( _pSdrObject );
        if ( pVirtualObject )
            pFormObject = dynamic_cast< const FmFormObj* >( &pVirtualObject->GetReferencedObj() );
    }
    return pFormObject;
}

//------------------------------------------------------------------
void FmFormObj::SetUnoControlModel( const Reference< com::sun::star::awt::XControlModel >& _rxModel )
{
    SdrUnoObj::SetUnoControlModel( _rxModel );

    // TODO: call something like formObjectInserted at the form page, to tell it the new model
}

//------------------------------------------------------------------
FASTBOOL FmFormObj::EndCreate( SdrDragStat& rStat, SdrCreateCmd eCmd )
{
	bool bResult = SdrUnoObj::EndCreate(rStat, eCmd);
	if ( bResult && SDRCREATE_FORCEEND == eCmd && rStat.GetView() )
	{
        if ( pPage )
        {
            FmFormPage& rPage = dynamic_cast< FmFormPage& >( *pPage );

            try
            {
                Reference< XFormComponent >  xContent( xUnoControlModel, UNO_QUERY_THROW );
                Reference< XForm > xParentForm( xContent->getParent(), UNO_QUERY );

                Reference< XIndexContainer > xFormToInsertInto;

                if ( !xParentForm.is() )
                {   // model is not yet part of a form component hierachy
                    xParentForm.set( rPage.GetImpl().findPlaceInFormComponentHierarchy( xContent ), UNO_SET_THROW );
                    xFormToInsertInto.set( xParentForm, UNO_QUERY_THROW );
		        }

                rPage.GetImpl().setUniqueName( xContent, xParentForm );

                if ( xFormToInsertInto.is() )
			        xFormToInsertInto->insertByIndex( xFormToInsertInto->getCount(), makeAny( xContent ) );
            }
            catch( const Exception& )
            {
            	DBG_UNHANDLED_EXCEPTION();
            }
        }

        FmFormView* pView( dynamic_cast< FmFormView* >( rStat.GetView() ) );
        FmXFormView* pViewImpl = pView ? pView->GetImpl() : NULL;
        OSL_ENSURE( pViewImpl, "FmFormObj::EndCreate: no view!?" );
        if ( pViewImpl )
            pViewImpl->onCreatedFormObject( *this );
	}
	return bResult;
}

//------------------------------------------------------------------------------
void FmFormObj::BrkCreate( SdrDragStat& rStat )
{
    SdrUnoObj::BrkCreate( rStat );
    impl_isolateControlModel_nothrow();
}

// -----------------------------------------------------------------------------
sal_Int32 FmFormObj::getType() const
{
	return m_nType;
}

// -----------------------------------------------------------------------------
// #i70852# overload Layer interface to force to FormColtrol layer

SdrLayerID FmFormObj::GetLayer() const
{
	// #i72535#
	// i70852 was too radical, in SW obects (and thus, FormControls, too)
	// get moved to invisible layers to hide them (e.g. in hidden sections).
	// This means that form controls ARE allowed to be on other layers than
	// the form control layer ATM and that being member of form control layer
	// is no criteria to find all FormControls of a document.
	// To fix, use parent functionality
	return SdrUnoObj::GetLayer();
}

void FmFormObj::NbcSetLayer(SdrLayerID nLayer)
{
	// #i72535#
	// See above. To fix, use parent functionality
	return SdrUnoObj::NbcSetLayer(nLayer);
}

// eof
