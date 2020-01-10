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
#include <tools/gen.hxx>
#include "AccessiblePageHeaderArea.hxx"
#include "AccessibleText.hxx"
#include "AccessibilityHints.hxx"
#include "unoguard.hxx"
#include "editsrc.hxx"
#include "prevwsh.hxx"
#include "prevloc.hxx"
#include "scresid.hxx"
#ifndef SC_SC_HRC
#include "sc.hrc"
#endif

#ifndef _COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLEROLE_HPP_
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#endif
#ifndef _COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLESTATETYPE_HPP_
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#endif
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <svx/editobj.hxx>
#include <svx/AccessibleTextHelper.hxx>
#include <rtl/uuid.h>
#include <unotools/accessiblestatesethelper.hxx>
#include <rtl/ustrbuf.hxx>
#include <toolkit/helper/convert.hxx>

using namespace	::com::sun::star;
using namespace	::com::sun::star::accessibility;

	//=====  internal  ========================================================

ScAccessiblePageHeaderArea::ScAccessiblePageHeaderArea(
        const uno::Reference<XAccessible>& rxParent,
        ScPreviewShell* pViewShell,
        const EditTextObject* pEditObj,
        sal_Bool bHeader,
        SvxAdjust eAdjust)
        : ScAccessibleContextBase(rxParent, AccessibleRole::TEXT),
        mpEditObj(pEditObj->Clone()),
    	mpTextHelper(NULL),
        mpViewShell(pViewShell),
        mbHeader(bHeader),
        meAdjust(eAdjust)
{
	if (mpViewShell)
		mpViewShell->AddAccessibilityObject(*this);
}

ScAccessiblePageHeaderArea::~ScAccessiblePageHeaderArea(void)
{
	if (!ScAccessibleContextBase::IsDefunc() && !rBHelper.bInDispose)
	{
		// increment refcount to prevent double call off dtor
		osl_incrementInterlockedCount( &m_refCount );
		dispose();
	}
}

void SAL_CALL ScAccessiblePageHeaderArea::disposing()
{
    ScUnoGuard aGuard;
	if (mpViewShell)
	{
		mpViewShell->RemoveAccessibilityObject(*this);
		mpViewShell = NULL;
	}
	if (mpTextHelper)
		DELETEZ(mpTextHelper);
    if (mpEditObj)
        DELETEZ(mpEditObj);

	ScAccessibleContextBase::disposing();
}

//=====  SfxListener  =====================================================

void ScAccessiblePageHeaderArea::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
{
	if (rHint.ISA( SfxSimpleHint ) )
	{
		const SfxSimpleHint& rRef = (const SfxSimpleHint&)rHint;
		// only notify if child exist, otherwise it is not necessary
        if (rRef.GetId() == SC_HINT_ACC_VISAREACHANGED)
        {
            if (mpTextHelper)
                mpTextHelper->UpdateChildren();

            AccessibleEventObject aEvent;
            aEvent.EventId = AccessibleEventId::VISIBLE_DATA_CHANGED;
			aEvent.Source = uno::Reference< XAccessibleContext >(this);
            CommitChange(aEvent);
        }
    }
	ScAccessibleContextBase::Notify(rBC, rHint);
}
	//=====  XAccessibleComponent  ============================================

uno::Reference< XAccessible > SAL_CALL ScAccessiblePageHeaderArea::getAccessibleAtPoint(
		const awt::Point& rPoint )
		throw (uno::RuntimeException)
{
    uno::Reference<XAccessible> xRet;
    if (containsPoint(rPoint))
    {
 	    ScUnoGuard aGuard;
        IsObjectValid();

    	if(!mpTextHelper)
	    	CreateTextHelper();

        xRet = mpTextHelper->GetAt(rPoint);
    }

    return xRet;
}

	//=====  XAccessibleContext  ==============================================

sal_Int32 SAL_CALL
	ScAccessiblePageHeaderArea::getAccessibleChildCount(void)
    				throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	if (!mpTextHelper)
		CreateTextHelper();
	return mpTextHelper->GetChildCount();
}

uno::Reference< XAccessible > SAL_CALL
	ScAccessiblePageHeaderArea::getAccessibleChild(sal_Int32 nIndex)
        throw (uno::RuntimeException,
		lang::IndexOutOfBoundsException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	if (!mpTextHelper)
		CreateTextHelper();
	return mpTextHelper->GetChild(nIndex);
}

uno::Reference<XAccessibleStateSet> SAL_CALL
	ScAccessiblePageHeaderArea::getAccessibleStateSet(void)
    throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
	uno::Reference<XAccessibleStateSet> xParentStates;
	if (getAccessibleParent().is())
	{
		uno::Reference<XAccessibleContext> xParentContext = getAccessibleParent()->getAccessibleContext();
		xParentStates = xParentContext->getAccessibleStateSet();
	}
	utl::AccessibleStateSetHelper* pStateSet = new utl::AccessibleStateSetHelper();
	if (IsDefunc())
		pStateSet->AddState(AccessibleStateType::DEFUNC);
    else
    {
	    pStateSet->AddState(AccessibleStateType::ENABLED);
	    pStateSet->AddState(AccessibleStateType::MULTI_LINE);
	    if (isShowing())
		    pStateSet->AddState(AccessibleStateType::SHOWING);
	    if (isVisible())
		    pStateSet->AddState(AccessibleStateType::VISIBLE);
    }
	return pStateSet;
}

