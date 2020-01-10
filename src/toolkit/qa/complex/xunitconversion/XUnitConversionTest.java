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

package complex.xunitconversion;

import com.sun.star.awt.XUnitConversion;
import com.sun.star.uno.UnoRuntime;
import complexlib.ComplexTestCase;
import com.sun.star.awt.XWindow;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.awt.XWindowPeer;

import util.DesktopTools;

/**
 * This complex test is only for testing the com.sun.star.awt.XUnitConversion methods
 * These are converter methods to get the size of a well known awt component
 * in a com.sun.star.util.MeasureUnit you want.
 * You don't need to know the factors to calculate by hand.
 * 
 * @author ll93751
 */
public class XUnitConversionTest extends ComplexTestCase 
{
    public String[] getTestMethodNames()
        {
            return new String[]{"testXUnitConversion"}; // function name of the test method
        }

        /**
         * returns the delta value between a and b
         * @param a
         * @param b
         * @return
         */
    private int delta(int a, int b)
    {
        final int n = Math.abs(a - b);
        return n;
    }

    private XUnitConversion m_xConversion = null;

    /**
 * Not really a check,
 * only a simple test call to convertSizeToLogic(...) with different parameters
 * @param _aSize
 * @param _aMeasureUnit
 * @param _sEinheit
 */
    private void checkSize(com.sun.star.awt.Size _aSize, short _aMeasureUnit, String _sEinheit)
    {
        try
        {
            com.sun.star.awt.Size aSizeIn = m_xConversion.convertSizeToLogic(_aSize, _aMeasureUnit);
            log.println("Window size:");
            log.println("Width:" + aSizeIn.Width + " " + _sEinheit);
            log.println("Height:" + aSizeIn.Height + " " + _sEinheit);
            log.println("");
        }
        catch (com.sun.star.lang.IllegalArgumentException e)
        {
            log.println("Caught IllegalArgumentException in convertSizeToLogic with '" + _sEinheit + "' " + e.getMessage());
        }
    }
    
/**
 * The real test function
 * 1. try to get the XMultiServiceFactory of an already running office. Therefore make sure an (open|star)office is running with
 *    parameters like -accept="socket,host=localhost,port=8100;urp;"
 * 2. try to create an empty window
 * 3. try to convert the WindowPeer to an XWindow
 * 4. try to resize and move the window to an other position, so we get a well knowing position and size.
 * 5. run some more tests
 * 
 * If no test fails, the test is well done and returns with 'PASSED, OK'
 * 
 */    public void testXUnitConversion() 
        {
            XMultiServiceFactory xMSF = (XMultiServiceFactory) param.getMSF();
            assure("failed: There is no office.", xMSF != null);

            // create a window
            XWindowPeer xWindowPeer = DesktopTools.createFloatingWindow(xMSF);
            assure("failed: there is no window peer", xWindowPeer != null);


            // resize and move the window to a well known position and size
            XWindow xWindow = (XWindow) UnoRuntime.queryInterface(XWindow.class, xWindowPeer);
            assure("failed: there is no window, cast wrong?", xWindow != null);

            xWindow.setVisible(Boolean.TRUE);

            int x = 100;
            int y = 100;
            int width = 640;
            int height = 480;
            xWindow.setPosSize(x, y, width, height, com.sun.star.awt.PosSize.POSSIZE);

            com.sun.star.awt.Rectangle aRect = xWindow.getPosSize();
            com.sun.star.awt.Point aPoint = new com.sun.star.awt.Point(aRect.X, aRect.Y);
            com.sun.star.awt.Size aSize = new com.sun.star.awt.Size(aRect.Width, aRect.Height);

            log.println("Window position and size in pixel:");
            log.println("X:" + aPoint.X);
            log.println("Y:" + aPoint.Y);
            log.println("Width:" + aSize.Width);
            log.println("Height:" + aSize.Height);
            log.println("");

            assure("Window pos size wrong", aSize.Width == width && aSize.Height == height && aPoint.X == x && aPoint.Y == y);

            // XToolkit aToolkit = xWindowPeer.getToolkit();
            m_xConversion = (XUnitConversion) UnoRuntime.queryInterface(XUnitConversion.class, xWindowPeer);

            // try to get the position of the window in 1/100mm with the XUnitConversion method
            try
            {
                com.sun.star.awt.Point aPointInMM_100TH = m_xConversion.convertPointToLogic(aPoint, com.sun.star.util.MeasureUnit.MM_100TH);
                log.println("Window position:");
                log.println("X:" + aPointInMM_100TH.X + " 1/100mm");
                log.println("Y:" + aPointInMM_100TH.Y + " 1/100mm");
                log.println("");
            }
            catch (com.sun.star.lang.IllegalArgumentException e)
            {
                assure("failed: IllegalArgumentException caught in convertPointToLogic " + e.getMessage(), Boolean.FALSE);
            }

            // try to get the size of the window in 1/100mm with the XUnitConversion method
            com.sun.star.awt.Size aSizeInMM_100TH = null;
            com.sun.star.awt.Size aSizeInMM_10TH = null;
            try
            {
                aSizeInMM_100TH = m_xConversion.convertSizeToLogic(aSize, com.sun.star.util.MeasureUnit.MM_100TH);
                log.println("Window size:");
                log.println("Width:" + aSizeInMM_100TH.Width + " 1/100mm");
                log.println("Height:" + aSizeInMM_100TH.Height + " 1/100mm");
                log.println("");

                // try to get the size of the window in 1/10mm with the XUnitConversion method

                aSizeInMM_10TH = m_xConversion.convertSizeToLogic(aSize, com.sun.star.util.MeasureUnit.MM_10TH);
                log.println("Window size:");
                log.println("Width:" + aSizeInMM_10TH.Width + " 1/10mm");
                log.println("Height:" + aSizeInMM_10TH.Height + " 1/10mm");
                log.println("");

                // check the size with a delta which must be smaller a given difference
                assure("Size.Width  not correct", delta(aSizeInMM_100TH.Width, aSizeInMM_10TH.Width * 10) < 10);
                assure("Size.Height not correct", delta(aSizeInMM_100TH.Height, aSizeInMM_10TH.Height * 10) < 10);

                // new
                checkSize(aSize, com.sun.star.util.MeasureUnit.PIXEL, "pixel");
                checkSize(aSize, com.sun.star.util.MeasureUnit.APPFONT, "appfont");
                checkSize(aSize, com.sun.star.util.MeasureUnit.SYSFONT, "sysfont");

                // simply check some more parameters
                checkSize(aSize, com.sun.star.util.MeasureUnit.MM, "mm");
                checkSize(aSize, com.sun.star.util.MeasureUnit.CM, "cm");
                checkSize(aSize, com.sun.star.util.MeasureUnit.INCH_1000TH, "1/1000inch");
                checkSize(aSize, com.sun.star.util.MeasureUnit.INCH_100TH, "1/100inch");
                checkSize(aSize, com.sun.star.util.MeasureUnit.INCH_10TH, "1/10inch");
                checkSize(aSize, com.sun.star.util.MeasureUnit.INCH, "inch");
                // checkSize(aSize, com.sun.star.util.MeasureUnit.M, "m");
                checkSize(aSize, com.sun.star.util.MeasureUnit.POINT, "point");
                checkSize(aSize, com.sun.star.util.MeasureUnit.TWIP, "twip");
                // checkSize(aSize, com.sun.star.util.MeasureUnit.KM, "km");
                // checkSize(aSize, com.sun.star.util.MeasureUnit.PICA, "pica");
                // checkSize(aSize, com.sun.star.util.MeasureUnit.FOOT, "foot");
                // checkSize(aSize, com.sun.star.util.MeasureUnit.MILE, "mile");
            }
            catch (com.sun.star.lang.IllegalArgumentException e)
            {
                assure("failed: IllegalArgumentException caught in convertSizeToLogic " + e.getMessage(), Boolean.FALSE);
            }

            // convert the 1/100mm window size back to pixel
            try
            {
                com.sun.star.awt.Size aNewSize = m_xConversion.convertSizeToPixel(aSizeInMM_100TH, com.sun.star.util.MeasureUnit.MM_100TH);
                log.println("Window size:");
                log.println("Width:" + aNewSize.Width + " pixel");
                log.println("Height:" + aNewSize.Height + " pixel");

                // assure the pixels are the same as we already know
                assure("failed: Size from pixel to 1/100mm to pixel", aSize.Width == aNewSize.Width && aSize.Height == aNewSize.Height);
            }
            catch (com.sun.star.lang.IllegalArgumentException e)
            {
                assure("failed: IllegalArgumentException caught in convertSizeToPixel " + e.getMessage(), Boolean.FALSE);
            }

            // close the window.
            // IMHO a little bit stupid, but the XWindow doesn't support a XCloseable interface
            xWindow.dispose();
    } 
}
