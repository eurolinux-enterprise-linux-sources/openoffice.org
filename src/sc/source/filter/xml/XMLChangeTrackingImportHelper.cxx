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
#include "XMLChangeTrackingImportHelper.hxx"
#include "XMLConverter.hxx"
#include "cell.hxx"
#include "document.hxx"
#include "chgviset.hxx"
#include "rangeutl.hxx"
#include <tools/debug.hxx>
#include <tools/datetime.hxx>
#include <svtools/zforlist.hxx>
#include <xmloff/xmluconv.hxx>

#define SC_CHANGE_ID_PREFIX "ct"

ScMyCellInfo::ScMyCellInfo()
	: pCell(NULL),
	sFormulaAddress(),
	sFormula(),
	sInputString(),
	fValue(0.0),
	nMatrixCols(0),
	nMatrixRows(0),
    eGrammar( formula::FormulaGrammar::GRAM_STORAGE_DEFAULT),
	nType(NUMBERFORMAT_ALL),
	nMatrixFlag(MM_NONE)
{
}

ScMyCellInfo::ScMyCellInfo(ScBaseCell* pTempCell, const rtl::OUString& rFormulaAddress, const rtl::OUString& rFormula,
            const formula::FormulaGrammar::Grammar eTempGrammar, const rtl::OUString& rInputString,
			const double& rValue, const sal_uInt16 nTempType, const sal_uInt8 nTempMatrixFlag, const sal_Int32 nTempMatrixCols,
			const sal_Int32 nTempMatrixRows)
	: pCell(pTempCell),
	sFormulaAddress(rFormulaAddress),
	sFormula(rFormula),
    sInputString(rInputString),
	fValue(rValue),
	nMatrixCols(nTempMatrixCols),
	nMatrixRows(nTempMatrixRows),
    eGrammar( eTempGrammar),
	nType(nTempType),
	nMatrixFlag(nTempMatrixFlag)
{
}

ScMyCellInfo::~ScMyCellInfo()
{
	if (pCell)
		pCell->Delete();
}

ScBaseCell* ScMyCellInfo::CreateCell(ScDocument* pDoc)
{
	if (pDoc)
	{
		if (!pCell && sFormula.getLength() && sFormulaAddress.getLength())
		{
			ScAddress aPos;
			sal_Int32 nOffset(0);
			ScRangeStringConverter::GetAddressFromString(aPos, sFormulaAddress, pDoc, ::formula::FormulaGrammar::CONV_OOO, nOffset);
			pCell = new ScFormulaCell(pDoc, aPos, sFormula, eGrammar, nMatrixFlag);
			static_cast<ScFormulaCell*>(pCell)->SetMatColsRows(static_cast<SCCOL>(nMatrixCols), static_cast<SCROW>(nMatrixRows));
		}

		if ((nType == NUMBERFORMAT_DATE || nType == NUMBERFORMAT_TIME) && sInputString.Len() == 0)
		{
            sal_uInt32 nFormat(0);
            if (nType == NUMBERFORMAT_DATE)
				nFormat = pDoc->GetFormatTable()->GetStandardFormat( NUMBERFORMAT_DATE, ScGlobal::eLnge );
			else if (nType == NUMBERFORMAT_TIME)
				nFormat = pDoc->GetFormatTable()->GetStandardFormat( NUMBERFORMAT_TIME, ScGlobal::eLnge );
            pDoc->GetFormatTable()->GetInputLineString(fValue, nFormat, sInputString);
		}
	}

    return pCell ? pCell->CloneWithoutNote( *pDoc ) : 0;
}

ScMyDeleted::ScMyDeleted()
	: pCellInfo(NULL)
{
}

ScMyDeleted::~ScMyDeleted()
{
	if (pCellInfo)
		delete pCellInfo;
}

ScMyGenerated::ScMyGenerated(ScMyCellInfo* pTempCellInfo, const ScBigRange& aTempBigRange)
	: aBigRange(aTempBigRange),
	nID(0),
	pCellInfo(pTempCellInfo)
{
}

ScMyGenerated::~ScMyGenerated()
{
	if (pCellInfo)
		delete pCellInfo;
}

ScMyBaseAction::ScMyBaseAction(const ScChangeActionType nTempActionType)
	: aDependencies(),
	aDeletedList(),
	nActionNumber(0),
	nRejectingNumber(0),
	nPreviousAction(0),
	nActionType(nTempActionType),
	nActionState(SC_CAS_VIRGIN)
{
}

