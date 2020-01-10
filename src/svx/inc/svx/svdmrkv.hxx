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

#ifndef _SVDMRKV_HXX
#define _SVDMRKV_HXX

#include <svx/svdmark.hxx>
#include <svx/svdhdl.hxx>
#include <svx/svdsnpv.hxx>
#include <svx/svdtypes.hxx>
#include "svx/svxdllapi.h"

//************************************************************
//   Defines 
//************************************************************

// folgendes ist noch nicht bzw. erst zum Teil implementiert:
#define SDRSEARCH_DEEP         0x0001 /* rekursiv in Gruppenobjekte hinein */
#define SDRSEARCH_ALSOONMASTER 0x0002 /* MasterPages werden auch durchsucht */
#define SDRSEARCH_WHOLEPAGE    0x0004 /* Nicht nur die ObjList der PageView */
#define SDRSEARCH_TESTMARKABLE 0x0008 /* nur markierbare Objekte/Punkte/Handles/... */
#define SDRSEARCH_TESTMACRO    0x0010 /* Nur Objekte mit Macro */
#define SDRSEARCH_TESTTEXTEDIT 0x0020 /* Nur TextEdit-faehige Objekte */
#define SDRSEARCH_WITHTEXT     0x0040 /* Nur Objekte mit Text */
#define SDRSEARCH_TESTTEXTAREA 0x0080 /* Den Textbereich von Objekten mit Text (TextEditHit) */
#define SDRSEARCH_BACKWARD     0x0100 /* Rueckwaertssuche */
#define SDRSEARCH_NEXT         0x0200 /* Suche startet hinter dem uebergebenen Objekt/Punkt/... */
#define SDRSEARCH_MARKED       0x0400 /* Nur markierte Objekte/Punkte/... */
#define SDRSEARCH_PASS2BOUND   0x0800 /* Wenn nix gefunden, dann neuer 2. Versuch mit BoundRectHit */
#define SDRSEARCH_PASS3NEAREST 0x1000 /* Wenn nix gefunden, dann neuer 3. Versuch mit NearestBoundRectHit */

#define SDRSEARCH_PICKMARKABLE (SDRSEARCH_TESTMARKABLE)
#define SDRSEARCH_PICKTEXTEDIT (SDRSEARCH_DEEP|SDRSEARCH_TESTMARKABLE|SDRSEARCH_TESTTEXTEDIT)
#define SDRSEARCH_PICKMACRO    (SDRSEARCH_DEEP|SDRSEARCH_ALSOONMASTER|SDRSEARCH_WHOLEPAGE|SDRSEARCH_TESTMACRO)

// SDRSEARCHPASS_... ist Rueckgabeparameterwert bei PickObj().
#define SDRSEARCHPASS_DIRECT       0x0000 /* Objekt durch direkten Hit getroffen     */
#define SDRSEARCHPASS_INACTIVELIST 0x0001 /* Obj auf der Page, nicht jedoch in der AktGroup (bei WHOLEPAGE) */
#define SDRSEARCHPASS_MASTERPAGE   0x0002 /* Objekt auf der MasterPage gefunden      */
#define SDRSEARCHPASS_BOUND        0x0010 /* Objekt erst beim Bound-Check gefunden   */
#define SDRSEARCHPASS_NEAREST      0x0020 /* Objekt erst beim Nearest-Check gefunden */

enum SdrHitKind {SDRHIT_NONE,      // Nicht getroffen, Schnaps gesoffen
                 SDRHIT_OBJECT,    // Treffer versenkt
                 SDRHIT_BOUNDRECT, // Treffer am BoundRect
                 SDRHIT_BOUNDTL,   // Treffer am BoundRect TopLeft
                 SDRHIT_BOUNDTC,   // Treffer am BoundRect TopCenter
                 SDRHIT_BOUNDTR,   // Treffer am BoundRect TopRight
                 SDRHIT_BOUNDCL,   // Treffer am BoundRect CenterLeft
                 SDRHIT_BOUNDCR,   // Treffer am BoundRect CenterRight
                 SDRHIT_BOUNDBL,   // Treffer am BoundRect BottomLeft
                 SDRHIT_BOUNDBC,   // Treffer am BoundRect BottomCenter
                 SDRHIT_BOUNDBR,/*,*/ // Treffer am BoundRect BottomRight
                 /*SDRHIT_REFPOINT*/ // Referenzpunkt (Rotationsachse,Spiegelachse) getroffen
                 // ab hier neu am 01-07-1996:
                 SDRHIT_HANDLE,          // Markierungshandle
                 SDRHIT_HELPLINE,        // Hilfslinie
                 SDRHIT_GLUEPOINT,       // Klebepunkt
                 SDRHIT_TEXTEDIT,        // Offene OutlinerView getroffen
                 SDRHIT_TEXTEDITOBJ,     // Objekt fuer SdrBeginTextEdit (Textbereich)
                 SDRHIT_URLFIELD,        // Feld im TextObj getroffen (wird gerade nicht editiert)
                 SDRHIT_MACRO,           // Objekt fuer BegMacroObj
                 SDRHIT_MARKEDOBJECT,    // Markiertes Objekt (z.B. zum Draggen)
                 SDRHIT_UNMARKEDOBJECT, // nichtmarkiertes Objekt (z.B. zum markieren)
				 SDRHIT_CELL};			// hit on a cell inside a table shape (outside of the cells text area)

