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

#ifndef _SVDXCGV_HXX
#define _SVDXCGV_HXX

#include <svx/svdedxv.hxx>

#ifndef _GDIMTF_HXX //autogen
#include <vcl/gdimtf.hxx>
#endif
#include "svx/svxdllapi.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@@@@ @@   @@  @@@@  @@  @@  @@@@  @@  @@  @@@@  @@@@@  @@ @@ @@ @@@@@ @@   @@
//  @@    @@@ @@@ @@  @@ @@  @@ @@  @@ @@@ @@ @@  @@ @@     @@ @@ @@ @@    @@   @@
//  @@     @@@@@  @@     @@  @@ @@  @@ @@@@@@ @@     @@     @@ @@ @@ @@    @@ @ @@
//  @@@@    @@@   @@     @@@@@@ @@@@@@ @@@@@@ @@ @@@ @@@@   @@@@@ @@ @@@@  @@@@@@@
//  @@     @@@@@  @@     @@  @@ @@  @@ @@ @@@ @@  @@ @@      @@@  @@ @@    @@@@@@@
//  @@    @@@ @@@ @@  @@ @@  @@ @@  @@ @@  @@ @@  @@ @@      @@@  @@ @@    @@@ @@@
//  @@@@@ @@   @@  @@@@  @@  @@ @@  @@ @@  @@  @@@@@ @@@@@    @   @@ @@@@@ @@   @@
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class SVX_DLLPUBLIC SdrExchangeView: public SdrObjEditView
{
	friend class SdrPageView;

protected:

    void                ImpGetPasteObjList(Point& rPos, SdrObjList*& rpLst);
	void                ImpPasteObject(SdrObject* pObj, SdrObjList& rLst, const Point& rCenter, const Size& rSiz, const MapMode& rMap, UINT32 nOptions);
	BOOL                ImpGetPasteLayer(const SdrObjList* pObjList, SdrLayerID& rLayer) const;
	Point               GetPastePos(SdrObjList* pLst, OutputDevice* pOut=NULL);

	// liefert True, wenn rPt geaendert wurde
	BOOL                ImpLimitToWorkArea(Point& rPt) const;

protected:
	// #i71538# make constructors of SdrView sub-components protected to avoid incomplete incarnations which may get casted to SdrView
    SdrExchangeView(SdrModel* pModel1, OutputDevice* pOut = 0L);

public:
	// Alle markierten Objekte auf dem angegebenen OutputDevice ausgeben.
	virtual void        DrawMarkedObj(OutputDevice& rOut) const;

	// Z.B. fuer's Clipboard, Drag&Drop, ...
	// Alle markierten Objekte in ein Metafile stecken. Z.Zt. noch etwas
	// buggee (Offset..., Fremdgrafikobjekte (SdrGrafObj), Virtuelle
	// Objektkopien (SdrVirtObj) mit Ankerpos<>(0,0)).
	virtual GDIMetaFile GetMarkedObjMetaFile(BOOL bNoVDevIfOneMtfMarked=FALSE) const;

	// Alle markierten Objekte auf eine Bitmap malen. Diese hat die Farbtiefe
	// und Aufloesung des Bildschirms.
	virtual Bitmap      GetMarkedObjBitmap(BOOL bNoVDevIfOneBmpMarked=FALSE) const;

	// Alle markierten Objekte in ein neues Model kopieren. Dieses neue Model
	// hat dann genau eine Page. Das Flag PageNotValid an diesem Model ist
	// gesetzt. Daran ist zu erkennen, dass nur die Objekte der Page Gueltikeit
	// haben, die Page sebst jedoch nicht (Seitengroesse, Raender). Das neue
	// Model wird auf dem Heap erzeugt und wird an den Aufrufer dieser Methode
	// uebergeben. Dieser hat es dann spaeter zu entsorgen.
	// Beim einfuegen der markierten Objekte in die eine Page des neuen Model
	// findet ein Merging der seitenlokalen Layer statt. Sollte kein Platz mehr
	// fuer weitere seitenlokale Layer sein, wird den entsprechenden Objekten
	// der Default-Layer zugewiesen (Layer 0, (dokumentglobaler Standardlayer).
	virtual SdrModel*   GetMarkedObjModel() const;

	GDIMetaFile     GetAllMarkedMetaFile(BOOL bNoVDevIfOneMtfMarked=FALSE) const { return GetMarkedObjMetaFile(bNoVDevIfOneMtfMarked); }
	Bitmap          GetAllMarkedBitmap(BOOL bNoVDevIfOneBmpMarked=FALSE) const { return GetMarkedObjBitmap(bNoVDevIfOneBmpMarked); }
	Graphic         GetAllMarkedGraphic() const;
	SdrModel*       GetAllMarkedModel() const { return GetMarkedObjModel(); }

    /** Generate a Graphic for the given draw object in the given model

		@param pModel
        Must not be NULL. Denotes the draw model the object is a part
        of.

        @param pObj
        The object (can also be a group object) to retrieve a Graphic
        for. Must not be NULL.

        @return a graphical representation of the given object, as it
        appears on screen (e.g. with rotation, if any, applied).
     */
    static Graphic  GetObjGraphic( const SdrModel* pModel, const SdrObject* pObj );

	// Bestimmung des View-Mittelpunktes, z.B. zum Pasten
	Point           GetViewCenter(const OutputDevice* pOut=NULL) const;

	// Bei allen Paste-Methoden werden die neuen Draw-Objekte markiert.
	// Wird der Parameter bAddMark auf TRUE gesetzt, so werden die neuen
	// DrawObjekte zu einer bereits bestehenden Selektion "hinzumarkiert".
	// Dieser Fall ist fuer Drag&Drop mit mehreren Items gedacht.
	// Die Methoden mit Point-Parameter fuegen neue Objekte zentriert an
	// dieser Position ein, die anderen zentriert am 1.OutputDevice der View.
	// Ist der Parameter pPg gesetzt, werden die Objekte and dieser Seite
	// eingefuegt. Die Positionierung (rPos bzw. Zentrierung) bezieht sich
	// dann nichtmehr auf die View sondern auf die Page.
	// Hinweis: SdrObjList ist Basisklasse von SdrPage.
	// Die Methoden liefern TRUE, wenn die Objekte erfolgreich erzeugt und
	// eingefuegt wurden. Bei pLst=FALSE und kein TextEdit aktiv kann man
	// sich dann auch darauf verlassen, dass diese an der View markiert sind.
	// Andernfalls erfolgt die Markierung nur, wenn pLst z.Zt. auch an der
	// View angezeigt wird.
	// Gueltige Werte fuer nOptions sind SDRINSERT_DONTMARK und
	// SDRINSERT_ADDMARK (siehe svdedtv.hxx).
	BOOL            Paste(const GDIMetaFile& rMtf, SdrObjList* pLst=NULL, OutputDevice* pOut=NULL, UINT32 nOptions=0) { return Paste(rMtf,GetPastePos(pLst,pOut),pLst,nOptions); }
	BOOL            Paste(const GDIMetaFile& rMtf, const Point& rPos, SdrObjList* pLst=NULL, UINT32 nOptions=0);
	BOOL            Paste(const Bitmap& rBmp, SdrObjList* pLst=NULL, OutputDevice* pOut=NULL, UINT32 nOptions=0) { return Paste(rBmp,GetPastePos(pLst,pOut),pLst,nOptions); }
	BOOL            Paste(const Bitmap& rBmp, const Point& rPos, SdrObjList* pLst=NULL, UINT32 nOptions=0);
	BOOL            Paste(const SdrModel& rMod, SdrObjList* pLst=NULL, OutputDevice* pOut=NULL, UINT32 nOptions=0) { return Paste(rMod,GetPastePos(pLst,pOut),pLst,nOptions); }
	virtual BOOL    Paste(const SdrModel& rMod, const Point& rPos, SdrObjList* pLst=NULL, UINT32 nOptions=0);
	BOOL            Paste(const String& rStr, SdrObjList* pLst=NULL, OutputDevice* pOut=NULL, UINT32 nOptions=0) { return Paste(rStr,GetPastePos(pLst,pOut),pLst,nOptions); }
	BOOL            Paste(const String& rStr, const Point& rPos, SdrObjList* pLst=NULL, UINT32 nOptions=0);
	// der USHORT eFormat nimmt Werte des enum EETextFormat entgegen
    BOOL            Paste(SvStream& rInput, const String& rBaseURL, USHORT eFormat, SdrObjList* pLst=NULL, OutputDevice* pOut=NULL, UINT32 nOptions=0) { return Paste(rInput,rBaseURL,eFormat,GetPastePos(pLst,pOut),pLst,nOptions); }
    BOOL            Paste(SvStream& rInput, const String& rBaseURL, USHORT eFormat, const Point& rPos, SdrObjList* pLst=NULL, UINT32 nOptions=0);

	// Feststellen, ob ein bestimmtes Format ueber Drag&Drop bzw. ueber's
	// Clipboard angenommen werden kann.
	BOOL            IsExchangeFormatSupported(ULONG nFormat) const;

	BOOL            Cut( ULONG nFormat = SDR_ANYFORMAT );
	void            CutMarked( ULONG nFormat=SDR_ANYFORMAT );

	BOOL            Yank( ULONG nFormat = SDR_ANYFORMAT );
	void            YankMarked( ULONG nFormat=SDR_ANYFORMAT );

    BOOL            Paste( Window* pWin = NULL, ULONG nFormat = SDR_ANYFORMAT );
	BOOL            PasteClipboard( OutputDevice* pOut = NULL, ULONG nFormat = SDR_ANYFORMAT, UINT32 nOptions = 0 );
};

#endif //_SVDXCGV_HXX
