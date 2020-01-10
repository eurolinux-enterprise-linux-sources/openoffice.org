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
#include "precompiled_sd.hxx"

#include "precompiled_sd.hxx"

#include "SlsViewCacheContext.hxx"

#include "model/SlideSorterModel.hxx"
#include "model/SlsPageDescriptor.hxx"
#include "model/SlsPageEnumerationProvider.hxx"
#include "view/SlideSorterView.hxx"
#include "view/SlsPageObjectViewObjectContact.hxx"
#include "sdpage.hxx"
#include "Window.hxx"
#include "drawdoc.hxx"
#include "tools/IdleDetection.hxx"
#include <svx/svdpage.hxx>
#include <svx/sdr/contact/viewcontact.hxx>
#include <vcl/window.hxx>
#include <svx/sdr/contact/objectcontact.hxx>

namespace sd { namespace slidesorter { namespace view {


ViewCacheContext::ViewCacheContext (
    model::SlideSorterModel& rModel,
    SlideSorterView& rView)
    : mrModel(rModel),
      mrView(rView)
{
}




ViewCacheContext::~ViewCacheContext (void)
{
}




void ViewCacheContext::NotifyPreviewCreation (
    cache::CacheKey aKey,
    const ::boost::shared_ptr<BitmapEx>& rPreview)
{
    (void)rPreview;
    const model::SharedPageDescriptor pDescriptor (GetDescriptor(aKey));
    if (pDescriptor.get() != NULL)
    {
        // Use direct view-invalidate here and no ActionChanged() at the VC
        // since the VC is a PageObjectViewObjectContact and in its ActionChanged()
        // implementation invalidates the cache entry again.
        view::PageObjectViewObjectContact* pContact = pDescriptor->GetViewObjectContact();
        if (pContact != NULL)
            pContact->GetObjectContact().InvalidatePartOfView(pContact->getObjectRange());
    }
    else
    {
        OSL_ASSERT(pDescriptor.get() != NULL);
    }
}




bool ViewCacheContext::IsIdle (void)
{
    sal_Int32 nIdleState (tools::IdleDetection::GetIdleState(mrView.GetWindow()));
    if (nIdleState == tools::IdleDetection::IDET_IDLE)
        return true;
    else
        return false;
}




bool ViewCacheContext::IsVisible (cache::CacheKey aKey)
{
    return GetDescriptor(aKey)->IsVisible();
}




const SdrPage* ViewCacheContext::GetPage (cache::CacheKey aKey)
{
    return static_cast<const SdrPage*>(aKey);
}




::boost::shared_ptr<std::vector<cache::CacheKey> > ViewCacheContext::GetEntryList (bool bVisible)
{
    ::boost::shared_ptr<std::vector<cache::CacheKey> > pKeys (new std::vector<cache::CacheKey>());

    model::PageEnumeration aPageEnumeration (
        bVisible
            ? model::PageEnumerationProvider::CreateVisiblePagesEnumeration(mrModel)
            : model::PageEnumerationProvider::CreateAllPagesEnumeration(mrModel));

    while (aPageEnumeration.HasMoreElements())
    {
        model::SharedPageDescriptor pDescriptor (aPageEnumeration.GetNextElement());
        pKeys->push_back(pDescriptor->GetPage());
    }

    return pKeys;
}




sal_Int32 ViewCacheContext::GetPriority (cache::CacheKey aKey)
{
    return - (static_cast<const SdrPage*>(aKey)->GetPageNum()-1) / 2;
}




model::SharedPageDescriptor ViewCacheContext::GetDescriptor (cache::CacheKey aKey)
{
    sal_uInt16 nPageIndex ((static_cast<const SdrPage*>(aKey)->GetPageNum() - 1) / 2);
    return mrModel.GetPageDescriptor(nPageIndex);
}




::com::sun::star::uno::Reference<com::sun::star::uno::XInterface> ViewCacheContext::GetModel (void)
{
    if (mrModel.GetDocument() == NULL)
        return NULL;
    else
        return mrModel.GetDocument()->getUnoModel();
}

} } } // end of namespace ::sd::slidesorter::view
