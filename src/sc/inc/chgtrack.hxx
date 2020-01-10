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

#ifndef SC_CHGTRACK_HXX
#define SC_CHGTRACK_HXX


#include <tools/string.hxx>
#include <tools/datetime.hxx>
#include <tools/table.hxx>
#include <tools/stack.hxx>
#include <tools/queue.hxx>
#include <tools/mempool.hxx>
#include <tools/link.hxx>
#include <svtools/lstner.hxx>
#include "global.hxx"
#include "bigrange.hxx"
#include "collect.hxx"
#include "scdllapi.h"

#ifdef SC_CHGTRACK_CXX
// core/inc
#include "refupdat.hxx"
#endif

#define DEBUG_CHANGETRACK 0


class ScBaseCell;
class ScDocument;


enum ScChangeActionType
{
	SC_CAT_NONE,
	SC_CAT_INSERT_COLS,
	SC_CAT_INSERT_ROWS,
	SC_CAT_INSERT_TABS,
	SC_CAT_DELETE_COLS,
	SC_CAT_DELETE_ROWS,
	SC_CAT_DELETE_TABS,
	SC_CAT_MOVE,
	SC_CAT_CONTENT,
	SC_CAT_REJECT
};


enum ScChangeActionState
{
	SC_CAS_VIRGIN,
	SC_CAS_ACCEPTED,
	SC_CAS_REJECTED
};


enum ScChangeActionClipMode
{
	SC_CACM_NONE,
	SC_CACM_CUT,
	SC_CACM_COPY,
	SC_CACM_PASTE
};

class SvStream;

// --- ScChangeActionLinkEntry ---------------------------------------------

// Fuegt sich selbst am Beginn einer Kette ein, bzw. vor einem anderen
// LinkEntry, on delete selbstaendiges ausklinken auch des gelinkten.
// ppPrev == &previous->pNext oder Adresse des Pointers auf Beginn der Kette,
// *ppPrev == this

class ScChangeAction;

class ScChangeActionLinkEntry
{
								// not implemented, prevent usage
								ScChangeActionLinkEntry(
									const ScChangeActionLinkEntry& );
	ScChangeActionLinkEntry&	operator=( const ScChangeActionLinkEntry& );

protected:

	ScChangeActionLinkEntry*	pNext;
	ScChangeActionLinkEntry**	ppPrev;
	ScChangeAction*				pAction;
	ScChangeActionLinkEntry*	pLink;

public:

	DECL_FIXEDMEMPOOL_NEWDEL( ScChangeActionLinkEntry )

								ScChangeActionLinkEntry(
										ScChangeActionLinkEntry** ppPrevP,
										ScChangeAction* pActionP )
									:	pNext( *ppPrevP ),
										ppPrev( ppPrevP ),
										pAction( pActionP ),
										pLink( NULL )
									{
										if ( pNext )
											pNext->ppPrev = &pNext;
										*ppPrevP = this;
									}

	virtual						~ScChangeActionLinkEntry()
									{
										ScChangeActionLinkEntry* p = pLink;
										UnLink();
										Remove();
										if ( p )
											delete p;
									}

			void				SetLink( ScChangeActionLinkEntry* pLinkP )
									{
										UnLink();
										if ( pLinkP )
										{
											pLink = pLinkP;
											pLinkP->pLink = this;
										}
									}

			void				UnLink()
									{
										if ( pLink )
										{
											pLink->pLink = NULL;
											pLink = NULL;
										}
									}

			void				Remove()
									{
										if ( ppPrev )
										{
                                            if ( ( *ppPrev = pNext ) != NULL )
												pNext->ppPrev = ppPrev;
											ppPrev = NULL;	// not inserted
										}
									}

			void				Insert( ScChangeActionLinkEntry** ppPrevP )
									{
										if ( !ppPrev )
										{
											ppPrev = ppPrevP;
											if ( (pNext = *ppPrevP) )
												pNext->ppPrev = &pNext;
											*ppPrevP = this;
										}
									}

	const ScChangeActionLinkEntry*	GetLink() const		{ return pLink; }
	ScChangeActionLinkEntry*		GetLink()			{ return pLink; }
	const ScChangeActionLinkEntry*	GetNext() const		{ return pNext; }
	ScChangeActionLinkEntry*		GetNext()			{ return pNext; }
	const ScChangeAction*			GetAction() const	{ return pAction; }
	ScChangeAction*					GetAction()			{ return pAction; }
#if DEBUG_CHANGETRACK
    String                          ToString() const;
#endif // DEBUG_CHANGETRACK
};

// --- ScChangeActionCellListEntry -----------------------------------------
// this is only for the XML Export in the hxx
class ScChangeActionContent;

class ScChangeActionCellListEntry
{
	friend class ScChangeAction;
	friend class ScChangeActionDel;
	friend class ScChangeActionMove;
	friend class ScChangeTrack;

			ScChangeActionCellListEntry*	pNext;
			ScChangeActionContent*			pContent;

								ScChangeActionCellListEntry(
									ScChangeActionContent* pContentP,
									ScChangeActionCellListEntry* pNextP )
									:	pNext( pNextP ),
										pContent( pContentP )
									{}

public:
	const ScChangeActionCellListEntry* GetNext() const { return pNext; } // this is only for the XML Export public
	const ScChangeActionContent* GetContent() const { return pContent; } // this is only for the XML Export public

	DECL_FIXEDMEMPOOL_NEWDEL( ScChangeActionCellListEntry )
};

// --- ScChangeAction -------------------------------------------------------

class ScChangeTrack;
class ScChangeActionIns;
class ScChangeActionDel;
class ScChangeActionContent;

class ScChangeAction
{
	friend class ScChangeTrack;
	friend class ScChangeActionIns;
	friend class ScChangeActionDel;
	friend class ScChangeActionMove;
	friend class ScChangeActionContent;

								// not implemented, prevent usage
								ScChangeAction( const ScChangeAction& );
			ScChangeAction&		operator=( const ScChangeAction& );

protected:

