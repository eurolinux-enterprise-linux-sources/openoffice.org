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

#ifndef _CTRLTOOL_HXX
#define _CTRLTOOL_HXX

#include "svtools/svtdllapi.h"

#ifndef _SAL_TYPES_H
#include <sal/types.h>
#endif
#include <tools/list.hxx>
#ifndef _METRIC_HXX
#include <vcl/metric.hxx>
#endif

class ImplFontListNameInfo;
class OutputDevice;

/*************************************************************************

Beschreibung
============

class FontList

Diese Klasse verwaltet alle Fonts, die auf einem oder zwei Ausgabegeraeten
dargestellt werden koennen. Zusaetzlich bietet die Klasse Methoden an, um
aus Fett und Kursiv den StyleName zu generieren oder aus einem Stylename
die fehlenden Attribute. Zusaetzlich kann diese Klasse syntetisch nachgebildete
Fonts verarbeiten. Diese Klasse kann mit verschiedenen Standard-Controls und
Standard-Menus zusammenarbeiten.

Querverweise

class FontNameBox, class FontStyleBox, class FontSizeBox,
class FontNameMenu, class FontStyleMenu, class FontSizeMenu

--------------------------------------------------------------------------

FontList::FontList( OutputDevice* pDevice, OutputDevice* pDevice2 = NULL,
					BOOL bAll = TRUE );

Konstruktor der Klasse FontList. Vom uebergebenen OutputDevice werden die
entsprechenden Fonts abgefragt. Das OutputDevice muss solange existieren,
wie auch die Klasse FontList existiert. Optional kann noch ein 2tes
Ausgabedevice uebergeben werden, damit man zum Beispiel die Fonts von
einem Drucker und dem Bildschirm zusammen in einer FontListe verwalten kann
und somit auch den FontListen und FontMenus die Fonts von beiden OutputDevices
zu uebergeben. Auch das pDevice2 muss solange existieren, wie die Klasse
FontList existiert.

Das OutputDevice, welches als erstes uebergeben wird, sollte das bevorzugte
sein. Dies sollte im normalfall der Drucker sein. Denn wenn 2 verschiede
Device-Schriften (eine fuer Drucker und eine fuer den Bildschirm) vorhanden
sind, wird die vom uebergebenen Device "pDevice" bevorzugt.

Mit dem dritten Parameter kann man angeben, ob nur skalierbare Schriften
abgefragt werden sollen oder alle. Wenn TRUE uebergeben wird, werden auch
Bitmap-Schriften mit abgefragt. Bei FALSE werden Vector-Schriften und
scalierbare Schriften abgefragt.

--------------------------------------------------------------------------

String FontList::GetStyleName( const FontInfo& rInfo ) const;

Diese Methode gibt den StyleName von einer FontInfo zurueck. Falls kein
StyleName gesetzt ist, wird aus den gesetzten Attributen ein entsprechender
Name generiert, der dem Anwender praesentiert werden kann.

--------------------------------------------------------------------------

XubString FontList::GetFontMapText( const FontInfo& rInfo ) const;

Diese Methode gibt einen Matchstring zurueck, der dem Anwender
anzeigen soll, welche Probleme es mit diesem Font geben kann.

--------------------------------------------------------------------------

FontInfo FontList::Get( const String& rName, const String& rStyleName ) const;

Diese Methode sucht aus dem uebergebenen Namen und dem uebergebenen StyleName
die entsprechende FontInfo-Struktur raus. Der Stylename kann in dieser
Methode auch ein syntetischer sein. In diesem Fall werden die entsprechenden
Werte in der FontInfo-Struktur entsprechend gesetzt. Wenn ein StyleName
uebergeben wird, kann jedoch eine FontInfo-Struktur ohne Stylename
zurueckgegeben werden. Um den StyleName dem Anwender zu repraesentieren,
muss GetStyleName() mit dieser FontInfo-Struktur aufgerufen werden.

Querverweise

FontList::GetStyleName()

--------------------------------------------------------------------------

FontInfo FontList::Get( const String& rName, FontWeight eWeight,
						FontItalic eItalic ) const;

Diese Methode sucht aus dem uebergebenen Namen und den uebergebenen Styles
die entsprechende FontInfo-Struktur raus. Diese Methode kann auch eine
FontInfo-Struktur ohne Stylename zurueckgegeben. Um den StyleName dem
Anwender zu repraesentieren, muss GetStyleName() mit dieser FontInfo-Struktur
aufgerufen werden.

Querverweise

FontList::GetStyleName()

--------------------------------------------------------------------------

const long* FontList::GetSizeAry( const FontInfo& rInfo ) const;

Diese Methode liefert zum uebergebenen Font die vorhandenen Groessen.
Falls es sich dabei um einen skalierbaren Font handelt, werden Standard-
Groessen zurueckgegeben. Das Array enthaelt die Hoehen des Fonts in 10tel
Point. Der letzte Wert des Array ist 0. Das Array, was zurueckgegeben wird,
wird von der FontList wieder zerstoert. Nach dem Aufruf der naechsten Methode
von der FontList, sollte deshalb das Array nicht mehr referenziert werden.

*************************************************************************/

