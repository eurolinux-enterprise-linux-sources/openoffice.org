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


#include "AccessibleSpreadsheet.hxx"
#include "AccessibilityHints.hxx"
#include "AccessibleCell.hxx"
#include "AccessibleDocument.hxx"
#include "tabvwsh.hxx"
#include "document.hxx"
#include "unoguard.hxx"
#include "hints.hxx"
#include "scmod.hxx"

#ifndef _UTL_ACCESSIBLESTATESETHELPER_HXX
#include <unotools/accessiblestatesethelper.hxx>
#endif
#ifndef _COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLEROLE_HPP_
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#endif
#ifndef _COM_SUN_STAR_ACCESSIBILITY_XACCESSIBLESTATETYPE_HPP_
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#endif
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleTableModelChangeType.hpp>
#include <rtl/uuid.h>
#include <tools/debug.hxx>
#include <tools/gen.hxx>
#include <svtools/colorcfg.hxx>

#include <algorithm>

using namespace	::com::sun::star;
using namespace	::com::sun::star::accessibility;

//=====  internal  ============================================================

ScAccessibleSpreadsheet::ScAccessibleSpreadsheet(
        ScAccessibleDocument* pAccDoc,
		ScTabViewShell* pViewShell,
		SCTAB nTab,
		ScSplitPos eSplitPos)
	:
	ScAccessibleTableBase (pAccDoc, GetDocument(pViewShell),
        ScRange(ScAddress(0, 0, nTab),ScAddress(MAXCOL, MAXROW, nTab))),
    mbIsSpreadsheet( sal_True )
{
    ConstructScAccessibleSpreadsheet( pAccDoc, pViewShell, nTab, eSplitPos );
}

ScAccessibleSpreadsheet::ScAccessibleSpreadsheet(
        ScAccessibleSpreadsheet& rParent, const ScRange& rRange ) :
    ScAccessibleTableBase( rParent.mpAccDoc, rParent.mpDoc, rRange),
    mbIsSpreadsheet( sal_False )
{
    ConstructScAccessibleSpreadsheet( rParent.mpAccDoc, rParent.mpViewShell, rParent.mnTab, rParent.meSplitPos );
}

ScAccessibleSpreadsheet::~ScAccessibleSpreadsheet()
{
	if (mpMarkedRanges)
		delete mpMarkedRanges;
	if (mpSortedMarkedCells)
		delete mpSortedMarkedCells;
	if (mpViewShell)
		mpViewShell->RemoveAccessibilityObject(*this);
}

void ScAccessibleSpreadsheet::ConstructScAccessibleSpreadsheet(
    ScAccessibleDocument* pAccDoc,
    ScTabViewShell* pViewShell,
    SCTAB nTab,
    ScSplitPos eSplitPos)
{
    mpViewShell = pViewShell;
    mpMarkedRanges = 0;
    mpSortedMarkedCells = 0;
    mpAccDoc = pAccDoc;
    mpAccCell = 0;
    meSplitPos = eSplitPos;
    mnTab = nTab;
    mbHasSelection = sal_False;
    mbDelIns = sal_False;
    mbIsFocusSend = sal_False;
    maVisCells = GetVisCells(GetVisArea(mpViewShell, meSplitPos));
    if (mpViewShell)
    {
        mpViewShell->AddAccessibilityObject(*this);

        const ScViewData& rViewData = *mpViewShell->GetViewData();
        const ScMarkData& rMarkData = rViewData.GetMarkData();
        maActiveCell = rViewData.GetCurPos();
        mbHasSelection = rMarkData.GetTableSelect(maActiveCell.Tab()) &&
                    (rMarkData.IsMarked() || rMarkData.IsMultiMarked());
        mpAccCell = GetAccessibleCellAt(maActiveCell.Row(), maActiveCell.Col());
        mpAccCell->acquire();
        mpAccCell->Init();
    }
}

void SAL_CALL ScAccessibleSpreadsheet::disposing()
{
    ScUnoGuard aGuard;
	if (mpViewShell)
	{
		mpViewShell->RemoveAccessibilityObject(*this);
		mpViewShell = NULL;
	}
    if (mpAccCell)
    {
        mpAccCell->release();
        mpAccCell = NULL;
    }

	ScAccessibleTableBase::disposing();
}

