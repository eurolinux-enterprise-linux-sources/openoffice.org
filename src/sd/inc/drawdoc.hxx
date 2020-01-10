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

#ifndef _DRAWDOC_HXX
#define _DRAWDOC_HXX

#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/text/WritingMode.hpp>
#include <com/sun/star/frame/XModel.hdl>
#include <vcl/print.hxx>
#include <svx/fmmodel.hxx>
#include "pres.hxx"
#include <svx/pageitem.hxx>
#include <unotools/charclass.hxx>
#include <sot/storage.hxx>
#include <rsc/rscsfx.hxx>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/text/WritingMode.hpp>

// #107844#
#include <svx/svdundo.hxx>

#include <vector>

#ifndef INCLUDED_MEMORY
#include <memory>
#define INCLUDED_MEMORY
#endif
#include "sddllapi.h"
#include "sdpage.hxx"

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace embed
            {
                class XStorage;
            }
            namespace io
            {
                class XStream;
            }
			namespace presentation
			{
				class XPresentation2;
			}
        }
    }
}

namespace sd
{
	class FrameView;
	class Outliner;
}


class Timer;
class SfxObjectShell;
class SdDrawDocShell;
class SdPage;
class SdAnimationInfo;
class SdIMapInfo;
class IMapObject;
class SdStyleSheetPool;
class SfxMedium;
class SvxSearchItem;
class SdrOle2Obj;
class EditStatus;
class Graphic;
class Point;
class Window;
class SdTransferable;
struct SpellCallbackInfo;
struct StyleRequestData;
class SdDrawDocument;

namespace sd
{
#ifndef SV_DECL_DRAW_DOC_SHELL_DEFINED
#define SV_DECL_DRAW_DOC_SHELL_DEFINED
SV_DECL_REF(DrawDocShell)
#endif
class DrawDocShell;
class UndoManager;
class ShapeList;
}

class ImpDrawPageListWatcher;
class ImpMasterPageListWatcher;

struct StyleReplaceData
{
	SfxStyleFamily  nFamily;
	SfxStyleFamily  nNewFamily;
	String          aName;
	String          aNewName;
};

enum DocCreationMode
{
	NEW_DOC,
	DOC_LOADED
};

namespace sd
{
	struct PresentationSettings
	{
		rtl::OUString maPresPage;
		sal_Bool mbAll;
		sal_Bool mbEndless;
		sal_Bool mbCustomShow;
		sal_Bool mbManual;
		sal_Bool mbMouseVisible;
		sal_Bool mbMouseAsPen;
		sal_Bool mbLockedPages;
		sal_Bool mbAlwaysOnTop;
		sal_Bool mbFullScreen;
		sal_Bool mbAnimationAllowed;
		sal_Int32 mnPauseTimeout;
		sal_Bool mbShowPauseLogo;
		sal_Bool mbStartWithNavigator;

		PresentationSettings();
		PresentationSettings( const PresentationSettings& r );
	};
}

// ------------------
// - SdDrawDocument -
// ------------------

class SdDrawDocument : public FmFormModel
{
private:
	::sd::Outliner*		mpOutliner;		    // local outliner for outline mode
	::sd::Outliner*		mpInternalOutliner;  // internal outliner for creation of text objects
	Timer*			    mpWorkStartupTimer;
	Timer*              mpOnlineSpellingTimer;
	sd::ShapeList*		mpOnlineSpellingList;
	SvxSearchItem*      mpOnlineSearchItem;
	List*               mpFrameViewList;
	List*               mpCustomShowList;
	::sd::DrawDocShell* mpDocSh;
    SdTransferable *    mpCreatingTransferable;
	BOOL                mbHasOnlineSpellErrors;
	BOOL                mbInitialOnlineSpellingEnabled;
	String              maBookmarkFile; 
	::sd::DrawDocShellRef   mxBookmarkDocShRef; 

	sd::PresentationSettings maPresentationSettings;

	::com::sun::star::uno::Reference< ::com::sun::star::presentation::XPresentation2 > mxPresentation;

	BOOL			    mbNewOrLoadCompleted;

