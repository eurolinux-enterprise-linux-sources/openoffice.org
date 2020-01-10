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

#ifndef _SVTOOLS_INETTBC_HXX
#define _SVTOOLS_INETTBC_HXX

// includes *****************************************************************

#include "svtools/svtdllapi.h"
#include <tools/string.hxx>
#include <tools/urlobj.hxx>

#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif

class IUrlFilter;
class SvtMatchContext_Impl;
class SvtURLBox_Impl;
class SVT_DLLPUBLIC SvtURLBox : public ComboBox
{
friend class SvtMatchContext_Impl;
friend class SvtURLBox_Impl;
	Link							aOpenHdl;
	String							aBaseURL;
	String							aPlaceHolder;
	SvtMatchContext_Impl*			pCtx;
	SvtURLBox_Impl*					pImp;
	INetProtocol					eSmartProtocol;
    BOOL            				bAutoCompleteMode   : 1;
    BOOL							bOnlyDirectories    : 1;
    BOOL							bModified           : 1;
    BOOL                            bTryAutoComplete    : 1;
    BOOL							bCtrlClick          : 1;
    BOOL							bHistoryDisabled    : 1;
    BOOL							bNoSelection        : 1;
    BOOL                            bIsAutoCompleteEnabled : 1;

	SVT_DLLPRIVATE BOOL            				ProcessKey( const KeyCode& rCode );
	SVT_DLLPRIVATE void            				TryAutoComplete( BOOL bForce );
    SVT_DLLPRIVATE void                            UpdatePicklistForSmartProtocol_Impl();
    DECL_DLLPRIVATE_LINK(                      AutoCompleteHdl_Impl, void* );
    using Window::ImplInit;
    SVT_DLLPRIVATE void                            ImplInit();

protected:
	virtual long					Notify( NotifyEvent& rNEvt );
	virtual void					Select();
	virtual void					Modify();
	virtual long					PreNotify( NotifyEvent& rNEvt );

public:
                                    SvtURLBox( Window* pParent, INetProtocol eSmart = INET_PROT_NOT_VALID );
                                    SvtURLBox( Window* pParent, WinBits _nStyle, INetProtocol eSmart = INET_PROT_NOT_VALID );
                                    SvtURLBox( Window* pParent, const ResId& _rResId, INetProtocol eSmart = INET_PROT_NOT_VALID );
                                    ~SvtURLBox();

	void							SetBaseURL( const String& rURL );
	const String&					GetBaseURL() const { return aBaseURL; }
	void							SetOpenHdl( const Link& rLink ) { aOpenHdl = rLink; }
	const Link& 					GetOpenHdl() const { return aOpenHdl; }
	void							SetOnlyDirectories( BOOL bDir = TRUE );
	void							SetNoURLSelection( BOOL bSet = TRUE );
	INetProtocol					GetSmartProtocol() const { return eSmartProtocol; }
    void                            SetSmartProtocol( INetProtocol eProt );
    BOOL                            IsCtrlOpen()
                                        { return bCtrlClick; }
    String                          GetURL();
	void							DisableHistory();

	void							UpdatePickList( );

    static String                   ParseSmart( String aText, String aBaseURL, String aWorkDir );

	void							SetFilter(const String& _sFilter);
    void                            SetUrlFilter( const IUrlFilter* _pFilter );
    const IUrlFilter*               GetUrlFilter( ) const;

    inline void                     EnableAutocompletion( BOOL _bEnable = TRUE )
                                        { bIsAutoCompleteEnabled = _bEnable; }
    void SetPlaceHolder( const String& sPlaceHolder ) { aPlaceHolder = sPlaceHolder; }
    String GetPlaceHolder() { return aPlaceHolder; }
    bool MatchesPlaceHolder( const String& sToMatch ) { return ( ( aPlaceHolder.Len() > 0 ) && ( aPlaceHolder == sToMatch ) ); }
};

#endif