void ScAccessibleSpreadsheet::CompleteSelectionChanged(sal_Bool bNewState)
{
	if (mpMarkedRanges)
		DELETEZ(mpMarkedRanges);
	if (mpSortedMarkedCells)
		DELETEZ(mpSortedMarkedCells);

	mbHasSelection = bNewState;

    AccessibleEventObject aEvent;
	aEvent.EventId = AccessibleEventId::STATE_CHANGED;
	if (bNewState)
		aEvent.NewValue = uno::makeAny(AccessibleStateType::SELECTED);
	else
		aEvent.OldValue = uno::makeAny(AccessibleStateType::SELECTED);
	aEvent.Source = uno::Reference< XAccessibleContext >(this);

	CommitChange(aEvent);
}

void ScAccessibleSpreadsheet::LostFocus()
{
	AccessibleEventObject aEvent;
	aEvent.EventId = AccessibleEventId::ACTIVE_DESCENDANT_CHANGED;
	aEvent.Source = uno::Reference< XAccessibleContext >(this);
    uno::Reference< XAccessible > xOld = mpAccCell;
	aEvent.OldValue <<= xOld;

	CommitChange(aEvent);

    CommitFocusLost();
}

void ScAccessibleSpreadsheet::GotFocus()
{
    CommitFocusGained();

	AccessibleEventObject aEvent;
	aEvent.EventId = AccessibleEventId::ACTIVE_DESCENDANT_CHANGED;
	aEvent.Source = uno::Reference< XAccessibleContext >(this);
    uno::Reference< XAccessible > xNew = mpAccCell;
	aEvent.NewValue <<= xNew;

	CommitChange(aEvent);
}

void ScAccessibleSpreadsheet::BoundingBoxChanged()
{
    AccessibleEventObject aEvent;
    aEvent.EventId = AccessibleEventId::BOUNDRECT_CHANGED;
    aEvent.Source = uno::Reference< XAccessibleContext >(this);

    CommitChange(aEvent);
}

void ScAccessibleSpreadsheet::VisAreaChanged()
{
	AccessibleEventObject aEvent;
	aEvent.EventId = AccessibleEventId::VISIBLE_DATA_CHANGED;
	aEvent.Source = uno::Reference< XAccessibleContext >(this);

	CommitChange(aEvent);
}

	//=====  SfxListener  =====================================================

