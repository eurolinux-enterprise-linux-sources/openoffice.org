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
#ifndef _SFXAPP_HXX
#define _SFXAPP_HXX

#include "sal/config.h"
#include "sfx2/dllapi.h"
#include "sal/types.h"
#include <tools/solar.h>
#include <svtools/smplhint.hxx>
#include <svtools/poolitem.hxx>
#include <tools/ref.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/script/XLibraryContainer.hpp>
#include <com/sun/star/task/XStatusIndicator.hpp>

// too many files including sfx2/app.hxx use VCL Application class but don't include the
// header file because in former times SfxApplication was derived from it
#include <vcl/svapp.hxx>

#include <sfx2/shell.hxx>

class Timer;
class ApplicationEvent;
class WorkWindow;
class ISfxTemplateCommon;
class BasicManager;
class DdeService;
class GenLink;
class PrinterDialog;
class Point;
class Rectangle;
class AppSettings;
struct SfxChildWinContextFactory;
class SfxAppData_Impl;
class SfxBindings;
class SfxCancelManager;
class SfxChildWinFactArr_Impl;
class SfxChildWindow;
class SfxDispatcher;
class SfxEventConfiguration;
class SfxEventHint;
class SfxItemSet;
class SfxMacroConfig;
class SfxMedium;
class SfxMediumList;
class SfxMenuCtrlFactArr_Impl;
class SfxNewFileDialog;
class SfxObjectShell;
class SfxObjectShellArr_Impl;
class SfxOptions;
class SfxProgress;
class SfxResourceManager;
class SfxSlotPool;
class SfxStbCtrlFactArr_Impl;
class SfxTbxCtrlFactArr_Impl;
class SfxTemplateDialog;
class SfxViewFrame;
class SfxViewFrameArr_Impl;
class SfxViewShell;
class SfxViewShellArr_Impl;
class StarBASIC;
class SfxWorkWindow;
class SfxFilterMatcher;
class SvUShorts;
class SfxModule;
class SfxModuleArr_Impl;
class Window;
class INetURLObject;
class SfxMiscCfg;
struct SfxConstant;
struct SfxChildWinFactory;
struct SfxMenuCtrlFactory;
struct SfxStbCtrlFactory;
struct SfxTbxCtrlFactory;
class SimpleResMgr;
class ModalDialog;

namespace sfx2
{
	class SvLinkSource;
}

//====================================================================

class SfxLinkItem : public SfxPoolItem
{
    Link aLink;
public:
    virtual SfxPoolItem*     Clone( SfxItemPool* = 0 ) const
    {   return new SfxLinkItem( *this ); }
    virtual int              operator==( const SfxPoolItem& rL) const
    {   return ((SfxLinkItem&)rL).aLink == aLink; }
    SfxLinkItem( USHORT nWhichId, const Link& rValue ) : SfxPoolItem( nWhichId )
    {   aLink = rValue; }
    const Link&              GetValue() const { return aLink; }
};

//TODO/CLEANUP
//wird anscheinend nur in SfxPickList/SfxFrameLoader benutzt
DECL_OBJHINT( SfxStringHint, String );

/*
typedef SfxPoolItem* (*SfxItemCreateFunc)();
class SfxItemFactory_Impl;
class SfxItemFactoryList
{
    List aList;
public:
    ~SfxItemFactoryList();

    SfxItemFactory_Impl* GetFactory_Impl( const SvGlobalName& rName ) const;
    SfxItemFactory_Impl* GetFactory_Impl( TypeId ) const;
    const SvGlobalName* GetGlobalName( const SfxPoolItem* pItem ) const;
    SfxPoolItem* Create(
        const SvGlobalName& rName, USHORT nId, SvStream* pStrm = 0) const;
    void         RegisterItemFactory(
        const SvGlobalName& rName, SfxItemCreateFunc );
};

#define REGISTER_ITEM( ItemClass, aGlobName )                                 \
RegisterItemFactory( aGlobName, ( SfxItemCreateFunc) ItemClass::StaticType() );
*/

#ifndef SFX_DECL_OBJECTSHELL_DEFINED
#define SFX_DECL_OBJECTSHELL_DEFINED
SV_DECL_REF(SfxObjectShell)
#endif

class SfxObjectShellLock;

class SFX2_DLLPUBLIC SfxApplication: public SfxShell
{
    SAL_DLLPRIVATE static ::osl::Mutex gMutex;
	SAL_DLLPRIVATE static SfxApplication* pApp;

