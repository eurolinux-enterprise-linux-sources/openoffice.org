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

package mod._sd;

import java.io.PrintWriter;
import java.util.Comparator;

import lib.Status;
import lib.StatusException;
import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.DesktopTools;
import util.SOfficeFactory;
import util.utils;

import com.sun.star.awt.XWindow;
import com.sun.star.container.XIndexAccess;
import com.sun.star.drawing.XDrawPage;
import com.sun.star.drawing.XDrawPages;
import com.sun.star.drawing.XDrawPagesSupplier;
import com.sun.star.drawing.XShape;
import com.sun.star.drawing.XShapes;
import com.sun.star.frame.XController;
import com.sun.star.frame.XDesktop;
import com.sun.star.frame.XFrame;
import com.sun.star.frame.XModel;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.util.XModifiable;

/**
* Test for object which is represented by service
* <code>com.sun.star.drawing.DrawingDocumentDrawView</code>. <p>
* Object implements the following interfaces :
* <ul>
*  <li> <code>com::sun::star::drawing::DrawingDocumentDrawView</code></li>
*  <li> <code>com::sun::star::lang::XComponent</code></li>
*  <li> <code>com::sun::star::lang::XServiceInfo</code></li>
*  <li> <code>com::sun::star::frame::XController</code></li>
*  <li> <code>com::sun::star::beans::XPropertySet</code></li>
*  <li> <code>com::sun::star::view::XSelectionSupplier</code></li>
*  <li> <code>com::sun::star::drawing::XDrawView</code></li>
* </ul>
* @see com.sun.star.drawing.DrawingDocumentDrawView
* @see com.sun.star.lang.XComponent
* @see com.sun.star.lang.XServiceInfo
* @see com.sun.star.frame.XController
* @see com.sun.star.beans.XPropertySet
* @see com.sun.star.view.XSelectionSupplier
* @see com.sun.star.drawing.XDrawView
* @see ifc.drawing._DrawingDocumentDrawView
* @see ifc.lang._XComponent
* @see ifc.lang._XServiceInfo
* @see ifc.frame._XController
* @see ifc.beans._XPropertySet
* @see ifc.view._XSelectionSupplier
* @see ifc.drawing._XDrawView
*/
public class DrawController_DrawView extends TestCase {
    static XDesktop the_Desk;
    static XComponent xDrawDoc;
    static XComponent xSecondDrawDoc;

    /**
    * Creates the instance of the service
    * <code>com.sun.star.frame.Desktop</code>.
    * @see com.sun.star.frame.Desktop
    */
    protected void initialize(TestParameters Param, PrintWriter log) {
        the_Desk = (XDesktop)
            UnoRuntime.queryInterface(
                XDesktop.class, DesktopTools.createDesktop(
                                    (XMultiServiceFactory)Param.getMSF()) );
    }

    /**
    * Called while disposing a <code>TestEnvironment</code>.
    * Disposes Impress documents.
    * @param tParam test parameters
    * @param tEnv the environment to cleanup
    * @param log writer to log information while testing
    */
    protected void cleanup( TestParameters Param, PrintWriter log) {
        log.println("disposing impress documents");
        util.DesktopTools.closeDoc(xDrawDoc);
        util.DesktopTools.closeDoc(xSecondDrawDoc);
    }