void ScAccessibleSpreadsheet::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
{
	if (rHint.ISA( SfxSimpleHint ) )
	{
		const SfxSimpleHint& rRef = (const SfxSimpleHint&)rHint;
		// only notify if child exist, otherwise it is not necessary
		if ((rRef.GetId() == SC_HINT_ACC_CURSORCHANGED))
		{
			if (mpViewShell)
			{
				ScAddress aNewCell = mpViewShell->GetViewData()->GetCurPos();
				sal_Bool bNewMarked(mpViewShell->GetViewData()->GetMarkData().GetTableSelect(aNewCell.Tab()) &&
					(mpViewShell->GetViewData()->GetMarkData().IsMarked() ||
					mpViewShell->GetViewData()->GetMarkData().IsMultiMarked()));
				sal_Bool bNewCellSelected(isAccessibleSelected(aNewCell.Row(), aNewCell.Col()));
				if ((bNewMarked != mbHasSelection) ||
					(!bNewCellSelected && bNewMarked) ||
					(bNewCellSelected && mbHasSelection))
				{
					if (mpMarkedRanges)
						DELETEZ(mpMarkedRanges);
					if (mpSortedMarkedCells)
						DELETEZ(mpSortedMarkedCells);
					AccessibleEventObject aEvent;
					aEvent.EventId = AccessibleEventId::SELECTION_CHANGED;
					aEvent.Source = uno::Reference< XAccessibleContext >(this);

					mbHasSelection = bNewMarked;

					CommitChange(aEvent);
				}

                // active descendant changed event (new cell selected)
                bool bFireActiveDescChanged = (aNewCell != maActiveCell) &&
                    (aNewCell.Tab() == maActiveCell.Tab()) && IsFocused();

                /*  Remember old active cell and set new active cell.
                    #i82409# always update the class members mpAccCell and
                    maActiveCell, even if the sheet is not focused, e.g. when
                    using the name box in the toolbar. */
                uno::Reference< XAccessible > xOld = mpAccCell;
                mpAccCell->release();
                mpAccCell = GetAccessibleCellAt(aNewCell.Row(), aNewCell.Col());
                mpAccCell->acquire();
                mpAccCell->Init();
                uno::Reference< XAccessible > xNew = mpAccCell;
                maActiveCell = aNewCell;

                // #i14108# fire event only if sheet is focused
                if( bFireActiveDescChanged )
                {
                    AccessibleEventObject aEvent;
                    aEvent.EventId = AccessibleEventId::ACTIVE_DESCENDANT_CHANGED;
                    aEvent.Source = uno::Reference< XAccessibleContext >(this);
                    aEvent.OldValue <<= xOld;
                    aEvent.NewValue <<= xNew;
					CommitChange(aEvent);
                }
			}
		}
		else if ((rRef.GetId() == SC_HINT_DATACHANGED))
		{
			if (!mbDelIns)
				CommitTableModelChange(maRange.aStart.Row(), maRange.aStart.Col(), maRange.aEnd.Row(), maRange.aEnd.Col(), AccessibleTableModelChangeType::UPDATE);
			else
				mbDelIns = sal_False;
		}
        // no longer needed, because the document calls the VisAreaChanged method
/*		else if (rRef.GetId() == SC_HINT_ACC_VISAREACHANGED)
		{
			AccessibleEventObject aEvent;
			aEvent.EventId = AccessibleEventId::VISIBLE_DATA_CHANGED;
			aEvent.Source = uno::Reference< XAccessibleContext >(this);

			CommitChange(aEvent);*/
        // commented out, because to use a ModelChangeEvent is not the right way
        // at the moment there is no way, but the Java/Gnome Api should be extended sometime
/*			if (mpViewShell)
			{
				Rectangle aNewVisCells(GetVisCells(GetVisArea(mpViewShell, meSplitPos)));

				Rectangle aNewPos(aNewVisCells);

				if (aNewVisCells.IsOver(maVisCells))
					aNewPos.Union(maVisCells);
				else
					CommitTableModelChange(maVisCells.Top(), maVisCells.Left(), maVisCells.Bottom(), maVisCells.Right(), AccessibleTableModelChangeType::UPDATE);

				maVisCells = aNewVisCells;

				CommitTableModelChange(aNewPos.Top(), aNewPos.Left(), aNewPos.Bottom(), aNewPos.Right(), AccessibleTableModelChangeType::UPDATE);
			}
		}*/
        // no longer needed, because the document calls the BoundingBoxChanged method
/*        else if (rRef.GetId() == SC_HINT_ACC_WINDOWRESIZED)
        {
			AccessibleEventObject aEvent;
			aEvent.EventId = AccessibleEventId::BOUNDRECT_CHANGED;
			aEvent.Source = uno::Reference< XAccessibleContext >(this);

			CommitChange(aEvent);
        }*/
	}
	else if (rHint.ISA( ScUpdateRefHint ))
	{
		const ScUpdateRefHint& rRef = (const ScUpdateRefHint&)rHint;
		if (rRef.GetMode() == URM_INSDEL && rRef.GetDz() == 0) //#107250# test whether table is inserted or deleted
		{
			if (((rRef.GetRange().aStart.Col() == maRange.aStart.Col()) &&
				(rRef.GetRange().aEnd.Col() == maRange.aEnd.Col())) ||
				((rRef.GetRange().aStart.Row() == maRange.aStart.Row()) &&
				(rRef.GetRange().aEnd.Row() == maRange.aEnd.Row())))
			{
				// ignore next SC_HINT_DATACHANGED notification
				mbDelIns = sal_True;

				sal_Int16 nId(0);
                SCsCOL nX(rRef.GetDx());
                SCsROW nY(rRef.GetDy());
                ScRange aRange(rRef.GetRange());
				if ((nX < 0) || (nY < 0))
                {
                    DBG_ASSERT(!((nX < 0) && (nY < 0)), "should not be possible to remove row and column at the same time");
					nId = AccessibleTableModelChangeType::DELETE;
                    if (nX < 0)
                    {
                        nX = -nX;
                        nY = aRange.aEnd.Row() - aRange.aStart.Row();
                    }
                    else
                    {
                        nY = -nY;
                        nX = aRange.aEnd.Col() - aRange.aStart.Col();
                    }
                }
				else if ((nX > 0) || (nY > 0))
                {
                    DBG_ASSERT(!((nX > 0) && (nY > 0)), "should not be possible to add row and column at the same time");
					nId = AccessibleTableModelChangeType::INSERT;
                    if (nX < 0)
                        nY = aRange.aEnd.Row() - aRange.aStart.Row();
                    else
                        nX = aRange.aEnd.Col() - aRange.aStart.Col();
                }
				else
				{
					DBG_ERROR("is it a deletion or a insertion?");
				}

				CommitTableModelChange(rRef.GetRange().aStart.Row(),
                    rRef.GetRange().aStart.Col(),
                    rRef.GetRange().aStart.Row() + nY,
                    rRef.GetRange().aStart.Col() + nX, nId);

				AccessibleEventObject aEvent;
				aEvent.EventId = AccessibleEventId::ACTIVE_DESCENDANT_CHANGED;
				aEvent.Source = uno::Reference< XAccessibleContext >(this);
                uno::Reference< XAccessible > xNew = mpAccCell;
				aEvent.NewValue <<= xNew;

				CommitChange(aEvent);
			}
		}
	}

	ScAccessibleTableBase::Notify(rBC, rHint);
}

	//=====  XAccessibleTable  ================================================