	BOOL			    mbOnlineSpell;
    BOOL                mbSummationOfParagraphs;
	bool				mbStartWithPresentation;		// is set to true when starting with command line parameter -start
	LanguageType	    meLanguage;
	LanguageType	    meLanguageCJK;
	LanguageType	    meLanguageCTL;
	SvxNumType		    mePageNumType;
	::sd::DrawDocShellRef   mxAllocedDocShRef;   // => AllocModel()
	BOOL			    mbAllocDocSh;		// => AllocModel()
	DocumentType        meDocType;
	CharClass*		    mpCharClass;
	::com::sun::star::lang::Locale* mpLocale;

	// #109538#
	::std::auto_ptr<ImpDrawPageListWatcher> mpDrawPageListWatcher;
	::std::auto_ptr<ImpMasterPageListWatcher> mpMasterPageListWatcher;

	void                UpdatePageObjectsInNotes(USHORT nStartPos);
    void                UpdatePageRelativeURLs(SdPage* pPage, USHORT nPos, sal_Int32 nIncrement);
	void                FillOnlineSpellingList(SdPage* pPage);
	void                SpellObject(SdrTextObj* pObj);

	                    DECL_LINK(WorkStartupHdl, Timer*);
	                    DECL_LINK(OnlineSpellingHdl, Timer*);
	                    DECL_LINK(OnlineSpellEventHdl, EditStatus*);

    std::vector< rtl::OUString > maAnnotationAuthors;
    
protected:

	virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > createUnoModel();

public:

    class InsertBookmarkAsPage_PageFunctorBase;

                	    TYPEINFO();

	                    SdDrawDocument(DocumentType eType, SfxObjectShell* pDocSh);
	                    ~SdDrawDocument();

	virtual SdrModel*   AllocModel() const;
	virtual SdrPage*    AllocPage(FASTBOOL bMasterPage);
	virtual const SdrModel* LoadModel(const String& rFileName);
	virtual void        DisposeLoadedModels();
	virtual FASTBOOL    IsReadOnly() const;
	virtual void        SetChanged(sal_Bool bFlag = sal_True);
	virtual SvStream*   GetDocumentStream(SdrDocumentStreamInfo& rStreamInfo) const;

	SfxItemPool&	    GetPool() { return( *pItemPool ); }

	::sd::Outliner* GetOutliner(BOOL bCreateOutliner=TRUE);
	SD_DLLPUBLIC ::sd::Outliner* GetInternalOutliner(BOOL bCreateOutliner=TRUE);
                        
	::sd::DrawDocShell*     GetDocSh() const { return mpDocSh; }

	LanguageType	    GetLanguage( const USHORT nId ) const;
	void			    SetLanguage( const LanguageType eLang, const USHORT nId );

	SvxNumType          GetPageNumType() const;
	void			    SetPageNumType(SvxNumType eType) { mePageNumType = eType; }
	SD_DLLPUBLIC String              CreatePageNumValue(USHORT nNum) const;

	DocumentType        GetDocumentType() const { return meDocType; }

	void			    SetAllocDocSh(BOOL bAlloc);

	void	            CreatingDataObj( SdTransferable* pTransferable ) { mpCreatingTransferable = pTransferable; }
                        
	/** if the document does not contain at least one handout, one slide and one notes page with
		at least one master each this methods creates them.
		If a reference document is given, the sizes and border settings of that document are used
		for newly created slides.
	*/
	SD_DLLPUBLIC void	CreateFirstPages( SdDrawDocument* pRefDocument = 0 );
	SD_DLLPUBLIC BOOL                CreateMissingNotesAndHandoutPages();

	void	            MovePage(USHORT nPgNum, USHORT nNewPos);
	void	            InsertPage(SdrPage* pPage, USHORT nPos=0xFFFF);
	void	            DeletePage(USHORT nPgNum);
	SdrPage*            RemovePage(USHORT nPgNum);

    virtual void     InsertMasterPage(SdrPage* pPage, USHORT nPos=0xFFFF);
	virtual SdrPage* RemoveMasterPage(USHORT nPgNum);