    SfxAppData_Impl*            pAppData_Impl;

//#if 0 // _SOLAR__PRIVATE
    DECL_DLLPRIVATE_LINK(       GlobalBasicErrorHdl_Impl, StarBASIC* );
    SAL_DLLPRIVATE BOOL         SaveAll_Impl(BOOL bPrompt = FALSE, BOOL bAutoSave = FALSE);
    SAL_DLLPRIVATE short        QuerySave_Impl(SfxObjectShell &, BOOL bAutoSave = FALSE);
	SAL_DLLPRIVATE void			InitializeDisplayName_Impl();
//#endif

    static SfxApplication*      Create();
    void                        Init();
    void                        Exit();
    void                		SettingsChange( USHORT, const AppSettings & );
    void                        Main( );
    void                		PreInit( );
    void                		Quit();
    void                        Deinitialize();

public:
                                TYPEINFO();
                                SFX_DECL_INTERFACE(SFX_INTERFACE_SFXAPP)

								SfxApplication();
                                ~SfxApplication();
	static SfxApplication*		GetOrCreate();

    // Resource Manager
    bool                        InitLabelResMgr( const char* _pLabelPrefix, bool _bException = false );
    SfxResourceManager&         GetResourceManager() const;
    ResMgr*                     GetSfxResManager();
	SimpleResMgr*				GetSimpleResManager();
    ResMgr*                     GetLabelResManager() const;
	static ResMgr*              CreateResManager( const char *pPrefix );
	SimpleResMgr*				CreateSimpleResManager();

    // DDE
    long                		DdeExecute( const String& rCmd );
	long						DdeGetData( const String& rItem,
											const String& rMimeType,
										::com::sun::star::uno::Any & rValue );
	long						DdeSetData( const String& rItem,
											const String& rMimeType,
								const ::com::sun::star::uno::Any & rValue );
    ::sfx2::SvLinkSource*       DdeCreateLinkSource( const String& rItem );
    BOOL                        InitializeDde();
    const DdeService*           GetDdeService() const;
    DdeService*                 GetDdeService();
    void                        AddDdeTopic( SfxObjectShell* );
    void                        RemoveDdeTopic( SfxObjectShell* );

    // "static" methods
    ULONG                       LoadTemplate( SfxObjectShellLock& xDoc, const String& rFileName, BOOL bCopy=TRUE, SfxItemSet* pArgs = 0 );
    ::com::sun::star::uno::Reference< ::com::sun::star::task::XStatusIndicator > GetStatusIndicator() const;
    SfxTemplateDialog*          GetTemplateDialog();
    Window*                     GetTopWindow() const;

    // TODO/CLEANUP: make currently selected family a view property and so we don't need to query the status from the "TemplateCommon"
    ISfxTemplateCommon*         GetCurrentTemplateCommon( SfxBindings& );

    // members
    SfxFilterMatcher&           GetFilterMatcher();
    SfxCancelManager*           GetCancelManager() const;
    SfxMacroConfig*             GetMacroConfig() const;
    SfxProgress*                GetProgress() const;
    const String&               GetLastSaveDirectory() const;
    USHORT                      GetFreeIndex();
    void                        ReleaseIndex(USHORT i);
    SfxEventConfiguration*      GetEventConfig() const;
	SfxMiscCfg*					GetMiscConfig();

    // Basic/Scripting
    static sal_Bool				IsXScriptURL( const String& rScriptURL );
    static ::rtl::OUString		ChooseScript();
    static void         		MacroOrganizer( INT16 nTabId );
    BasicManager*               GetBasicManager();
	com::sun::star::uno::Reference< com::sun::star::script::XLibraryContainer >
								GetDialogContainer();
	com::sun::star::uno::Reference< com::sun::star::script::XLibraryContainer >
								GetBasicContainer();
    StarBASIC*                  GetBasic();
    USHORT                      SaveBasicManager() const;
    USHORT                      SaveBasicAndDialogContainer() const;
    void                        EnterBasicCall();
    FASTBOOL                    IsInBasicCall() const;
    void                        LeaveBasicCall();
    void                        RegisterBasicConstants( const char *pPrefix,
                                                        const SfxConstant *pConsts,
                                                        USHORT nCount );

    // misc.
    BOOL                        GetOptions(SfxItemSet &);
    void                        SetOptions(const SfxItemSet &);
    virtual void                Invalidate(USHORT nId = 0);
    void                        NotifyEvent(const SfxEventHint& rEvent, FASTBOOL bSynchron = TRUE );
    BOOL                        IsDowning() const;
    BOOL                        IsSecureURL( const INetURLObject &rURL, const String *pReferer ) const;
    static SfxObjectShellRef    DocAlreadyLoaded( const String &rName,
                                                  BOOL bSilent,
                                                  BOOL bActivate,
                                                  BOOL bForbidVisible = FALSE,
                                                  const String* pPostStr = 0);
    void                        ResetLastDir();

//#if 0 // _SOLAR__PRIVATE
	SAL_DLLPRIVATE static SfxApplication* Is_Impl() { return pApp;}
    SAL_DLLPRIVATE SfxDispatcher* GetAppDispatcher_Impl();
    SAL_DLLPRIVATE SfxDispatcher* GetDispatcher_Impl();