uno::Reference< XAccessibleTable > SAL_CALL ScAccessibleSpreadsheet::getAccessibleRowHeaders(  )
                    throw (uno::RuntimeException)
{
    ScUnoGuard aGuard;
    IsObjectValid();
    uno::Reference< XAccessibleTable > xAccessibleTable;
    if( mpDoc && mbIsSpreadsheet )
    {
        if( const ScRange* pRowRange = mpDoc->GetRepeatRowRange( mnTab ) )
        {
            SCROW nStart = pRowRange->aStart.Row();
            SCROW nEnd = pRowRange->aEnd.Row();
            if( (0 <= nStart) && (nStart <= nEnd) && (nEnd <= MAXROW) )
                xAccessibleTable.set( new ScAccessibleSpreadsheet( *this, ScRange( 0, nStart, mnTab, MAXCOL, nEnd, mnTab ) ) );
        }
    }
    return xAccessibleTable;
}

uno::Reference< XAccessibleTable > SAL_CALL ScAccessibleSpreadsheet::getAccessibleColumnHeaders(  )
                    throw (uno::RuntimeException)
{
    ScUnoGuard aGuard;
    IsObjectValid();
    uno::Reference< XAccessibleTable > xAccessibleTable;
    if( mpDoc && mbIsSpreadsheet )
    {
        if( const ScRange* pColRange = mpDoc->GetRepeatColRange( mnTab ) )
        {
            SCCOL nStart = pColRange->aStart.Col();
            SCCOL nEnd = pColRange->aEnd.Col();
            if( (0 <= nStart) && (nStart <= nEnd) && (nEnd <= MAXCOL) )
                xAccessibleTable.set( new ScAccessibleSpreadsheet( *this, ScRange( nStart, 0, mnTab, nEnd, MAXROW, mnTab ) ) );
        }
    }
    return xAccessibleTable;
}

uno::Sequence< sal_Int32 > SAL_CALL ScAccessibleSpreadsheet::getSelectedAccessibleRows(  )
    				throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	uno::Sequence<sal_Int32> aSequence;
	if (mpViewShell && mpViewShell->GetViewData())
	{
		aSequence.realloc(maRange.aEnd.Row() - maRange.aStart.Row() + 1);
		const ScMarkData& rMarkdata = mpViewShell->GetViewData()->GetMarkData();
		sal_Int32* pSequence = aSequence.getArray();
		sal_Int32 nCount(0);
		for (SCROW i = maRange.aStart.Row(); i <= maRange.aEnd.Row(); ++i)
		{
			if (rMarkdata.IsRowMarked(i))
			{
				pSequence[nCount] = i;
				++nCount;
			}
		}
		aSequence.realloc(nCount);
	}
	else
		aSequence.realloc(0);
	return aSequence;
}

uno::Sequence< sal_Int32 > SAL_CALL ScAccessibleSpreadsheet::getSelectedAccessibleColumns(  )
    				throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	uno::Sequence<sal_Int32> aSequence;
	if (mpViewShell && mpViewShell->GetViewData())
	{
		aSequence.realloc(maRange.aEnd.Col() - maRange.aStart.Col() + 1);
		const ScMarkData& rMarkdata = mpViewShell->GetViewData()->GetMarkData();
		sal_Int32* pSequence = aSequence.getArray();
		sal_Int32 nCount(0);
		for (SCCOL i = maRange.aStart.Col(); i <= maRange.aEnd.Col(); ++i)
		{
			if (rMarkdata.IsColumnMarked(i))
			{
				pSequence[nCount] = i;
				++nCount;
			}
		}
		aSequence.realloc(nCount);
	}
	else
		aSequence.realloc(0);
	return aSequence;
}

sal_Bool SAL_CALL ScAccessibleSpreadsheet::isAccessibleRowSelected( sal_Int32 nRow )
    throw (uno::RuntimeException, lang::IndexOutOfBoundsException)
{
	ScUnoGuard aGuard;
    IsObjectValid();

    if ((nRow > (maRange.aEnd.Row() - maRange.aStart.Row())) || (nRow < 0))
        throw lang::IndexOutOfBoundsException();

	sal_Bool bResult(sal_False);
	if (mpViewShell && mpViewShell->GetViewData())
	{
		const ScMarkData& rMarkdata = mpViewShell->GetViewData()->GetMarkData();
		bResult = rMarkdata.IsRowMarked((SCROW)nRow);
	}
	return bResult;
}