	void	            RemoveUnnecessaryMasterPages( SdPage* pMaster=NULL, BOOL bOnlyDuplicatePages=FALSE, BOOL bUndo=TRUE );
	SD_DLLPUBLIC void 	SetMasterPage(USHORT nSdPageNum, const String& rLayoutName,
					    	          SdDrawDocument* pSourceDoc, BOOL bMaster, BOOL bCheckMasters);
                        
	SD_DLLPUBLIC SdDrawDocument* OpenBookmarkDoc(const String& rBookmarkFile);
	SdDrawDocument*     OpenBookmarkDoc(SfxMedium& rMedium);
	BOOL                InsertBookmark(List* pBookmarkList, List* pExchangeList, BOOL bLink,
			            				BOOL bReplace, USHORT nPgPos, BOOL bNoDialogs,
			            				::sd::DrawDocShell* pBookmarkDocSh, BOOL bCopy,
			            				Point* pObjPos);

	bool IsStartWithPresentation() const;
	void SetStartWithPresentation( bool bStartWithPresentation );

	/** Insert pages into this document

    	This method inserts whole pages into this document, either
    	selected ones (specified via pBookmarkList/pExchangeList), or
    	all from the source document.

        @attention Beware! This method in it's current state does not
        handle all combinations of their input parameters
        correctly. For example, for pBookmarkList=NULL, bReplace=TRUE
        is ignored (no replace happens).

        @param pBookmarkList
        A list of strings, denoting the names of the pages to be copied

        @param pExchangeList
        A list of strings, denoting the names of the pages to be renamed

        @param bLink
        Whether the inserted pages should be links to the bookmark document

        @param bReplace
        Whether the pages should not be inserted, but replace the pages in
        the destination document

        @param nPgPos
        Insertion point/start of replacement

        @param bNoDialogs
        Whether query dialogs are allowed (e.g. for page scaling)

        @param pBookmarkDocSh
        DocShell of the source document (used e.g. to extract the filename
        for linked pages)

        @param bCopy
        Whether the source document should be treated as immutable (i.e.
        inserted pages are not removed from it, but cloned)

        @param bMergeMasterPages
        Whether the source document's master pages should be copied, too.

        @param bPreservePageNames
        Whether the replace operation should take the name from the new
        page, or preserve the old name
     */
	BOOL                InsertBookmarkAsPage(List* pBookmarkList, List* pExchangeList,
			            					  BOOL bLink, BOOL bReplace, USHORT nPgPos,
			            					  BOOL bNoDialogs, ::sd::DrawDocShell* pBookmarkDocSh,
			            					  BOOL bCopy, BOOL bMergeMasterPages,
                                              BOOL bPreservePageNames);
	BOOL                InsertBookmarkAsObject(List* pBookmarkList, List* pExchangeListL,
			            						BOOL bLink, ::sd::DrawDocShell* pBookmarkDocSh,
			            						Point* pObjPos);
    void 				IterateBookmarkPages( SdDrawDocument* pBookmarkDoc, List* pBookmarkList,
                                              USHORT nBMSdPageCount,
                                              InsertBookmarkAsPage_PageFunctorBase& rPageIterator );
	SD_DLLPUBLIC void	CloseBookmarkDoc();
                        
	SdrObject*          GetObj(const String& rObjName) const;

    /** Return the first page that has the given name.  Regular pages and
        notes pages are searched first.  When not found then the master
        pages are searched.
        @param rPgName
            Name of the page to return.
        @param rbIsMasterPage
            Is set by the method to indicate whether the returned index
            belongs to a master page (<TRUE/>) or a regular or notes page
            (<FALSE/>). The given value is ignored.
        @return
            Returns the index of the page with the given name or
            SDRPAGE_NOTFOUND (=0xffff) when such a page does not exist.
    */
	USHORT GetPageByName(const String& rPgName, BOOL& rbIsMasterPage ) const;
	SD_DLLPUBLIC SdPage*GetSdPage(USHORT nPgNum, PageKind ePgKind) const;
	SD_DLLPUBLIC USHORT	GetSdPageCount(PageKind ePgKind) const;

	void	            SetSelected(SdPage* pPage, BOOL bSelect);
	BOOL	            MovePages(USHORT nTargetPage);
                        