			ScBigRange	  		aBigRange;		 	// Ins/Del/MoveTo/ContentPos
			DateTime			aDateTime;			//! UTC
			String				aUser;				// wer war's
			String				aComment;			// Benutzerkommentar
			ScChangeAction*		pNext;				// naechster in Kette
			ScChangeAction*		pPrev;				// vorheriger in Kette
			ScChangeActionLinkEntry*	pLinkAny;	// irgendwelche Links
			ScChangeActionLinkEntry*	pLinkDeletedIn;	// Zuordnung zu
													// geloeschten oder
													// druebergemoveten oder
													// rejecteten Insert
													// Bereichen
			ScChangeActionLinkEntry*	pLinkDeleted;	// Links zu geloeschten
			ScChangeActionLinkEntry*	pLinkDependent;	// Links zu abhaengigen
			ULONG				nAction;
			ULONG				nRejectAction;
			ScChangeActionType	eType;
			ScChangeActionState	eState;


								ScChangeAction( ScChangeActionType,
												const ScRange& );

								// only to be used in the XML import
								ScChangeAction( ScChangeActionType,
												const ScBigRange&,
												const ULONG nAction,
												const ULONG nRejectAction,
												const ScChangeActionState eState,
												const DateTime& aDateTime,
												const String& aUser,
												const String& aComment );
								// only to be used in the XML import
								ScChangeAction( ScChangeActionType,
												const ScBigRange&,
												const ULONG nAction);

	virtual						~ScChangeAction();

			String				GetRefString( const ScBigRange&,
									ScDocument*, BOOL bFlag3D = FALSE ) const;

			void				SetActionNumber( ULONG n ) { nAction = n; }
			void				SetRejectAction( ULONG n ) { nRejectAction = n; }
			void				SetUser( const String& r ) { aUser = r; }
			void				SetType( ScChangeActionType e ) { eType = e; }
			void				SetState( ScChangeActionState e ) { eState = e; }
			void				SetRejected();

			ScBigRange& 		GetBigRange() { return aBigRange; }

			ScChangeActionLinkEntry*	AddLink( ScChangeAction* p,
											ScChangeActionLinkEntry* pL )
									{
										ScChangeActionLinkEntry* pLnk =
											new ScChangeActionLinkEntry(
											&pLinkAny, p );
										pLnk->SetLink( pL );
										return pLnk;
									}
			void				RemoveAllAnyLinks();

	virtual	ScChangeActionLinkEntry*	GetDeletedIn() const
											{ return pLinkDeletedIn; }
	virtual	ScChangeActionLinkEntry**	GetDeletedInAddress()
											{ return &pLinkDeletedIn; }
			ScChangeActionLinkEntry*	AddDeletedIn( ScChangeAction* p )
									{
										return new ScChangeActionLinkEntry(
											GetDeletedInAddress(), p );
									}
			BOOL				RemoveDeletedIn( const ScChangeAction* );
			void				RemoveAllDeletedIn();
			void				SetDeletedIn( ScChangeAction* );

			ScChangeActionLinkEntry*	AddDeleted( ScChangeAction* p )
									{
										return new ScChangeActionLinkEntry(
											&pLinkDeleted, p );
									}
			void				RemoveAllDeleted();

			ScChangeActionLinkEntry*	AddDependent( ScChangeAction* p )
									{
										return new ScChangeActionLinkEntry(
											&pLinkDependent, p );
									}
			void				RemoveAllDependent();

			void				RemoveAllLinks();

	virtual	void				AddContent( ScChangeActionContent* ) = 0;
	virtual	void				DeleteCellEntries() = 0;

	virtual	void 				UpdateReference( const ScChangeTrack*,
									UpdateRefMode, const ScBigRange&,
									INT32 nDx, INT32 nDy, INT32 nDz );

			void				Accept();
	virtual	BOOL				Reject( ScDocument* ) = 0;
			void				RejectRestoreContents( ScChangeTrack*,
									SCsCOL nDx, SCsROW nDy );

								// used in Reject() instead of IsRejectable()
			BOOL				IsInternalRejectable() const;

                                // Derived classes that hold a pointer to the
                                // ChangeTrack must return that. Otherwise NULL.
    virtual const ScChangeTrack*    GetChangeTrack() const = 0;

public:

			BOOL				IsInsertType() const
									{
										return eType == SC_CAT_INSERT_COLS ||
											eType == SC_CAT_INSERT_ROWS ||
											eType == SC_CAT_INSERT_TABS;
									}
			BOOL				IsDeleteType() const
									{
										return eType == SC_CAT_DELETE_COLS ||
											eType == SC_CAT_DELETE_ROWS ||
											eType == SC_CAT_DELETE_TABS;
									}
			BOOL				IsVirgin() const
									{ return eState == SC_CAS_VIRGIN; }
			BOOL				IsAccepted() const
									{ return eState == SC_CAS_ACCEPTED; }
			BOOL				IsRejected() const
									{ return eState == SC_CAS_REJECTED; }

								// Action rejects another Action
			BOOL				IsRejecting() const
									{ return nRejectAction != 0; }

								// ob Action im Dokument sichtbar ist
			BOOL				IsVisible() const;

								// ob Action anfassbar ist
			BOOL				IsTouchable() const;

								// ob Action ein Eintrag in Dialog-Root ist
			BOOL				IsDialogRoot() const;

								// ob ein Eintrag im Dialog aufklappbar sein soll
			BOOL				IsDialogParent() const;

								// ob Action ein Delete ist, unter dem
								// aufgeklappt mehrere einzelne Deletes sind
			BOOL				IsMasterDelete() const;

								// ob Action akzeptiert/selektiert/abgelehnt
								// werden kann
			BOOL				IsClickable() const;

								// ob Action abgelehnt werden kann
			BOOL				IsRejectable() const;

			const ScBigRange& 	GetBigRange() const { return aBigRange; }
			SC_DLLPUBLIC DateTime			GetDateTime() const;		// local time
			const DateTime&		GetDateTimeUTC() const		// UTC time
									{ return aDateTime; }
			const String&		GetUser() const { return aUser; }
			const String&		GetComment() const { return aComment; }
			ScChangeActionType	GetType() const { return eType; }
			ScChangeActionState	GetState() const { return eState; }
			ULONG				GetActionNumber() const { return nAction; }
			ULONG				GetRejectAction() const { return nRejectAction; }

			ScChangeAction*		GetNext() const { return pNext; }
			ScChangeAction*		GetPrev() const { return pPrev; }

			BOOL				IsDeletedIn() const
									{ return GetDeletedIn() != NULL; }
			BOOL				IsDeleted() const
									{ return IsDeleteType() || IsDeletedIn(); }
			BOOL				IsDeletedIn( const ScChangeAction* ) const;
			BOOL				IsDeletedInDelType( ScChangeActionType ) const;

