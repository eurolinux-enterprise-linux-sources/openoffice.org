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
#ifndef CHART2_COMMANDDISPATCHCONTAINER_HXX
#define CHART2_COMMANDDISPATCHCONTAINER_HXX

#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/chart2/XUndoManager.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/DispatchDescriptor.hpp>

#include <cppuhelper/interfacecontainer.hxx>

#include <set>
#include <map>

namespace chart
{

/** @HTML

    Helper class for implementing the <code>XDispatchProvider</code> interface
    of the ChartController. This class handles all commands to queryDispatch and
    queryDispatches in the following way:

    <ul>
      <li>Check if there is a cached <code>XDispatch</code> for a given command.
        If so, use it.</li>
      <li>Check if the command is handled by this class, e.g. Undo.  If so,
        return a corresponding <code>XDispatch</code> implementation, and cache
        this implementation for later use</li>
      <li>Otherwise send the command to the fallback dispatch provider, if it
        can handle this dispatch (determined by the list of commands given in
        <code>setFallbackDispatch()</code>).</li>
    </ul>

    <p>The <code>XDispatch</code>Provider is designed to return different
    <code>XDispatch</code> implementations for each command.  This class here
    decides which implementation to use for which command.</p>

    <p>As most commands need much information of the controller and are
    implemented there, the controller handles most of the commands itself (it
    also implements <code>XDispatch</code>).  Therefore it is set here as
    fallback dispatch.</p>
 */
class CommandDispatchContainer
{
public:
    // note: the fallback dispatcher should be removed when all commands are
    // handled by other dispatchers.  (Fallback is currently the controller
    // itself)
    explicit CommandDispatchContainer(
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::uno::XComponentContext > & xContext );

    void setModel(
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::frame::XModel > & xModel );
//     void setUndoManager(
//         const ::com::sun::star::uno::Reference<
//             ::com::sun::star::chart2::XUndoManager > & xUndoManager );

    /** Set a fallback dispatcher that is used for all commands contained in
        rFallbackCommands
     */
    void setFallbackDispatch(
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::frame::XDispatch > xFallbackDispatch,
        const ::std::set< ::rtl::OUString > & rFallbackCommands );

    /** Returns the dispatch that is able to do the command given in rURL, if
        implemented here.  If the URL is not implemented here, it should be
        checked whether the command is one of the commands given as fallback via
        the setFallbackDispatch() method.  If so, call the fallback dispatch.

        <p>If all this fails, return an empty dispatch.</p>
     */
    ::com::sun::star::uno::Reference<
            ::com::sun::star::frame::XDispatch > getDispatchForURL(
                const ::com::sun::star::util::URL & rURL );

    ::com::sun::star::uno::Sequence<
        ::com::sun::star::uno::Reference<
            ::com::sun::star::frame::XDispatch > > getDispatchesForURLs(
                const ::com::sun::star::uno::Sequence<
                    ::com::sun::star::frame::DispatchDescriptor > & aDescriptors );

    void DisposeAndClear();

    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch >
        getContainerDispatchForURL(
            const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XController > & xChartController,
            const ::com::sun::star::util::URL & rURL );

private:
    typedef
        ::std::map< ::rtl::OUString,
            ::com::sun::star::uno::Reference<
                ::com::sun::star::frame::XDispatch > >
        tDispatchMap;

    typedef
        ::std::vector< ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > > tDisposeVector;

    mutable tDispatchMap m_aCachedDispatches;
    mutable tDisposeVector m_aToBeDisposedDispatches;

    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > m_xContext;
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel > m_xModel;
    ::com::sun::star::uno::Reference< ::com::sun::star::chart2::XUndoManager > m_xUndoManager;

    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > m_xFallbackDispatcher;
    ::std::set< ::rtl::OUString >                                          m_aFallbackCommands;

    ::std::set< ::rtl::OUString >                                          m_aContainerDocumentCommands;
};

} //  namespace chart

// CHART2_COMMANDDISPATCHCONTAINER_HXX
#endif