ScMyBaseAction::~ScMyBaseAction()
{
}

ScMyInsAction::ScMyInsAction(const ScChangeActionType nActionTypeP)
    : ScMyBaseAction(nActionTypeP)
{
}

ScMyInsAction::~ScMyInsAction()
{
}

ScMyDelAction::ScMyDelAction(const ScChangeActionType nActionTypeP)
    : ScMyBaseAction(nActionTypeP),
	aGeneratedList(),
	pInsCutOff(NULL),
	aMoveCutOffs(),
	nD(0)
{
}

ScMyDelAction::~ScMyDelAction()
{
	if (pInsCutOff)
		delete pInsCutOff;
}

ScMyMoveAction::ScMyMoveAction()
	: ScMyBaseAction(SC_CAT_MOVE),
	aGeneratedList(),
	pMoveRanges(NULL)
{
}

ScMyMoveAction::~ScMyMoveAction()
{
	if (pMoveRanges)
		delete pMoveRanges;
}


ScMyContentAction::ScMyContentAction()
	: ScMyBaseAction(SC_CAT_CONTENT),
	pCellInfo(NULL)
{
}

ScMyContentAction::~ScMyContentAction()
{
	if (pCellInfo)
		delete pCellInfo;
}

ScMyRejAction::ScMyRejAction()
	: ScMyBaseAction(SC_CAT_REJECT)
{
}

ScMyRejAction::~ScMyRejAction()
{
}

ScXMLChangeTrackingImportHelper::ScXMLChangeTrackingImportHelper()
    : aUsers(),
	aActions(),
	pDoc(NULL),
	pTrack(NULL),
	pCurrentAction(NULL),
    sIDPrefix(RTL_CONSTASCII_USTRINGPARAM(SC_CHANGE_ID_PREFIX)),
	nMultiSpanned(0),
	nMultiSpannedSlaveCount(0),
	bChangeTrack(sal_False)
{
	nPrefixLength = sIDPrefix.getLength();
}

ScXMLChangeTrackingImportHelper::~ScXMLChangeTrackingImportHelper()
{
}

void ScXMLChangeTrackingImportHelper::StartChangeAction(const ScChangeActionType nActionType)
{
	DBG_ASSERT(!pCurrentAction, "a not inserted action");
	switch (nActionType)
	{
		case SC_CAT_INSERT_COLS:
		case SC_CAT_INSERT_ROWS:
		case SC_CAT_INSERT_TABS:
		{
			pCurrentAction = new ScMyInsAction(nActionType);
		}
		break;
		case SC_CAT_DELETE_COLS:
		case SC_CAT_DELETE_ROWS:
		case SC_CAT_DELETE_TABS:
		{
			pCurrentAction = new ScMyDelAction(nActionType);
		}
		break;
		case SC_CAT_MOVE:
		{
			pCurrentAction = new ScMyMoveAction();
		}
		break;
		case SC_CAT_CONTENT:
		{
			pCurrentAction = new ScMyContentAction();
		}
		break;
		case SC_CAT_REJECT:
		{
			pCurrentAction = new ScMyRejAction();
		}
		break;
        default:
        {
            // added to avoid warnings
        }
	}
}

sal_uInt32 ScXMLChangeTrackingImportHelper::GetIDFromString(const rtl::OUString& sID)
{
	sal_uInt32 nResult(0);
	sal_uInt32 nLength(sID.getLength());
	if (nLength)
	{
		if (sID.compareTo(sIDPrefix, nPrefixLength) == 0)
		{
			rtl::OUString sValue(sID.copy(nPrefixLength, nLength - nPrefixLength));
			sal_Int32 nValue;
			SvXMLUnitConverter::convertNumber(nValue, sValue);
			DBG_ASSERT(nValue > 0, "wrong change action ID");
			nResult = nValue;
		}
		else
		{
			DBG_ERROR("wrong change action ID");
		}
	}
	return nResult;
}

void ScXMLChangeTrackingImportHelper::SetActionInfo(const ScMyActionInfo& aInfo)
{
	pCurrentAction->aInfo = aInfo;
	String aUser(aInfo.sUser);
	StrData* pStrData = new StrData( aUser );
	if ( !aUsers.Insert( pStrData ) )
		delete pStrData;
}