			const ScChangeActionLinkEntry* GetFirstDeletedEntry() const
									{ return pLinkDeleted; }
			const ScChangeActionLinkEntry* GetFirstDependentEntry() const
									{ return pLinkDependent; }
			BOOL				HasDependent() const
									{ return pLinkDependent != NULL; }
			BOOL				HasDeleted() const
									{ return pLinkDeleted != NULL; }

								// Description wird an String angehaengt.
								// Mit bSplitRange wird bei Delete nur
								// eine Spalte/Zeile beruecksichtigt (fuer
								// Auflistung der einzelnen Eintraege).
	virtual	void				GetDescription( String&, ScDocument*,
									BOOL bSplitRange = FALSE, bool bWarning = true ) const;

	virtual void				GetRefString( String&, ScDocument*,
									BOOL bFlag3D = FALSE ) const;

								// fuer DocumentMerge altes Datum einer anderen
								// Action setzen, mit GetDateTimeUTC geholt
			void				SetDateTimeUTC( const DateTime& rDT )
									{ aDateTime = rDT; }

								// Benutzerkommentar setzen
			void				SetComment( const String& rStr )
									{ aComment = rStr; }

								// only to be used in the XML import
			void				SetDeletedInThis( ULONG nActionNumber,
										const ScChangeTrack* pTrack );
								// only to be used in the XML import
			void				AddDependent( ULONG nActionNumber,
										const ScChangeTrack* pTrack );
#if DEBUG_CHANGETRACK
            String              ToString( ScDocument* pDoc ) const;
#endif // DEBUG_CHANGETRACK
};


// --- ScChangeActionIns ----------------------------------------------------

class ScChangeActionIns : public ScChangeAction
{
	friend class ScChangeTrack;

								ScChangeActionIns( const ScRange& rRange );
	virtual						~ScChangeActionIns();

	virtual	void				AddContent( ScChangeActionContent* ) {}
	virtual	void				DeleteCellEntries() {}

	virtual	BOOL				Reject( ScDocument* );

    virtual const ScChangeTrack*    GetChangeTrack() const { return 0; }

public:
								ScChangeActionIns(const ULONG nActionNumber,
										const ScChangeActionState eState,
										const ULONG nRejectingNumber,
										const ScBigRange& aBigRange,
										const String& aUser,
										const DateTime& aDateTime,
										const String &sComment,
										const ScChangeActionType eType); // only to use in the XML import

	virtual	void				GetDescription( String&, ScDocument*,
									BOOL bSplitRange = FALSE, bool bWarning = true ) const;
};


// --- ScChangeActionDel ----------------------------------------------------

class ScChangeActionMove;

class ScChangeActionDelMoveEntry : public ScChangeActionLinkEntry
{
	friend class ScChangeActionDel;
	friend class ScChangeTrack;

			short		   		nCutOffFrom;
			short		   		nCutOffTo;


								ScChangeActionDelMoveEntry(
                                    ScChangeActionDelMoveEntry** ppPrevP,
									ScChangeActionMove* pMove,
									short nFrom, short nTo )
									:	ScChangeActionLinkEntry(
											(ScChangeActionLinkEntry**)
                                                ppPrevP,
											(ScChangeAction*) pMove ),
										nCutOffFrom( nFrom ),
										nCutOffTo( nTo )
									{}

			ScChangeActionDelMoveEntry*	GetNext()
									{
										return (ScChangeActionDelMoveEntry*)
										ScChangeActionLinkEntry::GetNext();
									}
			ScChangeActionMove*	GetMove()
									{
										return (ScChangeActionMove*)
										ScChangeActionLinkEntry::GetAction();
									}

public:
			const ScChangeActionDelMoveEntry*	GetNext() const
									{
										return (const ScChangeActionDelMoveEntry*)
										ScChangeActionLinkEntry::GetNext();
									}
			const ScChangeActionMove*	GetMove() const
									{
										return (const ScChangeActionMove*)
										ScChangeActionLinkEntry::GetAction();
									}
			short				GetCutOffFrom() const { return nCutOffFrom; }
			short				GetCutOffTo() const { return nCutOffTo; }
};


class ScChangeActionDel : public ScChangeAction
{
	friend class ScChangeTrack;
	friend void ScChangeAction::Accept();

			ScChangeTrack*		pTrack;
			ScChangeActionCellListEntry* pFirstCell;
			ScChangeActionIns*	pCutOff;		// abgeschnittener Insert
			short				nCutOff;		// +: Start  -: End
			ScChangeActionDelMoveEntry* pLinkMove;
			SCsCOL				nDx;
			SCsROW				nDy;

								ScChangeActionDel( const ScRange& rRange,
									SCsCOL nDx, SCsROW nDy, ScChangeTrack* );
	virtual						~ScChangeActionDel();

			ScChangeActionIns*	GetCutOffInsert() { return pCutOff; }

	virtual	void				AddContent( ScChangeActionContent* );
	virtual	void				DeleteCellEntries();

			void				UndoCutOffMoves();
			void				UndoCutOffInsert();

	virtual	void 				UpdateReference( const ScChangeTrack*,
									UpdateRefMode, const ScBigRange&,
									INT32 nDx, INT32 nDy, INT32 nDz );

	virtual	BOOL				Reject( ScDocument* );

    virtual const ScChangeTrack*    GetChangeTrack() const { return pTrack; }

public:
								ScChangeActionDel(const ULONG nActionNumber,
												const ScChangeActionState eState,
												const ULONG nRejectingNumber,
												const ScBigRange& aBigRange,
												const String& aUser,
												const DateTime& aDateTime,
												const String &sComment,
												const ScChangeActionType eType,
												const SCsCOLROW nD,
												ScChangeTrack* pTrack); // only to use in the XML import
																		// wich of nDx and nDy is set is depend on the type

								// ob dieses das unterste einer Reihe (oder
								// auch einzeln) ist
			BOOL				IsBaseDelete() const;

								// ob dieses das oberste einer Reihe (oder
								// auch einzeln) ist
			BOOL				IsTopDelete() const;

								// ob dieses ein Teil einer Reihe ist
			BOOL				IsMultiDelete() const;

								// ob es eine Col ist, die zu einem TabDelete gehoert
			BOOL				IsTabDeleteCol() const;

