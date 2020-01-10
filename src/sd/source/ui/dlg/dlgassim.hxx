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

#include <svtools/dateitem.hxx>
#include "drawdoc.hxx"
#include <svx/svdotext.hxx>
#include <svtools/svtreebx.hxx>

#include "sdresid.hxx"

class SdPageListControl : public SvTreeListBox
{
private:
	SvLBoxButtonData* 	m_pCheckButton;

	SvLBoxEntry* InsertPage( const String& rPageName );
	void InsertTitle( SvLBoxEntry* pEntry, const String& rTitle );

public:
	SdPageListControl( Window* pParent, const ResId& rResId );
	~SdPageListControl();

	void Fill( SdDrawDocument* pDoc );
	void Clear();

	USHORT GetSelectedPage();
	BOOL IsPageChecked( USHORT nPage );

	DECL_LINK( CheckButtonClickHdl, SvLBoxButtonData * );

	virtual void DataChanged( const DataChangedEvent& rDCEvt );

};

class TemplateCacheInfo
{
private:
	DateTime	m_aDateTime;
	String		m_aFile;
	BOOL		m_bImpress;
	BOOL		m_bValid;
	BOOL		m_bModified;

public:
	TemplateCacheInfo();
	TemplateCacheInfo( const String& rFile, const DateTime& rDateTime, BOOL bImpress );

	BOOL IsValid() const { return m_bValid; }
	void SetValid( BOOL bValid = TRUE ) { m_bValid = bValid; }

	BOOL IsImpress() const { return m_bImpress; }
	void SetImpress( BOOL bImpress = TRUE ) { m_bImpress = bImpress; }

	const String& GetFile() const { return m_aFile; }
	void SetFile( const String& rFile ) { m_aFile = rFile; }

	const DateTime& GetDateTime() const { return m_aDateTime; }
	void SetDateTime( const DateTime& rDateTime ) { m_aDateTime = rDateTime; }

	BOOL IsModified() const { return m_bModified; }
	void SetModified( BOOL bModified = TRUE ) { m_bModified = bModified; }

	friend SvStream& operator >> (SvStream& rIn, TemplateCacheInfo& rInfo);
	friend SvStream& operator << (SvStream& rOut, const TemplateCacheInfo& rInfo);
};

DECLARE_LIST( TemplateCacheInfoList, TemplateCacheInfo * )

class TemplateCacheDirEntry
{
public:
	String						m_aPath;
	TemplateCacheInfoList		m_aFiles;
};

DECLARE_LIST( TemplateCacheDirEntryList, TemplateCacheDirEntry * )

class TemplateCache
{
private:
	TemplateCacheDirEntryList	m_aDirs;
	TemplateCacheDirEntry* GetDirEntry( const String& rPath );
	void Clear();
public:
	TemplateCache();
	~TemplateCache();

	void Load();
	void Save();

	TemplateCacheInfo* GetFileInfo( const String& rPath );
	TemplateCacheInfo* AddFileInfo( const String& rPath );

	BOOL ClearInvalidEntrys();
};