enum SdrViewEditMode {SDREDITMODE_EDIT,           // Auch bekannt aus Pfeil- oder Zeigermodus
                      SDREDITMODE_CREATE,         // Objekterzeugungswerkzeug
                      SDREDITMODE_GLUEPOINTEDIT}; // Klebepunkteditiermodus

#define IMPSDR_MARKOBJDESCRIPTION    0x0000
#define IMPSDR_POINTSDESCRIPTION     0x0001
#define IMPSDR_GLUEPOINTSDESCRIPTION 0x0002

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@   @@  @@@@  @@@@@  @@  @@  @@ @@ @@ @@@@@ @@   @@
//  @@@ @@@ @@  @@ @@  @@ @@  @@  @@ @@ @@ @@    @@   @@
//  @@@@@@@ @@  @@ @@  @@ @@ @@   @@ @@ @@ @@    @@ @ @@
//  @@@@@@@ @@@@@@ @@@@@  @@@@    @@@@@ @@ @@@@  @@@@@@@
//  @@ @ @@ @@  @@ @@  @@ @@ @@    @@@  @@ @@    @@@@@@@
//  @@   @@ @@  @@ @@  @@ @@  @@   @@@  @@ @@    @@@ @@@
//  @@   @@ @@  @@ @@  @@ @@  @@    @   @@ @@@@@ @@   @@
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class ImplMarkingOverlay;

class SVX_DLLPUBLIC SdrMarkView : public SdrSnapView 
{
	friend class				SdrPageView;

	// #114409#-3 Migrate selections
	ImplMarkingOverlay*									mpMarkObjOverlay;
	ImplMarkingOverlay*									mpMarkPointsOverlay;
	ImplMarkingOverlay*									mpMarkGluePointsOverlay;

protected:
    SdrObject*					pMarkedObj;       // Wenn nur ein Objekt markiert ist.
    SdrPageView*				pMarkedPV;        // Wenn alle markierten Objekt auf derselben PageView liegen.

    Point						aRef1;            // Persistent - Drehmittelpunkt/Spiegelachse
    Point						aRef2;            // Persistent
    Point						aLastCrookCenter; // Persistent
    SdrHdlList					aHdl;
	sdr::ViewSelection*			mpSdrViewSelection;

    Rectangle					aMarkedObjRect;
    Rectangle					aMarkedPointsRect;
    Rectangle					aMarkedGluePointsRect;

    USHORT						nFrameHandlesLimit;
    ULONG					mnInsPointNum;      // Nummer des InsPoint
    ULONG						nMarkableObjCount;

    SdrDragMode					eDragMode;        // Persistent
    SdrViewEditMode				eEditMode;      // Persistent
    SdrViewEditMode				eEditMode0;     // Persistent

    //HMHunsigned					bHdlShown : 1;
    unsigned					bRefHdlShownOnly : 1; // Spiegelachse waerend Dragging (ni)
    unsigned					bDesignMode : 1;      // DesignMode fuer SdrUnoObj
    unsigned					bForceFrameHandles : 1; // Persistent - FrameDrag auch bei Einzelobjekten
    unsigned					bPlusHdlAlways : 1;   // Persistent
    unsigned					bMarkHdlWhenTextEdit : 1; // Persistent, default=FALSE
    unsigned					bInsPolyPoint : 1;     // z.Zt InsPolyPointDragging
    unsigned					bMarkedObjRectDirty : 1;
    unsigned					bMrkPntDirty : 1;
    unsigned					bMarkedPointsRectsDirty : 1;
    unsigned					bMarkableObjCountDirty : 1;
    
