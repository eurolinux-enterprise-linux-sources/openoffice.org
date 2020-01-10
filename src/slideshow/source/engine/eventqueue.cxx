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
#include "precompiled_slideshow.hxx"

// must be first
#include <canvas/debug.hxx>
#include <tools/diagnose_ex.h>
#include <canvas/verbosetrace.hxx>

#include <comphelper/anytostring.hxx>
#include <cppuhelper/exc_hlp.hxx>

#include <event.hxx>
#include <eventqueue.hxx>
#include <slideshowexceptions.hxx>

#include <boost/shared_ptr.hpp>
#include <limits>


using namespace ::com::sun::star;

namespace slideshow
{
    namespace internal
    {
        bool EventQueue::EventEntry::operator<( const EventEntry& rEvent ) const
        {
            // negate comparison, we want priority queue to be sorted
            // in increasing order of activation times
            return this->nTime > rEvent.nTime;
        }


        EventQueue::EventQueue(
            boost::shared_ptr<canvas::tools::ElapsedTime> const & pPresTimer )
            : maMutex(),
              maEvents(),
              maNextEvents(),
              mpTimer( pPresTimer )
        {
        }

        EventQueue::~EventQueue()
        {
            // add in all that have been added explicitly for this round:
            EventEntryVector::const_iterator const iEnd( maNextEvents.end() );
            for ( EventEntryVector::const_iterator iPos( maNextEvents.begin() );
                  iPos != iEnd; ++iPos ) 
            {
                maEvents.push(*iPos);
            }
            EventEntryVector().swap( maNextEvents );
            
            // dispose event queue
            while( !maEvents.empty() )
            {
                try 
                {
                    maEvents.top().pEvent->dispose();
                }
                catch (uno::Exception &) 
                {
                    OSL_ENSURE( false, rtl::OUStringToOString(
                                    comphelper::anyToString(
                                        cppu::getCaughtException() ),
                                    RTL_TEXTENCODING_UTF8 ).getStr() );
                }
                maEvents.pop();
            }
        }

        bool EventQueue::addEvent( const EventSharedPtr& rEvent )
        {
            ::osl::MutexGuard aGuard( maMutex );

            ENSURE_OR_RETURN( rEvent,
                               "EventQueue::addEvent: event ptr NULL" );

            // prepare entry
            
            // A seemingly obvious optimization cannot be used here,
            // because it breaks assumed order of notification: zero
            // timeout events could be fired() immediately, but that
            // would not unwind the stack and furthermore changes
            // order of notification

            // add entry
            maEvents.push( EventEntry( rEvent, rEvent->getActivationTime(
                                           mpTimer->getElapsedTime()) ) );
            return true;
        }
    
        bool EventQueue::addEventForNextRound( EventSharedPtr const& rEvent )
        {
            ::osl::MutexGuard aGuard( maMutex );

            ENSURE_OR_RETURN( rEvent.get() != NULL, 
                               "EventQueue::addEvent: event ptr NULL" );
            maNextEvents.push_back(
                EventEntry( rEvent, rEvent->getActivationTime(
                                mpTimer->getElapsedTime()) ) );
            return true;
        }
    
        void EventQueue::forceEmpty()
        {
            ::osl::MutexGuard aGuard( maMutex );

            process_(true);
        }

        void EventQueue::process()
        {
            ::osl::MutexGuard aGuard( maMutex );

            process_(false);
        }
    
        void EventQueue::process_( bool bFireAllEvents )
        {
            VERBOSE_TRACE( "EventQueue: heartbeat" );

            // add in all that have been added explicitly for this round:
            EventEntryVector::const_iterator const iEnd( maNextEvents.end() );
            for ( EventEntryVector::const_iterator iPos( maNextEvents.begin() );
                  iPos != iEnd; ++iPos ) {
                maEvents.push(*iPos);
            }
            EventEntryVector().swap( maNextEvents );
            
            // perform topmost, ready-to-execute event
            // =======================================

            const double nCurrTime( mpTimer->getElapsedTime() );
            
            // process ready/elapsed events. Note that the 'perceived'
            // current time remains constant for this loop, thus we're
            // processing only those events which where ready when we
            // entered this method.
            while( !maEvents.empty() &&
                   (bFireAllEvents || maEvents.top().nTime <= nCurrTime) )
            {
                EventEntry event( maEvents.top() );
                maEvents.pop();

                // only process event, if it is still 'charged',
                // i.e. the fire() call effects something. This is
                // used when e.g. having events registered at multiple
                // places, which should fire only once: after the
                // initial fire() call, those events become inactive
                // and return false on isCharged. This frees us from
                // the need to prune queues of those inactive shells.
                if( event.pEvent->isCharged() )
                {
                    try
                    {
#if OSL_DEBUG_LEVEL > 0
                        VERBOSE_TRACE( "Firing event: unknown (0x%X), timeout was: %f", 
                                       event.pEvent.get(),
                                       event.pEvent->getActivationTime(0.0) );
#endif

                        event.pEvent->fire();
                    }
                    catch( uno::RuntimeException& )
                    {
                        throw;
                    }
                    catch( uno::Exception& )
                    {
                        // catch anything here, we don't want
                        // to leave this scope under _any_
                        // circumstance. Although, do _not_
                        // reinsert an activity that threw
                        // once.
                    
                        // NOTE: we explicitely don't catch(...) here,
                        // since this will also capture segmentation
                        // violations and the like. In such a case, we
                        // still better let our clients now...
                        OSL_ENSURE( false,
                                    rtl::OUStringToOString(
                                        comphelper::anyToString( cppu::getCaughtException() ),
                                        RTL_TEXTENCODING_UTF8 ).getStr() ); 
                    }
                    catch( SlideShowException& )
                    {
                        // catch anything here, we don't want
                        // to leave this scope under _any_
                        // circumstance. Although, do _not_
                        // reinsert an activity that threw
                        // once.
                    
                        // NOTE: we explicitely don't catch(...) here,
                        // since this will also capture segmentation
                        // violations and the like. In such a case, we
                        // still better let our clients now...
                        OSL_TRACE( "::presentation::internal::EventQueue: Event threw a SlideShowException, action might not have been fully performed" );
                    }
                }
                else
                {
#if OSL_DEBUG_LEVEL > 0
                    VERBOSE_TRACE( "Ignoring discharged event: unknown (0x%X), timeout was: %f", 
                                   event.pEvent.get(),
                                   event.pEvent->getActivationTime(0.0) );
#endif
                }
            }
        }
        
        bool EventQueue::isEmpty() const
        {
            ::osl::MutexGuard aGuard( maMutex );

            return maEvents.empty();
        }

        double EventQueue::nextTimeout() const
        {
            ::osl::MutexGuard aGuard( maMutex );

            // return time for next entry (if any)
            return isEmpty() ? 
                ::std::numeric_limits<double>::max() :
                maEvents.top().nTime - mpTimer->getElapsedTime();
        }

        void EventQueue::clear()
        {
            ::osl::MutexGuard aGuard( maMutex );

            // TODO(P1): Maybe a plain vector and vector.swap will
            // be faster here. Profile.
            maEvents = ImplQueueType();
        }
    }
}