	SD_DLLPUBLIC SdPage*GetMasterSdPage(USHORT nPgNum, PageKind ePgKind);
	SD_DLLPUBLIC USHORT	GetMasterSdPageCount(PageKind ePgKind) const;
                        
	USHORT	            GetMasterPageUserCount(SdrPage* pMaster) const;
                        
	const sd::PresentationSettings& getPresentationSettings() const { return maPresentationSettings; }
	sd::PresentationSettings& getPresentationSettings() { return maPresentationSettings; }
                                            
	const ::com::sun::star::uno::Reference< ::com::sun::star::presentation::XPresentation2 >& getPresentation() const;

   	void                SetSummationOfParagraphs( BOOL bOn = TRUE ) { mbSummationOfParagraphs = bOn; }
	BOOL	        IsSummationOfParagraphs() const { return mbSummationOfParagraphs; }

    /** Set the mode that controls whether (and later how) the formatting of the document
        depends on the current printer metrics.
        @param nMode
            Use <const
            scope="com::sun::star::document::PrinterIndependentLayout">ENABLED</const>
            to make formatting printer-independent and <const
            scope="com::sun::star::document::PrinterIndependentLayout">DISABLED</const>
            to make formatting depend on the current printer metrics.
    */
    void SetPrinterIndependentLayout (sal_Int32 nMode);

    /** Get the flag that controls whether the formatting of the document
        depends on the current printer metrics.
        @return
            Use <const
            scope="com::sun::star::document::PrinterIndependentLayout">ENABLED</const>
            when formatting is printer-independent and <const
            scope="com::sun::star::document::PrinterIndependentLayout">DISABLED</const>
            when formatting depends on the current printer metrics.
    */
    sal_Int32 GetPrinterIndependentLayout (void);

	void                SetOnlineSpell( BOOL bIn );
	BOOL                GetOnlineSpell() const { return mbOnlineSpell; }
	void                StopOnlineSpelling();
	void                StartOnlineSpelling(BOOL bForceSpelling=TRUE);

	void                ImpOnlineSpellCallback(SpellCallbackInfo* pInfo, SdrObject* pObj, SdrOutliner* pOutl);

	void                InsertObject(SdrObject* pObj, SdPage* pPage);
	void                RemoveObject(SdrObject* pObj, SdPage* pPage);

	ULONG               GetLinkCount();

	List*               GetFrameViewList() const { return mpFrameViewList; }
	SD_DLLPUBLIC List*  GetCustomShowList(BOOL bCreate = FALSE);
                        
	void                NbcSetChanged(sal_Bool bFlag = sal_True);

	void                SetTextDefaults() const;

	void                CreateLayoutTemplates();
	void                RenameLayoutTemplate(const String& rOldLayoutName, const String& rNewName);
                        
	void				CreateDefaultCellStyles();

	SD_DLLPUBLIC void   StopWorkStartupDelay();
                        
	void                NewOrLoadCompleted(DocCreationMode eMode);
	void				NewOrLoadCompleted( SdPage* pPage, SdStyleSheetPool* pSPool );
	BOOL                IsNewOrLoadCompleted() const {return mbNewOrLoadCompleted; }

	::sd::FrameView* GetFrameView(ULONG nPos) {
        return static_cast< ::sd::FrameView*>(
            mpFrameViewList->GetObject(nPos));}

	/** deprecated*/
	SdAnimationInfo*    GetAnimationInfo(SdrObject* pObject) const;

	SD_DLLPUBLIC static 	SdAnimationInfo* GetShapeUserData(SdrObject& rObject, bool bCreate = false );

	SdIMapInfo*         GetIMapInfo( SdrObject* pObject ) const;
	IMapObject*         GetHitIMapObject( SdrObject* pObject, const Point& rWinPoint, const ::Window& rCmpWnd );

	CharClass*	        GetCharClass() const { return mpCharClass; }

	void                RestoreLayerNames();
	void                MakeUniqueLayerNames();

	void	            UpdateAllLinks();

	void                CheckMasterPages();

