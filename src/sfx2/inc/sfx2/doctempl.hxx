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
#ifndef _SFXDOCTEMPL_HXX
#define _SFXDOCTEMPL_HXX

#include "sal/config.h"
#include "sfx2/dllapi.h"
#include "sal/types.h"
#include <tools/ref.hxx>
#include <tools/string.hxx>

// CLASS -----------------------------------------------------------------

class SfxObjectShell;

#ifndef SFX_DECL_OBJECTSHELL_DEFINED
#define SFX_DECL_OBJECTSHELL_DEFINED
SV_DECL_REF(SfxObjectShell)
#endif

class SfxDocTemplate_Impl;

#ifndef SFX_DECL_DOCTEMPLATES_DEFINED
#define SFX_DECL_DOCTEMPLATES_DEFINED
SV_DECL_REF(SfxDocTemplate_Impl)
#endif

// class SfxDocumentTemplates --------------------------------------------

class SFX2_DLLPUBLIC SfxDocumentTemplates
{
private:
	SfxDocTemplate_ImplRef	pImp;

	SAL_DLLPRIVATE BOOL CopyOrMove( USHORT nTargetRegion, USHORT nTargetIdx,
									USHORT nSourceRegion, USHORT nSourceIdx, BOOL bMove );
public:
						SfxDocumentTemplates();
						SfxDocumentTemplates(const SfxDocumentTemplates &);
						~SfxDocumentTemplates();

	BOOL				IsConstructed() { return pImp != NULL; }
	void				Construct();

	static BOOL			SaveDir( /*SfxTemplateDir &rEntry */ ) ;
	const SfxDocumentTemplates &operator=(const SfxDocumentTemplates &);

	BOOL				Rescan( );		// Aktualisieren
    void                ReInitFromComponent();

	BOOL                IsRegionLoaded( USHORT nIdx ) const;
	USHORT				GetRegionCount() const;
	const String&		GetRegionName(USHORT nIdx) const;					//dv!
	String 				GetFullRegionName(USHORT nIdx) const;
	USHORT				GetRegionNo( const String &rRegionName ) const;

	USHORT				GetCount(USHORT nRegion) const;
	USHORT				GetCount( const String &rName) const;
	const String&		GetName(USHORT nRegion, USHORT nIdx) const;			//dv!
	String				GetFileName(USHORT nRegion, USHORT nIdx) const;
	String				GetPath(USHORT nRegion, USHORT nIdx) const;

	String				GetDefaultTemplatePath(const String &rLongName);

	// Pfad zur Vorlage geben lassen; logischer Name muss angegeben
	// werden, damit beim Ueberschreiben einer Vorlage der
	// richtige Dateiname gefunden werden kann
	String				GetTemplatePath(USHORT nRegion, const String &rLongName) const;

	// Allows to retrieve the target template URL from the UCB
	::rtl::OUString		GetTemplateTargetURLFromComponent( const ::rtl::OUString& aGroupName,
														 const ::rtl::OUString& aTitle );

	// Speichern als Vorlage hat geklappt -> Aktualisieren
	void			NewTemplate(USHORT nRegion,
								const String &rLongName,
								const String &rFileName);

	BOOL			Copy(USHORT nTargetRegion,
						 USHORT nTargetIdx,
						 USHORT nSourceRegion,
						 USHORT nSourceIdx);
	BOOL			Move(USHORT nTargetRegion,
						 USHORT nTargetIdx,
						 USHORT nSourceRegion,
						 USHORT nSourceIdx);
	BOOL			Delete(USHORT nRegion, USHORT nIdx);
	BOOL			InsertDir(const String &rText, USHORT nRegion);
	BOOL			SetName(const String &rName, USHORT nRegion, USHORT nIdx);

	BOOL			CopyTo(USHORT nRegion, USHORT nIdx, const String &rName) const;
	BOOL			CopyFrom(USHORT nRegion, USHORT nIdx, String &rName);

	SfxObjectShellRef CreateObjectShell(USHORT nRegion, USHORT nIdx);
	BOOL 			DeleteObjectShell(USHORT, USHORT);

	BOOL 			GetFull( const String& rRegion, const String& rName, String& rPath );
	BOOL 			GetLogicNames( const String& rPath, String& rRegion, String& rName ) const;

	/** updates the configuration where the document templates structure is stored.

		<p>The info about the document templates (which files, which groups etc.) is stored in the
		configuration. This means that just by copying files into OOo's template directories, this
		change is not reflected in the SfxDocumentTemplates - 'cause the configuration is not synchronous with
		the file system. This can be enforced with this method.</p>

	@param _bSmart
		The update of the configuration is rather expensive - nothing you want to do regulary if you don't really
		need it. So you have the possibility to do a smart update - it first checks if the update if necessary.
		In case the update is needed, the additional check made it somewhat more expensive. In case it's not
		necessary (which should be the usual case), the check alone is (much) less expensive than the real update.
		<br/>
		So set <arg>_bSmart</arg> to <TRUE/> to do a check for necessity first.
	*/
	void			Update( sal_Bool _bSmart = sal_True );

	// allows to detect whether it is allowed to delete ( at least partially )
	// a group or a template, or to edit a template
	sal_Bool		HasUserContents( sal_uInt16 nRegion, sal_uInt16 nIdx ) const;
};

#endif // #ifndef _SFXDOCTEMPL_HXX


