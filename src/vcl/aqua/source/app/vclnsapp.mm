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

 // MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_vcl.hxx"

#include "vclnsapp.h"
#include "salinst.h"
#include "saldata.hxx"
#include "salframe.h"
#include "salframeview.h"

#include "vcl/window.hxx"
#include "vcl/svapp.hxx"
#include "vcl/cmdevt.hxx"
#include "rtl/ustrbuf.hxx"

#include "premac.h"
#import "Carbon/Carbon.h"
#import "apple_remote/RemoteControl.h"
#include "postmac.h"

 
@implementation CocoaThreadEnabler
-(void)enableCocoaThreads:(id)param
{
    // do nothing, this is just to start an NSThread and therefore put
    // Cocoa into multithread mode
}
@end

@implementation VCL_NSApplication
-(void)sendEvent:(NSEvent*)pEvent
{
    NSEventType eType = [pEvent type];
    if( eType == NSApplicationDefined )
        GetSalData()->mpFirstInstance->handleAppDefinedEvent( pEvent );
    else if( eType == NSKeyDown && ([pEvent modifierFlags] & NSCommandKeyMask) != 0 )
    {
        NSWindow* pKeyWin = [NSApp keyWindow];
        if( pKeyWin && [pKeyWin isKindOfClass: [SalFrameWindow class]] )
        {
            AquaSalFrame* pFrame = [(SalFrameWindow*)pKeyWin getSalFrame];
            // handle Cmd-W
            // FIXME: the correct solution would be to handle this in framework
            // in the menu code
            // however that is currently being revised, so let's use a preliminary solution here
            // this hack is based on assumption
            // a) Cmd-W is the same in all languages in OOo's menu conig
            // b) Cmd-W is the same in all languages in on MacOS
            // for now this seems to be true
            unsigned int nModMask = ([pEvent modifierFlags] & (NSShiftKeyMask|NSControlKeyMask|NSAlternateKeyMask|NSCommandKeyMask));
            if( (pFrame->mnStyleMask & NSClosableWindowMask) != 0 )
            {
                if( nModMask == NSCommandKeyMask
                    && [[pEvent charactersIgnoringModifiers] isEqualToString: @"w"] )
                {
                    [pFrame->getWindow() windowShouldClose: nil];
                    return;
                }
            }
          
            /*
             * #i98949# - Cmd-M miniaturize window, Cmd-Option-M miniaturize all windows
             */ 
            if( [[pEvent charactersIgnoringModifiers] isEqualToString: @"m"] )
            {
                if ( nModMask == NSCommandKeyMask && ([pFrame->getWindow() styleMask] & NSMiniaturizableWindowMask) )
                {
                    [pFrame->getWindow() performMiniaturize: nil];
                    return;
                }

                if ( nModMask == ( NSCommandKeyMask | NSAlternateKeyMask ) )
                {
                    [NSApp miniaturizeAll: nil];
                    return;
                }
            }
            
            // #i90083# handle frame switching
            // FIXME: lousy workaround
            if( (nModMask & (NSControlKeyMask|NSAlternateKeyMask)) == 0 )
            {
                if( [[pEvent characters] isEqualToString: @"<"] ||
                    [[pEvent characters] isEqualToString: @"~"] )
                {
                    [self cycleFrameForward: pFrame];
                    return;
                }
                else if( [[pEvent characters] isEqualToString: @">"] ||
                         [[pEvent characters] isEqualToString: @"`"] )
                {
                    [self cycleFrameBackward: pFrame];
                    return;
                }
            }
 
            // get information whether the event was handled; keyDown returns nothing
            GetSalData()->maKeyEventAnswer[ pEvent ] = false;
            bool bHandled = false;
            
            // dispatch to view directly to avoid the key event being consumed by the menubar
            // popup windows do not get the focus, so they don't get these either
            // simplest would be dispatch this to the key window always if it is without parent
            // however e.g. in document we want the menu shortcut if e.g. the stylist has focus
            if( pFrame->mpParent && (pFrame->mnStyle & SAL_FRAME_STYLE_FLOAT) == 0 ) 
            {
                [[pKeyWin contentView] keyDown: pEvent];
                bHandled = GetSalData()->maKeyEventAnswer[ pEvent ];
            }
            
            // see whether the main menu consumes this event
            // if not, we want to dispatch it ourselves. Unless we do this "trick"
            // the main menu just beeps for an unknown or disabled key equivalent
            // and swallows the event wholesale
            NSMenu* pMainMenu = [NSApp mainMenu];
            if( ! bHandled && (pMainMenu == 0 || ! [pMainMenu performKeyEquivalent: pEvent]) )
            {
                [[pKeyWin contentView] keyDown: pEvent];
                bHandled = GetSalData()->maKeyEventAnswer[ pEvent ];
            }
            else
                bHandled = true;  // event handled already or main menu just handled it

            GetSalData()->maKeyEventAnswer.erase( pEvent );
            if( bHandled )
                return;
        }
        else if( pKeyWin )
        {
            // #i94601# a window not of vcl's making has the focus.
            // Since our menus do not invoke the usual commands
            // try to play nice with native windows like the file dialog
            // and emulate them
            // precondition: this ONLY works because CMD-V (paste), CMD-C (copy) and CMD-X (cut) are
            // NOT localized, that is the same in all locales. Should this be
            // different in any locale, this hack will fail.
            unsigned int nModMask = ([pEvent modifierFlags] & (NSShiftKeyMask|NSControlKeyMask|NSAlternateKeyMask|NSCommandKeyMask));
            if( nModMask == NSCommandKeyMask )
            {
                
                if( [[pEvent charactersIgnoringModifiers] isEqualToString: @"v"] )
                {
                    if( [NSApp sendAction: @selector(paste:) to: nil from: nil] )
                        return;
                }
                else if( [[pEvent charactersIgnoringModifiers] isEqualToString: @"c"] )
                {
                    if( [NSApp sendAction: @selector(copy:) to: nil from: nil] )
                        return;
                }
                else if( [[pEvent charactersIgnoringModifiers] isEqualToString: @"x"] )
                {
                    if( [NSApp sendAction: @selector(cut:) to: nil from: nil] )
                        return;
                }
            }
        }
    }
    else if( eType == NSScrollWheel && ( GetSalData()->mnSystemVersion < VER_LEOPARD /* fixed in Leopard and above */ ) )
    {

        NSWindow* pWin = [pEvent window];
        // on Tiger wheel events do not reach non key windows
        // which probably should be considered a bug
        if( [pWin isKindOfClass: [SalFrameWindow class]] && [pWin canBecomeKeyWindow] == NO )
        {
            [[pWin contentView] scrollWheel: pEvent];
            return;
        }
    }
    [super sendEvent: pEvent];
}

