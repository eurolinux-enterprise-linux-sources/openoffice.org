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
#ifndef _SVX_FMVWIMP_HXX
#define _SVX_FMVWIMP_HXX

#include "svx/svdmark.hxx"
#include "fmdocumentclassification.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/form/XForm.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XEnumeration.hpp>
#include <com/sun/star/form/XFormController.hpp>
#include <com/sun/star/container/XContainerListener.hpp>
#include <com/sun/star/container/ContainerEvent.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/awt/XFocusListener.hpp>
#include <com/sun/star/sdb/SQLErrorEvent.hpp>
#include <com/sun/star/sdbc/XDataSource.hpp>
/** === end UNO includes === **/

#include <comphelper/stl_types.hxx>
#include <tools/link.hxx>
#include <tools/string.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase3.hxx>
#include <comphelper/uno3.hxx>
#include <comphelper/componentcontext.hxx>

//class SdrPageViewWinRec;
class SdrPageWindow;

class SdrPageView;
class SdrObject;
class FmFormObj;
class FmFormModel;
class FmFormView;
class FmFormShell;
class Window;
class OutputDevice;
class SdrUnoObj;
class SdrView;

FORWARD_DECLARE_INTERFACE(awt,XControl)
FORWARD_DECLARE_INTERFACE(awt,XWindow)
FORWARD_DECLARE_INTERFACE(beans,XPropertySet)
FORWARD_DECLARE_INTERFACE(util,XNumberFormats)

class FmXFormController;
class FmXFormView;

namespace svx {
	class ODataAccessDescriptor;
	struct OXFormsDescriptor;
}

//==================================================================
// FmXPageViewWinRec
//==================================================================
class FmXPageViewWinRec : public ::cppu::WeakImplHelper1< ::com::sun::star::container::XIndexAccess>
{
	friend class FmXFormView;

