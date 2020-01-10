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

#ifndef SC_ACREDLIN_HXX
#define SC_ACREDLIN_HXX

#ifndef _MOREBTN_HXX //autogen
#include <vcl/morebtn.hxx>
#endif
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif
#include <svtools/headbar.hxx>
#include <svtools/svtabbx.hxx>


#include "rangenam.hxx"
#include "anyrefdg.hxx"
#include <vcl/lstbox.hxx>

#ifndef _SVX_ACREDLIN_HXX
#include <svx/ctredlin.hxx>
#endif
#include <svx/simptabl.hxx>

#ifndef _SVARRAY_HXX
#define _SVARRAY_HXX
#include <svtools/svarray.hxx>
#endif
#include "chgtrack.hxx"
#include "chgviset.hxx"
#include <vcl/timer.hxx>

class ScViewData;
class ScDocument;

#define FLT_DATE_BEFORE		0
#define FLT_DATE_SINCE		1
#define FLT_DATE_EQUAL		2
#define FLT_DATE_NOTEQUAL	3
#define FLT_DATE_BETWEEN	4
#define FLT_DATE_SAVE		5


class ScViewEntryPtr
{
private:
	String*			pAction;
	String*			pPos;
	String*			pAuthor;
	String*			pDate;
	String*			pComment;
	void*			pData;

public:

	String*			GetpAction()	{return pAction; }
	String*			GetpPos()		{return pPos;    }
	String*			GetpAuthor()	{return	pAuthor; }
	String*			GetpDate()		{return	pDate;   }
	String*			GetpComment()	{return	pComment;}
	void*			GetpData()		{return pData;	 }

	void		SetpAction (String* pString)	{pAction= pString;}
	void		SetpPos    (String* pString)	{pPos	= pString;}
	void		SetpAuthor (String* pString)	{pAuthor= pString;}
	void		SetpDate   (String* pString)	{pDate	= pString;}
	void		SetpComment(String* pString)	{pComment=pString;}
	void		SetpData   (void*   pdata)		{pData	 =pdata;}
};

class ScViewEntryPtrList
{
	ScViewEntryPtrList* pNext;
	ScViewEntryPtrList* pLast;

	ScViewEntryPtr*	pData;
};


class ScRedlinData : public RedlinData
{
public:

					ScRedlinData();
					~ScRedlinData();
	SCTAB			nTable;
	SCCOL			nCol;
	SCROW			nRow;
	ULONG			nActionNo;
	ULONG			nInfo;
	BOOL			bIsRejectable;
	BOOL			bIsAcceptable;
};

typedef	long LExpNum;

//@ Expand-Entrys nicht eindeutig, daher gestrichen
//DECLARE_TABLE( ScChgTrackExps, LExpNum)
//==================================================================

class ScAcceptChgDlg : public SfxModelessDialog
{
private:

	Timer					aSelectionTimer;
	Timer					aReOpenTimer;
	SvxAcceptChgCtr 		aAcceptChgCtr;
	ScViewData*				pViewData;
	ScDocument*				pDoc;
	ScRangeName				aLocalRangeName;
	Selection				theCurSel;
	SvxTPFilter*			pTPFilter;
	SvxTPView*				pTPView;
	SvxRedlinTable*			pTheView; // PB 2006/02/02 #i48648 now SvHeaderTabListBox
	Size					MinSize;
	ScRangeList				aRangeList;
	ScChangeViewSettings	aChangeViewSet;
	String					aStrInsertCols;
	String					aStrInsertRows;
	String					aStrInsertTabs;
	String					aStrDeleteCols;
	String					aStrDeleteRows;
	String					aStrDeleteTabs;
	String					aStrMove;
	String					aStrContent;
	String					aStrReject;
	String					aUnknown;
	String					aStrAllAccepted;
	String					aStrAllRejected;
	String					aStrNoEntry;
	String					aStrContentWithChild;
	String					aStrChildContent;
	String					aStrChildOrgContent;
	String					aStrEmpty;
	ULONG					nAcceptCount;
	ULONG					nRejectCount;
	BOOL					bAcceptEnableFlag;
	BOOL					bRejectEnableFlag;
	BOOL					bNeedsUpdate;
	BOOL					bIgnoreMsg;
	BOOL					bNoSelection;
	BOOL					bHasFilterEntry;
	BOOL					bUseColor;
	//ScChgTrackExps			aExpandArray;