void ScXMLChangeTrackingImportHelper::SetPreviousChange(const sal_uInt32 nPreviousAction,
							ScMyCellInfo* pCellInfo)
{
	DBG_ASSERT(pCurrentAction->nActionType == SC_CAT_CONTENT, "wrong action type");
	ScMyContentAction* pAction = static_cast<ScMyContentAction*>(pCurrentAction);
	pAction->nPreviousAction = nPreviousAction;
	pAction->pCellInfo = pCellInfo;
}

void ScXMLChangeTrackingImportHelper::SetPosition(const sal_Int32 nPosition, const sal_Int32 nCount, const sal_Int32 nTable)
{
	DBG_ASSERT(((pCurrentAction->nActionType != SC_CAT_MOVE) &&
				(pCurrentAction->nActionType != SC_CAT_CONTENT) &&
				(pCurrentAction->nActionType != SC_CAT_REJECT)), "wrong action type");
	DBG_ASSERT(nCount > 0, "wrong count");
	switch(pCurrentAction->nActionType)
	{
		case SC_CAT_INSERT_COLS:
		case SC_CAT_DELETE_COLS:
		{
			pCurrentAction->aBigRange.Set(nPosition, nInt32Min, nTable,
										nPosition + nCount - 1, nInt32Max, nTable);
		}
		break;
		case SC_CAT_INSERT_ROWS:
		case SC_CAT_DELETE_ROWS:
		{
			pCurrentAction->aBigRange.Set(nInt32Min, nPosition, nTable,
										nInt32Max, nPosition + nCount - 1, nTable);
		}
		break;
		case SC_CAT_INSERT_TABS:
		case SC_CAT_DELETE_TABS:
		{
			pCurrentAction->aBigRange.Set(nInt32Min, nInt32Min, nPosition,
										nInt32Max, nInt32Max, nPosition + nCount - 1);
		}
		break;
        default:
        {
            // added to avoid warnings
        }
	}
}

void ScXMLChangeTrackingImportHelper::AddDeleted(const sal_uInt32 nID)
{
	ScMyDeleted* pDeleted = new ScMyDeleted();
	pDeleted->nID = nID;
	pCurrentAction->aDeletedList.push_front(pDeleted);
}

void ScXMLChangeTrackingImportHelper::AddDeleted(const sal_uInt32 nID, ScMyCellInfo* pCellInfo)
{
	ScMyDeleted* pDeleted = new ScMyDeleted();
	pDeleted->nID = nID;
	pDeleted->pCellInfo = pCellInfo;
	pCurrentAction->aDeletedList.push_front(pDeleted);
}