	// flag to completely disable handles at the view
	unsigned					mbMarkHandlesHidden : 1;

private:
    SVX_DLLPRIVATE void ImpClearVars();
    SVX_DLLPRIVATE void ImpSetPointsRects() const;
    void UndirtyMrkPnt() const;

protected:
    virtual void Notify(SfxBroadcaster& rBC, const SfxHint& rHint);
    virtual void ModelHasChanged(); // Wird von der PaintView gerufen
    virtual void SetMarkHandles();                                           // aHdl (Liste der Handles) fuellen
    virtual void SetMarkRects();                                             // Rects an den PageViews
    virtual void CheckMarked();                                              // MarkList nach Del und Lock Layer durchsuchen...
    virtual void AddDragModeHdl(SdrDragMode eMode);

	// add custom handles (used by other apps, e.g. AnchorPos)
    virtual void AddCustomHdl();

    void ForceRefToMarked();
    void ForceUndirtyMrkPnt() const                                       { if (bMrkPntDirty) UndirtyMrkPnt(); }

    //HMHvoid ImpShowMarkHdl(bool bNoRefHdl);
    virtual SdrObject* CheckSingleSdrObjectHit(const Point& rPnt, USHORT nTol, SdrObject* pObj, SdrPageView* pPV, ULONG nOptions, const SetOfByte* pMVisLay) const;
    virtual SdrObject* CheckSingleSdrObjectHit(const Point& rPnt, USHORT nTol, SdrObjList* pOL, SdrPageView* pPV, ULONG nOptions, const SetOfByte* pMVisLay, SdrObject*& rpRootObj) const;
    BOOL ImpIsFrameHandles() const;
    void ImpTakeDescriptionStr(USHORT nStrCacheID, String& rStr, USHORT nVal=0, USHORT nOpt=0) const;

    // Macht aus einer Winkelangabe in 1/100deg einen String inkl. Grad-Zeichen
    BOOL ImpMarkPoint(SdrHdl* pHdl, SdrMark* pMark, BOOL bUnmark);
    virtual BOOL MarkPoints(const Rectangle* pRect, BOOL bUnmark);
    BOOL MarkGluePoints(const Rectangle* pRect, BOOL bUnmark);

	void SetMoveOutside(BOOL bOn);
	BOOL IsMoveOutside() const;

protected:
	// #i71538# make constructors of SdrView sub-components protected to avoid incomplete incarnations which may get casted to SdrView
    SdrMarkView(SdrModel* pModel1, OutputDevice* pOut = 0L);
	virtual ~SdrMarkView();

public:
    virtual BOOL IsAction() const;
    virtual void MovAction(const Point& rPnt);
    virtual void EndAction();
    virtual void BckAction();
    virtual void BrkAction();
    virtual void TakeActionRect(Rectangle& rRect) const;

    virtual void ClearPageView();
    virtual void HideSdrPage();
    virtual BOOL IsObjMarkable(SdrObject* pObj, SdrPageView* pPV) const;

    // Liefert TRUE wenn Objekte, Punkte oder Klebepunkte durch Rahmenaufziehen
    // selektiert werden (solange wie der Rahmen aufgezogen wird).
    sal_Bool IsMarking() const { return IsMarkObj() || IsMarkPoints() || IsMarkGluePoints(); }

    // Objekte durch Aufziehen eines Selektionsrahmens markieren
    sal_Bool BegMarkObj(const Point& rPnt, sal_Bool bUnmark = sal_False);
    void MovMarkObj(const Point& rPnt);
    sal_Bool EndMarkObj();
    void BrkMarkObj();
    sal_Bool IsMarkObj() const { return (0L != mpMarkObjOverlay); }

    // DragModes: SDRDRAG_CREATE,SDRDRAG_MOVE,SDRDRAG_RESIZE,SDRDRAG_ROTATE,SDRDRAG_MIRROR,SDRDRAG_SHEAR,SDRDRAG_CROOK
    // Move==Resize
    // Das Interface wird hier evtl noch geaendert wg. Ortho-Drag
    void SetDragMode(SdrDragMode eMode);
    SdrDragMode GetDragMode() const { return eDragMode; }
    BOOL ChkDragMode(SdrDragMode eMode) const;
    void SetFrameHandles(BOOL bOn);
    BOOL IsFrameHandles() const { return bForceFrameHandles; }
    
	// Limit, ab wann implizit auf FrameHandles umgeschaltet wird. default=50.
    void SetFrameHandlesLimit(USHORT nAnz) { nFrameHandlesLimit=nAnz; }
    USHORT GetFrameHandlesLimit() const { return nFrameHandlesLimit; }

    void SetEditMode(SdrViewEditMode eMode);
    SdrViewEditMode GetEditMode() const { return eEditMode; }