sal_Bool SAL_CALL ScAccessibleSpreadsheet::isAccessibleColumnSelected( sal_Int32 nColumn )
    throw (uno::RuntimeException, lang::IndexOutOfBoundsException)
{
	ScUnoGuard aGuard;
    IsObjectValid();

    if ((nColumn > (maRange.aEnd.Col() - maRange.aStart.Col())) || (nColumn < 0))
        throw lang::IndexOutOfBoundsException();

    sal_Bool bResult(sal_False);
	if (mpViewShell && mpViewShell->GetViewData())
	{
		const ScMarkData& rMarkdata = mpViewShell->GetViewData()->GetMarkData();
		bResult = rMarkdata.IsColumnMarked((SCCOL)nColumn);
	}
	return bResult;
}

ScAccessibleCell* ScAccessibleSpreadsheet::GetAccessibleCellAt(sal_Int32 nRow, sal_Int32 nColumn)
{
    ScAccessibleCell* pAccessibleCell = NULL;
	ScAddress aCellAddress(static_cast<SCCOL>(maRange.aStart.Col() + nColumn),
		static_cast<SCROW>(maRange.aStart.Row() + nRow), maRange.aStart.Tab());
    if ((aCellAddress == maActiveCell) && mpAccCell)
    {
        pAccessibleCell = mpAccCell;
    }
    else
	    pAccessibleCell = new ScAccessibleCell(this, mpViewShell, aCellAddress, getAccessibleIndex(nRow, nColumn), meSplitPos, mpAccDoc);

    return pAccessibleCell;
}

uno::Reference< XAccessible > SAL_CALL ScAccessibleSpreadsheet::getAccessibleCellAt( sal_Int32 nRow, sal_Int32 nColumn )
    				throw (uno::RuntimeException, lang::IndexOutOfBoundsException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
    if (nRow > (maRange.aEnd.Row() - maRange.aStart.Row()) ||
        nRow < 0 ||
        nColumn > (maRange.aEnd.Col() - maRange.aStart.Col()) ||
        nColumn < 0)
        throw lang::IndexOutOfBoundsException();

    uno::Reference<XAccessible> xAccessible;
    ScAccessibleCell* pAccessibleCell = GetAccessibleCellAt(nRow, nColumn);
    xAccessible = pAccessibleCell;
	pAccessibleCell->Init();
	return xAccessible;
}

sal_Bool SAL_CALL ScAccessibleSpreadsheet::isAccessibleSelected( sal_Int32 nRow, sal_Int32 nColumn )
    throw (uno::RuntimeException, lang::IndexOutOfBoundsException)
{
	ScUnoGuard aGuard;
    IsObjectValid();

    if ((nColumn > (maRange.aEnd.Col() - maRange.aStart.Col())) || (nColumn < 0) ||
        (nRow > (maRange.aEnd.Row() - maRange.aStart.Row())) || (nRow < 0))
        throw lang::IndexOutOfBoundsException();

	sal_Bool bResult(sal_False);
	if (mpViewShell)
	{
		const ScMarkData& rMarkdata = mpViewShell->GetViewData()->GetMarkData();
		bResult = rMarkdata.IsCellMarked(static_cast<SCCOL>(nColumn), static_cast<SCROW>(nRow));
	}
	return bResult;
}

	//=====  XAccessibleComponent  ============================================

uno::Reference< XAccessible > SAL_CALL ScAccessibleSpreadsheet::getAccessibleAtPoint(
	const awt::Point& rPoint )
		throw (uno::RuntimeException)
{
	uno::Reference< XAccessible > xAccessible;
    if (containsPoint(rPoint))
    {
    	ScUnoGuard aGuard;
        IsObjectValid();
	    if (mpViewShell)
	    {
		    SCsCOL nX;
            SCsROW nY;
		    mpViewShell->GetViewData()->GetPosFromPixel( rPoint.X, rPoint.Y, meSplitPos, nX, nY);
		    xAccessible = getAccessibleCellAt(nY, nX);
	    }
    }
	return xAccessible;
}

void SAL_CALL ScAccessibleSpreadsheet::grabFocus(  )
		throw (uno::RuntimeException)
{
	if (getAccessibleParent().is())
	{
		uno::Reference<XAccessibleComponent> xAccessibleComponent(getAccessibleParent()->getAccessibleContext(), uno::UNO_QUERY);
		if (xAccessibleComponent.is())
			xAccessibleComponent->grabFocus();
	}
}