			SCsCOL				GetDx() const { return nDx; }
			SCsROW				GetDy() const { return nDy; }
			ScBigRange			GetOverAllRange() const;	// BigRange + (nDx, nDy)

			const ScChangeActionCellListEntry* GetFirstCellEntry() const
									{ return pFirstCell; }
			const ScChangeActionDelMoveEntry* GetFirstMoveEntry() const
									{ return pLinkMove; }
			const ScChangeActionIns*	GetCutOffInsert() const { return pCutOff; }
			short				GetCutOffCount() const { return nCutOff; }

	virtual	void				GetDescription( String&, ScDocument*,
									BOOL bSplitRange = FALSE, bool bWarning = true ) const;
			void				SetCutOffInsert( ScChangeActionIns* p, short n )
									{ pCutOff = p; nCutOff = n; }	// only to use in the XML import
																	// this should be protected, but for the XML import it is public
			// only to use in the XML import
			// this should be protected, but for the XML import it is public
			ScChangeActionDelMoveEntry*	AddCutOffMove( ScChangeActionMove* pMove,
										short nFrom, short nTo )
									{
										return new ScChangeActionDelMoveEntry(
										&pLinkMove, pMove, nFrom, nTo );
									}
};


// --- ScChangeActionMove ---------------------------------------------------

class ScChangeActionMove : public ScChangeAction
{
	friend class ScChangeTrack;
	friend class ScChangeActionDel;

			ScBigRange			aFromRange;
			ScChangeTrack*		pTrack;
			ScChangeActionCellListEntry* pFirstCell;
			ULONG				nStartLastCut;	// fuer PasteCut Undo
			ULONG				nEndLastCut;

								ScChangeActionMove( const ScRange& rFromRange,
									const ScRange& rToRange,
									ScChangeTrack* pTrackP )
									: ScChangeAction( SC_CAT_MOVE, rToRange ),
										aFromRange( rFromRange ),
										pTrack( pTrackP ),
										pFirstCell( NULL ),
										nStartLastCut(0),
										nEndLastCut(0)
									{}
	virtual						~ScChangeActionMove();

	virtual	void				AddContent( ScChangeActionContent* );
	virtual	void				DeleteCellEntries();

			ScBigRange&			GetFromRange() { return aFromRange; }

			void				SetStartLastCut( ULONG nVal ) { nStartLastCut = nVal; }
			ULONG				GetStartLastCut() const { return nStartLastCut; }
			void				SetEndLastCut( ULONG nVal )	{ nEndLastCut = nVal; }
			ULONG				GetEndLastCut() const { return nEndLastCut; }

	virtual	void 				UpdateReference( const ScChangeTrack*,
									UpdateRefMode, const ScBigRange&,
									INT32 nDx, INT32 nDy, INT32 nDz );

	virtual	BOOL				Reject( ScDocument* );

    virtual const ScChangeTrack*    GetChangeTrack() const { return pTrack; }

protected:
    using ScChangeAction::GetRefString;

public:
								ScChangeActionMove(const ULONG nActionNumber,
												const ScChangeActionState eState,
												const ULONG nRejectingNumber,
												const ScBigRange& aToBigRange,
												const String& aUser,
												const DateTime& aDateTime,
												const String &sComment,
												const ScBigRange& aFromBigRange,
												ScChangeTrack* pTrack); // only to use in the XML import
			const ScChangeActionCellListEntry* GetFirstCellEntry() const
									{ return pFirstCell; } // only to use in the XML export

			const ScBigRange&	GetFromRange() const { return aFromRange; }
	SC_DLLPUBLIC		void				GetDelta( INT32& nDx, INT32& nDy, INT32& nDz ) const;

	virtual	void				GetDescription( String&, ScDocument*,
									BOOL bSplitRange = FALSE, bool bWarning = true ) const;

	virtual void				GetRefString( String&, ScDocument*,
									BOOL bFlag3D = FALSE ) const;
};


// --- ScChangeActionContent ------------------------------------------------

enum ScChangeActionContentCellType
{
	SC_CACCT_NONE = 0,
	SC_CACCT_NORMAL,
	SC_CACCT_MATORG,
	SC_CACCT_MATREF
};

class Stack;

class ScChangeActionContent : public ScChangeAction
{
	friend class ScChangeTrack;

			String				aOldValue;
			String				aNewValue;
			ScBaseCell*			pOldCell;
			ScBaseCell*			pNewCell;
		ScChangeActionContent*	pNextContent;	// an gleicher Position
		ScChangeActionContent*	pPrevContent;
		ScChangeActionContent*	pNextInSlot;	// in gleichem Slot
		ScChangeActionContent**	ppPrevInSlot;

			void				InsertInSlot( ScChangeActionContent** pp )
									{
										if ( !ppPrevInSlot )
										{
											ppPrevInSlot = pp;
                                            if ( ( pNextInSlot = *pp ) != NULL )
												pNextInSlot->ppPrevInSlot = &pNextInSlot;
											*pp = this;
										}
									}
			void				RemoveFromSlot()
									{
										if ( ppPrevInSlot )
										{
                                            if ( ( *ppPrevInSlot = pNextInSlot ) != NULL )
												pNextInSlot->ppPrevInSlot = ppPrevInSlot;
											ppPrevInSlot = NULL;	// not inserted
										}
									}
		ScChangeActionContent*	GetNextInSlot() { return pNextInSlot; }

			void				ClearTrack();

	static	void				GetStringOfCell( String& rStr,
									const ScBaseCell* pCell,
									const ScDocument* pDoc,
									const ScAddress& rPos );

	static	void				GetStringOfCell( String& rStr,
									const ScBaseCell* pCell,
									const ScDocument* pDoc,
									ULONG nFormat );

	static	void				SetValue( String& rStr, ScBaseCell*& pCell,
									const ScAddress& rPos,
									const ScBaseCell* pOrgCell,
									const ScDocument* pFromDoc,
									ScDocument* pToDoc );

	static	void				SetValue( String& rStr, ScBaseCell*& pCell,
									ULONG nFormat,
									const ScBaseCell* pOrgCell,
									const ScDocument* pFromDoc,
									ScDocument* pToDoc );

	static	void				SetCell( String& rStr, ScBaseCell* pCell,
									ULONG nFormat, const ScDocument* pDoc );