void ScXMLChangeTrackingImportHelper::SetMultiSpanned(const sal_Int16 nTempMultiSpanned)
{
	if (nTempMultiSpanned)
	{
		DBG_ASSERT(((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
					(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS)), "wrong action type");
		nMultiSpanned = nTempMultiSpanned;
		nMultiSpannedSlaveCount = 0;
	}
}

void ScXMLChangeTrackingImportHelper::SetInsertionCutOff(const sal_uInt32 nID, const sal_Int32 nPosition)
{
	if ((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
		(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS))
	{
		static_cast<ScMyDelAction*>(pCurrentAction)->pInsCutOff = new ScMyInsertionCutOff(nID, nPosition);
	}
	else
	{
		DBG_ERROR("wrong action type");
	}
}

void ScXMLChangeTrackingImportHelper::AddMoveCutOff(const sal_uInt32 nID, const sal_Int32 nStartPosition, const sal_Int32 nEndPosition)
{
	if ((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
		(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS))
	{
		static_cast<ScMyDelAction*>(pCurrentAction)->aMoveCutOffs.push_front(ScMyMoveCutOff(nID, nStartPosition, nEndPosition));
	}
	else
	{
		DBG_ERROR("wrong action type");
	}
}

void ScXMLChangeTrackingImportHelper::SetMoveRanges(const ScBigRange& aSourceRange, const ScBigRange& aTargetRange)
{
	if (pCurrentAction->nActionType == SC_CAT_MOVE)
	{
 		static_cast<ScMyMoveAction*>(pCurrentAction)->pMoveRanges = new ScMyMoveRanges(aSourceRange, aTargetRange);
	}
	else
	{
		DBG_ERROR("wrong action type");
	}
}

void ScXMLChangeTrackingImportHelper::GetMultiSpannedRange()
{
	if ((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
		(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS))
	{
		if (nMultiSpannedSlaveCount)
		{
			static_cast<ScMyDelAction*>(pCurrentAction)->nD = nMultiSpannedSlaveCount;
		}
		++nMultiSpannedSlaveCount;
		if (nMultiSpannedSlaveCount >= nMultiSpanned)
		{
			nMultiSpanned = 0;
			nMultiSpannedSlaveCount = 0;
		}
	}
	else
	{
		DBG_ERROR("wrong action type");
	}
}

void ScXMLChangeTrackingImportHelper::AddGenerated(ScMyCellInfo* pCellInfo, const ScBigRange& aBigRange)
{
	ScMyGenerated* pGenerated = new ScMyGenerated(pCellInfo, aBigRange);
	if (pCurrentAction->nActionType == SC_CAT_MOVE)
	{
		static_cast<ScMyMoveAction*>(pCurrentAction)->aGeneratedList.push_back(pGenerated);
	}
	else if ((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
		(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS))
	{
		static_cast<ScMyDelAction*>(pCurrentAction)->aGeneratedList.push_back(pGenerated);
	}
	else
	{
		DBG_ERROR("try to insert a generated action to a wrong action");
	}
}

void ScXMLChangeTrackingImportHelper::EndChangeAction()
{
	if ((pCurrentAction->nActionType == SC_CAT_DELETE_COLS) ||
		(pCurrentAction->nActionType == SC_CAT_DELETE_ROWS))
		GetMultiSpannedRange();
	if (pCurrentAction && pCurrentAction->nActionNumber > 0)
		aActions.push_back(pCurrentAction);
	else
	{
		DBG_ERROR("no current action");
	}
	pCurrentAction = NULL;
}

void ScXMLChangeTrackingImportHelper::ConvertInfo(const ScMyActionInfo& aInfo, String& rUser, DateTime& aDateTime)
{
	Date aDate(aInfo.aDateTime.Day, aInfo.aDateTime.Month, aInfo.aDateTime.Year);
	Time aTime(aInfo.aDateTime.Hours, aInfo.aDateTime.Minutes, aInfo.aDateTime.Seconds, aInfo.aDateTime.HundredthSeconds);
    aDateTime.SetDate( aDate.GetDate() );
    aDateTime.SetTime( aTime.GetTime() );

    // #97286# old files didn't store 100th seconds, enable again
    if ( aInfo.aDateTime.HundredthSeconds )
        pTrack->SetTime100thSeconds( TRUE );

    StrData aStrData( aInfo.sUser );
    USHORT nPos;
    if ( pTrack->GetUserCollection().Search( &aStrData, nPos ) )
    {
        const StrData* pUser = static_cast<const StrData*>( pTrack->GetUserCollection().At( nPos ) );
        if ( pUser )
            rUser = pUser->GetString();
        else
            rUser = aInfo.sUser;    // shouldn't happen
    }
    else
        rUser = aInfo.sUser;    // shouldn't happen
}

ScChangeAction*	ScXMLChangeTrackingImportHelper::CreateInsertAction(ScMyInsAction* pAction)
{
    DateTime aDateTime( Date(0), Time(0) );
    String aUser;
    ConvertInfo(pAction->aInfo, aUser, aDateTime);

	String sComment (pAction->aInfo.sComment);

	ScChangeAction* pNewAction = new ScChangeActionIns(pAction->nActionNumber, pAction->nActionState, pAction->nRejectingNumber,
		pAction->aBigRange, aUser, aDateTime, sComment, pAction->nActionType);
	return pNewAction;
}

ScChangeAction*	ScXMLChangeTrackingImportHelper::CreateDeleteAction(ScMyDelAction* pAction)
{
    DateTime aDateTime( Date(0), Time(0) );
    String aUser;
    ConvertInfo(pAction->aInfo, aUser, aDateTime);

	String sComment (pAction->aInfo.sComment);

	ScChangeAction* pNewAction = new ScChangeActionDel(pAction->nActionNumber, pAction->nActionState, pAction->nRejectingNumber,
		pAction->aBigRange, aUser, aDateTime, sComment, pAction->nActionType, pAction->nD, pTrack);
	return pNewAction;
}

ScChangeAction*	ScXMLChangeTrackingImportHelper::CreateMoveAction(ScMyMoveAction* pAction)
{
	DBG_ASSERT(pAction->pMoveRanges, "no move ranges");
	if (pAction->pMoveRanges)
	{
        DateTime aDateTime( Date(0), Time(0) );
        String aUser;
        ConvertInfo(pAction->aInfo, aUser, aDateTime);

		String sComment (pAction->aInfo.sComment);

		ScChangeAction* pNewAction = new ScChangeActionMove(pAction->nActionNumber, pAction->nActionState, pAction->nRejectingNumber,
			pAction->pMoveRanges->aTargetRange, aUser, aDateTime, sComment, pAction->pMoveRanges->aSourceRange , pTrack);
		return pNewAction;
	}
	return NULL;
}

ScChangeAction*	ScXMLChangeTrackingImportHelper::CreateRejectionAction(ScMyRejAction* pAction)
{
    DateTime aDateTime( Date(0), Time(0) );
    String aUser;
    ConvertInfo(pAction->aInfo, aUser, aDateTime);

	String sComment (pAction->aInfo.sComment);

	ScChangeAction* pNewAction = new ScChangeActionReject(pAction->nActionNumber, pAction->nActionState, pAction->nRejectingNumber,
		pAction->aBigRange, aUser, aDateTime, sComment);
	return pNewAction;
}

ScChangeAction*	ScXMLChangeTrackingImportHelper::CreateContentAction(ScMyContentAction* pAction)
{
	ScBaseCell* pCell = NULL;
	if (pAction->pCellInfo)
		 pCell = pAction->pCellInfo->CreateCell(pDoc);

    DateTime aDateTime( Date(0), Time(0) );
    String aUser;
    ConvertInfo(pAction->aInfo, aUser, aDateTime);

	String sComment (pAction->aInfo.sComment);

	ScChangeAction* pNewAction = new ScChangeActionContent(pAction->nActionNumber, pAction->nActionState, pAction->nRejectingNumber,
		pAction->aBigRange, aUser, aDateTime, sComment, pCell, pDoc, pAction->pCellInfo->sInputString);
	return pNewAction;
}

void ScXMLChangeTrackingImportHelper::CreateGeneratedActions(ScMyGeneratedList& rList)
{
	if (!rList.empty())
	{
		ScMyGeneratedList::iterator aItr(rList.begin());
		ScMyGeneratedList::iterator aEndItr(rList.end());
		while (aItr != aEndItr)
		{
			if (((*aItr)->nID == 0))
			{
				ScBaseCell* pCell = NULL;
				if ((*aItr)->pCellInfo)
					pCell = (*aItr)->pCellInfo->CreateCell(pDoc);

				if (pCell)
				{
					(*aItr)->nID = pTrack->AddLoadedGenerated(pCell, (*aItr)->aBigRange, (*aItr)->pCellInfo->sInputString );
					DBG_ASSERT((*aItr)->nID, "could not insert generated action");
				}
			}
			++aItr;
		}
	}
}

void ScXMLChangeTrackingImportHelper::SetDeletionDependencies(ScMyDelAction* pAction, ScChangeActionDel* pDelAct)
{
	if (!pAction->aGeneratedList.empty())
	{
		DBG_ASSERT(((pAction->nActionType == SC_CAT_DELETE_COLS) ||
			(pAction->nActionType == SC_CAT_DELETE_ROWS) ||
			(pAction->nActionType == SC_CAT_DELETE_TABS)), "wrong action type");
		if (pDelAct)
		{
			ScMyGeneratedList::iterator aItr(pAction->aGeneratedList.begin());
			ScMyGeneratedList::iterator aEndItr(pAction->aGeneratedList.end());
			while (aItr != aEndItr)
			{
				DBG_ASSERT((*aItr)->nID, "a not inserted generated action");
				pDelAct->SetDeletedInThis((*aItr)->nID, pTrack);
				if (*aItr)
					delete *aItr;
				aItr = pAction->aGeneratedList.erase(aItr);
			}
		}
	}
	if (pAction->pInsCutOff)
	{
		DBG_ASSERT(((pAction->nActionType == SC_CAT_DELETE_COLS) ||
			(pAction->nActionType == SC_CAT_DELETE_ROWS) ||
			(pAction->nActionType == SC_CAT_DELETE_TABS)), "wrong action type");
		ScChangeAction* pChangeAction = pTrack->GetAction(pAction->pInsCutOff->nID);
		if (pChangeAction && pChangeAction->IsInsertType())
		{
			ScChangeActionIns* pInsAction = static_cast<ScChangeActionIns*>(pChangeAction);
			if (pInsAction && pDelAct)
				pDelAct->SetCutOffInsert(pInsAction, static_cast<sal_Int16>(pAction->pInsCutOff->nPosition));
		}
		else
		{
			DBG_ERROR("no cut off insert action");
		}
	}
	if (!pAction->aMoveCutOffs.empty())
	{
		DBG_ASSERT(((pAction->nActionType == SC_CAT_DELETE_COLS) ||
			(pAction->nActionType == SC_CAT_DELETE_ROWS) ||
			(pAction->nActionType == SC_CAT_DELETE_TABS)), "wrong action type");
		ScMyMoveCutOffs::iterator aItr(pAction->aMoveCutOffs.begin());
		ScMyMoveCutOffs::iterator aEndItr(pAction->aMoveCutOffs.end());
		while(aItr != aEndItr)
		{
			ScChangeAction* pChangeAction = pTrack->GetAction(aItr->nID);
			if (pChangeAction && (pChangeAction->GetType() == SC_CAT_MOVE))
			{
				ScChangeActionMove* pMoveAction = static_cast<ScChangeActionMove*>(pChangeAction);
				if (pMoveAction && pDelAct)
					pDelAct->AddCutOffMove(pMoveAction, static_cast<sal_Int16>(aItr->nStartPosition),
										static_cast<sal_Int16>(aItr->nEndPosition));
			}
			else
			{
				DBG_ERROR("no cut off move action");
			}
			aItr = pAction->aMoveCutOffs.erase(aItr);
		}
	}
}

void ScXMLChangeTrackingImportHelper::SetMovementDependencies(ScMyMoveAction* pAction, ScChangeActionMove* pMoveAct)
{
	if (!pAction->aGeneratedList.empty())
	{
		if (pAction->nActionType == SC_CAT_MOVE)
		{
			if (pMoveAct)
			{
				ScMyGeneratedList::iterator aItr(pAction->aGeneratedList.begin());
				ScMyGeneratedList::iterator aEndItr(pAction->aGeneratedList.end());
				while (aItr != aEndItr)
				{
					DBG_ASSERT((*aItr)->nID, "a not inserted generated action");
					pMoveAct->SetDeletedInThis((*aItr)->nID, pTrack);
					if (*aItr)
						delete *aItr;
					aItr = pAction->aGeneratedList.erase(aItr);
				}
			}
		}
	}
}

void ScXMLChangeTrackingImportHelper::SetContentDependencies(ScMyContentAction* pAction, ScChangeActionContent* pActContent)
{
	if (pAction->nPreviousAction)
	{
		DBG_ASSERT(pAction->nActionType == SC_CAT_CONTENT, "wrong action type");
		ScChangeAction* pPrevAct = pTrack->GetAction(pAction->nPreviousAction);
		if (pPrevAct)
		{
			ScChangeActionContent* pPrevActContent = static_cast<ScChangeActionContent*>(pPrevAct);
			if (pPrevActContent && pActContent)
			{
				pActContent->SetPrevContent(pPrevActContent);
				pPrevActContent->SetNextContent(pActContent);
				const ScBaseCell* pOldCell = pActContent->GetOldCell();
				if (pOldCell)
				{
                    ScBaseCell* pNewCell = pOldCell->CloneWithoutNote( *pDoc );
					if (pNewCell)
                    {
						pPrevActContent->SetNewCell(pNewCell, pDoc, EMPTY_STRING);
                        pPrevActContent->SetNewValue(pActContent->GetOldCell(), pDoc);
                    }
				}
			}
		}
	}
}

void ScXMLChangeTrackingImportHelper::SetDependencies(ScMyBaseAction* pAction)
{
	ScChangeAction* pAct = pTrack->GetAction(pAction->nActionNumber);
	if (pAct)
	{
		if (!pAction->aDependencies.empty())
		{
			ScMyDependencies::iterator aItr(pAction->aDependencies.begin());
			ScMyDependencies::iterator aEndItr(pAction->aDependencies.end());
			while(aItr != aEndItr)
			{
				pAct->AddDependent(*aItr, pTrack);
				aItr = pAction->aDependencies.erase(aItr);
			}
		}
		if (!pAction->aDeletedList.empty())
		{
			ScMyDeletedList::iterator aItr(pAction->aDeletedList.begin());
			ScMyDeletedList::iterator aEndItr(pAction->aDeletedList.end());
			while(aItr != aEndItr)
			{
				pAct->SetDeletedInThis((*aItr)->nID, pTrack);
				ScChangeAction* pDeletedAct = pTrack->GetAction((*aItr)->nID);
				if ((pDeletedAct->GetType() == SC_CAT_CONTENT) && (*aItr)->pCellInfo)
				{
					ScChangeActionContent* pContentAct = static_cast<ScChangeActionContent*>(pDeletedAct);
					if (pContentAct && (*aItr)->pCellInfo)
					{
						ScBaseCell* pCell = (*aItr)->pCellInfo->CreateCell(pDoc);
						if (!ScBaseCell::CellEqual(pCell, pContentAct->GetNewCell()))
                        {
                            // #i40704# Don't overwrite SetNewCell result by calling SetNewValue,
                            // instead pass the input string to SetNewCell.
                            pContentAct->SetNewCell(pCell, pDoc, (*aItr)->pCellInfo->sInputString);
                        }
					}
				}
				if (*aItr)
					delete *aItr;
				aItr = pAction->aDeletedList.erase(aItr);
			}
		}
		if ((pAction->nActionType == SC_CAT_DELETE_COLS) ||
			(pAction->nActionType == SC_CAT_DELETE_ROWS))
			SetDeletionDependencies(static_cast<ScMyDelAction*>(pAction), static_cast<ScChangeActionDel*>(pAct));
		else if (pAction->nActionType == SC_CAT_MOVE)
			SetMovementDependencies(static_cast<ScMyMoveAction*>(pAction), static_cast<ScChangeActionMove*>(pAct));
		else if (pAction->nActionType == SC_CAT_CONTENT)
			SetContentDependencies(static_cast<ScMyContentAction*>(pAction), static_cast<ScChangeActionContent*>(pAct));
	}
	else
	{
		DBG_ERROR("could not find the action");
	}
}

void ScXMLChangeTrackingImportHelper::SetNewCell(ScMyContentAction* pAction)
{
	ScChangeAction* pChangeAction = pTrack->GetAction(pAction->nActionNumber);
	if (pChangeAction)
	{
		ScChangeActionContent* pChangeActionContent = static_cast<ScChangeActionContent*>(pChangeAction);
		if (pChangeActionContent)
		{
			if (pChangeActionContent->IsTopContent() && !pChangeActionContent->IsDeletedIn())
			{
				sal_Int32 nCol, nRow, nTab, nCol2, nRow2, nTab2;
				pAction->aBigRange.GetVars(nCol, nRow, nTab, nCol2, nRow2, nTab2);
				if ((nCol >= 0) && (nCol <= MAXCOL) &&
					(nRow >= 0) && (nRow <= MAXROW) &&
					(nTab >= 0) && (nTab <= MAXTAB))
				{
					ScAddress aAddress (static_cast<SCCOL>(nCol),
										static_cast<SCROW>(nRow),
										static_cast<SCTAB>(nTab));
					ScBaseCell* pCell = pDoc->GetCell(aAddress);
					if (pCell)
					{
						ScBaseCell* pNewCell = NULL;
						if (pCell->GetCellType() != CELLTYPE_FORMULA)
                            pNewCell = pCell->CloneWithoutNote( *pDoc );
						else
						{
							sal_uInt8 nMatrixFlag = static_cast<ScFormulaCell*>(pCell)->GetMatrixFlag();
							String sFormula;
                            // With GRAM_ODFF reference detection is faster on compilation.
                            /* FIXME: new cell should be created with a clone
                             * of the token array instead. Any reason why this
                             * wasn't done? */
							static_cast<ScFormulaCell*>(pCell)->GetFormula(sFormula,formula::FormulaGrammar::GRAM_ODFF);
							rtl::OUString sOUFormula(sFormula);

                            // #i87826# [Collaboration] Rejected move destroys formulas
                            // FIXME: adjust ScFormulaCell::GetFormula(), so that the right formula string
                            //        is returned and no further string handling is necessary
                            rtl::OUString sOUFormula2;
                            if ( nMatrixFlag != MM_NONE )
                            {
                                sOUFormula2 = sOUFormula.copy( 2, sOUFormula.getLength() - 3 );
                            }
                            else
                            {
                                sOUFormula2 = sOUFormula.copy( 1, sOUFormula.getLength() - 1 );
                            }

							String sFormula2(sOUFormula2);
							pNewCell = new ScFormulaCell(pDoc, aAddress, sFormula2,formula::FormulaGrammar::GRAM_ODFF, nMatrixFlag);
							if (pNewCell)
							{
								if (nMatrixFlag == MM_FORMULA)
								{
									SCCOL nCols;
									SCROW nRows;
									static_cast<ScFormulaCell*>(pCell)->GetMatColsRows(nCols, nRows);
									static_cast<ScFormulaCell*>(pNewCell)->SetMatColsRows(nCols, nRows);
								}
								static_cast<ScFormulaCell*>(pNewCell)->SetInChangeTrack(sal_True);
							}
						}
                        pChangeActionContent->SetNewCell(pNewCell, pDoc, EMPTY_STRING);
                        // #i40704# don't overwrite the formula string from above with pCell's content
                        if (pCell->GetCellType() != CELLTYPE_FORMULA)
                            pChangeActionContent->SetNewValue(pCell, pDoc);
					}
				}
				else
				{
					DBG_ERROR("wrong cell position");
				}
			}
		}
	}
}

void ScXMLChangeTrackingImportHelper::CreateChangeTrack(ScDocument* pTempDoc)
{
	pDoc = pTempDoc;
	if (pDoc)
	{
		pTrack = new ScChangeTrack(pDoc, aUsers);
        // #97286# old files didn't store 100th seconds, disable until encountered
        pTrack->SetTime100thSeconds( FALSE );

		ScMyActions::iterator aItr(aActions.begin());
		ScMyActions::iterator aEndItr(aActions.end());
		while (aItr != aEndItr)
		{
			ScChangeAction* pAction = NULL;

			switch ((*aItr)->nActionType)
			{
				case SC_CAT_INSERT_COLS:
				case SC_CAT_INSERT_ROWS:
				case SC_CAT_INSERT_TABS:
				{
					pAction = CreateInsertAction(static_cast<ScMyInsAction*>(*aItr));
				}
				break;
				case SC_CAT_DELETE_COLS:
				case SC_CAT_DELETE_ROWS:
				case SC_CAT_DELETE_TABS:
				{
					ScMyDelAction* pDelAct = static_cast<ScMyDelAction*>(*aItr);
					pAction = CreateDeleteAction(pDelAct);
					CreateGeneratedActions(pDelAct->aGeneratedList);
				}
				break;
				case SC_CAT_MOVE:
				{
					ScMyMoveAction* pMovAct = static_cast<ScMyMoveAction*>(*aItr);
					pAction = CreateMoveAction(pMovAct);
					CreateGeneratedActions(pMovAct->aGeneratedList);
				}
				break;
				case SC_CAT_CONTENT:
				{
					pAction = CreateContentAction(static_cast<ScMyContentAction*>(*aItr));
				}
				break;
				case SC_CAT_REJECT:
				{
					pAction = CreateRejectionAction(static_cast<ScMyRejAction*>(*aItr));
				}
				break;
                default:
                {
                    // added to avoid warnings
                }
			}

			if (pAction)
				pTrack->AppendLoaded(pAction);
			else
			{
				DBG_ERROR("no action");
			}

			++aItr;
		}
		if (pTrack->GetLast())
			pTrack->SetActionMax(pTrack->GetLast()->GetActionNumber());

        aItr = aActions.begin();
        aEndItr = aActions.end();
		while (aItr != aEndItr)
		{
			SetDependencies(*aItr);

			if ((*aItr)->nActionType == SC_CAT_CONTENT)
				++aItr;
			else
			{
				if (*aItr)
					delete (*aItr);
				aItr = aActions.erase(aItr);
			}
		}

		aItr = aActions.begin();
        aEndItr = aActions.end();
		while (aItr != aEndItr)
		{
			DBG_ASSERT((*aItr)->nActionType == SC_CAT_CONTENT, "wrong action type");
			SetNewCell(static_cast<ScMyContentAction*>(*aItr));
			if (*aItr)
				delete (*aItr);
			aItr = aActions.erase(aItr);
		}
		if (aProtect.getLength())
			pTrack->SetProtection(aProtect);
        else if (pDoc->GetChangeTrack() && pDoc->GetChangeTrack()->IsProtected())
            pTrack->SetProtection(pDoc->GetChangeTrack()->GetProtection());

	    if ( pTrack->GetLast() )
		    pTrack->SetLastSavedActionNumber(pTrack->GetLast()->GetActionNumber());

		pDoc->SetChangeTrack(pTrack);
	}
}
