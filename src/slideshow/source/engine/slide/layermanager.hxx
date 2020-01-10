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

#ifndef INCLUDED_SLIDESHOW_LAYERMANAGER_HXX
#define INCLUDED_SLIDESHOW_LAYERMANAGER_HXX

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <cppcanvas/spritecanvas.hxx>

#include "unoview.hxx"
#include "unoviewcontainer.hxx"
#include "attributableshape.hxx"
#include "layer.hxx"
#include "tools.hxx"

#include <vector>
#include <map>
#include <hash_map>
#include <algorithm>
#include <functional>

namespace basegfx {
    class B2DRange;
}

namespace slideshow
{
    namespace internal
    {
		/* Definition of Layermanager class */

        /** This class manages all of a slide's layers (and shapes)

			Since layer content changes when animations start or end,
			the layer manager keeps track of this and also handles
			starting/stopping of Shape animations. Note that none of
			the methods actually perform a screen update, this is
			always delayed until the ActivitiesQueue explicitely
			performs it.

            @see Layer
            @see Shape
         */
        class LayerManager : private boost::noncopyable
        {
        public:
            /** Create a new layer manager for the given page bounds

                @param rViews
                Views currently registered

                @param rPageBounds
                Overall page bounds, in user space coordinates

                @param bDisableAnimationZOrder
                When true, all sprite animations run in the
                foreground.  That is, no extra layers are created, and
                the slideshow runs potentially faster.
             */
            LayerManager( const UnoViewContainer&    rViews,
                          const ::basegfx::B2DRange& rPageBounds,
                          bool                       bDisableAnimationZOrder );

            /** Activate the LayerManager

                This method activates the LayerManager. Prior to
                activation, this instance will be passive, i.e. won't
                render anything to any view.

                @param bSlideBackgoundPainted
                When true, the initial slide content on the background
                layer is already rendered (e.g. from a previous slide
                transition). When false, LayerManager also renders
                initial content of background layer on next update()
                call.
             */
            void activate( bool bSlideBackgoundPainted );

            /** Deactivate the LayerManager

                This method deactivates the LayerManager. After
                deactivation, this instance will be passive,
                i.e. don't render anything to any view.  Furthermore,
                if there's currently more than one Layer active, this
                method also removes all but one.
             */
            void deactivate();

            // From ViewEventHandler, forwarded by SlideImpl
            /// Notify new view added to UnoViewContainer
            void viewAdded( const UnoViewSharedPtr& rView );
            /// Notify view removed from UnoViewContainer
            void viewRemoved( const UnoViewSharedPtr& rView );
            void viewChanged( const UnoViewSharedPtr& rView );
            void viewsChanged();

            /** Add the shape to this object

            	This method adds a shape to the page.
             */
            void addShape( const ShapeSharedPtr& rShape );

            /** Remove shape from this object

            	This method removes a shape from the shape.
             */
            bool removeShape( const ShapeSharedPtr& rShape );
            
            /** Lookup a Shape from an XShape model object

            	This method looks up the internal shape map for one
            	representing the given XShape.

                @param xShape
                The XShape object, for which the representing Shape
                should be looked up.
             */
            ShapeSharedPtr lookupShape( const ::com::sun::star::uno::Reference< 
                                           ::com::sun::star::drawing::XShape >& xShape ) const;

            /** Query a subset of the given original shape

				This method queries a new (but not necessarily unique)
				shape, which displays only the given subset of the
				original one.
             */
            AttributableShapeSharedPtr getSubsetShape( const AttributableShapeSharedPtr& 	rOrigShape,
                                                       const DocTreeNode&					rTreeNode );

            /** Revoke a previously queried subset shape.

            	With this method, a previously requested subset shape
            	is revoked again. If the last client revokes a given
            	subset, it will cease to be displayed, and the
            	original shape will again show the subset data.

                @param rOrigShape
                The shape the subset was created from

                @param rSubsetShape
                The subset created from rOrigShape
             */
            void revokeSubset( const AttributableShapeSharedPtr& rOrigShape,
                               const AttributableShapeSharedPtr& rSubsetShape );

            /** Notify the LayerManager that the given Shape starts an
                animation now.

				This method enters animation mode for the Shape on all
				registered views.
             */
            void enterAnimationMode( const AnimatableShapeSharedPtr& rShape );

            /** Notify the LayerManager that the given Shape is no
                longer animated.

				This methods ends animation mode for the given Shape
				on all registered views.
             */
            void leaveAnimationMode( const AnimatableShapeSharedPtr& rShape );

            /** Notify that a shape needs an update

            	This method notifies the layer manager that a shape
            	update is necessary. This is useful if, during
            	animation playback, changes occur to shapes which make
            	an update necessary on an update() call. Otherwise,
            	update() will not render anything, which is not
            	triggered by calling one of the other LayerManager
            	methods.

                @param rShape
                Shape which needs an update
             */
            void notifyShapeUpdate( const ShapeSharedPtr& rShape);

            /** Check whether any update operations  are pending.

            	@return true, if this LayerManager has any updates
            	pending, i.e. needs to repaint something for the next
            	frame.
             */
            bool isUpdatePending() const;
            
            /** Update the content

				This method updates the content on all layers on all
				registered views. It does not issues a
				View::updateScreen() call on registered views. Please
				note that this method only takes into account changes
				to shapes induced directly by calling methods of the
				LayerManager. If a shape needs an update, because of
				some external event unknown to the LayerManager (most
				notably running animations), you have to notify the
				LayerManager via notifyShapeUpdate().

                @see LayerManager::updateScreen()

	            @return whether the update finished successfully.
            */
            bool update();