	static	BOOL				NeedsNumberFormat( const ScBaseCell* );

			void				SetValueString( String& rValue,
									ScBaseCell*& pCell,	const String& rStr,
									ScDocument* pDoc );

			void				GetValueString( String& rStr,
									const String& rValue,
									const ScBaseCell* pCell ) const;

			void				GetFormulaString( String& rStr,
									const ScFormulaCell* pCell ) const;

	virtual	void				AddContent( ScChangeActionContent* ) {}
	virtual	void				DeleteCellEntries() {}

	virtual	void 				UpdateReference( const ScChangeTrack*,
									UpdateRefMode, const ScBigRange&,
									INT32 nDx, INT32 nDy, INT32 nDz );

	virtual	BOOL				Reject( ScDocument* );

    virtual const ScChangeTrack*    GetChangeTrack() const { return 0; }

								// pRejectActions!=NULL: reject actions get
								// stacked, no SetNewValue, no Append
			BOOL				Select( ScDocument*, ScChangeTrack*,
									BOOL bOldest, Stack* pRejectActions );

			void				PutValueToDoc( ScBaseCell*, const String&,
									ScDocument*, SCsCOL nDx, SCsROW nDy ) const;

protected:
    using ScChangeAction::GetRefString;

public:

	DECL_FIXEDMEMPOOL_NEWDEL( ScChangeActionContent )

								ScChangeActionContent( const ScRange& rRange )
									: ScChangeAction( SC_CAT_CONTENT, rRange ),
										pOldCell( NULL ),
										pNewCell( NULL ),
										pNextContent( NULL ),
										pPrevContent( NULL ),
										pNextInSlot( NULL ),
										ppPrevInSlot( NULL )
									{}
								ScChangeActionContent(const ULONG nActionNumber,
												const ScChangeActionState eState,
												const ULONG nRejectingNumber,
												const ScBigRange& aBigRange,
												const String& aUser,
												const DateTime& aDateTime,
												const String &sComment,
												ScBaseCell* pOldCell,
												ScDocument* pDoc,
												const String& sOldValue); // to use for XML Import
								ScChangeActionContent(const ULONG nActionNumber,
												ScBaseCell* pNewCell,
												const ScBigRange& aBigRange,
												ScDocument* pDoc,
                                                const String& sNewValue); // to use for XML Import of Generated Actions
	virtual						~ScChangeActionContent();

		ScChangeActionContent*	GetNextContent() const { return pNextContent; }
		ScChangeActionContent*	GetPrevContent() const { return pPrevContent; }
		ScChangeActionContent*	GetTopContent() const;
			BOOL				IsTopContent() const
									{ return pNextContent == NULL; }

	virtual	ScChangeActionLinkEntry*  	GetDeletedIn() const;
	virtual	ScChangeActionLinkEntry**	GetDeletedInAddress();

			void				PutOldValueToDoc( ScDocument*,
									SCsCOL nDx, SCsROW nDy ) const;
			void				PutNewValueToDoc( ScDocument*,
									SCsCOL nDx, SCsROW nDy ) const;

			void				SetOldValue( const ScBaseCell*,
									const ScDocument* pFromDoc,
									ScDocument* pToDoc,
									ULONG nFormat );
			void				SetOldValue( const ScBaseCell*,
									const ScDocument* pFromDoc,
									ScDocument* pToDoc );
			void				SetNewValue( const ScBaseCell*,	ScDocument* );

								// Used in import filter AppendContentOnTheFly,
								// takes ownership of cells.
			void				SetOldNewCells( ScBaseCell* pOldCell,
									ULONG nOldFormat, ScBaseCell* pNewCell,
									ULONG nNewFormat, ScDocument* pDoc );

								// Use this only in the XML import,
								// takes ownership of cell.
			void				SetNewCell( ScBaseCell* pCell, ScDocument* pDoc, const String& rFormatted );

								// These functions should be protected but for
								// the XML import they are public.
			void				SetNextContent( ScChangeActionContent* p )
									{ pNextContent = p; }
			void				SetPrevContent( ScChangeActionContent* p )
									{ pPrevContent = p; }

								// moeglichst nicht verwenden,
								// setzt nur String bzw. generiert Formelzelle
			void				SetOldValue( const String& rOld, ScDocument* );
			void				SetNewValue( const String& rNew, ScDocument* );

			void				GetOldString( String& ) const;
			void				GetNewString( String& ) const;
			const ScBaseCell*	GetOldCell() const { return pOldCell; }
			const ScBaseCell*	GetNewCell() const { return pNewCell; }
	virtual	void				GetDescription( String&, ScDocument*,
									BOOL bSplitRange = FALSE, bool bWarning = true ) const;
	virtual void				GetRefString( String&, ScDocument*,
									BOOL bFlag3D = FALSE ) const;

	static	ScChangeActionContentCellType	GetContentCellType( const ScBaseCell* );

								// NewCell
			BOOL				IsMatrixOrigin() const
									{
										return GetContentCellType( GetNewCell() )
											== SC_CACCT_MATORG;
									}
			BOOL				IsMatrixReference() const
									{
										return GetContentCellType( GetNewCell() )
											== SC_CACCT_MATREF;
									}
								// OldCell
			BOOL				IsOldMatrixOrigin() const
									{
										return GetContentCellType( GetOldCell() )
											== SC_CACCT_MATORG;
									}
			BOOL				IsOldMatrixReference() const
									{
										return GetContentCellType( GetOldCell() )
											== SC_CACCT_MATREF;
									}

};


// --- ScChangeActionReject -------------------------------------------------

class Stack;

class ScChangeActionReject : public ScChangeAction
{
	friend class ScChangeTrack;
	friend class ScChangeActionContent;

								ScChangeActionReject( ULONG nReject )
									: ScChangeAction( SC_CAT_REJECT, ScRange() )
									{
										SetRejectAction( nReject );
										SetState( SC_CAS_ACCEPTED );
									}

	virtual	void				AddContent( ScChangeActionContent* ) {}
	virtual	void				DeleteCellEntries() {}

	virtual	BOOL				Reject( ScDocument* ) { return FALSE; }

    virtual const ScChangeTrack*    GetChangeTrack() const { return 0; }

public:
								ScChangeActionReject(const ULONG nActionNumber,
												const ScChangeActionState eState,
												const ULONG nRejectingNumber,
												const ScBigRange& aBigRange,
												const String& aUser,
												const DateTime& aDateTime,
												const String &sComment); // only to use in the XML import
};