    void SetEditMode(BOOL bOn=TRUE) { SetEditMode(bOn?SDREDITMODE_EDIT:SDREDITMODE_CREATE); }
    BOOL IsEditMode() const { return eEditMode==SDREDITMODE_EDIT; }
    void SetCreateMode(BOOL bOn=TRUE) { SetEditMode(bOn?SDREDITMODE_CREATE:SDREDITMODE_EDIT); }
    BOOL IsCreateMode() const { return eEditMode==SDREDITMODE_CREATE; }
    void SetGluePointEditMode(BOOL bOn=TRUE) { SetEditMode(bOn?SDREDITMODE_GLUEPOINTEDIT:eEditMode0); }
    BOOL IsGluePointEditMode() const { return eEditMode==SDREDITMODE_GLUEPOINTEDIT;; }

    void SetDesignMode(BOOL bOn=TRUE);
    BOOL IsDesignMode() const { return bDesignMode; }

    void SetFrameDragSingles(BOOL bOn=TRUE) { SetFrameHandles(bOn); }
    BOOL IsFrameDragSingles() const { return IsFrameHandles(); }

    // Feststellen, ob und wo ein Objekt bzw. ob ein Referenzpunkt
    // (Rotationsmittelpunkt,Spiegelachse) getroffen wird (SW special).
    SdrHitKind PickSomething(const Point& rPnt, short nTol=-2) const;
    BOOL HasMarkableObj() const;
    ULONG GetMarkableObjCount() const;
	//HMHvoid ShowMarkHdl(bool bNoRefHdl = false);
    //HMHvoid HideMarkHdl(bool bNoRefHdl = false);
    //HMHBOOL IsMarkHdlShown() const { return bHdlShown; }

////////////////////////////////////////////////////////////////////////////////////////////////////
// migrate selections

protected:
	// all available changing methods
	SdrMarkList& GetMarkedObjectListWriteAccess() { return mpSdrViewSelection->GetMarkedObjectListWriteAccess(); }
	void SetEdgesOfMarkedNodesDirty() { mpSdrViewSelection->SetEdgesOfMarkedNodesDirty(); }

public:
	// all available const methods for read access to selection
    const SdrMarkList& GetMarkedObjectList() const { return mpSdrViewSelection->GetMarkedObjectList(); }
    ULONG TryToFindMarkedObject(const SdrObject* pObj) const { return GetMarkedObjectList().FindObject(pObj); }
    SdrPageView* GetSdrPageViewOfMarkedByIndex(ULONG nNum) const { return GetMarkedObjectList().GetMark(nNum)->GetPageView(); }
    SdrMark* GetSdrMarkByIndex(ULONG nNum) const { return GetMarkedObjectList().GetMark(nNum); }
    SdrObject* GetMarkedObjectByIndex(ULONG nNum) const { return (GetMarkedObjectList().GetMark(nNum))->GetMarkedSdrObj(); }
    ULONG GetMarkedObjectCount() const { return GetMarkedObjectList().GetMarkCount(); }
    void SortMarkedObjects() const { GetMarkedObjectList().ForceSort(); }
	sal_Bool AreObjectsMarked() const { return (0L != GetMarkedObjectList().GetMarkCount()); }
    String GetDescriptionOfMarkedObjects() const { return GetMarkedObjectList().GetMarkDescription(); }
    String GetDescriptionOfMarkedPoints() const { return GetMarkedObjectList().GetPointMarkDescription(); }
    String GetDescriptionOfMarkedGluePoints() const { return GetMarkedObjectList().GetGluePointMarkDescription(); }
	sal_Bool GetBoundRectFromMarkedObjects(SdrPageView* pPageView, Rectangle& rRect) const { return GetMarkedObjectList().TakeBoundRect(pPageView, rRect); }
	sal_Bool GetSnapRectFromMarkedObjects(SdrPageView* pPageView, Rectangle& rRect) const { return GetMarkedObjectList().TakeSnapRect(pPageView, rRect); }

    // Eine Liste aller an markierten Knoten gebundenen Kanten holen,
    // die selbst jedoch nicht markiert sind.
	const SdrMarkList& GetEdgesOfMarkedNodes() const { return mpSdrViewSelection->GetEdgesOfMarkedNodes(); }
	const SdrMarkList& GetMarkedEdgesOfMarkedNodes() const { return mpSdrViewSelection->GetMarkedEdgesOfMarkedNodes(); }
	const List& GetTransitiveHullOfMarkedObjects() const { return mpSdrViewSelection->GetAllMarkedObjects(); }

////////////////////////////////////////////////////////////////////////////////////////////////////