-(void)sendSuperEvent:(NSEvent*)pEvent
{
    [super sendEvent: pEvent];
}

-(void)cycleFrameForward: (AquaSalFrame*)pCurFrame
{
    // find current frame in list
    std::list< AquaSalFrame* >& rFrames( GetSalData()->maFrames );
    std::list< AquaSalFrame* >::iterator it = rFrames.begin();
    for( ; it != rFrames.end() && *it != pCurFrame; ++it )
        ;
    if( it != rFrames.end() )
    {
        // now find the next frame (or end)
        do
        {
            ++it;
            if( it != rFrames.end() )
            {
                if( (*it)->mpDockMenuEntry != NULL &&
                    (*it)->mbShown )
                {
                    [(*it)->getWindow() makeKeyAndOrderFront: NSApp];
                    return;
                }
            }
        } while( it != rFrames.end() );
        // cycle around, find the next up to pCurFrame
        it = rFrames.begin();
        while( *it != pCurFrame )
        {
            if( (*it)->mpDockMenuEntry != NULL &&
                (*it)->mbShown )
            {
                [(*it)->getWindow() makeKeyAndOrderFront: NSApp];
                return;
            }
            ++it;
        }
    }
}

-(void)cycleFrameBackward: (AquaSalFrame*)pCurFrame
{
    // do the same as cycleFrameForward only with a reverse iterator
    
    // find current frame in list
    std::list< AquaSalFrame* >& rFrames( GetSalData()->maFrames );
    std::list< AquaSalFrame* >::reverse_iterator it = rFrames.rbegin();
    for( ; it != rFrames.rend() && *it != pCurFrame; ++it )
        ;
    if( it != rFrames.rend() )
    {
        // now find the next frame (or end)
        do
        {
            ++it;
            if( it != rFrames.rend() )
            {
                if( (*it)->mpDockMenuEntry != NULL &&
                    (*it)->mbShown )
                {
                    [(*it)->getWindow() makeKeyAndOrderFront: NSApp];
                    return;
                }
            }
        } while( it != rFrames.rend() );
        // cycle around, find the next up to pCurFrame
        it = rFrames.rbegin();
        while( *it != pCurFrame )
        {
            if( (*it)->mpDockMenuEntry != NULL &&
                (*it)->mbShown )
            {
                [(*it)->getWindow() makeKeyAndOrderFront: NSApp];
                return;
            }
            ++it;
        }
    }
}
 
-(NSMenu*)applicationDockMenu:(NSApplication *)sender
{
    return AquaSalInstance::GetDynamicDockMenu();
}