// --- ScChangeTrack --------------------------------------------------------

enum ScChangeTrackMsgType
{
	SC_CTM_NONE,
	SC_CTM_APPEND,		// Actions angehaengt
	SC_CTM_REMOVE,		// Actions weggenommen
	SC_CTM_CHANGE,		// Actions geaendert
	SC_CTM_PARENT		// war kein Parent und ist jetzt einer
};

struct ScChangeTrackMsgInfo
{
	DECL_FIXEDMEMPOOL_NEWDEL( ScChangeTrackMsgInfo )

	ScChangeTrackMsgType	eMsgType;
	ULONG					nStartAction;
	ULONG					nEndAction;
};

// MsgQueue fuer Benachrichtigung via ModifiedLink
DECLARE_QUEUE( ScChangeTrackMsgQueue, ScChangeTrackMsgInfo* )
DECLARE_STACK( ScChangeTrackMsgStack, ScChangeTrackMsgInfo* )

enum ScChangeTrackMergeState
{
	SC_CTMS_NONE,
	SC_CTMS_PREPARE,
	SC_CTMS_OWN,
    SC_CTMS_UNDO,
	SC_CTMS_OTHER
};

// zusaetzlich zu pFirst/pNext/pLast/pPrev eine Table, um schnell sowohl
// per ActionNumber als auch ueber Liste zugreifen zu koennen
DECLARE_TABLE( ScChangeActionTable, ScChangeAction* )

// Intern generierte Actions beginnen bei diesem Wert (fast alle Bits gesetzt)
// und werden runtergezaehlt, um sich in einer Table wertemaessig nicht mit den
// "normalen" Actions in die Quere zu kommen.
#define SC_CHGTRACK_GENERATED_START	((UINT32) 0xfffffff0)

// SfxListener an der Applikation, um Aenderungen des Usernamens mitzubekommen

class ScChangeTrack : public SfxListener
{
	friend void ScChangeAction::RejectRestoreContents( ScChangeTrack*, SCsCOL, SCsROW );
	friend BOOL ScChangeActionDel::Reject( ScDocument* pDoc );
	friend void ScChangeActionDel::DeleteCellEntries();
	friend void ScChangeActionMove::DeleteCellEntries();
	friend BOOL ScChangeActionMove::Reject( ScDocument* pDoc );

    static	const SCROW         nContentRowsPerSlot;
    static	const SCSIZE        nContentSlots;

	com::sun::star::uno::Sequence< sal_Int8 >	aProtectPass;
			ScChangeActionTable	aTable;
			ScChangeActionTable	aGeneratedTable;
			ScChangeActionTable	aPasteCutTable;
		ScChangeTrackMsgQueue	aMsgQueue;
		ScChangeTrackMsgStack	aMsgStackTmp;
		ScChangeTrackMsgStack	aMsgStackFinal;
			ScStrCollection		aUserCollection;
			String				aUser;
			Link				aModifiedLink;
			ScRange				aInDeleteRange;
			DateTime			aFixDateTime;
			ScChangeAction*		pFirst;
			ScChangeAction*		pLast;
		ScChangeActionContent*	pFirstGeneratedDelContent;
		ScChangeActionContent**	ppContentSlots;
		ScChangeActionMove*		pLastCutMove;
	ScChangeActionLinkEntry*	pLinkInsertCol;
	ScChangeActionLinkEntry*	pLinkInsertRow;
	ScChangeActionLinkEntry*	pLinkInsertTab;
	ScChangeActionLinkEntry*	pLinkMove;
		ScChangeTrackMsgInfo*	pBlockModifyMsg;
			ScDocument*			pDoc;
			ULONG				nActionMax;
			ULONG				nGeneratedMin;
			ULONG				nMarkLastSaved;
			ULONG				nStartLastCut;
			ULONG				nEndLastCut;
			ULONG				nLastMerge;
		ScChangeTrackMergeState	eMergeState;
			USHORT				nLoadedFileFormatVersion;
			BOOL				bLoadSave;
			BOOL				bInDelete;
			BOOL				bInDeleteUndo;
			BOOL				bInDeleteTop;
			BOOL				bInPasteCut;
			BOOL				bUseFixDateTime;
            BOOL                bTime100thSeconds;

								// not implemented, prevent usage
								ScChangeTrack( const ScChangeTrack& );
			ScChangeTrack&		operator=( const ScChangeTrack& );

#ifdef SC_CHGTRACK_CXX
	static	SCROW				InitContentRowsPerSlot();

								// TRUE if one is MM_FORMULA and the other is
								// not, or if both are and range differs
	static	BOOL				IsMatrixFormulaRangeDifferent(
									const ScBaseCell* pOldCell,
									const ScBaseCell* pNewCell );

	virtual	void				Notify( SfxBroadcaster&, const SfxHint& );
			void				Init();
			void				DtorClear();
			void				SetLoadSave( BOOL bVal ) { bLoadSave = bVal; }
			void				SetInDeleteRange( const ScRange& rRange )
									{ aInDeleteRange = rRange; }
			void				SetInDelete( BOOL bVal )
									{ bInDelete = bVal; }
			void				SetInDeleteTop( BOOL bVal )
									{ bInDeleteTop = bVal; }
			void				SetInDeleteUndo( BOOL bVal )
									{ bInDeleteUndo = bVal; }
			void				SetInPasteCut( BOOL bVal )
									{ bInPasteCut = bVal; }
			void				SetMergeState( ScChangeTrackMergeState eState )
									{ eMergeState = eState; }
		ScChangeTrackMergeState	GetMergeState() const { return eMergeState; }
			void				SetLastMerge( ULONG nVal ) { nLastMerge = nVal; }
			ULONG				GetLastMerge() const { return nLastMerge; }

			void				SetLastCutMoveRange( const ScRange&, ScDocument* );

								// ModifyMsg blockweise und nicht einzeln erzeugen
			void				StartBlockModify( ScChangeTrackMsgType,
									ULONG nStartAction );
			void				EndBlockModify( ULONG nEndAction );

			void				AddDependentWithNotify( ScChangeAction* pParent,
									ScChangeAction* pDependent );