            /** Render the content to given canvas

                This is a one-shot operation, which simply draws all
                shapes onto the given canvas, without any caching or
                other fuzz. Don't use that for repeated output onto
                the same canvas, the View concept is more optimal
                then.

                @param rTargetCanvas
                Target canvas to output onto.
             */
            bool renderTo( const ::cppcanvas::CanvasSharedPtr& rTargetCanvas ) const;

        private:             
            /** A hash map which maps the XShape to the corresponding Shape object.

                Provides quicker lookup than ShapeSet for simple mappings
             */
            typedef ::std::hash_map<
                ::com::sun::star::uno::Reference<
                    ::com::sun::star::drawing::XShape >,
                ShapeSharedPtr,
                hash< ::com::sun::star::uno::Reference<
                      ::com::sun::star::drawing::XShape > > > XShapeHash;

            /** Element of all-shapes set
             */
            struct ShapeEntry
            {
                /// Shape this entry stands for
                ShapeSharedPtr    mpShape;

                /// Layer this shape is currently displayed on
                LayerWeakPtr      mpLayer;

                explicit ShapeEntry( ShapeSharedPtr const& rShape ) : 
                    mpShape(rShape), 
                    mpLayer()
                {}

                ShapeSharedPtr const& getShape() const { return mpShape; }

                bool operator<( const ShapeEntry& rRHS ) const
                {
                    return Shape::lessThanShape::compare(mpShape.get(),
                                                         rRHS.mpShape.get());
                }
            };

            /** Set of all shapes
             */
            typedef ::std::set< ShapeEntry > LayerShapeSet;
            typedef ::std::set< ShapeSharedPtr > ShapeUpdateSet;


            ////////////////////////////////////////////////////////////////////////


            /// Adds shape area to containing layer's damage area
            void addUpdateArea( ShapeSharedPtr const& rShape );

            LayerSharedPtr createForegroundLayer() const;

            /** Push changes from updateShapeLayerAssociations() to current layer

                Factored-out method that resizes layer, if necessary,
                assigns correct layer priority, and repaints contained shapes.

                @param nCurrLayerIndex
                Index of current layer in maLayers

                @param aFirstLayerShape
                Valid iterator out of maAllShapes, denoting the first
                shape from nCurrLayerIndex
                
                @param aEndLayerShapes
                Valid iterator or end iterator out of maAllShapes,
                denoting one-behind-the-last shape of nCurrLayerIndex
             */
            void           commitLayerChanges( std::size_t                    nCurrLayerIndex, 
                                               LayerShapeSet::const_iterator  aFirstLayerShape,
                                               LayerShapeSet::const_iterator  aEndLayerShapes );
 
            /** Init Shape layers with background layer.
             */
            void          putShape2BackgroundLayer( const ShapeEntry& rShapeEntry );

            /** Commits any pending layer reorg, due to shapes either
                entering or leaving animation mode

                @param bBackgroundLayerPainted
                When true, LayerManager does not render anything on
                the background layer. Use this, if background has been
                updated by other means (e.g. slide transition)
            */
            void          updateShapeLayers( bool bBackgroundLayerPainted );

            /** Common stuff when adding a shape
             */
            void          implAddShape( const ShapeSharedPtr& rShape );

            /** Common stuff when removing a shape
             */ 
            void          implRemoveShape( const ShapeSharedPtr& rShape );
            
            /** Add or remove views

                Sharing duplicate code from viewAdded and viewRemoved
                method. The only point of variation at those places
                are removal vs. adding.
             */
            template<typename LayerFunc,
                     typename ShapeFunc> void manageViews( LayerFunc layerFunc,
                                                           ShapeFunc shapeFunc );
            
            bool updateSprites();

            /// Registered views
            const UnoViewContainer&  mrViews;

            /// All layers of this object. Vector owns the layers
            LayerVector              maLayers;

            /** Contains all shapes with their XShape reference as the key
             */
            XShapeHash				 maXShapeHash;

            /** Set of shapes this LayerManager own

                Contains the same set of shapes as XShapeHash, but is
                sorted in z order, for painting and layer
                association. Set entries are enriched with two flags
                for buffering animation enable/disable changes, and
                shape update requests.
            */
            LayerShapeSet            maAllShapes;

            /** Set of shapes that have requested an update
                
                When a shape is member of this set, its maShapes entry
                has bNeedsUpdate set to true. We maintain this
                redundant information for faster update processing.
             */
            ShapeUpdateSet           maUpdateShapes;

            /** Overall slide bounds (in user coordinate
                system). shapes that exceed this boundary are clipped,
                thus, layers only need to be of this size.
             */
            const basegfx::B2DRange  maPageBounds;

            /// Number of shape sprites currenly active on this LayerManager
            sal_Int32                mnActiveSprites;

            /// TRUE, if shapes might need to move to different layer
            bool                     mbLayerAssociationDirty;

            /// FALSE when deactivated
            bool                     mbActive;

            /** When true, all sprite animations run in the foreground.  That
                is, no extra layers are created, and the slideshow runs
                potentially faster.
             */
            bool                     mbDisableAnimationZOrder;
        };

        typedef ::boost::shared_ptr< LayerManager > LayerManagerSharedPtr;
    }
}

#endif /* INCLUDED_SLIDESHOW_LAYERMANAGER_HXX */