	// mechanism to complete disable handles at the view. Handles will be hidden and deleted
	// when set, no new ones created, no interaction allowed. Handles will be recreated and shown
	// when reset. Default is false.
    void hideMarkHandles();
    void showMarkHandles();
    bool areMarkHandlesHidden() const { return mbMarkHandlesHidden; }

	BOOL IsMarkedHit(const Point& rPnt, short nTol=-2) const { return IsMarkedObjHit(rPnt,nTol); }
    BOOL IsMarkedObjHit(const Point& rPnt, short nTol=-2) const;

    // Pick: Unterstuetzte Optionen fuer nOptions sind SEARCH_NEXT, SEARCH_BACKWARD (ni)
    SdrHdl* PickHandle(const Point& rPnt, ULONG nOptions=0, SdrHdl* pHdl0=NULL) const;
    
	// Pick: Unterstuetzte Optionen fuer nOptions sind:
    // SDRSEARCH_DEEP SDRSEARCH_ALSOONMASTER SDRSEARCH_TESTMARKABLE SDRSEARCH_TESTTEXTEDIT
    // SDRSEARCH_WITHTEXT SDRSEARCH_TESTTEXTAREA SDRSEARCH_BACKWARD SDRSEARCH_MARKED
    // SDRSEARCH_WHOLEPAGE
    virtual BOOL PickObj(const Point& rPnt, short nTol, SdrObject*& rpObj, SdrPageView*& rpPV, ULONG nOptions, SdrObject** ppRootObj, ULONG* pnMarkNum=NULL, USHORT* pnPassNum=NULL) const;
    virtual BOOL PickObj(const Point& rPnt, short nTol, SdrObject*& rpObj, SdrPageView*& rpPV, ULONG nOptions=0) const;
    // BOOL PickObj(const Point& rPnt, SdrObject*& rpObj, SdrPageView*& rpPV, ULONG nOptions=0) const { return PickObj(rPnt,nHitTolLog,rpObj,rpPV,nOptions); }
    BOOL MarkObj(const Point& rPnt, short nTol=-2, BOOL bToggle=FALSE, BOOL bDeep=FALSE);

    // Pick: Unterstuetzte Optionen fuer nOptions sind SDRSEARCH_PASS2BOUND und SDRSEARCH_PASS3NEAREST
    BOOL PickMarkedObj(const Point& rPnt, SdrObject*& rpObj, SdrPageView*& rpPV, ULONG* pnMarkNum=NULL, ULONG nOptions=0) const;
    
	// Sucht sich das Oberste der markierten Objekte (O1) und sucht von dort
    // aus in Richtung nach Unten dann das erste nichtmarkierte Objekt (O2).
    // Bei Erfolg wird die Markierung von O1 entfernt, an O2 gesetzt und TRUE
    // returniert. Mit dem Parameter bPrev=TRUE geht die Suche genau in die
    // andere Richtung.
    BOOL MarkNextObj(BOOL bPrev=FALSE);
    
	// Sucht sich das Oberste der markierten Objekte (O1) das von rPnt/nTol
    // getroffen wird und sucht von dort aus in Richtung nach Unten dann das
    // erste nichtmarkierte Objekt (O2). Bei Erfolg wird die Markierung von
    // O1 entfernt, an O2 gesetzt und TRUE returniert.
    // Mit dem Parameter bPrev=TRUE geht die Suche genau in die andere Richtung.
    BOOL MarkNextObj(const Point& rPnt, short nTol=-2, BOOL bPrev=FALSE);
    
	// Alle Objekte innerhalb eines rechteckigen Bereichs markieren
    // Markiert werden nur Objekte, die vollstaendig eingeschlossen sind.
    BOOL MarkObj(const Rectangle& rRect, BOOL bUnmark=FALSE);
    void MarkObj(SdrObject* pObj, SdrPageView* pPV, BOOL bUnmark=FALSE, BOOL bImpNoSetMarkHdl=FALSE);
    void MarkAllObj(SdrPageView* pPV=NULL); // pPage=NULL => alle angezeigten Seiten
    void UnmarkAllObj(SdrPageView* pPV=NULL); // pPage=NULL => alle angezeigten Seiten
    
	// Diese Funktion kostet etwas Zeit, da die MarkList durchsucht werden muss.
    BOOL IsObjMarked(SdrObject* pObj) const;
    // void MarkAll(SdrPageView* pPV=NULL) { MarkAllObj(pPV); } -> replace with inline
    void UnMarkAll(SdrPageView* pPV=NULL) { UnmarkAllObj(pPV); }