	::std::vector< ::com::sun::star::uno::Reference< ::com::sun::star::form::XFormController > >	m_aControllerList;
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >                    m_xControlContainer;
    ::comphelper::ComponentContext                                                                  m_aContext;
	FmXFormView*				m_pViewImpl;
	Window*						m_pWindow;

public:
	FmXPageViewWinRec(	const ::comphelper::ComponentContext& _rContext,
		const SdrPageWindow&, FmXFormView* pView);
		//const SdrPageViewWinRec*, FmXFormView* pView);
	~FmXPageViewWinRec();

// UNO Anbindung

// ::com::sun::star::container::XElementAccess
	virtual ::com::sun::star::uno::Type SAL_CALL getElementType() throw(::com::sun::star::uno::RuntimeException);
	virtual sal_Bool SAL_CALL hasElements() throw(::com::sun::star::uno::RuntimeException);

// ::com::sun::star::container::XEnumerationAccess
	virtual ::com::sun::star::uno::Reference< ::com::sun::star::container::XEnumeration >  SAL_CALL createEnumeration() throw(::com::sun::star::uno::RuntimeException);

// ::com::sun::star::container::XIndexAccess
	virtual sal_Int32 SAL_CALL getCount() throw(::com::sun::star::uno::RuntimeException);
	virtual ::com::sun::star::uno::Any SAL_CALL getByIndex(sal_Int32 _Index) throw(::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

	const ::std::vector< ::com::sun::star::uno::Reference< ::com::sun::star::form::XFormController > >& GetList() {return m_aControllerList;}

protected:
	::com::sun::star::uno::Reference< ::com::sun::star::form::XFormController >  getController( const ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >& xForm ) const;
	void setController(	const ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >& xForm,
						FmXFormController* pParent = NULL);
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >  getControlContainer() const { return m_xControlContainer; }
	void updateTabOrder( const ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >& _rxForm );
	void dispose();
	Window* getWindow() const {return m_pWindow;}
};

typedef ::std::vector<FmXPageViewWinRec*> FmWinRecList;
typedef ::std::set  <   ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >
                    ,   ::comphelper::OInterfaceCompare< ::com::sun::star::form::XForm >
                    >   SetOfForms;
typedef ::std::map  <   ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >
                    ,   SetOfForms
                    ,   ::comphelper::OInterfaceCompare< ::com::sun::star::awt::XControlContainer >
                    >   MapControlContainerToSetOfForms;
class SdrModel;
//==================================================================
// FmXFormView
//==================================================================
class FmXFormView :	public ::cppu::WeakImplHelper3<
							::com::sun::star::form::XFormControllerListener,
							::com::sun::star::awt::XFocusListener,
							::com::sun::star::container::XContainerListener>
{
	friend class FmFormView;
	friend class FmFormShell;
	friend class FmXFormShell;
	friend class FmXPageViewWinRec;
	class ObjectRemoveListener;
	friend class ObjectRemoveListener;

    ::comphelper::ComponentContext                                                      m_aContext;
	::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindow>					m_xWindow;
    ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >           m_xLastCreatedControlModel;

	FmFormObj*		m_pMarkedGrid;
	FmFormView*		m_pView;
	sal_uIntPtr		m_nActivationEvent;
	sal_uIntPtr		m_nErrorMessageEvent;	// event for an asynchronous error message. See also m_aAsyncError
	sal_uIntPtr		m_nAutoFocusEvent;		// event for asynchronously setting the focus to a control
    sal_uIntPtr		m_nControlWizardEvent;  // event for asynchronously setting the focus to a control

	::com::sun::star::sdb::SQLErrorEvent
					m_aAsyncError;			// error event which is to be displayed asyn. See m_nErrorMessageEvent.

	FmWinRecList	m_aWinList;				// to be filled in alive mode only
    MapControlContainerToSetOfForms
                    m_aNeedTabOrderUpdate;

	// Liste der markierten Object, dient zur Restauration beim Umschalten von Alive in DesignMode
	SdrMarkList				m_aMark;
	ObjectRemoveListener*	m_pWatchStoredList;

	bool            m_bFirstActivation;
    bool            m_isTabOrderUpdateSuspended;

	FmFormShell* GetFormShell() const;

	void removeGridWindowListening();

protected:
	FmXFormView( const ::comphelper::ComponentContext& _rContext, FmFormView* _pView );
	~FmXFormView();

	void	saveMarkList( sal_Bool _bSmartUnmark = sal_True );
	void	restoreMarkList( SdrMarkList& _rRestoredMarkList );
	void	stopMarkListWatching();
	void	startMarkListWatching();

	void	notifyViewDying( );
		// notifies this impl class that the anti-impl instance (m_pView) is going to die

public:
	// UNO Anbindung

// ::com::sun::star::lang::XEventListener
	virtual void SAL_CALL disposing(const ::com::sun::star::lang::EventObject& Source) throw(::com::sun::star::uno::RuntimeException);

// ::com::sun::star::container::XContainerListener
	virtual void SAL_CALL elementInserted(const  ::com::sun::star::container::ContainerEvent& rEvent) throw(::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL elementReplaced(const  ::com::sun::star::container::ContainerEvent& rEvent) throw(::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL elementRemoved(const  ::com::sun::star::container::ContainerEvent& rEvent) throw(::com::sun::star::uno::RuntimeException);

// ::com::sun::star::form::XFormControllerListener
	virtual void SAL_CALL formActivated(const ::com::sun::star::lang::EventObject& rEvent) throw(::com::sun::star::uno::RuntimeException);
	virtual void SAL_CALL formDeactivated(const ::com::sun::star::lang::EventObject& rEvent) throw(::com::sun::star::uno::RuntimeException);

	// XFocusListener
	virtual void SAL_CALL focusGained( const ::com::sun::star::awt::FocusEvent& e ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL focusLost( const ::com::sun::star::awt::FocusEvent& e ) throw (::com::sun::star::uno::RuntimeException);

	FmFormView* getView() const {return m_pView;}
	FmWinRecList::const_iterator findWindow( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >& _rxCC ) const;
	const FmWinRecList& getWindowList() const {return m_aWinList;}

    ::com::sun::star::uno::Reference< ::com::sun::star::form::XFormController >
            getFormController( const ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >& _rxForm, const OutputDevice& _rDevice ) const;

	// activation handling
	inline	bool        hasEverBeenActivated( ) const { return !m_bFirstActivation; }
	inline	void		setHasBeenActivated( ) { m_bFirstActivation = false; }

			void		onFirstViewActivation( const FmFormModel* _pDocModel );

    /** suspends the calls to activateTabOrder, which normally happen whenever for any ControlContainer of the view,
        new controls are inserted. Cannot be nested, i.e. you need to call resumeTabOrderUpdate before calling
        suspendTabOrderUpdate, again.
    */
    void    suspendTabOrderUpdate();

    /** resumes calls to activateTabOrder, and also does all pending calls which were collected since the last
        suspendTabOrderUpdate call.
    */
    void    resumeTabOrderUpdate();

    void    onCreatedFormObject( FmFormObj& _rFormObject );

private:
	FmWinRecList::iterator findWindow( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >& _rxCC );
	//void addWindow(const SdrPageViewWinRec*);
	void addWindow(const SdrPageWindow&);
	void removeWindow( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XControlContainer >& _rxCC );
	void Activate(sal_Bool bSync = sal_False);
	void Deactivate(BOOL bDeactivateController = TRUE);

	SdrObject*	implCreateFieldControl( const ::svx::ODataAccessDescriptor& _rColumnDescriptor );
	SdrObject*	implCreateXFormsControl( const ::svx::OXFormsDescriptor &_rDesc );

	static bool createControlLabelPair(
        const ::comphelper::ComponentContext& _rContext,
        OutputDevice& _rOutDev,
        sal_Int32 _nXOffsetMM,
        sal_Int32 _nYOffsetMM,
        const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& _rxField,
        const ::com::sun::star::uno::Reference< ::com::sun::star::util::XNumberFormats >& _rxNumberFormats,
        sal_uInt16 _nControlObjectID,
        const ::rtl::OUString& _rFieldPostfix,
        UINT32 _nInventor,
        UINT16 _nLabelObjectID,
        SdrPage* _pLabelPage,
        SdrPage* _pControlPage,
        SdrModel* _pModel,
        SdrUnoObj*& _rpLabel,
        SdrUnoObj*& _rpControl
	);

    bool    createControlLabelPair(
        OutputDevice& _rOutDev,
        sal_Int32 _nXOffsetMM,
        sal_Int32 _nYOffsetMM,
        const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& _rxField,
        const ::com::sun::star::uno::Reference< ::com::sun::star::util::XNumberFormats >& _rxNumberFormats,
        sal_uInt16 _nControlObjectID,
        const ::rtl::OUString& _rFieldPostfix,
        SdrUnoObj*& _rpLabel,
        SdrUnoObj*& _rpControl,
        const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XDataSource >& _rxDataSource = NULL,
        const ::rtl::OUString& _rDataSourceName = ::rtl::OUString(),
        const ::rtl::OUString& _rCommand= ::rtl::OUString(),
        const sal_Int32 _nCommandType = -1
    );

	void ObjectRemovedInAliveMode(const SdrObject* pObject);

	// asynchronously displays an error message. See also OnDelayedErrorMessage.
	void	displayAsyncErrorMessage( const ::com::sun::star::sdb::SQLErrorEvent& _rEvent );

	// cancels all pending async events
	void cancelEvents();

	/// the the auto focus to the first (in terms of the tab order) control
	void AutoFocus( sal_Bool _bSync = sal_False );
	DECL_LINK( OnActivate, void* );
	DECL_LINK( OnAutoFocus, void* );
	DECL_LINK( OnDelayedErrorMessage, void* );
    DECL_LINK( OnStartControlWizard, void* );

private:
    ::svxform::DocumentType impl_getDocumentType() const;
};



#endif // _SVX_FMVWIMP_HXX