			void				Dependencies( ScChangeAction* );
			void				UpdateReference( ScChangeAction*, BOOL bUndo );
			void				UpdateReference( ScChangeAction** ppFirstAction,
									ScChangeAction* pAct, BOOL bUndo );
			void				Append( ScChangeAction* pAppend, ULONG nAction );
	SC_DLLPUBLIC		void				AppendDeleteRange( const ScRange&,
									ScDocument* pRefDoc, SCsTAB nDz,
									ULONG nRejectingInsert );
			void				AppendOneDeleteRange( const ScRange& rOrgRange,
									ScDocument* pRefDoc,
									SCsCOL nDx, SCsROW nDy, SCsTAB nDz,
									ULONG nRejectingInsert );
			void				LookUpContents( const ScRange& rOrgRange,
									ScDocument* pRefDoc,
									SCsCOL nDx, SCsROW nDy, SCsTAB nDz );
			void				Remove( ScChangeAction* );
			void				MasterLinks( ScChangeAction* );

								// Content on top an Position
		ScChangeActionContent*	SearchContentAt( const ScBigAddress&,
									ScChangeAction* pButNotThis ) const;
			void				DeleteGeneratedDelContent(
									ScChangeActionContent* );
		ScChangeActionContent*	GenerateDelContent( const ScAddress&,
									const ScBaseCell*,
									const ScDocument* pFromDoc );
			void				DeleteCellEntries(
									ScChangeActionCellListEntry*&,
									ScChangeAction* pDeletor );

								// Action und alle abhaengigen rejecten,
								// Table stammt aus vorherigem GetDependents,
								// ist nur bei Insert und Move (MasterType)
								// noetig, kann ansonsten NULL sein.
								// bRecursion == Aufruf aus Reject mit Table
			BOOL				Reject( ScChangeAction*,
									ScChangeActionTable*, BOOL bRecursion );

#endif	// SC_CHGTRACK_CXX

			void				ClearMsgQueue();

public:

	static	SCSIZE				ComputeContentSlot( INT32 nRow )
									{
										if ( nRow < 0 || nRow > MAXROW )
											return nContentSlots - 1;
                                        return static_cast< SCSIZE >( nRow / nContentRowsPerSlot );
									}

            SC_DLLPUBLIC        ScChangeTrack( ScDocument* );
								ScChangeTrack( ScDocument*,
											const ScStrCollection& ); // only to use in the XML import
            SC_DLLPUBLIC virtual ~ScChangeTrack();
			void				Clear();

			ScChangeActionContent*	GetFirstGenerated() const { return pFirstGeneratedDelContent; }
			ScChangeAction*		GetFirst() const { return pFirst; }
			ScChangeAction*		GetLast() const	{ return pLast; }
			ULONG				GetActionMax() const { return nActionMax; }
			BOOL				IsGenerated( ULONG nAction ) const
									{ return nAction >= nGeneratedMin; }
			ScChangeAction*		GetAction( ULONG nAction ) const
									{ return aTable.Get( nAction ); }
			ScChangeAction*		GetGenerated( ULONG nGenerated ) const
									{ return aGeneratedTable.Get( nGenerated ); }
			ScChangeAction*		GetActionOrGenerated( ULONG nAction ) const
									{
										return IsGenerated( nAction ) ?
											GetGenerated( nAction ) :
											GetAction( nAction );
									}
			ULONG				GetLastSavedActionNumber() const
									{ return nMarkLastSaved; }
            void                SetLastSavedActionNumber(ULONG nNew)
                                    { nMarkLastSaved = nNew; }
			ScChangeAction*		GetLastSaved() const
									{ return aTable.Get( nMarkLastSaved ); }
		ScChangeActionContent**	GetContentSlots() const { return ppContentSlots; }

			BOOL				IsLoadSave() const { return bLoadSave; }
			const ScRange&		GetInDeleteRange() const
									{ return aInDeleteRange; }
			BOOL				IsInDelete() const { return bInDelete; }
			BOOL				IsInDeleteTop() const { return bInDeleteTop; }
			BOOL				IsInDeleteUndo() const { return bInDeleteUndo; }
			BOOL				IsInPasteCut() const { return bInPasteCut; }
	SC_DLLPUBLIC		void				SetUser( const String& );
			const String&		GetUser() const { return aUser; }
			const ScStrCollection&	GetUserCollection() const
									{ return aUserCollection; }
			ScDocument*			GetDocument() const { return pDoc; }
								// for import filter
			const DateTime&		GetFixDateTime() const { return aFixDateTime; }

								// set this if the date/time set with
								// SetFixDateTime...() shall be applied to
								// appended actions
			void				SetUseFixDateTime( BOOL bVal )
									{ bUseFixDateTime = bVal; }
								// for MergeDocument, apply original date/time as UTC
			void				SetFixDateTimeUTC( const DateTime& rDT )
									{ aFixDateTime = rDT; }
								// for import filter, apply original date/time as local time
			void				SetFixDateTimeLocal( const DateTime& rDT )
									{ aFixDateTime = rDT; aFixDateTime.ConvertToUTC(); }

			void				Append( ScChangeAction* );

								// pRefDoc may be NULL => no lookup of contents
								// => no generation of deleted contents
	SC_DLLPUBLIC		void				AppendDeleteRange( const ScRange&,
									ScDocument* pRefDoc,
									ULONG& nStartAction, ULONG& nEndAction,
									SCsTAB nDz = 0 );
									// nDz: Multi-TabDel, LookUpContent ist
									// um -nDz verschoben zu suchen

								// nachdem neuer Wert im Dokument gesetzt wurde,
								// alter Wert aus RefDoc/UndoDoc
			void				AppendContent( const ScAddress& rPos,
									ScDocument* pRefDoc );
								// nachdem neue Werte im Dokument gesetzt wurden,
								// alte Werte aus RefDoc/UndoDoc
			void				AppendContentRange( const ScRange& rRange,
									ScDocument* pRefDoc,
									ULONG& nStartAction, ULONG& nEndAction,
									ScChangeActionClipMode eMode = SC_CACM_NONE );
								// nachdem neuer Wert im Dokument gesetzt wurde,
								// alter Wert aus pOldCell, nOldFormat,
								// RefDoc==NULL => Doc
			void				AppendContent( const ScAddress& rPos,
									const ScBaseCell* pOldCell,
									ULONG nOldFormat, ScDocument* pRefDoc = NULL );
								// nachdem neuer Wert im Dokument gesetzt wurde,
								// alter Wert aus pOldCell, Format aus Doc
			void				AppendContent( const ScAddress& rPos,
									const ScBaseCell* pOldCell );
								// nachdem neue Werte im Dokument gesetzt wurden,
								// alte Werte aus RefDoc/UndoDoc.
								// Alle Contents, wo im RefDoc eine Zelle steht.
			void				AppendContentsIfInRefDoc( ScDocument* pRefDoc,
									ULONG& nStartAction, ULONG& nEndAction );

