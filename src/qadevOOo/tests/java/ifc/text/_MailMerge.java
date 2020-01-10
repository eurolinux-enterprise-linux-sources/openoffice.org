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

package ifc.text;

import lib.MultiPropertyTest;

import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.sdbc.XConnection;
import com.sun.star.sdbc.XResultSet;
import com.sun.star.uno.UnoRuntime;

public class _MailMerge extends MultiPropertyTest {

    /**
     * Custom tester for properties which contains URLs.
     * Switches between two valid folders
     */
    protected PropertyTester URLTester = new PropertyTester() {
        protected Object getNewValue(String propName, Object oldValue) {
            if (oldValue.equals(util.utils.getOfficeTemp((XMultiServiceFactory)tParam.getMSF())))
                return util.utils.getFullTestURL(""); else
                return util.utils.getOfficeTemp((XMultiServiceFactory)tParam.getMSF());
        }
    } ;

    /**
     * Custom tester for properties which contains document URLs.
     * Switches between two document URLs.
     */
    protected PropertyTester DocumentURLTester = new PropertyTester() {
        protected Object getNewValue(String propName, Object oldValue) {
            if (oldValue.equals(util.utils.getFullTestURL("MailMerge.sxw")))
                return util.utils.getFullTestURL("sForm.sxw"); else
                return util.utils.getFullTestURL("MailMerge.sxw");
        }
    } ;
    /**
     * Tested with custom property tester.
     */
    public void _ResultSet() {
        String propName = "ResultSet";
        try{

            log.println("try to get value from property...");
            XResultSet oldValue = (XResultSet) UnoRuntime.queryInterface(XResultSet.class,oObj.getPropertyValue(propName));

            log.println("try to get value from object relation...");
            XResultSet newValue = (XResultSet) UnoRuntime.queryInterface(XResultSet.class,tEnv.getObjRelation("MailMerge.XResultSet"));

            log.println("set property to a new value...");
            oObj.setPropertyValue(propName, newValue);
            
            log.println("get the new value...");
            XResultSet getValue = (XResultSet) UnoRuntime.queryInterface(XResultSet.class,oObj.getPropertyValue(propName));

            tRes.tested(propName, this.compare(newValue, getValue));
        } catch (com.sun.star.beans.PropertyVetoException e){
            log.println("could not set property '"+ propName +"' to a new value!");
            tRes.tested(propName, false);
        } catch (com.sun.star.lang.IllegalArgumentException e){
            log.println("could not set property '"+ propName +"' to a new value!");
            tRes.tested(propName, false);
        } catch (com.sun.star.beans.UnknownPropertyException e){
            if (this.isOptional(propName)){
                    // skipping optional property test
                    log.println("Property '" + propName
                            + "' is optional and not supported");
                    tRes.tested(propName,true);
                    
            } else {
                log.println("could not get property '"+ propName +"' from XPropertySet!");
                tRes.tested(propName, false);
            }
        } catch (com.sun.star.lang.WrappedTargetException e){
            log.println("could not get property '"+ propName +"' from XPropertySet!");
            tRes.tested(propName, false);
        }
    }

    /**
     * Tested with custom property tester.
     */

    public void _ActiveConnection() {
        String propName = "ActiveConnection";
        try{

            log.println("try to get value from property...");
            XConnection oldValue = (XConnection) UnoRuntime.queryInterface(XConnection.class,oObj.getPropertyValue(propName));

            log.println("try to get value from object relation...");
            XConnection newValue = (XConnection) UnoRuntime.queryInterface(XConnection.class,tEnv.getObjRelation("MailMerge.XConnection"));

            log.println("set property to a new value...");
            oObj.setPropertyValue(propName, newValue);
            
            log.println("get the new value...");
            XConnection getValue = (XConnection) UnoRuntime.queryInterface(XConnection.class,oObj.getPropertyValue(propName));

            tRes.tested(propName, this.compare(newValue, getValue));
        } catch (com.sun.star.beans.PropertyVetoException e){
            log.println("could not set property '"+ propName +"' to a new value! " + e.toString());
            tRes.tested(propName, false);
        } catch (com.sun.star.lang.IllegalArgumentException e){
            log.println("could not set property '"+ propName +"' to a new value! " + e.toString());
            tRes.tested(propName, false);
        } catch (com.sun.star.beans.UnknownPropertyException e){
            if (this.isOptional(propName)){
                    // skipping optional property test
                    log.println("Property '" + propName
                            + "' is optional and not supported");
                    tRes.tested(propName,true);
                    
            } else {
                log.println("could not get property '"+ propName +"' from XPropertySet!");
                tRes.tested(propName, false);
            }
        } catch (com.sun.star.lang.WrappedTargetException e){
            log.println("could not get property '"+ propName +"' from XPropertySet!");
            tRes.tested(propName, false);
        }
    }
    
    /**
     * Tested with custom property tester.
     */
    public void _DocumentURL() {
        log.println("Testing with custom Property tester") ;
        testProperty("DocumentURL", DocumentURLTester) ;
    }

    /**
     * Tested with custom property tester.
     */
    public void _OutputURL() {
        log.println("Testing with custom Property tester") ;
        testProperty("OutputURL", URLTester) ;
    }

    /**
    * Forces environment recreation.
    */
    protected void after() {
        disposeEnvironment();
    }


} //finish class _MailMerge

