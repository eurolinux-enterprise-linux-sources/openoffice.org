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

#ifndef _SVDMODEL_HXX
#define _SVDMODEL_HXX

#include <com/sun/star/uno/Sequence.hxx>
#include <cppuhelper/weakref.hxx>
#include <sot/storage.hxx>
#include <tools/link.hxx>
#include <tools/contnr.hxx>
#include <tools/weakbase.hxx>
#ifndef _MAPMOD_HXX //autogen
#include <vcl/mapmod.hxx>
#endif
#include <svtools/brdcst.hxx>
#include <tools/string.hxx>
#include <tools/datetime.hxx>
#include <svtools/hint.hxx>

#include <svtools/style.hxx>
#include <svx/pageitem.hxx>
#include <vcl/field.hxx>

#include <boost/shared_ptr.hpp>

class OutputDevice;
#include <svx/svdtypes.hxx> // fuer enum RepeatFuncts
#include <vcl/field.hxx>
#include "svx/svxdllapi.h"

#include <vos/ref.hxx>

#if defined(UNX) || defined(WIN) || defined(WNT)
#define DEGREE_CHAR ((sal_Unicode)176)   /* 0xB0 = Ansi */
#endif

#if defined(DOS) || defined(OS2)
#define DEGREE_CHAR ((sal_Unicode)248)   /* 0xF8 = IBM PC (Erw. ASCII) */
#endif

#ifndef DEGREE_CHAR
#error unbekannte Plattrorm
#endif

class SdrOutliner;
class SdrLayerAdmin;
class SdrObjList;
class SdrObject;
class SdrPage;
class SdrPageView;
class SdrTextObj;
class SdrUndoAction;
class SdrUndoGroup;
class AutoTimer;
class SfxItemPool;
class SfxItemSet;
class SfxRepeatTarget;
class SfxStyleSheet;
class SfxUndoAction;
class SfxUndoManager;
class SvxLinkManager;
class XBitmapList;
class XBitmapTable;
class XColorTable;
class XDashList;
class XDashTable;
class XGradientList;
class XGradientTable;
class XHatchList;
class XHatchTable;
class XLineEndList;
class XLineEndTable;
class SvxForbiddenCharactersTable;
class SvNumberFormatter;
class SotStorage;
class SdrOutlinerCache;
class SotStorageRef;
class SdrUndoFactory;
namespace comphelper{
    class IEmbeddedHelper;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

#define SDR_SWAPGRAPHICSMODE_NONE		0x00000000
#define SDR_SWAPGRAPHICSMODE_TEMP		0x00000001
#define SDR_SWAPGRAPHICSMODE_DOC		0x00000002
#define SDR_SWAPGRAPHICSMODE_PURGE		0x00000100
#define SDR_SWAPGRAPHICSMODE_DEFAULT	(SDR_SWAPGRAPHICSMODE_TEMP|SDR_SWAPGRAPHICSMODE_DOC|SDR_SWAPGRAPHICSMODE_PURGE)

////////////////////////////////////////////////////////////////////////////////////////////////////

enum SdrHintKind
{
				  HINT_UNKNOWN,         // Unbekannt
				  HINT_LAYERCHG,        // Layerdefinition geaendert
				  HINT_LAYERORDERCHG,   // Layerreihenfolge geaendert (Insert/Remove/ChangePos)
				  HINT_PAGEORDERCHG,    // Reihenfolge der Seiten (Zeichenseiten oder Masterpages) geaendert (Insert/Remove/ChangePos)
				  HINT_OBJCHG,          // Objekt geaendert
				  HINT_OBJINSERTED,     // Neues Zeichenobjekt eingefuegt
				  HINT_OBJREMOVED,      // Zeichenobjekt aus Liste entfernt
				  HINT_MODELCLEARED,    // gesamtes Model geloescht (keine Pages mehr da). not impl.
				  HINT_REFDEVICECHG,    // RefDevice geaendert
				  HINT_DEFAULTTABCHG,   // Default Tabulatorweite geaendert
				  HINT_DEFFONTHGTCHG,   // Default FontHeight geaendert
				  HINT_MODELSAVED,      // Dokument wurde gesichert
				  HINT_SWITCHTOPAGE,    // #94278# UNDO/REDO at an object evtl. on another page
				  HINT_BEGEDIT,			// Is called after the object has entered text edit mode
				  HINT_ENDEDIT			// Is called after the object has left text edit mode
};

class SVX_DLLPUBLIC SdrHint: public SfxHint
{
public:
	Rectangle								maRectangle;
	const SdrPage*							mpPage;
	const SdrObject*						mpObj;
	const SdrObjList*						mpObjList;
	SdrHintKind								meHint;

public:
	TYPEINFO();

	SdrHint();
	SdrHint(SdrHintKind eNewHint);
	SdrHint(const SdrObject& rNewObj);
	SdrHint(const SdrObject& rNewObj, const Rectangle& rRect);

	void SetPage(const SdrPage* pNewPage);
	void SetObjList(const SdrObjList* pNewOL);
	void SetObject(const SdrObject* pNewObj);
	void SetKind(SdrHintKind eNewKind);
	void SetRect(const Rectangle& rNewRect);