	// Groesse der Markierungs-Handles abfragen/setzen. Angabe in Pixel.
    // Der Wert ist als Kantenlaenge zu betrachten. Gerade Werte werden
    // auf Ungerade aufgerundet: 3->3, 4->5, 5->5, 6->7, 7->7, ...
    // Defaultwert ist 7, Mindestwert 3 Pixel.
    USHORT GetMarkHdlSizePixel() const;
    void SetMarkHdlSizePixel(USHORT nSiz);
    
	// Die Groesse der Markierungs-Handles wird ueber die jeweilige Aufloesung
    // und die Groesse des Bereichs der markierten Objekte so angepasst, dass
    // sie sich bei einer Frame-Selektion moeglichst nicht ueberschneiden.
    // Dazu muessen die Handles ggf. verkleinert dargestellt werden. Mit der
    // MinMarkHdlSize kann man hierfuer eine Mindestgroesse angeben.
    // Defaultwert ist 3, Mindestwert 3 Pixel.
    BOOL IsSolidMarkHdl() const { return aHdl.IsFineHdl(); }
    void SetSolidMarkHdl(BOOL bOn);

    virtual BOOL HasMarkablePoints() const;
    virtual ULONG GetMarkablePointCount() const;
    virtual BOOL HasMarkedPoints() const;
    virtual ULONG GetMarkedPointCount() const;

    // Nicht alle Punkte lassen sich markieren:
    virtual BOOL IsPointMarkable(const SdrHdl& rHdl) const;
    virtual BOOL MarkPoint(SdrHdl& rHdl, BOOL bUnmark=FALSE);
	
	/** should only be used from outside svx for special ui elements */
	BOOL MarkPointHelper(SdrHdl* pHdl, SdrMark* pMark, BOOL bUnmark);

    // alle Punkte innerhalb dieses Rechtecks markieren (Viewkoordinaten)
    BOOL MarkPoints(const Rectangle& rRect, BOOL bUnmark=FALSE) { return MarkPoints(&rRect,bUnmark); }
    BOOL UnmarkPoint(SdrHdl& rHdl) { return MarkPoint(rHdl,TRUE); }
    BOOL UnMarkPoint(SdrHdl& rHdl) { return MarkPoint(rHdl,TRUE); }
    BOOL IsPointMarked(const SdrHdl& rHdl) const { ForceUndirtyMrkPnt(); return &rHdl!=NULL && rHdl.IsSelected(); }
    BOOL MarkAllPoints() { return MarkPoints(NULL,FALSE); }
    BOOL UnmarkAllPoints() { return MarkPoints(NULL,TRUE); }
    BOOL UnMarkAllPoints() { return MarkPoints(NULL,TRUE); }
    
	// Sucht sich den ersten markierten Punkt (P1) und sucht von dort
    // aus in den ersten nichtmarkierte Punkt (P2).
    // Bei Erfolg wird die Markierung von P1 entfernt, an P2 gesetzt und TRUE
    // returniert. Mit dem Parameter bPrev=TRUE geht die Suche genau in die
    // andere Richtung.
    BOOL MarkNextPoint(BOOL bPrev=FALSE);
    
	// Sucht sich den ersten markierten Punkt (P1) das von rPnt
    // getroffen wird und sucht von dort aus den
    // ersten nichtmarkierten Punkt (P2). Bei Erfolg wird die Markierung von
    // P1 entfernt, an P2 gesetzt und TRUE returniert.
    // Mit dem Parameter bPrev=TRUE geht die Suche genau in die andere Richtung.
    BOOL MarkNextPoint(const Point& rPnt, BOOL bPrev=FALSE);
    
	// Die Nummer des passenden Handles raussuchen. Nicht gefunden
    // liefert CONTAINER_ENTRY_NOTFOUND.
    ULONG GetHdlNum(SdrHdl* pHdl) const { return aHdl.GetHdlNum(pHdl); }
    SdrHdl* GetHdl(ULONG nHdlNum)  const { return aHdl.GetHdl(nHdlNum); }
    const SdrHdlList& GetHdlList() const { return aHdl; }

    // Selektionsrahmen fuer Punktmarkierung aufziehen.
    // Wird nur gestartet, wenn HasMarkablePoints() TRUE liefert.
    // BOOL BegMarkPoints(const Point& rPnt, OutputDevice* pOut);
    sal_Bool BegMarkPoints(const Point& rPnt, sal_Bool bUnmark = sal_False);
    void MovMarkPoints(const Point& rPnt);
    sal_Bool EndMarkPoints();
    void BrkMarkPoints();
    sal_Bool IsMarkPoints() const { return (0L != mpMarkPointsOverlay); }

