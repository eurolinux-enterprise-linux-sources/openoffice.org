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

package ifc.sdb;

import com.sun.star.sdb.XSingleSelectQueryComposer;
import lib.MultiMethodTest;
import com.sun.star.sdb.XSingleSelectQueryAnalyzer;
import com.sun.star.uno.AnyConverter;
import com.sun.star.uno.UnoRuntime;
import lib.StatusException;
import lib.Status;
import com.sun.star.beans.PropertyValue;
import com.sun.star.beans.XPropertySet;

/**
* Testing <code>com.sun.star.sdb.XSingleSelectQueryComposer</code>
* interface methods :
* <ul>
*  <li><code>setFilter()</code></li>
*  <li><code>setStructuredFilter()</code></li>
*  <li><code>appendFilterByColumn()</code></li>
*  <li><code>appendGroupByColumn()</code></li>
*  <li><code>setGroup()</code></li>
*  <li><code>setHavingClause()</code></li>
*  <li><code>setStructuredHavingClause()</code></li>
*  <li><code>appendHavingClauseByColumn()</code></li>
*  <li><code>appendOrderByColumn()</code></li>
*  <li><code>setOrder()</code></li>

* </ul> <p>
* @see com.sun.star.sdb.XSingleSelectQueryComposer
*/
public class _XSingleSelectQueryComposer extends MultiMethodTest {

    // oObj filled by MultiMethodTest
    public XSingleSelectQueryComposer oObj = null ;

    private String queryString = "SELECT * FROM \"biblio\"";
    
    private XSingleSelectQueryAnalyzer xQueryAna = null;
    
    private XPropertySet xProp = null;
    
    private String colName = null;
    