	const SdrPage* GetPage() const;
	const SdrObjList* GetObjList() const;
	const SdrObject* GetObject() const;
	SdrHintKind GetKind() const;
	const Rectangle& GetRect() const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Flag um nach dem Laden des Pools Aufzuraeumen (d.h. die RefCounts
// neu zu bestimmen und unbenutztes wegzuwerfen). FALSE == aktiv
#define LOADREFCOUNTS (FALSE)

struct SdrDocumentStreamInfo
{
	FASTBOOL	    mbDeleteAfterUse;
	String		    maUserData;
    com::sun::star::uno::Reference < com::sun::star::embed::XStorage > mxStorageRef;
	BOOL		    mbDummy1 : 1;
};

struct SdrModelImpl;

class SVX_DLLPUBLIC SdrModel : public SfxBroadcaster, public tools::WeakBase< SdrModel >
{
protected:
	DateTime       aReadDate;  // Datum des Einstreamens
	Container      maMaPag;     // StammSeiten (Masterpages)
	Container      maPages;
	Link           aUndoLink;  // Link fuer einen NotifyUndo-Handler
	Link           aIOProgressLink;
	String         aTablePath;
	Size           aMaxObjSize; // z.B. fuer Autogrowing Text
    Fraction       aObjUnit;   // Beschreibung der Koordinateneinheiten fuer ClipBoard, Drag&Drop, ...
    MapUnit        eObjUnit;   // see above
    FieldUnit      eUIUnit;      // Masseinheit, Masstab (z.B. 1/1000) fuer die UI (Statuszeile) wird von ImpSetUIUnit() gesetzt
    Fraction       aUIScale;     // see above
    String         aUIUnitStr;   // see above
    Fraction       aUIUnitFact;  // see above
    int            nUIUnitKomma; // see above
    FASTBOOL       bUIOnlyKomma; // see above

	SdrLayerAdmin*  pLayerAdmin;
	SfxItemPool*    pItemPool;
	FASTBOOL        bMyPool;        // zum Aufraeumen von pMyPool ab 303a
	comphelper::IEmbeddedHelper* 
                    m_pEmbeddedHelper; // helper for embedded objects to get rid of the SfxObjectShell
	SdrOutliner*    pDrawOutliner;  // ein Outliner zur Textausgabe
	SdrOutliner*    pHitTestOutliner;// ein Outliner fuer den HitTest
	ULONG           nDefTextHgt;    // Default Texthoehe in logischen Einheiten
	OutputDevice*   pRefOutDev;     // ReferenzDevice fuer die EditEngine
	ULONG           nProgressAkt;   // fuer den
	ULONG           nProgressMax;   // ProgressBar-
	ULONG           nProgressOfs;   // -Handler
	rtl::Reference< SfxStyleSheetBasePool > mxStyleSheetPool;
	SfxStyleSheet*	pDefaultStyleSheet;
	SvxLinkManager* pLinkManager;   // LinkManager
	Container*      pUndoStack;
	Container*      pRedoStack;
	SdrUndoGroup*   pAktUndoGroup;  // Fuer mehrstufige
	USHORT          nUndoLevel;     // Undo-Klammerung
	bool			mbUndoEnabled;	// If false no undo is recorded or we are during the execution of an undo action
	USHORT          nProgressPercent; // fuer den ProgressBar-Handler
	USHORT          nLoadVersion;   // Versionsnummer der geladenen Datei
	FASTBOOL        bExtColorTable; // Keinen eigenen ColorTable
	sal_Bool        mbChanged;
	FASTBOOL        bInfoChanged;
	FASTBOOL        bPagNumsDirty;
	FASTBOOL        bMPgNumsDirty;
	FASTBOOL        bPageNotValid;  // TRUE=Doc ist nur ObjektTraeger. Page ist nicht gueltig.
	FASTBOOL        bSavePortable;  // Metafiles portabel speichern
	FASTBOOL        bNoBitmapCaching;   // Bitmaps fuer Screenoutput cachen
	FASTBOOL        bReadOnly;
	FASTBOOL        bTransparentTextFrames;
	FASTBOOL        bSaveCompressed;
	FASTBOOL        bSwapGraphics;
	FASTBOOL        bPasteResize; // Objekte werden gerade resized wegen Paste mit anderem MapMode
	FASTBOOL		bSaveOLEPreview;      // save preview metafile of OLE objects
	UINT16          nStreamCompressMode;  // Komprimiert schreiben?
	UINT16          nStreamNumberFormat;
	UINT16          nDefaultTabulator;
	UINT32          nMaxUndoCount;
	FASTBOOL        bSaveNative;
	BOOL            bStarDrawPreviewMode;


//////////////////////////////////////////////////////////////////////////////
// sdr::Comment interface
private:
	// the next unique comment ID, used for counting added comments. Initialized
	// to 0. UI shows one more due to the fact that 0 is a no-no for users.
	sal_uInt32											mnUniqueCommentID;

public:
	// create a new, unique comment ID
	sal_uInt32 GetNextUniqueCommentID();

	// get the author name
	::rtl::OUString GetDocumentAuthorName() const;

	// for export
	sal_uInt32 GetUniqueCommentID() const { return mnUniqueCommentID; }

	// for import
	void SetUniqueCommentID(sal_uInt32 nNewID) { if(nNewID != mnUniqueCommentID) { mnUniqueCommentID = nNewID; } }
	/** cl: added this for OJ to complete his reporting engine, does not work
		correctly so only enable it for his model */
	bool IsAllowShapePropertyChangeListener() const;
	void SetAllowShapePropertyChangeListener( bool bAllow );

	UINT16          nStarDrawPreviewMasterPageNum;
	// Reserven fuer kompatible Erweiterungen
//-/	SfxItemPool*    pUndoItemPool;
	SotStorage*		pModelStorage;
	SvxForbiddenCharactersTable* mpForbiddenCharactersTable;
	ULONG			nSwapGraphicsMode;

	SdrOutlinerCache* mpOutlinerCache;
	SdrModelImpl*	mpImpl;
	UINT16          mnCharCompressType;
	UINT16          nReserveUInt5;
	UINT16          nReserveUInt6;
	UINT16          nReserveUInt7;
	FASTBOOL        mbModelLocked;
	FASTBOOL        mbKernAsianPunctuation;
    FASTBOOL        mbAddExtLeading;
	FASTBOOL        mbInDestruction;

	// Zeiger auf Paletten, Listen und Tabellen
	XColorTable*    pColorTable;
	XDashList*      pDashList;
	XLineEndList*   pLineEndList;
	XHatchList*     pHatchList;
	XGradientList*  pGradientList;
	XBitmapList*    pBitmapList;

	// New src638: NumberFormatter for drawing layer and
	// method for getting it. It is constructed on demand
	// and destroyed when destroying the SdrModel.
	SvNumberFormatter* mpNumberFormatter;
public:
	const SvNumberFormatter& GetNumberFormatter() const;
protected:

	virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > createUnoModel();

private:
	// Nicht implementiert:
	SVX_DLLPRIVATE SdrModel(const SdrModel& rSrcModel);
	SVX_DLLPRIVATE void operator=(const SdrModel& rSrcModel);
	SVX_DLLPRIVATE FASTBOOL operator==(const SdrModel& rCmpModel) const;
//#if 0 // _SOLAR__PRIVATE
	SVX_DLLPRIVATE void ImpPostUndoAction(SdrUndoAction* pUndo);
	SVX_DLLPRIVATE void ImpSetUIUnit();
	SVX_DLLPRIVATE void ImpSetOutlinerDefaults( SdrOutliner* pOutliner, BOOL bInit = FALSE );
	SVX_DLLPRIVATE void ImpReformatAllTextObjects();
	SVX_DLLPRIVATE void ImpReformatAllEdgeObjects();	// #103122#
	SVX_DLLPRIVATE void ImpCreateTables();
	SVX_DLLPRIVATE void ImpCtor(SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, bool bUseExtColorTable,
		bool bLoadRefCounts = true);

//#endif // __PRIVATE

	// this is a weak reference to a possible living api wrapper for this model
	::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > mxUnoModel;

public:
//#if 0 // _SOLAR__PRIVATE
	FASTBOOL IsPasteResize() const        { return bPasteResize; }
	void     SetPasteResize(FASTBOOL bOn) { bPasteResize=bOn; }
//#endif // __PRIVATE
	TYPEINFO();
	// Steckt man hier seinen eigenen Pool rein, so wird die Klasse auch
	// Aktionen an ihm vornehmen (Put(),Remove()). Bei Zerstoerung von
	// SdrModel wird dieser Pool ver delete geloescht!
	// Gibt man den Konstruktor stattdessen eine NULL mit, so macht sich
	// die Klasse einen eigenen Pool (SdrItemPool), den sie dann auch im
	// Destruktor zerstoert.
	// Bei Verwendung eines eigenen Pools ist darauf zu achten, dass dieser
	// von SdrItemPool abgeleitet ist, falls man von SdrAttrObj abgeleitete
	// Zeichenobjekte verwenden moechte. Setzt man degegen nur vom abstrakten
	// Basisobjekt SdrObject abgeleitete Objekte ein, so ist man frei in der
	// Wahl des Pools.
	SdrModel(SfxItemPool* pPool=NULL, ::comphelper::IEmbeddedHelper* pPers=NULL, sal_Bool bLoadRefCounts = LOADREFCOUNTS);
	SdrModel(const String& rPath, SfxItemPool* pPool=NULL, ::comphelper::IEmbeddedHelper* pPers=NULL, sal_Bool bLoadRefCounts = LOADREFCOUNTS);
	SdrModel(SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, FASTBOOL bUseExtColorTable, sal_Bool bLoadRefCounts = LOADREFCOUNTS);
	SdrModel(const String& rPath, SfxItemPool* pPool, ::comphelper::IEmbeddedHelper* pPers, FASTBOOL bUseExtColorTable, sal_Bool bLoadRefCounts = LOADREFCOUNTS);
	virtual ~SdrModel();
	void ClearModel(sal_Bool bCalledFromDestructor);

	// Hier kann man erfragen, ob das Model gerade eingrstreamt wird
	FASTBOOL IsLoading() const                  { return sal_False /*BFS01 bLoading */; }
	// Muss z.B. ueberladen werden, um das Swappen/LoadOnDemand von Grafiken
	// zu ermoeglichen. Wird rbDeleteAfterUse auf TRUE gesetzt, so wird
	// die SvStream-Instanz vom Aufrufer nach Gebrauch destruiert.
	// Wenn diese Methode NULL liefert, wird zum Swappen eine temporaere
	// Datei angelegt.
	// Geliefert werden muss der Stream, aus dem das Model geladen wurde
	// bzw. in den es zuletzt gespeichert wurde.
	virtual SvStream* GetDocumentStream( SdrDocumentStreamInfo& rStreamInfo ) const;
	// Die Vorlagenattribute der Zeichenobjekte in harte Attribute verwandeln.
	void BurnInStyleSheetAttributes();
	// Wer sich von SdrPage ableitet muss sich auch von SdrModel ableiten
	// und diese beiden VM AllocPage() und AllocModel() ueberladen...
	virtual SdrPage*  AllocPage(FASTBOOL bMasterPage);
	virtual SdrModel* AllocModel() const;

	// Aenderungen an den Layern setzen das Modified-Flag und broadcasten am Model!
	const SdrLayerAdmin& GetLayerAdmin() const                  { return *pLayerAdmin; }
	SdrLayerAdmin&       GetLayerAdmin()                        { return *pLayerAdmin; }

	const SfxItemPool&   GetItemPool() const                    { return *pItemPool; }
	SfxItemPool&         GetItemPool()                          { return *pItemPool; }

	SdrOutliner&         GetDrawOutliner(const SdrTextObj* pObj=NULL) const;

	/** returns a new created and non shared outliner.
		The outliner will not get updated when the SdrModel is changed.
	*/
	boost::shared_ptr< SdrOutliner > CreateDrawOutliner(const SdrTextObj* pObj=NULL);

	SdrOutliner&         GetHitTestOutliner() const { return *pHitTestOutliner; }
	const SdrTextObj*    GetFormattingTextObj() const;
	// Die TextDefaults (Font,Hoehe,Farbe) in ein Set putten
	void         		 SetTextDefaults() const;
	static void    		 SetTextDefaults( SfxItemPool* pItemPool, ULONG nDefTextHgt );

	// ReferenzDevice fuer die EditEngine
	void                 SetRefDevice(OutputDevice* pDev);
	OutputDevice*        GetRefDevice() const                   { return pRefOutDev; }
	// Wenn ein neuer MapMode am RefDevice gesetzt wird o.ae.
	void                 RefDeviceChanged(); // noch nicht implementiert
	// Default-Schrifthoehe in logischen Einheiten
	void                 SetDefaultFontHeight(ULONG nVal);
	ULONG                GetDefaultFontHeight() const           { return nDefTextHgt; }
	// Default-Tabulatorweite fuer die EditEngine
	void                 SetDefaultTabulator(USHORT nVal);
	USHORT               GetDefaultTabulator() const            { return nDefaultTabulator; }

	// Der DefaultStyleSheet wird jedem Zeichenobjekt verbraten das in diesem
	// Model eingefuegt wird und kein StyleSheet gesetzt hat.
	SfxStyleSheet*       GetDefaultStyleSheet() const             { return pDefaultStyleSheet; }
	void                 SetDefaultStyleSheet(SfxStyleSheet* pDefSS) { pDefaultStyleSheet = pDefSS; }

	SvxLinkManager*      GetLinkManager()                         { return pLinkManager; }
	void                 SetLinkManager(SvxLinkManager* pLinkMgr) { pLinkManager = pLinkMgr; }

    ::comphelper::IEmbeddedHelper*     GetPersist() const               { return m_pEmbeddedHelper; }
	void				 ClearPersist()                                 { m_pEmbeddedHelper = 0; }
	void				 SetPersist( ::comphelper::IEmbeddedHelper *p ) { m_pEmbeddedHelper = p; }

	// Masseinheit fuer die Zeichenkoordinaten.
	// Default ist 1 logische Einheit = 1/100mm (Unit=MAP_100TH_MM, Fract=(1,1)).
	// Beispiele:
	//   MAP_POINT,    Fraction(72,1)    : 1 log Einh = 72 Point   = 1 Inch
	//   MAP_POINT,    Fraction(1,20)    : 1 log Einh = 1/20 Point = 1 Twip
	//   MAP_TWIP,     Fraction(1,1)     : 1 log Einh = 1 Twip
	//   MAP_100TH_MM, Fraction(1,10)    : 1 log Einh = 1/1000mm
	//   MAP_MM,       Fraction(1000,1)  : 1 log Einh = 1000mm     = 1m
	//   MAP_CM,       Fraction(100,1)   : 1 log Einh = 100cm      = 1m
	//   MAP_CM,       Fraction(100,1)   : 1 log Einh = 100cm      = 1m
	//   MAP_CM,       Fraction(100000,1): 1 log Einh = 100000cm   = 1km
	// (PS: Lichtjahre sind somit also nicht darstellbar).
	// Die Skalierungseinheit wird benoetigt, damit die Engine das Clipboard
	// mit den richtigen Groessen beliefern kann.
	MapUnit          GetScaleUnit() const                       { return eObjUnit; }
	void             SetScaleUnit(MapUnit eMap);
	const Fraction&  GetScaleFraction() const                   { return aObjUnit; }
	void             SetScaleFraction(const Fraction& rFrac);
	// Beides gleichzeitig setzen ist etwas performanter
	void             SetScaleUnit(MapUnit eMap, const Fraction& rFrac);

	// Maximale Groesse z.B. fuer Autogrowing-Texte
	const Size&      GetMaxObjSize() const                      { return aMaxObjSize; }
	void             SetMaxObjSize(const Size& rSiz)            { aMaxObjSize=rSiz; }

	// Damit die View! in der Statuszeile vernuenftige Zahlen anzeigen kann:
	// Default ist mm.
	void             SetUIUnit(FieldUnit eUnit);
	FieldUnit        GetUIUnit() const                          { return eUIUnit; }
	// Der Masstab der Zeichnung. Default 1/1.
	void             SetUIScale(const Fraction& rScale);
	const Fraction&  GetUIScale() const                         { return aUIScale; }
	// Beides gleichzeitig setzen ist etwas performanter
	void             SetUIUnit(FieldUnit eUnit, const Fraction& rScale);

	const Fraction&  GetUIUnitFact() const                      { return aUIUnitFact; }
	const String&    GetUIUnitStr() const                       { return aUIUnitStr; }
	int              GetUIUnitKomma() const                     { return nUIUnitKomma; }
	FASTBOOL         IsUIOnlyKomma() const                      { return bUIOnlyKomma; }

	static void		 TakeUnitStr(FieldUnit eUnit, String& rStr);
	void             TakeMetricStr(long nVal, String& rStr, FASTBOOL bNoUnitChars=FALSE, sal_Int32 nNumDigits = -1) const;
	void             TakeWinkStr(long nWink, String& rStr, FASTBOOL bNoDegChar=FALSE) const;
	void             TakePercentStr(const Fraction& rVal, String& rStr, FASTBOOL bNoPercentChar=FALSE) const;

	// RecalcPageNums wird idR. nur von der Page gerufen.
	FASTBOOL         IsPagNumsDirty() const                     { return bPagNumsDirty; };
	FASTBOOL         IsMPgNumsDirty() const                     { return bMPgNumsDirty; };
	void             RecalcPageNums(FASTBOOL bMaster);
	// Nach dem Insert gehoert die Page dem SdrModel.
	virtual void     InsertPage(SdrPage* pPage, USHORT nPos=0xFFFF);
	virtual void     DeletePage(USHORT nPgNum);
	// Remove bedeutet Eigentumsuebereignung an den Aufrufer (Gegenteil von Insert)
	virtual SdrPage* RemovePage(USHORT nPgNum);
	virtual void     MovePage(USHORT nPgNum, USHORT nNewPos);
	const SdrPage* GetPage(sal_uInt16 nPgNum) const;
	SdrPage* GetPage(sal_uInt16 nPgNum);
	sal_uInt16 GetPageCount() const;
	// #109538#
	virtual void PageListChanged();

	// Masterpages
	virtual void     InsertMasterPage(SdrPage* pPage, USHORT nPos=0xFFFF);
	virtual void     DeleteMasterPage(USHORT nPgNum);
	// Remove bedeutet Eigentumsuebereignung an den Aufrufer (Gegenteil von Insert)
	virtual SdrPage* RemoveMasterPage(USHORT nPgNum);
	virtual void     MoveMasterPage(USHORT nPgNum, USHORT nNewPos);
	const SdrPage* GetMasterPage(sal_uInt16 nPgNum) const;
	SdrPage* GetMasterPage(sal_uInt16 nPgNum);
	sal_uInt16 GetMasterPageCount() const;
	// #109538#
	virtual void MasterPageListChanged();

	// Modified-Flag. Wird automatisch gesetzt, wenn an den Pages oder
	// Zeichenobjekten was geaendert wird. Zuruecksetzen muss man es
	// jedoch selbst (z.B. bei Save() ...).
	sal_Bool IsChanged() const { return mbChanged; }
	virtual void SetChanged(sal_Bool bFlg = sal_True);

	// PageNotValid bedeutet, dass das Model lediglich Objekte traegt die zwar
	// auf einer Page verankert sind, die Page aber nicht gueltig ist. Diese
	// Kennzeichnung wird fuers Clipboard/Drag&Drop benoetigt.
	FASTBOOL        IsPageNotValid() const                     { return bPageNotValid; }
	void            SetPageNotValid(FASTBOOL bJa=TRUE)         { bPageNotValid=bJa; }

	// Schaltet man dieses Flag auf TRUE, so werden Grafikobjekte
	// portabel gespeichert. Es findet dann beim Speichern ggf.
	// eine implizite Wandlung von Metafiles statt.
	// Default=FALSE. Flag ist nicht persistent.
	FASTBOOL        IsSavePortable() const                     { return bSavePortable; }
	void            SetSavePortable(FASTBOOL bJa=TRUE)         { bSavePortable=bJa; }

	// Schaltet man dieses Flag auf TRUE, so werden
	// Pixelobjekte (stark) komprimiert gespeichert.
	// Default=FALSE. Flag ist nicht persistent.
	FASTBOOL        IsSaveCompressed() const                   { return bSaveCompressed; }
	void            SetSaveCompressed(FASTBOOL bJa=TRUE)       { bSaveCompressed=bJa; }

	// Schaltet man dieses Flag auf TRUE, so werden
	// Grafikobjekte mit gesetztem Native-Link
	// native gespeichert.
	// Default=FALSE. Flag ist nicht persistent.
	FASTBOOL        IsSaveNative() const                       { return bSaveNative; }
	void            SetSaveNative(FASTBOOL bJa=TRUE)           { bSaveNative=bJa; }

	// Schaltet man dieses Flag auf TRUE, so werden die Grafiken
	// von Grafikobjekten:
	// - beim Laden eines Dokuments nicht sofort mitgeladen,
	//   sondern erst wenn sie gebraucht (z.B. angezeigt) werden.
	// - ggf. wieder aus dem Speicher geworfen, falls Sie gerade
    //   nicht benoetigt werden.
	// Damit das funktioniert, muss die virtuelle Methode
	// GetDocumentStream() ueberladen werden.
	// Default=FALSE. Flag ist nicht persistent.
	FASTBOOL        IsSwapGraphics() const { return bSwapGraphics; }
	void            SetSwapGraphics(FASTBOOL bJa=TRUE);
	void			SetSwapGraphicsMode(ULONG nMode) { nSwapGraphicsMode = nMode; }
	ULONG			GetSwapGraphicsMode() const { return nSwapGraphicsMode; }

	FASTBOOL        IsSaveOLEPreview() const          { return bSaveOLEPreview; }
	void            SetSaveOLEPreview( FASTBOOL bSet) { bSaveOLEPreview = bSet; }

	// Damit die Bildschirmausgabe von Bitmaps (insbesondere bei gedrehten)
	// etwas schneller wird, werden sie gecachet. Diesen Cache kann man mit
	// diesem Flag ein-/ausschalten. Beim naechsten Paint wird an den Objekten
	// dann ggf. ein Image gemerkt bzw. freigegeben. Wandert ein Bitmapobjekt
	// in's Undo, so wird der Cache fuer dieses Objekt sofort ausgeschaltet
	// (Speicher sparen).
	// Default=Cache eingeschaltet. Flag ist nicht persistent.
	FASTBOOL        IsBitmapCaching() const                     { return !bNoBitmapCaching; }
	void            SetBitmapCaching(FASTBOOL bJa=TRUE)         { bNoBitmapCaching=!bJa; }

	// Defaultmaessig (FALSE) kann man Textrahmen ohne Fuellung durch
	// Mausklick selektieren. Nach Aktivierung dieses Flags trifft man sie
	// nur noch in dem Bereich, wo sich auch tatsaechlich Text befindet.
	FASTBOOL        IsPickThroughTransparentTextFrames() const  { return bTransparentTextFrames; }
	void            SetPickThroughTransparentTextFrames(FASTBOOL bOn) { bTransparentTextFrames=bOn; }

	// Darf denn das Model ueberhaupt veraendert werden?
	// Wird nur von den Possibility-Methoden der View ausgewerdet.
	// Direkte Manipulationen am Model, ... berueksichtigen dieses Flag nicht.
	// Sollte ueberladen werden und entsprechend des ReadOnly-Status des Files
	// TRUE oder FALSE liefern (Methode wird oeffters gerufen, also ein Flag
	// verwenden!).
	virtual FASTBOOL IsReadOnly() const;
	virtual void     SetReadOnly(FASTBOOL bYes);

	// Vermischen zweier SdrModel. Zu beachten sei, dass rSourceModel nicht
	// const ist. Die Pages werden beim einfuegen nicht kopiert, sondern gemoved.
	// rSourceModel ist anschliessend u.U. weitgehend leer.
	// nFirstPageNum,nLastPageNum: Die aus rSourceModel zu uebernehmenden Seiten
	// nDestPos..................: Einfuegeposition
	// bMergeMasterPages.........: TRUE =benoetigte MasterPages werden aus
	//                                   rSourceModel ebenfalls uebernommen
	//                             FALSE=Die MasterPageDescriptoren der Seiten
	//                                   aus rSourceModel werden auf die
	//                                   vorhandenen MasterPages gemappt.
	// bUndo.....................: Fuer das Merging wird eine UndoAction generiert.
	//                             Undo ist nur fuer das ZielModel, nicht fuer
	//                             rSourceModel.
	// bTreadSourceAsConst.......: TRUE=Das SourceModel wird nicht veraendert,.
	//                             d.h die Seiten werden kopiert.
	virtual void Merge(SdrModel& rSourceModel,
			   USHORT nFirstPageNum=0, USHORT nLastPageNum=0xFFFF,
			   USHORT nDestPos=0xFFFF,
			   FASTBOOL bMergeMasterPages=FALSE, FASTBOOL bAllMasterPages=FALSE,
			   FASTBOOL bUndo=TRUE, FASTBOOL bTreadSourceAsConst=FALSE);

	// Ist wie Merge(SourceModel=DestModel,nFirst,nLast,nDest,FALSE,FALSE,bUndo,!bMoveNoCopy);
	void CopyPages(USHORT nFirstPageNum, USHORT nLastPageNum,
				   USHORT nDestPos,
				   FASTBOOL bUndo=TRUE, FASTBOOL bMoveNoCopy=FALSE);

	// Mit BegUndo() / EndUndo() ist es moeglich beliebig viele UndoActions
	// beliebig tief zu klammern. Als Kommentar der
	// UndoAction wird der des ersten BegUndo(String) aller Klammerungen
	// verwendet. Der NotifyUndoActionHdl wird in diesem Fall erst beim letzten
	// EndUndo() gerufen. Bei einer leeren Klammerung wird keine UndoAction
	// generiert.
	// Alle direkten Aktionen am SdrModel erzeugen keine UndoActions, die
	// Aktionen an der SdrView dagegen generieren solche.
	void BegUndo();                       // Undo-Klammerung auf
	void BegUndo(const String& rComment); // Undo-Klammerung auf
	void BegUndo(const String& rComment, const String& rObjDescr, SdrRepeatFunc eFunc=SDRREPFUNC_OBJ_NONE); // Undo-Klammerung auf
	void BegUndo(SdrUndoGroup* pUndoGrp); // Undo-Klammerung auf
	void EndUndo();                       // Undo-Klammerung zu
	void AddUndo(SdrUndoAction* pUndo);
	USHORT GetUndoBracketLevel() const                       { return nUndoLevel; }
	const SdrUndoGroup* GetAktUndoGroup() const              { return pAktUndoGroup; }
	// nur nach dem 1. BegUndo oder vor dem letzten EndUndo:
	void SetUndoComment(const String& rComment);
	void SetUndoComment(const String& rComment, const String& rObjDescr);

	// Das Undo-Managment findet nur statt, wenn kein NotifyUndoAction-Handler
	// gesetzt ist.
	// Default ist 16. Minimaler MaxUndoActionCount ist 1!
	void  SetMaxUndoActionCount(ULONG nAnz);
	ULONG GetMaxUndoActionCount() const { return nMaxUndoCount; }
	void  ClearUndoBuffer();
	// UndoAction(0) ist die aktuelle (also die zuletzt eingegangene)
	ULONG GetUndoActionCount() const                      { return pUndoStack!=NULL ? pUndoStack->Count() : 0; }
	const SfxUndoAction* GetUndoAction(ULONG nNum) const  { return (SfxUndoAction*)(pUndoStack!=NULL ? pUndoStack->GetObject(nNum) : NULL); }
	// RedoAction(0) ist die aktuelle (also die des letzten Undo)
	ULONG GetRedoActionCount() const                      { return pRedoStack!=NULL ? pRedoStack->Count() : 0; }
	const SfxUndoAction* GetRedoAction(ULONG nNum) const  { return (SfxUndoAction*)(pRedoStack!=NULL ? pRedoStack->GetObject(nNum) : NULL); }

	FASTBOOL Undo();
	FASTBOOL Redo();
	FASTBOOL Repeat(SfxRepeatTarget&);

	// Hier kann die Applikation einen Handler setzen, der die auflaufenden
	// UndoActions einsammelt. Der Handler hat folgendes Aussehen:
	//   void __EXPORT NotifyUndoActionHdl(SfxUndoAction* pUndoAction);
	// Beim Aufruf des Handlers findet eine Eigentumsuebereignung statt; die
	// UndoAction gehoert somit dem Handler, nicht mehr dem SdrModel.
	void        SetNotifyUndoActionHdl(const Link& rLink)    { aUndoLink=rLink; }
	const Link& GetNotifyUndoActionHdl() const               { return aUndoLink; }

	/** application can set it's own undo manager, BegUndo, EndUndo and AddUndoAction
		calls are routet to this interface if given */
	void SetSdrUndoManager( SfxUndoManager*	pUndoManager );

	/** applications can set their own undo factory to overide creation of
		undo actions. The SdrModel will become owner of the given SdrUndoFactory
		and delete it upon its destruction. */
	void SetSdrUndoFactory( SdrUndoFactory* pUndoFactory );

	/** returns the models undo factory. This must be used to create
		undo actions for this model. */
	SdrUndoFactory& GetSdrUndoFactory() const;

	// Hier kann man einen Handler setzen der beim Streamen mehrfach gerufen
	// wird und ungefaehre Auskunft ueber den Fortschreitungszustand der
	// Funktion gibt. Der Handler muss folgendes Aussehen haben:
	//   void __EXPORT class::IOProgressHdl(const USHORT& nPercent);
	// Der erste Aufruf des Handlers erfolgt grundsaetzlich mit 0, der letzte
	// mit 100. Dazwischen erfolgen maximal 99 Aufrufe mit Werten 1...99.
	// Man kann also durchaus bei 0 den Progressbar Initiallisieren und bei
	// 100 wieder schliessen. Zu beachten sei, dass der Handler auch gerufen
	// wird, wenn die App Draw-Daten im officeweiten Draw-Exchange-Format
	// bereitstellt, denn dies geschieht durch streamen in einen MemoryStream.
	void        SetIOProgressHdl(const Link& rLink)          { aIOProgressLink=rLink; }
	const Link& GetIOProgressHdl() const                     { return aIOProgressLink; }

	// Zugriffsmethoden fuer Paletten, Listen und Tabellen
	void            SetColorTable(XColorTable* pTable)       { pColorTable=pTable; }
	XColorTable*    GetColorTable() const                    { return pColorTable; }
	void            SetDashList(XDashList* pList)            { pDashList=pList; }
	XDashList*      GetDashList() const                      { return pDashList; }
	void            SetLineEndList(XLineEndList* pList)      { pLineEndList=pList; }
	XLineEndList*   GetLineEndList() const                   { return pLineEndList; }
	void            SetHatchList(XHatchList* pList)          { pHatchList=pList; }
	XHatchList*     GetHatchList() const                     { return pHatchList; }
	void            SetGradientList(XGradientList* pList)    { pGradientList=pList; }
	XGradientList*  GetGradientList() const                  { return pGradientList; }
	void            SetBitmapList(XBitmapList* pList)        { pBitmapList=pList; }
	XBitmapList*    GetBitmapList() const                    { return pBitmapList; }

	// Der StyleSheetPool wird der DrawingEngine nur bekanntgemacht.
	// Zu loeschen hat ihn schliesslich der, der ihn auch konstruiert hat.
	SfxStyleSheetBasePool* GetStyleSheetPool() const         { return mxStyleSheetPool.get(); }
	void SetStyleSheetPool(SfxStyleSheetBasePool* pPool)     { mxStyleSheetPool=pPool; }

	// Diese Methode fuert einen Konsistenzcheck auf die Struktur des Models
	// durch. Geprueft wird insbesondere die Verkettung von Verschachtelten
	// Gruppenobjekten, aber auch Stati wie bInserted sowie Model* und Page*
	// der Objects, SubLists und Pages. Bei korrekter Struktur liefert die
	// Methode TRUE, andernfalls FALSE.
	// Dieser Check steht nur zur Verfuegung, wenn die Engine mit DBG_UTIL
	// uebersetzt wurde. Andernfalls liefert die Methode immer TRUE. (ni)
	FASTBOOL CheckConsistence() const;

	void 	SetStarDrawPreviewMode(BOOL bPreview);
	BOOL 	IsStarDrawPreviewMode() { return bStarDrawPreviewMode; }

	SotStorage*	GetModelStorage() const { return pModelStorage; }
	void		SetModelStorage( SotStorage* pStor ) { pModelStorage = pStor; }

	::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > getUnoModel();
	void setUnoModel( ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > xModel );

	// these functions are used by the api to disable repaints during a
	// set of api calls.
	BOOL isLocked() const { return (BOOL)mbModelLocked; }
	void setLock( BOOL bLock );

	void			SetForbiddenCharsTable( vos::ORef<SvxForbiddenCharactersTable> xForbiddenChars );
	vos::ORef<SvxForbiddenCharactersTable>	GetForbiddenCharsTable() const;

	void SetCharCompressType( UINT16 nType );
	UINT16 GetCharCompressType() const { return mnCharCompressType; }

	void SetKernAsianPunctuation( sal_Bool bEnabled );
	sal_Bool IsKernAsianPunctuation() const { return (sal_Bool)mbKernAsianPunctuation; }

    void SetAddExtLeading( sal_Bool bEnabled );
    sal_Bool IsAddExtLeading() const { return (sal_Bool)mbAddExtLeading; }

	void ReformatAllTextObjects();

	FASTBOOL HasTransparentObjects( BOOL bCheckForAlphaChannel = FALSE ) const;

	SdrOutliner* createOutliner( USHORT nOutlinerMode );
	void disposeOutliner( SdrOutliner* pOutliner );

	sal_Bool IsWriter() const { return !bMyPool; }

	/** returns the numbering type that is used to format page fields in drawing shapes */
	virtual SvxNumType GetPageNumType() const;

	/** copies the items from the source set to the destination set. Both sets must have
		same ranges but can have different pools. If pNewModel is optional. If it is null,
		this model is used. */

	void MigrateItemSet( const SfxItemSet* pSourceSet, SfxItemSet* pDestSet, SdrModel* pNewModel );

	bool IsInDestruction() const;

    static const ::com::sun::star::uno::Sequence< sal_Int8 >& getUnoTunnelImplementationId();

	/** enables (true) or disables (false) recording of undo actions
		If undo actions are added while undo is disabled, they are deleted.
		Disabling undo does not clear the current undo buffer! */
	void EnableUndo( bool bEnable );

	/** returns true if undo is currently enabled
		This returns false if undo was disabled using EnableUndo( false ) and
		also during the runtime of the Undo() and Redo() methods. */
	bool IsUndoEnabled() const;

};

typedef tools::WeakReference< SdrModel > SdrModelWeakRef;

////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //_SVDMODEL_HXX

/* /////////////////////////////////////////////////////////////////////////////////////////////////
            +-----------+
			| SdrModel  |
            +--+------+-+
               |      +-----------+
          +----+-----+            |
		  |   ...    |            |
     +----+---+ +----+---+  +-----+--------+
	 |SdrPage | |SdrPage |  |SdrLayerAdmin |
     +---+----+ +-+--+--++  +---+-------+--+
         |        |  |  |       |       +-------------------+
    +----+----+           +-----+-----+             +-------+-------+
	|   ...   |           |    ...    |             |      ...      |
+---+---+ +---+---+  +----+----+ +----+----+  +-----+------+ +------+-----+
|SdrObj | |SdrObj |  |SdrLayer | |SdrLayer |  |SdrLayerSet | |SdrLayerSet |
+-------+ +-------+  +---------+ +---------+  +------------+ +------------+
Die Klasse SdrModel ist der Kopf des Datenmodells der StarView Drawing-Engine.

///////////////////////////////////////////////////////////////////////////////////////////////// */