	void                Merge(SdrModel& rSourceModel,
			                    USHORT nFirstPageNum=0, USHORT nLastPageNum=0xFFFF,
			                    USHORT nDestPos=0xFFFF,
			                    FASTBOOL bMergeMasterPages=FALSE, FASTBOOL bAllMasterPages=FALSE,
			                    FASTBOOL bUndo=TRUE, FASTBOOL bTreadSourceAsConst=FALSE);
                        
    SD_DLLPUBLIC ::com::sun::star::text::WritingMode GetDefaultWritingMode() const;
    void SetDefaultWritingMode( ::com::sun::star::text::WritingMode eMode );

	/** replacespOldPage from all custom shows with pNewPage or removes pOldPage from
		all custom shows if pNewPage is 0.
	*/
	void ReplacePageInCustomShows( const SdPage* pOldPage, const SdPage* pNewPage );

public:

    static SdDrawDocument* pDocLockedInsertingLinks;  // static to prevent recursions while resolving links

    /** This method acts as a simplified front end for the more complex
        <member>CreatePage()</member> method.
        @param nPageNum
            The page number as passed to the <member>GetSdPage()</member>
            method from which to use certain properties for the new pages.
            These include the auto layout.
        @return
            Returns an index of the inserted pages that can be used with the
            <member>GetSdPage()</member> method.
    */
    USHORT CreatePage (USHORT nPageNum);

    /** Create and insert a set of two new pages: a standard (draw) page and
        the associated notes page.  The new pages are inserted direclty
        after the specified page set.
        @param pCurrentPage
            This page is used to retrieve the layout for the page to
            create.
        @param ePageKind
            This specifies whether <argument>pCurrentPage</argument> is a
            standard (draw) page or a notes page.
        @param sStandardPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param sNotesPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param eStandardLayout
            Layout to use for the new standard page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a standard page.  In this case the layout is taken from the
            standard page associated with <argument>pCurrentPage</argument>.
        @param eNotesLayout
            Layout to use for the new notes page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a notes page.  In this case the layout is taken from the
            notes page associated with <argument>pCurrentPage</argument>.
        @param bIsPageBack
            This flag indicates whether to show the background shape.
        @param bIsPageObj
            This flag indicates whether to show the shapes on the master page.

        @return
            Returns an index of the inserted pages that can be used with the
            <member>GetSdPage()</member> method.
    */
    USHORT CreatePage (
        SdPage* pCurrentPage,
        PageKind ePageKind,
        const String& sStandardPageName,
        const String& sNotesPageName,
        AutoLayout eStandardLayout,
        AutoLayout eNotesLayout,
        BOOL bIsPageBack,
        BOOL bIsPageObj);

    /** This method acts as a simplified front end for the more complex
        <member>DuplicatePage()</member> method.
        @param nPageNum
            The page number as passed to the <member>GetSdPage()</member>
            method for which the standard page and the notes page are to be
            copied.
        @return
            Returns an index of the inserted pages that can be used with the
            <member>GetSdPage()</member> method.
    */
    USHORT DuplicatePage (USHORT nPageNum);

    /** Create and insert a set of two new pages that are copies of the
        given <argument>pCurrentPage</argument> and its associated notes
        resp. standard page.  The copies are inserted directly after the
        specified page set.
        @param pCurrentPage
            This page and its associated notes/standard page is copied.
        @param ePageKind
            This specifies whether <argument>pCurrentPage</argument> is a
            standard (draw) page or a notes page.
        @param sStandardPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param sNotesPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param eStandardLayout
            Layout to use for the new standard page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a standard page.  In this case the layout is taken from the
            standard page associated with <argument>pCurrentPage</argument>.
        @param eNotesLayout
            Layout to use for the new notes page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a notes page.  In this case the layout is taken from the
            notes page associated with <argument>pCurrentPage</argument>.
        @param bIsPageBack
            This flag indicates whether to show the background shape.
        @param bIsPageObj
            This flag indicates whether to show the shapes on the master page.

        @return
            Returns an index of the inserted pages that can be used with the
            <member>GetSdPage()</member> method.
    */
    USHORT DuplicatePage (
        SdPage* pCurrentPage,
        PageKind ePageKind,
        const String& sStandardPageName,
        const String& sNotesPageName,
        AutoLayout eStandardLayout,
        AutoLayout eNotesLayout,
        BOOL bIsPageBack,
        BOOL bIsPageObj);