	void			Init();
	void			InitFilter();
//UNUSED2008-05  void			SetMyStaticData();

	DECL_LINK( FilterHandle, SvxTPFilter* );
	DECL_LINK( RefHandle, SvxTPFilter* );
	DECL_LINK( FilterModified, SvxTPFilter* );
	DECL_LINK( MinSizeHandle, SvxAcceptChgCtr*);
	DECL_LINK( RejectHandle, SvxTPView*);
	DECL_LINK( AcceptHandle, SvxTPView*);
	DECL_LINK( RejectAllHandle, SvxTPView*);
	DECL_LINK( AcceptAllHandle, SvxTPView*);
	DECL_LINK( ExpandingHandle, SvxRedlinTable*);
	DECL_LINK( SelectHandle, SvxRedlinTable*);
	DECL_LINK( RefInfoHandle, String*);

	DECL_LINK( UpdateSelectionHdl, Timer*);
	DECL_LINK( ChgTrackModHdl, ScChangeTrack*);
	DECL_LINK( CommandHdl, Control*);
	DECL_LINK( ReOpenTimerHdl, Timer*);
	DECL_LINK( ColCompareHdl, SvSortData*);



protected:

	virtual void	Resize();
	virtual BOOL	Close();

	void			RejectFiltered();
	void			AcceptFiltered();

	BOOL			IsValidAction(const ScChangeAction* pScChangeAction);

	String*			MakeTypeString(ScChangeActionType eType);

	SvLBoxEntry*	InsertChangeAction(const ScChangeAction* pScChangeAction,ScChangeActionState eState,
									SvLBoxEntry* pParent=NULL,BOOL bDelMaster=FALSE,
									BOOL bDisabled=FALSE,ULONG nPos=LIST_APPEND);

	SvLBoxEntry*	InsertFilteredAction(const ScChangeAction* pScChangeAction,ScChangeActionState eState,
									SvLBoxEntry* pParent=NULL,BOOL bDelMaster=FALSE,
									BOOL bDisabled=FALSE,ULONG nPos=LIST_APPEND);


	SvLBoxEntry*	InsertChangeActionContent(const ScChangeActionContent* pScChangeAction,
											  SvLBoxEntry* pParent,ULONG nSpecial);

	void			GetDependents( const ScChangeAction* pScChangeAction,
								ScChangeActionTable& aActionTable,
								SvLBoxEntry* pEntry);

	BOOL			InsertContentChilds(ScChangeActionTable* pActionTable,SvLBoxEntry* pParent);

	BOOL			InsertAcceptedORejected(SvLBoxEntry* pParent);

	BOOL			InsertDeletedChilds(const ScChangeAction *pChangeAction, ScChangeActionTable* pActionTable,
										SvLBoxEntry* pParent);

	BOOL			InsertChilds(ScChangeActionTable* pActionTable,SvLBoxEntry* pParent);

	void			AppendChanges(ScChangeTrack* pChanges,ULONG nStartAction, ULONG nEndAction,
									ULONG nPos=LIST_APPEND);

	void			RemoveEntrys(ULONG nStartAction,ULONG nEndAction);
	void			UpdateEntrys(ScChangeTrack* pChgTrack, ULONG nStartAction,ULONG nEndAction);

	void			UpdateView();
	void			ClearView();

	BOOL			Expand(ScChangeTrack* pChanges,const ScChangeAction* pScChangeAction,
							SvLBoxEntry* pEntry, BOOL bFilter=FALSE);

public:
					ScAcceptChgDlg( SfxBindings* pB, SfxChildWindow* pCW, Window* pParent,
							   ScViewData*		ptrViewData);

					~ScAcceptChgDlg();

	void			ReInit(ScViewData* ptrViewData);

	virtual long	PreNotify( NotifyEvent& rNEvt );

	void			Initialize (SfxChildWinInfo* pInfo);
	virtual void    FillInfo(SfxChildWinInfo&) const;

};


#endif // SC_NAMEDLG_HXX

