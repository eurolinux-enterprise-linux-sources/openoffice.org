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

#ifndef INCLUDED_SLIDESHOW_ANIMATIONTRANSITIONFILTERNODE_HXX
#define INCLUDED_SLIDESHOW_ANIMATIONTRANSITIONFILTERNODE_HXX

#include "animationbasenode.hxx"
#include "com/sun/star/animations/XTransitionFilter.hpp"

namespace slideshow {
namespace internal {

class AnimationTransitionFilterNode : public AnimationBaseNode
{
public:
    AnimationTransitionFilterNode(
        ::com::sun::star::uno::Reference<
        ::com::sun::star::animations::XAnimationNode> const& xNode, 
        ::boost::shared_ptr<BaseContainerNode> const& pParent,
        NodeContext const& rContext )
        : AnimationBaseNode( xNode, pParent, rContext ),
          mxTransitionFilterNode( xNode, ::com::sun::star::uno::UNO_QUERY_THROW)
        {}
    
#if defined(VERBOSE)
    virtual const char* getDescription() const
        { return "AnimationTransitionFilterNode"; }
#endif
    
protected:
    virtual void dispose();
    
private:
    virtual AnimationActivitySharedPtr createActivity() const;
    
    ::com::sun::star::uno::Reference< 
        ::com::sun::star::animations::XTransitionFilter> mxTransitionFilterNode;
};

} // namespace internal
} // namespace slideshow

#endif /* INCLUDED_SLIDESHOW_ANIMATIONTRANSITIONFILTERNODE_HXX */