sal_Int32 SAL_CALL ScAccessibleSpreadsheet::getForeground(  )
        throw (uno::RuntimeException)
{
    return COL_BLACK;
}

sal_Int32 SAL_CALL ScAccessibleSpreadsheet::getBackground(  )
        throw (uno::RuntimeException)
{
    ScUnoGuard aGuard;
    IsObjectValid();
    return SC_MOD()->GetColorConfig().GetColorValue( ::svtools::DOCCOLOR ).nColor;
}

    //=====  XAccessibleContext  ==============================================

uno::Reference<XAccessibleRelationSet> SAL_CALL ScAccessibleSpreadsheet::getAccessibleRelationSet(void)
        throw (::com::sun::star::uno::RuntimeException)
{
    utl::AccessibleRelationSetHelper* pRelationSet = NULL;
    if(mpAccDoc)
        pRelationSet = mpAccDoc->GetRelationSet(NULL);
    if (!pRelationSet)
        pRelationSet = new utl::AccessibleRelationSetHelper();
    return pRelationSet;
}

uno::Reference<XAccessibleStateSet> SAL_CALL
	ScAccessibleSpreadsheet::getAccessibleStateSet(void)
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
	if (IsDefunc(xParentStates))
		pStateSet->AddState(AccessibleStateType::DEFUNC);
    else
    {
        pStateSet->AddState(AccessibleStateType::MANAGES_DESCENDANTS);
	    if (IsEditable(xParentStates))
		    pStateSet->AddState(AccessibleStateType::EDITABLE);
	    pStateSet->AddState(AccessibleStateType::ENABLED);
        pStateSet->AddState(AccessibleStateType::FOCUSABLE);
        if (IsFocused())
            pStateSet->AddState(AccessibleStateType::FOCUSED);
	    pStateSet->AddState(AccessibleStateType::MULTI_SELECTABLE);
	    pStateSet->AddState(AccessibleStateType::OPAQUE);
	    pStateSet->AddState(AccessibleStateType::SELECTABLE);
	    if (IsCompleteSheetSelected())
		    pStateSet->AddState(AccessibleStateType::SELECTED);
	    if (isShowing())
		    pStateSet->AddState(AccessibleStateType::SHOWING);
	    if (isVisible())
		    pStateSet->AddState(AccessibleStateType::VISIBLE);
    }
	return pStateSet;
}

	///=====  XAccessibleSelection  ===========================================

void SAL_CALL
		ScAccessibleSpreadsheet::selectAccessibleChild( sal_Int32 nChildIndex )
		throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
    if (nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    if (mpViewShell)
	{
		sal_Int32 nCol(getAccessibleColumn(nChildIndex));
		sal_Int32 nRow(getAccessibleRow(nChildIndex));

		SelectCell(nRow, nCol, sal_False);
	}
}

void SAL_CALL
		ScAccessibleSpreadsheet::clearAccessibleSelection(  )
		throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	if (mpViewShell)
	{
		mpViewShell->Unmark();
	}
}

void SAL_CALL
		ScAccessibleSpreadsheet::selectAllAccessibleChildren(  )
		throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	if (mpViewShell)
	{
		mpViewShell->SelectAll();
	}
}

sal_Int32 SAL_CALL
		ScAccessibleSpreadsheet::getSelectedAccessibleChildCount(  )
		throw (uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	sal_Int32 nResult(0);
	if (mpViewShell)
	{
		if (!mpMarkedRanges)
		{
			mpMarkedRanges = new ScRangeList();
            ScMarkData aMarkData(mpViewShell->GetViewData()->GetMarkData());
            aMarkData.MarkToMulti();
			aMarkData.FillRangeListWithMarks(mpMarkedRanges, sal_False);
		}
		// is possible, because there shouldn't be overlapped ranges in it
		if (mpMarkedRanges)
			nResult = mpMarkedRanges->GetCellCount();
	}
	return nResult;
}

uno::Reference<XAccessible > SAL_CALL
		ScAccessibleSpreadsheet::getSelectedAccessibleChild( sal_Int32 nSelectedChildIndex )
		throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();
	uno::Reference < XAccessible > xAccessible;
	if (mpViewShell)
	{
		if (!mpMarkedRanges)
		{
			mpMarkedRanges = new ScRangeList();
			mpViewShell->GetViewData()->GetMarkData().FillRangeListWithMarks(mpMarkedRanges, sal_False);
		}
		if (mpMarkedRanges)
		{
			if (!mpSortedMarkedCells)
				CreateSortedMarkedCells();
			if (mpSortedMarkedCells)
			{
				if ((nSelectedChildIndex < 0) ||
					(mpSortedMarkedCells->size() <= static_cast<sal_uInt32>(nSelectedChildIndex)))
					throw lang::IndexOutOfBoundsException();
				else
					xAccessible = getAccessibleCellAt((*mpSortedMarkedCells)[nSelectedChildIndex].Row(), (*mpSortedMarkedCells)[nSelectedChildIndex].Col());
			}
		}
	}
	return xAccessible;
}