// ------------
// - FontList -
// ------------

#define FONTLIST_FONTINFO_NOTFOUND	((USHORT)0xFFFF)

#define FONTLIST_FONTNAMETYPE_PRINTER			((USHORT)0x0001)
#define FONTLIST_FONTNAMETYPE_SCREEN			((USHORT)0x0002)
#define FONTLIST_FONTNAMETYPE_SCALABLE			((USHORT)0x0004)

class SVT_DLLPUBLIC FontList : private List
{
private:
	XubString				maMapBoth;
	XubString				maMapPrinterOnly;
	XubString				maMapScreenOnly;
	XubString				maMapSizeNotAvailable;
	XubString				maMapStyleNotAvailable;
	XubString				maMapNotAvailable;
	XubString				maLight;
	XubString				maLightItalic;
	XubString				maNormal;
	XubString				maNormalItalic;
	XubString				maBold;
	XubString				maBoldItalic;
	XubString				maBlack;
	XubString				maBlackItalic;
	long*					mpSizeAry;
	OutputDevice*			mpDev;
	OutputDevice*			mpDev2;

#ifdef CTRLTOOL_CXX
	SVT_DLLPRIVATE ImplFontListNameInfo*	ImplFind( const XubString& rSearchName, ULONG* pIndex ) const;
	SVT_DLLPRIVATE ImplFontListNameInfo*	ImplFindByName( const XubString& rStr ) const;
	SVT_DLLPRIVATE void					ImplInsertFonts( OutputDevice* pDev, BOOL bAll,
											 BOOL bInsertData );
#endif

public:
							FontList( OutputDevice* pDevice,
									  OutputDevice* pDevice2 = NULL,
									  BOOL bAll = TRUE );
							~FontList();

    FontList*               Clone() const;
    
    OutputDevice*           GetDevice() const { return mpDev; }
	OutputDevice*			GetDevice2() const { return mpDev2; }
	XubString				GetFontMapText( const FontInfo& rInfo ) const;
	USHORT					GetFontNameType( const XubString& rFontName ) const;

	const XubString&		GetNormalStr() const { return maNormal; }
	const XubString&		GetItalicStr() const { return maNormalItalic; }
	const XubString&		GetBoldStr() const { return maBold; }
	const XubString&		GetBoldItalicStr() const { return maBoldItalic; }
	const XubString&		GetStyleName( FontWeight eWeight, FontItalic eItalic ) const;
	XubString				GetStyleName( const FontInfo& rInfo ) const;

	FontInfo				Get( const XubString& rName,
								 const XubString& rStyleName ) const;
	FontInfo				Get( const XubString& rName,
								 FontWeight eWeight,
								 FontItalic eItalic ) const;

	BOOL					IsAvailable( const XubString& rName ) const;
	USHORT					GetFontNameCount() const
								{ return (USHORT)List::Count(); }
	const FontInfo& 		GetFontName( USHORT nFont ) const;
	USHORT					GetFontNameType( USHORT nFont ) const;
	sal_Handle				GetFirstFontInfo( const XubString& rName ) const;
	sal_Handle				GetNextFontInfo( sal_Handle hFontInfo ) const;
	const FontInfo& 		GetFontInfo( sal_Handle hFontInfo ) const;

	const long* 			GetSizeAry( const FontInfo& rInfo ) const;
	static const long*		GetStdSizeAry();

private:
							FontList( const FontList& );
	FontList&				operator =( const FontList& );
};


// -----------------
// - FontSizeNames -
// -----------------

class SVT_DLLPUBLIC FontSizeNames
{
private:
	struct ImplFSNameItem*	mpArray;
	ULONG					mnElem;

public:
							FontSizeNames( LanguageType eLanguage /* = LANGUAGE_DONTKNOW */ );

	ULONG					Count() const { return mnElem; }
	BOOL					IsEmpty() const { return !mnElem; }

	long					Name2Size( const String& ) const;
	String					Size2Name( long ) const;

	String					GetIndexName( ULONG nIndex ) const;
	long					GetIndexSize( ULONG nIndex ) const;
};

#endif	// _CTRLTOOL_HXX
