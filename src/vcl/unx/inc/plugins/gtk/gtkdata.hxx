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

#ifndef _VCL_GTKDATA_HXX
#define _VCL_GTKDATA_HXX

#include <tools/prex.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <tools/postx.h>

#include <saldisp.hxx>
#include <saldata.hxx>
#include <vcl/ptrstyle.hxx>

#include <list>

class GtkData : public X11SalData
{
public:
    GtkData() {}
    virtual ~GtkData();
    
    virtual void Init();
    
    virtual void initNWF();
    virtual void deInitNWF();
};

class GtkSalFrame;

class GtkSalDisplay : public SalDisplay
{
    GdkDisplay*						m_pGdkDisplay;
	GdkCursor                      *m_aCursors[ POINTER_COUNT ];
    bool                            m_bStartupCompleted;
	GdkCursor* getFromXPM( const char *pBitmap, const char *pMask,
						   int nWidth, int nHeight, int nXHot, int nYHot );
public:
			 GtkSalDisplay( GdkDisplay* pDisplay );
    virtual ~GtkSalDisplay();

    GdkDisplay* GetGdkDisplay() const { return m_pGdkDisplay; }

    virtual void deregisterFrame( SalFrame* pFrame );
	GdkCursor *getCursor( PointerStyle ePointerStyle );
	virtual int CaptureMouse( SalFrame* pFrame );
    virtual long Dispatch( XEvent *pEvent );
    virtual void initScreen( int nScreen ) const;

    static GdkFilterReturn filterGdkEvent( GdkXEvent* sys_event,
                                           GdkEvent* event,
                                           gpointer data );
	inline bool HasMoreEvents()     { return m_aUserEvents.size() > 1; }
	inline void EventGuardAcquire() { osl_acquireMutex( hEventGuard_ ); }
	inline void EventGuardRelease() { osl_releaseMutex( hEventGuard_ ); }
    void startupNotificationCompleted() { m_bStartupCompleted = true; }
    
    void screenSizeChanged( GdkScreen* );
    void monitorsChanged( GdkScreen* );
};


#endif // _VCL_GTKDATA_HXX