    /**
     * Retcieves the object relations:
    * <ul>
    *  <li><code>XSingleSelectQueryAnalyzer xQueryAna</code></li>
    *  <li><code>XPropertySet xProp</code></li>
    *  <li><code>String colName</code></li>
    * </ul> <p>
     * @see om.sun.star.sdb.XSingleSelectQueryAnalyzer
     * @see com.sun.star.beans.XPropertySet
     */
    protected void before() throws Exception {

        xQueryAna = (XSingleSelectQueryAnalyzer)
                      UnoRuntime.queryInterface(XSingleSelectQueryAnalyzer.class,
                      tEnv.getObjRelation("xQueryAna"));

        if (xQueryAna == null) {
            throw new StatusException(Status.failed(
           "Couldn't get object relation 'xQueryAna'. Test must be modified"));
            
        }
    
        xProp = (XPropertySet)
                      UnoRuntime.queryInterface(XPropertySet.class,
                      tEnv.getObjRelation("xProp"));

        if (xProp == null) {
            throw new StatusException(Status.failed(
           "Couldn't get object relation 'xProp'. Test must be modified"));
            
        }
    
        colName = AnyConverter.toString(tEnv.getObjRelation("colName"));

        if (colName == null) {
            throw new StatusException(Status.failed(
           "Couldn't get object relation 'colName'. Test must be modified"));
            
        }
    
    }
    
    
    /**
    * Object relation <code>xQueryAna</code> set a filter. This filter
    * must returned while calling <code>getFilter</code>
    */
    public void _setFilter() {
        try{
            String filter = "\"Identifier\" = 'BOR02b'";
            oObj.setFilter(filter);
            tRes.tested("setFilter()", (xQueryAna.getFilter().equals(filter)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setFilter()", false);
        }
    }
    
    /**
    * Object relation <code>xQueryAna</code> set a complex filter with method
    . <code>setFilter</code>. Then <code>getStructuredFilter</code> returns a
    * sequenze of <code>PropertyValue</code> which was set with method
    * <code>setStructuredFilter</code> from <code>xQueryAna</code>.
    * Then test has ok status if <code>getFilter</code> returns the complex filter.
    */
    public void _setStructuredFilter() {
        requiredMethod("setFilter()");
        try{
            xQueryAna.setQuery("SELECT \"Identifier\", \"Type\", \"Address\" FROM \"biblio\" \"biblio\"");
            String complexFilter = "( \"Identifier\" = '1' AND \"Type\" = '4' ) OR ( \"Identifier\" = '2' AND \"Type\" = '5' ) OR ( \"Identifier\" = '3' AND \"Type\" = '6' AND \"Address\" = '7' ) OR ( \"Address\" = '8' ) OR ( \"Type\" = '9' )";
            oObj.setFilter(complexFilter);
            PropertyValue[][] aStructuredFilter = xQueryAna.getStructuredFilter();
            oObj.setFilter("");
            oObj.setStructuredFilter(aStructuredFilter);
            tRes.tested("setStructuredFilter()", (xQueryAna.getFilter().equals(complexFilter)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setStructuredFilter()", false);
        } catch (com.sun.star.lang.IllegalArgumentException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setStructuredFilter()", false);
        }
    }
    
    /**
    * At first the object relation <code>xProp</code> was set as parameter. 
    * Relation <code>xQueryAna</code> was used to chek if realtion 
    * <code>colName</code> was found.
    * Second an empty <code>XPropertySet</code> was used as parameter. A
    * <code>com.sun.star.sdbc.SQLException</code> must be thrown.
    */
    public void _appendFilterByColumn() {
        boolean ok = true;
        try{
            
            oObj.appendFilterByColumn(xProp, true);
            log.println("appendFilterByColumn: " + xQueryAna.getFilter());
            ok = ok && (xQueryAna.getFilter().indexOf(colName) > 0);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("appendFilterByColumn()", false);
        }

        try{
            
            oObj.appendFilterByColumn(xProp, false);
            log.println("appendFilterByColumn: " + xQueryAna.getFilter());
            ok = ok && (xQueryAna.getFilter().indexOf(colName) > 0);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("appendFilterByColumn()", false);
        }

        try{
            XPropertySet dummy = null;
            oObj.appendFilterByColumn(dummy, true);
            log.println("unexpected Exception was not thorwn");
            tRes.tested("appendFilterByColumn()", false);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("expected Exception");
            ok = ok && true;
        }
        tRes.tested("appendFilterByColumn()", ok);
    }

    /**
    * At first the object relation <code>xProp</code> was used as parameter.
    * Relation <code>xQueryAna</code> was used to chek if realtion
    * <code>colName</code> was found.
    * Second an empty <code>XPropertySet</code> was used as parameter. An
    * <code>com.sun.star.sdbc.SQLException</code> must be thrown.
    */
    public void _appendGroupByColumn() {
        boolean ok = true;
        try{
            
            oObj.appendGroupByColumn(xProp);
            log.println("appendGroupByColumn: " + xQueryAna.getFilter());
            ok = ok && (xQueryAna.getFilter().indexOf(colName) > 0);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("appendGroupByColumn()", false);
        }
        try{
            XPropertySet dummy = null;
            oObj.appendGroupByColumn(dummy);
            log.println("unexpected Exception was not thorwn");
            tRes.tested("appendGroupByColumn()", false);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("expected Exception");
            ok = ok && true;
        }
        tRes.tested("appendGroupByColumn()", ok); 
    }

    /**
    * The group which was setted by <code>setGroup</code> must be returned
    * while calling from object relation <code>XQueryAna</code> 
    * method <code>getGroup</code>
    */
    public void _setGroup() {
        try{
            String group = "\"Identifier\"";
            oObj.setGroup(group);
            tRes.tested("setGroup()", (xQueryAna.getGroup().equals(group)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setGroup()", false);
        }
    }
    
    
    /**
    * The cluase which was setted by <code>setHavingClause</code> must be returned
    * while calling from object relation <code>XQueryAna</code> 
    * method <code>getHavingClause</code>
    */
    public void _setHavingClause() {
        try{
            String clause = "\"Identifier\" = 'BOR02b'";
            oObj.setHavingClause(clause);
            tRes.tested("setHavingClause()", (
                                   xQueryAna.getHavingClause().equals(clause)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setHavingClause()", false);
        }
    }
    
    /**
    * At first <code>setHavingClause</code> sets a complex clause.
    * Then method <code>getStructuredHavingClause</code> from object relation
    * <code>xQueryAna</code> returns a valid <code>PropertyValue[][]</code> 
    * Method <code>setHavingClause</code> was called with an empty sting to
    * reset filter. Now <code>setStructuredHavingClause</code> with the valid
    * <code>PropertyValue[][]</code> as parameter was called.
    * Test is ok if <code>getHavingClause</code> from <code>xQueryAna</code>
    * returns the complex clause from beginning.
    * <p>
    * required methods:
    *<ul>
    * <li><code>setHavingClause</code></li>
    * <li><code>setStructuredFilter</code></li>
    *</ul>
    */
    public void _setStructuredHavingClause() {
        requiredMethod("setHavingClause()");
        executeMethod("setStructuredFilter()");
        String complexFilter = "( \"Identifier\" = '1' AND \"Type\" = '4' ) OR ( \"Identifier\" = '2' AND \"Type\" = '5' ) OR ( \"Identifier\" = '3' AND \"Type\" = '6' AND \"Address\" = '7' ) OR ( \"Address\" = '8' ) OR ( \"Type\" = '9' )";
        
        try{
           oObj.setHavingClause(complexFilter);
           PropertyValue[][] aStructuredHaving = 
                                          xQueryAna.getStructuredHavingClause();
           oObj.setHavingClause("");
           oObj.setStructuredHavingClause(aStructuredHaving);
           tRes.tested("setStructuredHavingClause()", 
                           (xQueryAna.getHavingClause().equals(complexFilter)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setStructuredHavingClause()", false);
        }
    }

    /**
    * First object relation <code>xProp</code> was used as parameter. Relation
    * <code>xQueryAna</code> was used to chek if realtion <code>colName</code>
    * was found.
    * Second an empty <code>XPropertySet</code> was given as parameter. An
    * <code>com.sun.star.sdbc.SQLException</code> must be thrown.
    */
    public void _appendHavingClauseByColumn() {
        boolean ok = true;
        try{
            
            oObj.appendHavingClauseByColumn(xProp, true);
            log.println("appendHavingClauseByColumn: " + xQueryAna.getFilter());
            ok = ok && (xQueryAna.getFilter().indexOf(colName) > 0);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("appendHavingClauseByColumn()", false);
        }
        try{
            XPropertySet dummy = null;
            oObj.appendHavingClauseByColumn(dummy, true);
            log.println("unexpected Exception was not thorwn");
            tRes.tested("appendHavingClauseByColumn()", false);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("expected Exception");
            ok = ok && true;
        }
        tRes.tested("appendHavingClauseByColumn()", ok);
    }

    /**
    * First object relation <code>xProp</code> was set as parameter. Relation
    * <code>xQueryAna</code> was used to chek if realtion <code>colName</code>
    * was found.
    * Second an empty <code>XPropertySet</code> was given as parameter. An
    * <code>com.sun.star.sdbc.SQLException</code> must be thrown.
    */
    public void _appendOrderByColumn() {
        boolean ok = true;
        try{
            
            oObj.appendOrderByColumn(xProp, true);
            log.println("appendOrderByColumn: " + xQueryAna.getFilter());
            ok = ok && (xQueryAna.getFilter().indexOf(colName) > 0);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("appendOrderByColumn()", false);
        }
        try{
            XPropertySet dummy = null;
            oObj.appendOrderByColumn(dummy, true);
            log.println("unexpected Exception was not thorwn");
            tRes.tested("appendOrderByColumn()", false);
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("expected Exception");
            ok = ok && true;
        }
        tRes.tested("appendOrderByColumn()", ok);
    }


    /**
    * Method <code>getOrder</code> from object relation <code>xQueryAna</code>
    * checks the order which was setted while calling <code>setOrder</code>
    */
    public void _setOrder() {
        try{
            String order = "\"Identifier\"";
            oObj.setOrder(order);
            tRes.tested("setOrder()", (xQueryAna.getOrder().equals(order)));
            
        } catch (com.sun.star.sdbc.SQLException e){
            log.println("unexpected Exception: " + e.toString());
            tRes.tested("setOrder()", false);
        }
    }
    
    

}  // finish class _XSingleSelectQueryComposer