-(MacOSBOOL)application: (NSApplication*)app openFile: (NSString*)pFile
{
    const rtl::OUString aFile( GetOUString( pFile ) );
    if( ! AquaSalInstance::isOnCommandLine( aFile ) )
    {
        const ApplicationEvent* pAppEvent = new ApplicationEvent( String(), ApplicationAddress(),
                                                    APPEVENT_OPEN_STRING, aFile );
        AquaSalInstance::aAppEventList.push_back( pAppEvent );
    }
    return YES;
}

-(void)application: (NSApplication*) app openFiles: (NSArray*)files
{
    rtl::OUStringBuffer aFileList( 256 );
    
    NSEnumerator* it = [files objectEnumerator];
    NSString* pFile = nil;
    
    while( (pFile = [it nextObject]) != nil )
    {
        const rtl::OUString aFile( GetOUString( pFile ) );
        if( ! AquaSalInstance::isOnCommandLine( aFile ) )
        {
            if( aFileList.getLength() > 0 )
                aFileList.append( sal_Unicode( APPEVENT_PARAM_DELIMITER ) );
            aFileList.append( aFile );
        }
    }
    
    if( aFileList.getLength() )
    {
        // we have no back channel here, we have to assume success, in which case
        // replyToOpenOrPrint does not need to be called according to documentation
        // [app replyToOpenOrPrint: NSApplicationDelegateReplySuccess];
        const ApplicationEvent* pAppEvent = new ApplicationEvent( String(), ApplicationAddress(),
                                                    APPEVENT_OPEN_STRING, aFileList.makeStringAndClear() );
        AquaSalInstance::aAppEventList.push_back( pAppEvent );
    }
}

-(MacOSBOOL)application: (NSApplication*)app printFile: (NSString*)pFile
{
    const rtl::OUString aFile( GetOUString( pFile ) );
	const ApplicationEvent* pAppEvent = new ApplicationEvent( String(), ApplicationAddress(),
                                                APPEVENT_PRINT_STRING, aFile );
	AquaSalInstance::aAppEventList.push_back( pAppEvent );
    return YES;
}
-(NSApplicationPrintReply)application: (NSApplication *) app printFiles:(NSArray *)files withSettings: (NSDictionary *)printSettings showPrintPanels:(MacOSBOOL)bShowPrintPanels
{
    // currently ignores print settings an bShowPrintPanels
    rtl::OUStringBuffer aFileList( 256 );
    
    NSEnumerator* it = [files objectEnumerator];
    NSString* pFile = nil;
    
    while( (pFile = [it nextObject]) != nil )
    {
        if( aFileList.getLength() > 0 )
            aFileList.append( sal_Unicode( APPEVENT_PARAM_DELIMITER ) );
        aFileList.append( GetOUString( pFile ) );
    }
	const ApplicationEvent* pAppEvent = new ApplicationEvent( String(), ApplicationAddress(),
                                                APPEVENT_PRINT_STRING, aFileList.makeStringAndClear() );
	AquaSalInstance::aAppEventList.push_back( pAppEvent );
    // we have no back channel here, we have to assume success
    // correct handling would be NSPrintingReplyLater and then send [app replyToOpenOrPrint]
    return NSPrintingSuccess;
}

-(NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *) app
{
    SalData* pSalData = GetSalData();
    #if 1 // currently do some really bad hack
    if( ! pSalData->maFrames.empty() )
    {
        /* #i92766# something really weird is going on with the retain count of
           our windows; sometimes we get a duplicate free before exit on one of our
           NSWindows. The reason is unclear; to avoid this currently we retain them once more
           
           FIXME: this is a really bad hack, relying on the system to catch the leaked
           resources. Find out what really goes on here and fix it !
        */
        std::vector< NSWindow* > aHackRetainedWindows;
        for( std::list< AquaSalFrame* >::iterator it = pSalData->maFrames.begin();
             it != pSalData->maFrames.end(); ++it )
        {
            #if OSL_DEBUG_LEVEL > 1
            Window* pWin = (*it)->GetWindow();
            String aTitle = pWin->GetText();
            Window* pClient = pWin->ImplGetClientWindow();
            fprintf( stderr, "retaining %p (old count %d) windowtype=%s clienttyp=%s title=%s\n",
                (*it)->mpWindow, [(*it)->mpWindow retainCount],
                typeid(*pWin).name(), pClient ? typeid(*pClient).name() : "<nil>",
                rtl::OUStringToOString( aTitle, RTL_TEXTENCODING_UTF8 ).getStr()
                );
            #endif
            [(*it)->mpWindow retain];
            aHackRetainedWindows.push_back( (*it)->mpWindow ); 
        }
        if( pSalData->maFrames.front()->CallCallback( SALEVENT_SHUTDOWN, NULL ) )
        {
            for( std::vector< NSWindow* >::iterator it = aHackRetainedWindows.begin();
                 it != aHackRetainedWindows.end(); ++it )
            {
                // clean up the retaing count again from the shutdown workaround
                #if OSL_DEBUG_LEVEL > 1
                fprintf( stderr, "releasing %p\n", (*it) );
                #endif
                [(*it) release];
            }
            return NSTerminateCancel;
        }
        #if OSL_DEBUG_LEVEL > 1
        for( std::list< AquaSalFrame* >::iterator it = pSalData->maFrames.begin();
             it != pSalData->maFrames.end(); ++it )
        {
            Window* pWin = (*it)->GetWindow();
            String aTitle = pWin->GetText();
            Window* pClient = pWin->ImplGetClientWindow();
            fprintf( stderr, "frame still alive: NSWindow %p windowtype=%s clienttyp=%s title=%s\n",
                (*it)->mpWindow, typeid(*pWin).name(), pClient ? typeid(*pClient).name() : "<nil>",
                rtl::OUStringToOString( aTitle, RTL_TEXTENCODING_UTF8 ).getStr()
                );
        }
        #endif
    }
    #else // the clean version follows
    return pSalData->maFrames.front()->CallCallback( SALEVENT_SHUTDOWN, NULL ) ? NSTerminateCancel : NSTerminateNow;
    #endif
    return NSTerminateNow;
}