void SAL_CALL
		ScAccessibleSpreadsheet::deselectAccessibleChild( sal_Int32 nChildIndex )
		throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
{
	ScUnoGuard aGuard;
    IsObjectValid();

    if (nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    if (mpViewShell)
	{
		sal_Int32 nCol(getAccessibleColumn(nChildIndex));
		sal_Int32 nRow(getAccessibleRow(nChildIndex));

		if (mpViewShell->GetViewData()->GetMarkData().IsCellMarked(static_cast<SCCOL>(nCol), static_cast<SCROW>(nRow)))
			SelectCell(nRow, nCol, sal_True);
	}
}

void ScAccessibleSpreadsheet::SelectCell(sal_Int32 nRow, sal_Int32 nCol, sal_Bool bDeselect)
{
	mpViewShell->SetTabNo( maRange.aStart.Tab() );

	mpViewShell->DoneBlockMode( sal_True ); // continue selecting
	mpViewShell->InitBlockMode( static_cast<SCCOL>(nCol), static_cast<SCROW>(nRow), maRange.aStart.Tab(), bDeselect, sal_False, sal_False );

	mpViewShell->SelectionChanged();
}

void ScAccessibleSpreadsheet::CreateSortedMarkedCells()
{
	mpSortedMarkedCells = new std::vector<ScMyAddress>();
	mpSortedMarkedCells->reserve(mpMarkedRanges->GetCellCount());
	ScRange* pRange = mpMarkedRanges->First();
	while (pRange)
	{
		if (pRange->aStart.Tab() != pRange->aEnd.Tab())
		{
			if ((maActiveCell.Tab() >= pRange->aStart.Tab()) ||
				maActiveCell.Tab() <= pRange->aEnd.Tab())
			{
				ScRange aRange(*pRange);
				aRange.aStart.SetTab(maActiveCell.Tab());
				aRange.aEnd.SetTab(maActiveCell.Tab());
				AddMarkedRange(aRange);
			}
			else
			{
				DBG_ERROR("Range of wrong table");
			}
		}
		else if(pRange->aStart.Tab() == maActiveCell.Tab())
			AddMarkedRange(*pRange);
		else
		{
			DBG_ERROR("Range of wrong table");
		}
		pRange = mpMarkedRanges->Next();
	}
	std::sort(mpSortedMarkedCells->begin(), mpSortedMarkedCells->end());
}

void ScAccessibleSpreadsheet::AddMarkedRange(const ScRange& rRange)
{
	for (SCROW nRow = rRange.aStart.Row(); nRow <= rRange.aEnd.Row(); ++nRow)
	{
		for (SCCOL nCol = rRange.aStart.Col(); nCol <= rRange.aEnd.Col(); ++nCol)
		{
			ScMyAddress aCell(nCol, nRow, maActiveCell.Tab());
			mpSortedMarkedCells->push_back(aCell);
		}
	}
}

	//=====  XServiceInfo  ====================================================

::rtl::OUString SAL_CALL ScAccessibleSpreadsheet::getImplementationName(void)
        throw (uno::RuntimeException)
{
	return rtl::OUString(RTL_CONSTASCII_USTRINGPARAM ("ScAccessibleSpreadsheet"));
}

uno::Sequence< ::rtl::OUString> SAL_CALL
	ScAccessibleSpreadsheet::getSupportedServiceNames (void)
        throw (uno::RuntimeException)
{
	uno::Sequence< ::rtl::OUString > aSequence = ScAccessibleTableBase::getSupportedServiceNames();
    sal_Int32 nOldSize(aSequence.getLength());
    aSequence.realloc(nOldSize + 1);
    ::rtl::OUString* pNames = aSequence.getArray();

	pNames[nOldSize] = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.AccessibleSpreadsheet"));

	return aSequence;
}

//=====  XTypeProvider  =======================================================

uno::Sequence<sal_Int8> SAL_CALL
	ScAccessibleSpreadsheet::getImplementationId(void)
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

///=====  XAccessibleEventBroadcaster  =====================================

void SAL_CALL ScAccessibleSpreadsheet::addEventListener(const uno::Reference<XAccessibleEventListener>& xListener)
        throw (uno::RuntimeException)
{
    ScUnoGuard aGuard;
    IsObjectValid();
    ScAccessibleTableBase::addEventListener(xListener);

    if (!mbIsFocusSend)
    {
        mbIsFocusSend = sal_True;
        CommitFocusGained();

		AccessibleEventObject aEvent;
		aEvent.EventId = AccessibleEventId::ACTIVE_DESCENDANT_CHANGED;
		aEvent.Source = uno::Reference< XAccessibleContext >(this);
		aEvent.NewValue <<= getAccessibleCellAt(maActiveCell.Row(), maActiveCell.Col());

		CommitChange(aEvent);
    }
}

	//====  internal  =========================================================

Rectangle ScAccessibleSpreadsheet::GetBoundingBoxOnScreen() const
	throw (uno::RuntimeException)
{
	Rectangle aRect;
	if (mpViewShell)
	{
		Window* pWindow = mpViewShell->GetWindowByPos(meSplitPos);
		if (pWindow)
			aRect = pWindow->GetWindowExtentsRelative(NULL);
	}
	return aRect;
}

Rectangle ScAccessibleSpreadsheet::GetBoundingBox() const
	throw (uno::RuntimeException)
{
	Rectangle aRect;
	if (mpViewShell)
	{
		Window* pWindow = mpViewShell->GetWindowByPos(meSplitPos);
		if (pWindow)
            //#101986#; extends to the same window, because the parent is the document and it has the same window
			aRect = pWindow->GetWindowExtentsRelative(pWindow);
	}
	return aRect;
}

sal_Bool ScAccessibleSpreadsheet::IsDefunc(
	const uno::Reference<XAccessibleStateSet>& rxParentStates)
{
	return ScAccessibleContextBase::IsDefunc() || (mpViewShell == NULL) || !getAccessibleParent().is() ||
		(rxParentStates.is() && rxParentStates->contains(AccessibleStateType::DEFUNC));
}

sal_Bool ScAccessibleSpreadsheet::IsEditable(
    const uno::Reference<XAccessibleStateSet>& /* rxParentStates */)
{
	sal_Bool bProtected(sal_False);
	if (mpDoc && mpDoc->IsTabProtected(maRange.aStart.Tab()))
		bProtected = sal_True;
	return !bProtected;
}

sal_Bool ScAccessibleSpreadsheet::IsFocused()
{
    sal_Bool bFocused(sal_False);
    if (mpViewShell)
    {
        if (mpViewShell->GetViewData()->GetActivePart() == meSplitPos)
            bFocused = mpViewShell->GetActiveWin()->HasFocus();
    }
    return bFocused;
}

sal_Bool ScAccessibleSpreadsheet::IsCompleteSheetSelected()
{
	sal_Bool bResult(sal_False);
	if(mpViewShell)
	{
        //#103800#; use a copy of MarkData
        ScMarkData aMarkData(mpViewShell->GetViewData()->GetMarkData());
		aMarkData.MarkToMulti();
		if (aMarkData.IsAllMarked(maRange))
			bResult = sal_True;
	}
	return bResult;
}

ScDocument* ScAccessibleSpreadsheet::GetDocument(ScTabViewShell* pViewShell)
{
	ScDocument* pDoc = NULL;
	if (pViewShell)
		pDoc = pViewShell->GetViewData()->GetDocument();
	return pDoc;
}

Rectangle ScAccessibleSpreadsheet::GetVisArea(ScTabViewShell* pViewShell, ScSplitPos eSplitPos)
{
	Rectangle aVisArea;
	if (pViewShell)
	{
		Window* pWindow = pViewShell->GetWindowByPos(eSplitPos);
		if (pWindow)
		{
			aVisArea.SetPos(pViewShell->GetViewData()->GetPixPos(eSplitPos));
			aVisArea.SetSize(pWindow->GetSizePixel());
		}
	}
	return aVisArea;
}

Rectangle ScAccessibleSpreadsheet::GetVisCells(const Rectangle& rVisArea)
{
	if (mpViewShell)
	{
        SCsCOL nStartX, nEndX;
        SCsROW nStartY, nEndY;

		mpViewShell->GetViewData()->GetPosFromPixel( 1, 1, meSplitPos, nStartX, nStartY);
		mpViewShell->GetViewData()->GetPosFromPixel( rVisArea.GetWidth(), rVisArea.GetHeight(), meSplitPos, nEndX, nEndY);

		return Rectangle(nStartX, nStartY, nEndX, nEndY);
	}
	else
		return Rectangle();
}