    // Zusatzhandles dauerhaft sichtbar schalten
    void SetPlusHandlesAlwaysVisible(BOOL bOn);
    BOOL IsPlusHandlesAlwaysVisible() const { return bPlusHdlAlways; }

    // Handles sichrbar waerend TextEdit (in doppelter Groesse)?
    // Persistent, default=FALSE
    void SetMarkHdlWhenTextEdit(BOOL bOn) { bMarkHdlWhenTextEdit=bOn; }
    BOOL IsMarkHdlWhenTextEdit() const { return bMarkHdlWhenTextEdit; }
    
    BOOL HasMarkableGluePoints() const;
    ULONG GetMarkableGluePointCount() const;
    BOOL HasMarkedGluePoints() const;
    ULONG GetMarkedGluePointCount() const;

    // Ein Klebepunkt wird eindeutig identifiziert durch das SdrObject
    // (dem er zugehoert) sowie einem USHORT nId (da jedes SdrObject je
    // mehrere Klebepunkte haben kann. Hier an der View kommt zudem noch
    // eine SdrPageView, die stets korrekt gesetzt sein sollte.
    // Alternativ kann ein Klebepunkt durch ein SdrHdl bezeichnet werden.
    // Die SdrHdl-Instanz beinhaltet dann aller erforderlichen Informationen.
    // Der Klebepunkt ist in diesem Fall dann zwangslaeufig markiert (nur auf
    // markierten Klebepunkten sitzen Anfasser).
    // Achtung: Bei jeder Aenderung des Klebepunktmarkierungsstatus wird die
    // Handleliste erneut berechnet. Alle vorher gemerkten SdrHdl* sind
    // damit ungueltig, ebenso die Punkt-Id's!
    // Pick: Unterstuetzte Optionen fuer nOptions sind SEARCH_NEXT, SEARCH_BACKWARD
    BOOL PickGluePoint(const Point& rPnt, SdrObject*& rpObj, USHORT& rnId, SdrPageView*& rpPV, ULONG nOptions=0) const;
    BOOL MarkGluePoint(const SdrObject* pObj, USHORT nId, const SdrPageView* pPV, BOOL bUnmark=FALSE);
    BOOL UnmarkGluePoint(const SdrObject* pObj, USHORT nId, const SdrPageView* pPV) { return MarkGluePoint(pObj,nId,pPV,TRUE); }
    BOOL IsGluePointMarked(const SdrObject* pObj, USHORT nId) const;
    BOOL UnmarkGluePoint(const SdrHdl& rHdl);

    // Hdl eines markierten GluePoints holen. Nicht markierte
    // GluePoints haben keine Handles
    SdrHdl* GetGluePointHdl(const SdrObject* pObj, USHORT nId) const;
    BOOL IsGluePoint(const SdrHdl& rHdl) const { return &rHdl!=NULL && rHdl.GetKind()==HDL_GLUE; }
    
	// alle Punkte innerhalb dieses Rechtecks markieren (Viewkoordinaten)
    BOOL MarkGluePoints(const Rectangle& rRect) { return MarkGluePoints(&rRect,FALSE); }
    BOOL UnmarkGluePoints(const Rectangle& rRect) { return MarkGluePoints(&rRect,TRUE); }
    BOOL MarkAllGluePoints() { return MarkGluePoints(NULL,FALSE); }
    BOOL UnmarkAllGluePoints() { return MarkGluePoints(NULL,TRUE); }
    
	// Sucht sich den ersten markierten Klebepunkt (P1) und sucht von dort
    // aus in den ersten nichtmarkierte Klebepunkt (P2).
    // Bei Erfolg wird die Markierung von P1 entfernt, an P2 gesetzt und TRUE
    // returniert. Mit dem Parameter bPrev=TRUE geht die Suche genau in die
    // andere Richtung.
    BOOL MarkNextGluePoint(BOOL bPrev=FALSE);
    
	// Sucht sich den ersten markierten Klebepunkt (P1) das von rPnt
    // getroffen wird und sucht von dort aus den
    // ersten nichtmarkierten Klebepunkt (P2). Bei Erfolg wird die Markierung
    // von P1 entfernt, an P2 gesetzt und TRUE returniert.
    // Mit dem Parameter bPrev=TRUE geht die Suche genau in die andere Richtung.
    BOOL MarkNextGluePoint(const Point& rPnt, BOOL bPrev=FALSE);