//=====  XServiceInfo  ========================================================

::rtl::OUString SAL_CALL
   	ScAccessiblePageHeaderArea::getImplementationName(void)
    throw (uno::RuntimeException)
{
    return rtl::OUString(RTL_CONSTASCII_USTRINGPARAM ("ScAccessiblePageHeaderArea"));
}

uno::Sequence< ::rtl::OUString> SAL_CALL
   	ScAccessiblePageHeaderArea::getSupportedServiceNames(void)
    throw (uno::RuntimeException)
{
	uno::Sequence< ::rtl::OUString > aSequence = ScAccessibleContextBase::getSupportedServiceNames();
    sal_Int32 nOldSize(aSequence.getLength());
    aSequence.realloc(nOldSize + 1);
    ::rtl::OUString* pNames = aSequence.getArray();

	pNames[nOldSize] = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.sheet.AccessiblePageHeaderFooterAreasView"));

	return aSequence;
}

//=====  XTypeProvider  =======================================================

uno::Sequence<sal_Int8> SAL_CALL
	ScAccessiblePageHeaderArea::getImplementationId(void)
    throw (uno::RuntimeException)
{
    ScUnoGuard aGuard;
    IsObjectValid();
	static uno::Sequence<sal_Int8> aId;
	if (aId.getLength() == 0)
	{
		aId.realloc (16);
		rtl_createUuid (reinterpret_cast<sal_uInt8 *>(aId.getArray()), 0, sal_True);
	}
	return aId;
}

//===== internal ==============================================================
rtl::OUString SAL_CALL ScAccessiblePageHeaderArea::createAccessibleDescription(void) 
    throw(uno::RuntimeException)
{
    rtl::OUString sDesc;
    switch (meAdjust)
    {
    case SVX_ADJUST_LEFT :
        sDesc = String(ScResId(STR_ACC_LEFTAREA_DESCR));
        break;
    case SVX_ADJUST_RIGHT:
        sDesc = String(ScResId(STR_ACC_RIGHTAREA_DESCR));
        break;
    case SVX_ADJUST_CENTER:
        sDesc = String(ScResId(STR_ACC_CENTERAREA_DESCR));
        break;
    default:
        DBG_ERRORFILE("wrong adjustment found");
    }

    return sDesc;
}

rtl::OUString SAL_CALL ScAccessiblePageHeaderArea::createAccessibleName(void)
    throw (uno::RuntimeException)
{
    rtl::OUString sName;
    switch (meAdjust)
    {
    case SVX_ADJUST_LEFT :
        sName = String(ScResId(STR_ACC_LEFTAREA_NAME));
        break;
    case SVX_ADJUST_RIGHT:
        sName = String(ScResId(STR_ACC_RIGHTAREA_NAME));
        break;
    case SVX_ADJUST_CENTER:
        sName = String(ScResId(STR_ACC_CENTERAREA_NAME));
        break;
    default:
        DBG_ERRORFILE("wrong adjustment found");
    }

    return sName;
}

Rectangle ScAccessiblePageHeaderArea::GetBoundingBoxOnScreen(void) const
    throw(::com::sun::star::uno::RuntimeException)
{
	Rectangle aRect;
    if (mxParent.is())
    {
        uno::Reference<XAccessibleContext> xContext = mxParent->getAccessibleContext();
        uno::Reference<XAccessibleComponent> xComp(xContext, uno::UNO_QUERY);
        if (xComp.is())
        {
            // has the same size and position on screen like the parent
            aRect = Rectangle(VCLPoint(xComp->getLocationOnScreen()), VCLRectangle(xComp->getBounds()).GetSize());
        }
    }
	return aRect;
}

Rectangle ScAccessiblePageHeaderArea::GetBoundingBox(void) const 
    throw (::com::sun::star::uno::RuntimeException)
{
	Rectangle aRect;
    if (mxParent.is())
    {
        uno::Reference<XAccessibleContext> xContext = mxParent->getAccessibleContext();
        uno::Reference<XAccessibleComponent> xComp(xContext, uno::UNO_QUERY);
        if (xComp.is())
        {
            // has the same size and position on screen like the parent and so the pos is (0, 0)
            Rectangle aNewRect(Point(0, 0), VCLRectangle(xComp->getBounds()).GetSize());
            aRect = aNewRect;
        }
    }
    
	return aRect;
}

void ScAccessiblePageHeaderArea::CreateTextHelper()
{
	if (!mpTextHelper)
	{
		::std::auto_ptr < ScAccessibleTextData > pAccessibleHeaderTextData
			(new ScAccessibleHeaderTextData(mpViewShell, mpEditObj, mbHeader, meAdjust));
		::std::auto_ptr< SvxEditSource > pEditSource (new ScAccessibilityEditSource(pAccessibleHeaderTextData));

		mpTextHelper = new ::accessibility::AccessibleTextHelper(pEditSource );
        mpTextHelper->SetEventSource(this);
	}
}