								// Meant for import filter, creates and inserts
								// an unconditional content action of the two
								// cells without querying the document, not
								// even for number formats (though the number
								// formatter of the document may be used).
								// The action is returned and may be used to
								// set user name, description, date/time et al.
								// Takes ownership of the cells!
	SC_DLLPUBLIC	ScChangeActionContent*	AppendContentOnTheFly( const ScAddress& rPos,
									ScBaseCell* pOldCell,
									ScBaseCell* pNewCell,
									ULONG nOldFormat = 0,
									ULONG nNewFormat = 0 );

								// die folgenden beiden nur benutzen wenn's
								// nicht anders geht (setzen nur String fuer
								// NewValue bzw. Formelerzeugung)

								// bevor neuer Wert im Dokument gesetzt wird
			void				AppendContent( const ScAddress& rPos,
									const String& rNewValue,
									ScBaseCell* pOldCell );

	SC_DLLPUBLIC		void				AppendInsert( const ScRange& );

								// pRefDoc may be NULL => no lookup of contents
								// => no generation of deleted contents
	SC_DLLPUBLIC		void				AppendMove( const ScRange& rFromRange,
									const ScRange& rToRange,
									ScDocument* pRefDoc );

								// Cut to Clipboard
			void				ResetLastCut()
									{
										nStartLastCut = nEndLastCut = 0;
										if ( pLastCutMove )
										{
											delete pLastCutMove;
											pLastCutMove = NULL;
										}
									}
			BOOL				HasLastCut() const
									{
										return nEndLastCut > 0 &&
											nStartLastCut <= nEndLastCut &&
											pLastCutMove;
									}

	SC_DLLPUBLIC		void				Undo( ULONG nStartAction, ULONG nEndAction, bool bMerge = false );

								// fuer MergeDocument, Referenzen anpassen,
								//! darf nur in einem temporaer geoeffneten
								//! Dokument verwendet werden, der Track
								//! ist danach verhunzt
			void				MergePrepare( ScChangeAction* pFirstMerge, bool bShared = false );
			void				MergeOwn( ScChangeAction* pAct, ULONG nFirstMerge, bool bShared = false );
	static	BOOL				MergeIgnore( const ScChangeAction&, ULONG nFirstMerge );

								// Abhaengige in Table einfuegen.
								// Bei Insert sind es echte Abhaengige,
								// bei Move abhaengige Contents im FromRange
								// und geloeschte im ToRange bzw. Inserts in
								// FromRange oder ToRange,
								// bei Delete eine Liste der geloeschten,
								// bei Content andere Contents an gleicher
								// Position oder MatrixReferences zu MatrixOrigin.
								// Mit bListMasterDelete werden unter einem
								// MasterDelete alle zu diesem Delete gehoerenden
								// Deletes einer Reihe gelistet.
								// Mit bAllFlat werden auch alle Abhaengigen
								// der Abhaengigen flach eingefuegt.
	SC_DLLPUBLIC		void				GetDependents( ScChangeAction*,
									ScChangeActionTable&,
									BOOL bListMasterDelete = FALSE,
									BOOL bAllFlat = FALSE ) const;

								// Reject visible Action (und abhaengige)
            BOOL                Reject( ScChangeAction*, bool bShared = false );

								// Accept visible Action (und abhaengige)
	SC_DLLPUBLIC		BOOL				Accept( ScChangeAction* );

			void				AcceptAll();	// alle Virgins
			BOOL				RejectAll();	// alle Virgins

								// Selektiert einen Content von mehreren an
								// gleicher Position und akzeptiert diesen und
								// die aelteren, rejected die neueren.
								// Mit bOldest==TRUE wird der erste OldValue
								// einer Virgin-Content-Kette restauriert.
			BOOL				SelectContent( ScChangeAction*,
									BOOL bOldest = FALSE );

								// wenn ModifiedLink gesetzt, landen
								// Aenderungen in ScChangeTrackMsgQueue
			void				SetModifiedLink( const Link& r )
									{ aModifiedLink = r; ClearMsgQueue(); }
			const Link&			GetModifiedLink() const { return aModifiedLink; }
			ScChangeTrackMsgQueue& GetMsgQueue() { return aMsgQueue; }

			void				NotifyModified( ScChangeTrackMsgType eMsgType,
									ULONG nStartAction, ULONG nEndAction );

			USHORT				GetLoadedFileFormatVersion() const
									{ return nLoadedFileFormatVersion; }

			ULONG				AddLoadedGenerated(ScBaseCell* pOldCell,
												const ScBigRange& aBigRange, const String& sNewValue ); // only to use in the XML import
			void				AppendLoaded( ScChangeAction* pAppend ); // this is only for the XML import public, it should be protected
			void				SetActionMax(ULONG nTempActionMax)
									{ nActionMax = nTempActionMax; } // only to use in the XML import

            void                SetProtection( const com::sun::star::uno::Sequence< sal_Int8 >& rPass )
                                    { aProtectPass = rPass; }
    com::sun::star::uno::Sequence< sal_Int8 >   GetProtection() const
                                    { return aProtectPass; }
            BOOL                IsProtected() const
                                    { return aProtectPass.getLength() != 0; }

                                // If time stamps of actions of this
                                // ChangeTrack and a second one are to be
                                // compared including 100th seconds.
            void                SetTime100thSeconds( BOOL bVal )
                                    { bTime100thSeconds = bVal; }
            BOOL                IsTime100thSeconds() const
                                    { return bTime100thSeconds; }

            void                AppendCloned( ScChangeAction* pAppend );
    SC_DLLPUBLIC ScChangeTrack* Clone( ScDocument* pDocument ) const;
            void                MergeActionState( ScChangeAction* pAct, const ScChangeAction* pOtherAct );
#if DEBUG_CHANGETRACK
            String              ToString() const;
#endif // DEBUG_CHANGETRACK
};


#endif