    // Selektionsrahmen fuer Klebepunktmarkierung aufziehen.
    // Wird nur gestartet, wenn HasMarkableGluePoints() TRUE liefert.
    // Der GlueEditMode TRUE wird nicht abgefragt.
    // BOOL BegMarkGluePoints(const Point& rPnt, OutputDevice* pOut);
    sal_Bool BegMarkGluePoints(const Point& rPnt, sal_Bool bUnmark = sal_False);
    void MovMarkGluePoints(const Point& rPnt);
    sal_Bool EndMarkGluePoints();
    void BrkMarkGluePoints();
    sal_Bool IsMarkGluePoints() const { return (0L != mpMarkGluePointsOverlay); }
    
	// bRestraintPaint=FALSE bewirkt, dass die Handles nicht sofort wieder gemalt werden.
	// AdjustMarkHdl wird eh' nur gerufen, wenn was geaendert wurde; was idR ein Invalidate
	// zur Folge hat. Am Ende von des Redraw werden die Handles automatisch gezeichnet.
	// Der Zweck ist, unnoetiges Flackern zu vermeiden. -> Funkt noch nich, deshalb TRUE!
    void AdjustMarkHdl(); //HMHBOOL bRestraintPaint=TRUE);

    const Rectangle& GetMarkedObjRect() const; // SnapRects der Objekte, ohne Strichstaerke
    Rectangle GetMarkedObjBoundRect() const;   // inkl. Strichstaerke, ueberstehende Fetzen, ...
    const Rectangle& GetMarkedPointsRect() const;     // Umschliessendes Rechteck aller markierten Punkte
    const Rectangle& GetMarkedGluePointsRect() const; // Umschliessendes Rechteck aller markierten Klebepunkte
    const Rectangle& GetAllMarkedRect() const { return GetMarkedObjRect(); }
    Rectangle GetAllMarkedBoundRect() const { return GetMarkedObjBoundRect(); }
    const Rectangle& GetAllMarkedPointsRect() const  { return GetMarkedPointsRect(); }

    // Wird immer dann gerufen, wenn sich die Liste der markierten Objekte
    // moeglicherweise geaendert hat. Wer ueberlaed muss unbedingt auch die
    // Methode der Basisklasse rufen!
    virtual void MarkListHasChanged();

    // Betreten (Editieren) einer evtl markierten Objektgruppe. Sind mehrere
    // Objektgruppen markiert, so wird die Oberste genommen. Anschliessend
    // liegen alle Memberobjekte der Gruppe im direkten Zugriff. Alle anderen
    // Objekte koennen waerendessen nicht bearbeitet werden (bis zum naechsten
    // LeaveGroup()). Bei einer seitenuebergreifenden Markierung wird jede Page
    // separat abgearbeitet. Die Methode liefert TRUE, wenn wenigstens eine
    // Gruppe betreten wurde.
    BOOL EnterMarkedGroup();

    // Den Mittelpunkt des letzten Crook-Dragging abholen. Den kann man
    // bei einem anschliessenden Rotate sinnvoll als Drehmittelpunkt setzen.
    const Point& GetLastCrookCenter() const { return aLastCrookCenter; }
    
	// Wird automatisch von der DragView beim beenden eines Crook-Drag gesetzt.
    void SetLastCrookCenter(const Point& rPt) { aLastCrookCenter=rPt; }
    
	// Rotationsmittelpunkt bzw. Startpunkt der Spiegelachse
    const Point& GetRef1() const { return aRef1; }
    void SetRef1(const Point& rPt);
    
	// Endpunkt der Spiegelachse
    const Point& GetRef2() const { return aRef1; }
    void SetRef2(const Point& rPt);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// - Hit-Toleranzen:
//   Die muessen natuerlich immer in logischen Koordinaten angegeben werden. Also
//   immer brav den gewuenschten Pixelwert mit PixelToLogic in Logischen umrechnen.
//   Angenommen ein Wert 100 (logisch)
//   - Bei einer waagerechten Haarlinie (Objekt mit Hoehe 0) waere dann +/-100,
//     also ein vertikaler Bereich von 200 logischen Einheiten sensitiv.
//   - Bei Polygonen wird ein Rect mit der Groesse (200,200) generiert und ein
//     Touch-Test zwischen Poly und diesem Rect durchgefuehrt.
//   - Eine Sonderbehandlung erfahren Objekte, die bei SdrObject::HasEdit()==TRUE
//     liefern (z.B. Textrahmen). Hierbei wird ein weiterer sensitiver Bereich mit
//     einer Breite von 2*Tol (in diesem Beispiel also 200 Einheiten) um das Objekt
//     herumgelegt. Waerend ein Hit direkt ins Objekt die Edit-Methode ruft,
//     ermoeglicht ein Hit in den umliegenden sensitiven Bereich ein Dragging.
// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //_SVDMRKV_HXX