	/** return the document fonts for latin, cjk and ctl according to the current
		languages set at this document */
	void getDefaultFonts( Font& rLatinFont, Font& rCJKFont, Font& rCTLFont );

	sd::UndoManager* GetUndoManager() const;

	/* converts the given western font height to a corresponding ctl font height, deppending on the system language */
	static sal_uInt32 convertFontHeightToCTL( sal_uInt32 nWesternFontHeight );

	/** Get the style sheet pool if it was a SdStyleSheetPool.
	 */
	SD_DLLPUBLIC SdStyleSheetPool* GetSdStyleSheetPool() const;

   	void UpdatePageRelativeURLs(const String& rOldName, const String& rNewName);

	void SetCalcFieldValueHdl( ::Outliner* pOutliner);

    sal_uInt16 GetAnnotationAuthorIndex( const rtl::OUString& rAuthor );

private:
    /** This member stores the printer independent layout mode.  Please
        refer to <member>SetPrinterIndependentLayout()</member> for its
        values.
    */
    sal_Int32 mnPrinterIndependentLayout;

    /** Insert a given set of standard and notes page after the given <argument>pCurrentPage</argument>.
        @param pCurrentPage
            This page and its associated notes/standard page is copied.
        @param ePageKind
            This specifies whether <argument>pCurrentPage</argument> is a
            standard (draw) page or a notes page.
        @param sStandardPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param sNotesPageName
            Name of the standard page.  An empty string leads to using an
            automatically created name.
        @param eStandardLayout
            Layout to use for the new standard page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a standard page.  In this case the layout is taken from the
            standard page associated with <argument>pCurrentPage</argument>.
        @param eNotesLayout
            Layout to use for the new notes page.  Note that this layout
            is not used when the given <argument>pCurrentPage</argument> is
            not a notes page.  In this case the layout is taken from the
            notes page associated with <argument>pCurrentPage</argument>.
        @param bIsPageBack
            This flag indicates whether to show the background shape.
        @param bIsPageObj
            This flag indicates whether to show the shapes on the master page.
        @param pStandardPage
            The standard page to insert.
        @param pNotesPage
            The notes page to insert.

        @return
            Returns an index of the inserted pages that can be used with the
            <member>GetSdPage()</member> method.
    */
    USHORT InsertPageSet (
        SdPage* pCurrentPage,
        PageKind ePageKind,
        const String& sStandardPageName,
        const String& sNotesPageName,
        AutoLayout eStandardLayout,
        AutoLayout eNotesLayout,
        BOOL bIsPageBack,
        BOOL bIsPageObj,

        SdPage* pStandardPage,
        SdPage* pNotesPage);

    /** Set up a newly created page and insert it into the list of pages.
        @param pPreviousPage
            A page to take the size and border geometry from.
        @param pPage
            This is the page to set up and insert.
        @param sPageName
            The name of the new page.
        @param nInsertionPoint
            Index of the page before which the new page will be inserted.
        @param bIsPageBack
            This flag indicates whether to show the background shape.
        @param bIsPageObj
            This flag indicates whether to show the shapes on the master
            page.
    */
    void SetupNewPage (
        SdPage* pPreviousPage,
        SdPage* pPage,
        const String& sPageName,
        USHORT nInsertionPoint,
        BOOL bIsPageBack,
        BOOL bIsPageObj);

	// #109538#
	virtual void PageListChanged();
	virtual void MasterPageListChanged();
};

namespace sd
{

// an instance of this guard disables modification of a document
// during its lifetime
class ModifyGuard
{
public:
	ModifyGuard( DrawDocShell* pDocShell );
	ModifyGuard( SdDrawDocument* pDoc );
	~ModifyGuard();

private:
	void init();

	DrawDocShell* mpDocShell;
	SdDrawDocument* mpDoc;
	BOOL mbIsEnableSetModified;
	BOOL mbIsDocumentChanged;
};

}

#endif // _DRAWDOC_HXX
