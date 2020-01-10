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
#include <svx/sdr/properties/properties.hxx>
#include <svx/sdr/properties/itemsettools.hxx>
#include <svtools/itemset.hxx>
#include <svx/svdogrp.hxx>
#include <svditer.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace sdr
{
	namespace properties
	{
		BaseProperties::BaseProperties(SdrObject& rObj)
		:	mrObject(rObj)
		{
		}

		BaseProperties::BaseProperties(const BaseProperties& /*rProps*/, SdrObject& rObj)
		:	mrObject(rObj)
		{
		}

		BaseProperties::~BaseProperties()
		{
		}

		const SfxItemSet& BaseProperties::GetMergedItemSet() const
		{
			// default implementation falls back to GetObjectItemSet()
			return GetObjectItemSet();
		}

		void BaseProperties::SetMergedItemSet(const SfxItemSet& rSet, sal_Bool bClearAllItems)
		{
			// clear items if requested
			if(bClearAllItems)
			{
				ClearObjectItem();
			}

			// default implementation falls back to SetObjectItemSet()
			SetObjectItemSet(rSet);
		}

		void BaseProperties::SetMergedItem(const SfxPoolItem& rItem)
		{
			// default implementation falls back to SetObjectItem()
			SetObjectItem(rItem);
		}

		void BaseProperties::ClearMergedItem(const sal_uInt16 nWhich)
		{
			// default implementation falls back to ClearObjectItem()
			ClearObjectItem(nWhich);
		}

		void BaseProperties::Scale(const Fraction& /*rScale*/)
		{
			// default implementation does nothing; overload where
			// an ItemSet is implemented.
		}

		void BaseProperties::MoveToItemPool(SfxItemPool* /*pSrcPool*/, SfxItemPool* /*pDestPool*/, SdrModel* /*pNewModel*/)
		{
			// Move properties to a new ItemPool. Default implementation does nothing.
			// Overload where an ItemSet is implemented.
		}

		void BaseProperties::SetModel(SdrModel* /*pOldModel*/, SdrModel* /*pNewModel*/)
		{
			// Set new model. Default implementation does nothing.
			// Overload where an ItemSet is implemented.
		}

		void BaseProperties::ForceStyleToHardAttributes()
		{
			// force all attributes which come from styles to hard attributes
			// to be able to live without the style. Default implementation does nothing.
			// Overload where an ItemSet is implemented.
		}

		//void BaseProperties::SetItemAndBroadcast(const SfxPoolItem& rItem)
		//{
		//	ItemChangeBroadcaster aC(GetSdrObject());
		//	SetObjectItem(rItem);
		//	BroadcastItemChange(aC);
		//}

		//void BaseProperties::ClearItemAndBroadcast(const sal_uInt16 nWhich)
		//{
		//	ItemChangeBroadcaster aC(GetSdrObject());
		//	ClearObjectItem(nWhich);
		//	BroadcastItemChange(aC);
		//}

		void BaseProperties::SetMergedItemSetAndBroadcast(const SfxItemSet& rSet, sal_Bool bClearAllItems)
		{
			ItemChangeBroadcaster aC(GetSdrObject());
		
			if(bClearAllItems)
			{
				ClearObjectItem();
			}
		
			SetMergedItemSet(rSet);
			BroadcastItemChange(aC);
		}

		const SfxPoolItem& BaseProperties::GetItem(const sal_uInt16 nWhich) const
		{
			return GetObjectItemSet().Get(nWhich);
		}

		void BaseProperties::BroadcastItemChange(const ItemChangeBroadcaster& rChange)
		{
			const sal_uInt32 nCount(rChange.GetRectangleCount());

			// #110094#-14 Reduce to do only second change
			//// invalidate all remembered rectangles
			//for(sal_uInt32 a(0); a < nCount; a++)
			//{
			//	GetSdrObject().BroadcastObjectChange(rChange.GetRectangle(a));
			//}

			// invalidate all new rectangles
			if(GetSdrObject().ISA(SdrObjGroup))
			{
				SdrObjListIter aIter((SdrObjGroup&)GetSdrObject(), IM_DEEPNOGROUPS);

				while(aIter.IsMore())
				{
					SdrObject* pObj = aIter.Next();
					// This is done with ItemSetChanged
					// pObj->SetChanged();
					pObj->BroadcastObjectChange();
				}
			}
			else
			{
				// This is done with ItemSetChanged
				// GetSdrObject().SetChanged();
				GetSdrObject().BroadcastObjectChange();
			}

			// also send the user calls
			for(sal_uInt32 a(0L); a < nCount; a++)
			{
				GetSdrObject().SendUserCall(SDRUSERCALL_CHGATTR, rChange.GetRectangle(a));
			}
		}

        sal_uInt32 BaseProperties::getVersion() const
        {
            return 0;
        }
	} // end of namespace properties
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
