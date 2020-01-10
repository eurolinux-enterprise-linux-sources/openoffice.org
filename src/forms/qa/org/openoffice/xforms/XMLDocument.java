/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.openoffice.xforms;

import com.sun.star.container.NoSuchElementException;
import com.sun.star.container.XNameContainer;
import com.sun.star.lang.WrappedTargetException;
import com.sun.star.lang.XComponent;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.uno.Exception;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.xforms.XFormsSupplier;
import com.sun.star.xforms.XFormsUIHelper1;
import com.sun.star.xforms.XModel;
import integration.forms.DocumentType;

/**
 *
 * @author fs93730
 */
public class XMLDocument extends integration.forms.DocumentHelper
{
    private XFormsSupplier  m_formsSupplier;
    private XNameContainer  m_forms;

    /* ------------------------------------------------------------------ */
    public XMLDocument( XMultiServiceFactory _orb ) throws Exception
    {
        super( _orb, implLoadAsComponent( _orb, getDocumentFactoryURL( DocumentType.XMLFORM ) ) );
        impl_initialize( getDocument() );
    }

    /* ------------------------------------------------------------------ */
    public XMLDocument( XMultiServiceFactory _orb, XComponent _document )
    {
        super( _orb, _document );
        impl_initialize( _document );
    }

    /* ------------------------------------------------------------------ */
    private void impl_initialize( XComponent _document )
    {
        m_formsSupplier = (XFormsSupplier)UnoRuntime.queryInterface( XFormsSupplier.class,
            _document );

        if ( m_formsSupplier == null )
            throw new IllegalArgumentException();

        m_forms = m_formsSupplier.getXForms();
    }

    /* ------------------------------------------------------------------ */
    public String[] getXFormModelNames()
    {
        return m_forms.getElementNames();
    }

    /* ------------------------------------------------------------------ */
    public Model getXFormModel( String _modelName ) throws NoSuchElementException
    {
        try
        {
            return new Model(m_forms.getByName(_modelName));
        }
        catch (WrappedTargetException ex)
        {
            throw new NoSuchElementException();
        }
    }

    /* ------------------------------------------------------------------ */
    public Model addXFormModel( String _modelName )
    {
        XModel newModel = null;
        try
        {
            newModel = (XModel) UnoRuntime.queryInterface( XModel.class,
                getOrb().createInstance( "com.sun.star.xforms.Model" ) );
            newModel.setID(_modelName);
            XFormsUIHelper1 modelHelper = (XFormsUIHelper1) UnoRuntime.queryInterface(
                XFormsUIHelper1.class, newModel );
            modelHelper.newInstance( "Instance 1", new String(), true );
            newModel.initialize();

            m_forms.insertByName(_modelName, newModel);
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
        return new Model( newModel );
    }
}