-(void)systemColorsChanged: (NSNotification*) pNotification
{
    const SalData* pSalData = GetSalData();
	if( !pSalData->maFrames.empty() )
		pSalData->maFrames.front()->CallCallback( SALEVENT_SETTINGSCHANGED, NULL );
}

-(void)screenParametersChanged: (NSNotification*) pNotification
{
    SalData* pSalData = GetSalData();
    std::list< AquaSalFrame* >::iterator it;
    for( it = pSalData->maFrames.begin(); it != pSalData->maFrames.end(); ++it )
    {
        (*it)->screenParametersChanged();
    }
}

-(void)scrollbarVariantChanged: (NSNotification*) pNotification
{
    GetSalData()->mpFirstInstance->delayedSettingsChanged( true );
}

-(void)scrollbarSettingsChanged: (NSNotification*) pNotification
{
    GetSalData()->mpFirstInstance->delayedSettingsChanged( false );
}

-(void)addFallbackMenuItem: (NSMenuItem*)pNewItem
{
    AquaSalMenu::addFallbackMenuItem( pNewItem );
}

-(void)removeFallbackMenuItem: (NSMenuItem*)pItem
{
    AquaSalMenu::removeFallbackMenuItem( pItem );
}

-(void)addDockMenuItem: (NSMenuItem*)pNewItem
{
    NSMenu* pDock = AquaSalInstance::GetDynamicDockMenu();
    [pDock insertItem: pNewItem atIndex: [pDock numberOfItems]];
}

// for Apple Remote implementation

#pragma mark -
#pragma mark NSApplication Delegates
- (void)applicationWillBecomeActive:(NSNotification *)aNotification {
    if (GetSalData()->mpMainController->remoteControl) {

        // [remoteControl startListening: self];
        // does crash because the right thing to do is 
        // [GetSalData()->mpMainController->remoteControl startListening: self];
        // but the instance variable 'remoteControl' is declared protected
        // workaround : declare remoteControl instance variable as public in RemoteMainController.m

        [GetSalData()->mpMainController->remoteControl startListening: self];
#ifdef DEBUG
        NSLog(@"Apple Remote will become active - Using remote controls");
#endif
    }
}

- (void)applicationWillResignActive:(NSNotification *)aNotification {
    if (GetSalData()->mpMainController->remoteControl) {

        // [remoteControl stopListening: self];
        // does crash because the right thing to do is 
        // [GetSalData()->mpMainController->remoteControl stopListening: self];
        // but the instance variable 'remoteControl' is declared protected
        // workaround : declare remoteControl instance variable as public in RemoteMainController.m

        [GetSalData()->mpMainController->remoteControl stopListening: self]; 
#ifdef DEBUG
        NSLog(@"Apple Remote will resign active - Releasing remote controls");
#endif
    }
}

- (MacOSBOOL)applicationShouldHandleReopen: (NSApplication*)pApp hasVisibleWindows: (MacOSBOOL) bWinVisible
{
    NSObject* pHdl = GetSalData()->mpDockIconClickHandler;
    if( pHdl && [pHdl respondsToSelector: @selector(dockIconClicked:)] )
    {
        [pHdl performSelector:@selector(dockIconClicked:) withObject: self];
    }
    return YES;
}

-(void)setDockIconClickHandler: (NSObject*)pHandler
{
    GetSalData()->mpDockIconClickHandler = pHandler;
}


@end