    SAL_DLLPRIVATE SfxObjectShellLock NewDoc_Impl( const String& rFactory, const SfxItemSet* pSet = NULL );
    SAL_DLLPRIVATE BOOL    		QueryExit_Impl();
    SAL_DLLPRIVATE void         SetOptions_Impl(const SfxItemSet &);
    SAL_DLLPRIVATE FASTBOOL     Initialize_Impl();

    SAL_DLLPRIVATE SfxAppData_Impl* Get_Impl() const { return pAppData_Impl; }

    // Object-Factories/global arrays
    SAL_DLLPRIVATE void         RegisterChildWindow_Impl(SfxModule*, SfxChildWinFactory*);
    SAL_DLLPRIVATE void         RegisterChildWindowContext_Impl(SfxModule*, USHORT, SfxChildWinContextFactory*);
    SAL_DLLPRIVATE void         RegisterStatusBarControl_Impl(SfxModule*, SfxStbCtrlFactory*);
    SAL_DLLPRIVATE void         RegisterMenuControl_Impl(SfxModule*, SfxMenuCtrlFactory*);
    SAL_DLLPRIVATE void         RegisterToolBoxControl_Impl( SfxModule*, SfxTbxCtrlFactory*);
    SAL_DLLPRIVATE SfxTbxCtrlFactArr_Impl& GetTbxCtrlFactories_Impl() const;
    SAL_DLLPRIVATE SfxStbCtrlFactArr_Impl& GetStbCtrlFactories_Impl() const;
    SAL_DLLPRIVATE SfxMenuCtrlFactArr_Impl& GetMenuCtrlFactories_Impl() const;
    SAL_DLLPRIVATE SfxChildWinFactArr_Impl& GetChildWinFactories_Impl() const;
    SAL_DLLPRIVATE SfxViewFrameArr_Impl& GetViewFrames_Impl() const;
    SAL_DLLPRIVATE SfxViewShellArr_Impl& GetViewShells_Impl() const;
    SAL_DLLPRIVATE SfxObjectShellArr_Impl& GetObjectShells_Impl() const;
    SAL_DLLPRIVATE void         SetViewFrame_Impl(SfxViewFrame *pViewFrame);

    // Slot Methods
    // TODO/CLEANUP: still needed?
    SAL_DLLPRIVATE void         NewDocDirectExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         NewDocExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE const SfxPoolItem* NewDocDirectExec_ImplOld(SfxRequest &); // used by FrameLoader to work with the old behaviour ...
    SAL_DLLPRIVATE void         OpenDocExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         MiscExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         MiscState_Impl(SfxItemSet &);
    SAL_DLLPRIVATE void         PropExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         PropState_Impl(SfxItemSet &);
    SAL_DLLPRIVATE void         MacroExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         MacroState_Impl(SfxItemSet &);
    SAL_DLLPRIVATE void         INetExecute_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         INetState_Impl(SfxItemSet &);
    SAL_DLLPRIVATE void         OfaExec_Impl(SfxRequest &);
    SAL_DLLPRIVATE void         OfaState_Impl(SfxItemSet &);

    SAL_DLLPRIVATE void         SetProgress_Impl(SfxProgress *);
    SAL_DLLPRIVATE const String& GetLastDir_Impl() const;
    SAL_DLLPRIVATE void         SetLastDir_Impl( const String & );
    SAL_DLLPRIVATE void         PlayMacro_Impl( SfxRequest &rReq, StarBASIC *pBas );

    SAL_DLLPRIVATE void         EnterAsynchronCall_Impl();
    SAL_DLLPRIVATE FASTBOOL     IsInAsynchronCall_Impl() const;
    SAL_DLLPRIVATE void         LeaveAsynchronCall_Impl();
    SAL_DLLPRIVATE void         Registrations_Impl();
    SAL_DLLPRIVATE SfxWorkWindow* GetWorkWindow_Impl(const SfxViewFrame *pFrame=0) const;

    // TODO/CLEANUP: still needed?
    SAL_DLLPRIVATE SvUShorts*   GetDisabledSlotList_Impl();
    SAL_DLLPRIVATE SfxSlotPool& GetAppSlotPool_Impl() const;
    SAL_DLLPRIVATE SfxModule*   GetModule_Impl();
	SAL_DLLPRIVATE ResMgr*		GetOffResManager_Impl();
//#endif
};

#define SFX_APP() SfxGetpApp()

//--------------------------------------------------------------------

inline SfxApplication* SfxGetpApp()
{
    return SfxApplication::GetOrCreate();
}

#endif