    /**
    * Creating a Testenvironment for the interfaces to be tested.
    * Creates two impress documents. After creating
    * of the documents makes short
    * wait to allow frames to be loaded. Retrieves
    * the collection of the draw pages
    * from the first document and takes one of them. Inserts some shapes to the
    * retrieved draw page. Obtains a current controller from the first document
    * using the interface <code>XModel</code>. The obtained controller is the
    * instance of the service
    * <code>com.sun.star.drawing.DrawingDocumentDrawView</code>.
    * Object relations created :
    * <ul>
    *  <li> <code>'Pages'</code> for
    *      {@link ifc.drawing._XDrawView}(the retrieved collection of the draw
    *      pages) </li>
    *  <li> <code>'FirstModel'</code> for
    *      {@link ifc.frame._XController}(the interface <code>XModel</code> of
    *      the first created document) </li>
    *  <li> <code>'Frame'</code> for
    *      {@link ifc.frame._XController}(the frame of the created
    *      document) </li>
    *  <li> <code>'SecondModel'</code> for
    *      {@link ifc.frame._XController}(the interface <code>XModel</code> of
    *      the second created document) </li>
    *  <li> <code>'SecondController'</code> for
    *      {@link ifc.frame._XController}(the current controller of the second
    *      created document) </li>
    *  <li> <code>'DrawPage'</code> for
    *      {@link ifc.drawing.DrawingDocumentDrawView}(the draw page which will
    *      be new current page) </li>
    * </ul>
    * @see com.sun.star.frame.XModel
    * @see com.sun.star.drawing.DrawingDocumentDrawView
    */
    protected synchronized TestEnvironment createTestEnvironment
            (TestParameters Param, PrintWriter log) {

        log.println( "creating a test environment" );

        // get a soffice factory object
        SOfficeFactory SOF = SOfficeFactory.getFactory(
                                    (XMultiServiceFactory)Param.getMSF());

        try {
            log.println( "creating two impress documents" );
            xDrawDoc = SOF.createDrawDoc(null);
            shortWait();
            xSecondDrawDoc = SOF.createDrawDoc(null);
            shortWait();
        } catch (com.sun.star.uno.Exception e) {
            e.printStackTrace( log );
            throw new StatusException("Couldn't create document", e);
        }

        // get the drawpage of drawing here
        log.println( "getting Drawpage" );
        XDrawPagesSupplier oDPS = (XDrawPagesSupplier)
            UnoRuntime.queryInterface(XDrawPagesSupplier.class, xDrawDoc);
        XDrawPages the_pages = oDPS.getDrawPages();
        XIndexAccess oDPi = (XIndexAccess)
            UnoRuntime.queryInterface(XIndexAccess.class,the_pages);

        XDrawPage oDrawPage = null;
        try {
            oDrawPage = (XDrawPage) AnyConverter.toObject(
                        new Type(XDrawPage.class),oDPi.getByIndex(0));
        } catch (com.sun.star.lang.WrappedTargetException e) {
            e.printStackTrace( log );
            throw new StatusException("Couldn't get DrawPage", e);
        } catch (com.sun.star.lang.IndexOutOfBoundsException e) {
            e.printStackTrace( log );
            throw new StatusException("Couldn't get DrawPage", e);
        } catch (com.sun.star.lang.IllegalArgumentException e) {
            e.printStackTrace( log );
            throw new StatusException("Couldn't get DrawPage", e);
        }

        //put something on the drawpage
        log.println( "inserting some Shapes" );
        XShapes oShapes = (XShapes)
            UnoRuntime.queryInterface(XShapes.class, oDrawPage);
        XShape shape1 = SOF.createShape(
            xDrawDoc, 3000, 4500, 15000, 1000, "Ellipse");
        XShape shape2 = SOF.createShape(
            xDrawDoc, 5000, 3500, 7500, 5000, "Rectangle");
        XShape shape3 = SOF.createShape(
            xDrawDoc, 3000, 500, 5000, 1000, "Line");
        oShapes.add(shape1);
        oShapes.add(shape2);
        oShapes.add(shape3);
        shortWait();

        XModel aModel = (XModel)
            UnoRuntime.queryInterface(XModel.class, xDrawDoc);

        XInterface oObj = aModel.getCurrentController();

        XModel aModel2 = (XModel)
            UnoRuntime.queryInterface(XModel.class, xSecondDrawDoc);

        XWindow anotherWindow = (XWindow) UnoRuntime.queryInterface(
                                XWindow.class,aModel2.getCurrentController());

        log.println( "creating a new environment for impress view object" );
        TestEnvironment tEnv = new TestEnvironment( oObj );

        if (anotherWindow != null) {
            tEnv.addObjRelation("XWindow.AnotherWindow",anotherWindow);
        }

        Object oShapeCol1 = null;
        Object oShapeCol2 = null;
        try {
            oShapeCol1 = ((XMultiServiceFactory)Param.getMSF()).
                createInstance("com.sun.star.drawing.ShapeCollection");
            oShapeCol2 = ((XMultiServiceFactory)Param.getMSF()).
                createInstance("com.sun.star.drawing.ShapeCollection");
        } catch(com.sun.star.uno.Exception e) {
            e.printStackTrace(log);
            throw new StatusException(Status.failed("Couldn't create instance"));
        }

        XShapes xShapes1 = (XShapes)
            UnoRuntime.queryInterface(XShapes.class, oShapeCol1);
        XShapes xShapes2 = (XShapes)
            UnoRuntime.queryInterface(XShapes.class, oShapeCol2);
        xShapes1.add(shape2);
        xShapes1.add(shape3);
        xShapes2.add(shape1);
        shortWait();


        tEnv.addObjRelation("Selections", new Object[] {
            oDrawPage, oShapeCol1, oShapeCol2});
        tEnv.addObjRelation("Comparer", new Comparator() {
            public int compare(Object o1, Object o2) {
                XIndexAccess indAc1 = (XIndexAccess)
                    UnoRuntime.queryInterface(XIndexAccess.class, o1);
                XIndexAccess indAc2 = (XIndexAccess)
                    UnoRuntime.queryInterface(XIndexAccess.class, o2);
                if (indAc1 == null || indAc2 == null) return -1;
                if (indAc1.getCount() == indAc2.getCount()) {
                    return 0;
                }
                return 1;
            }
            public boolean equals(Object obj) {
                return compare(this, obj) == 0;
            } });



        tEnv.addObjRelation("Pages", the_pages);

        //Adding ObjRelations for XController
        tEnv.addObjRelation("FirstModel", aModel);

        tEnv.addObjRelation("XUserInputInterception.XModel", aModel);
        
        XFrame the_frame = the_Desk.getCurrentFrame();
        tEnv.addObjRelation("Frame", the_frame);

         aModel = (XModel)
            UnoRuntime.queryInterface(XModel.class, xSecondDrawDoc);
        //Adding ObjRelations for XController
        tEnv.addObjRelation("SecondModel", aModel);

        XController secondController = aModel.getCurrentController();
        tEnv.addObjRelation("SecondController", secondController);
        tEnv.addObjRelation("XDispatchProvider.URL",
                                    "slot:27009");

        //Adding relations for DrawingDocumentDrawView
        XDrawPage new_page = the_pages.insertNewByIndex(1);
        tEnv.addObjRelation("DrawPage", new_page);

        log.println("Implementation Name: "+utils.getImplName(oObj));

        XModifiable modify = (XModifiable)
            UnoRuntime.queryInterface(XModifiable.class,xDrawDoc);

        tEnv.addObjRelation("Modifiable",modify);
        
        tEnv.addObjRelation("XComponent.DisposeThis", xDrawDoc);

        return tEnv;

    } // finish method getTestEnvironment

    private void shortWait() {
        try {
            Thread.sleep(1000) ;
        } catch (InterruptedException e) {
            System.out.println("While waiting :" + e) ;
        }
    }


}

