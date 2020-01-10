package complex.storages;

import com.sun.star.uno.XInterface;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.lang.XSingleServiceFactory;

import com.sun.star.bridge.XUnoUrlResolver;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.io.XStream;
import com.sun.star.io.XInputStream;

import com.sun.star.embed.*;

import share.LogWriter;
import complex.storages.TestHelper;
import complex.storages.StorageTest;

public class RegressionTest_i30677 implements StorageTest {

	XMultiServiceFactory m_xMSF;
	XSingleServiceFactory m_xStorageFactory;
	TestHelper m_aTestHelper;

	public RegressionTest_i30677( XMultiServiceFactory xMSF, XSingleServiceFactory xStorageFactory, LogWriter aLogWriter )
	{
		m_xMSF = xMSF;
		m_xStorageFactory = xStorageFactory;
		m_aTestHelper = new TestHelper( aLogWriter, "RegressionTest_i30677: " );
	}

    public boolean test()
	{
		try
		{
			XStream xTempFileStream = m_aTestHelper.CreateTempFileStream( m_xMSF );
			if ( xTempFileStream == null )
				return false;
		
			// create storage based on the temporary stream
			Object pArgs[] = new Object[2];
			pArgs[0] = (Object) xTempFileStream;
			pArgs[1] = new Integer( ElementModes.WRITE );

			Object oTempStorage = m_xStorageFactory.createInstanceWithArguments( pArgs );
			XStorage xTempStorage = (XStorage) UnoRuntime.queryInterface( XStorage.class, oTempStorage );
			if ( xTempStorage == null )
			{
				m_aTestHelper.Error( "Can't create temporary storage representation!" );
				return false;
			}

			// open a new substorage
			XStorage xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
																	"SubStorage1",
																	ElementModes.WRITE );
			if ( xTempSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}
	
			// open a new subsubstorage
			XStorage xTempSubSubStorage = m_aTestHelper.openSubStorage( xTempSubStorage,
																		"SubSubStorage1",
																		ElementModes.WRITE );
			if ( xTempSubSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}

			byte pBytes1[] = { 1, 1, 1, 1, 1 };

			// open a new substream, set "MediaType" and "Compressed" properties to it and write some bytes
			if ( !m_aTestHelper.WriteBytesToSubstream( xTempSubSubStorage, "SubSubStream1", "MediaType1", true, pBytes1 ) )
				return false;

			// set "MediaType" property for storages and check that "IsRoot" and "OpenMode" properties are set correctly
			if ( !m_aTestHelper.setStorageTypeAndCheckProps( xTempStorage,
															"MediaType2",
															true,
															ElementModes.WRITE ) )
				return false;

			// set "MediaType" property for storages and check that "IsRoot" and "OpenMode" properties are set correctly
			if ( !m_aTestHelper.setStorageTypeAndCheckProps( xTempSubStorage,
															"MediaType3",
															false,
															ElementModes.WRITE ) )
				return false;

			// set "MediaType" property for storages and check that "IsRoot" and "OpenMode" properties are set correctly
			if ( !m_aTestHelper.setStorageTypeAndCheckProps( xTempSubSubStorage,
															"MediaType4",
															false,
															ElementModes.WRITE ) )
				return false;

			// ================================================
			// commit the storages
			// ================================================

			// commit lowlevel substorage first
			if ( !m_aTestHelper.commitStorage( xTempSubSubStorage ) )
				return false;

			// commit substorage
			if ( !m_aTestHelper.commitStorage( xTempSubStorage ) )
				return false;

			// commit substorage to let the renaming take place
			if ( !m_aTestHelper.commitStorage( xTempStorage ) )
				return false;

			// ================================================
			// dispose the storages
			// ================================================

			// dispose lowerest substorage
			if ( !m_aTestHelper.disposeStorage( xTempSubSubStorage ) )
				return false;

			// dispose substorage
			if ( !m_aTestHelper.disposeStorage( xTempSubStorage ) )
				return false;

			// dispose the temporary storage
			if ( !m_aTestHelper.disposeStorage( xTempStorage ) )
				return false;

			// ================================================
			// reopen the storage and rewrite the stream
			// ================================================

			oTempStorage = m_xStorageFactory.createInstanceWithArguments( pArgs );
			xTempStorage = (XStorage) UnoRuntime.queryInterface( XStorage.class, oTempStorage );
			if ( xTempStorage == null )
			{
				m_aTestHelper.Error( "Can't create temporary storage representation!" );
				return false;
			}

			// open the substorages

			xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
															"SubStorage1",
															ElementModes.WRITE );
			if ( xTempSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}

			// open the lowlevel substorages

			xTempSubSubStorage = m_aTestHelper.openSubStorage( xTempSubStorage,
																"SubSubStorage1",
																ElementModes.WRITE );
			if ( xTempSubSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}

			byte pBytes2[] = { 2, 2, 2, 2, 2 };

			// open a new substream, set "MediaType" and "Compressed" properties to it and write some bytes
			if ( !m_aTestHelper.WriteBytesToSubstream( xTempSubSubStorage, "SubSubStream1", "MediaType1", true, pBytes2 ) )
				return false;

			// ================================================
			// commit the storages
			// ================================================

			// commit lowlevel substorage first
			if ( !m_aTestHelper.commitStorage( xTempSubSubStorage ) )
				return false;

			// commit substorage
			if ( !m_aTestHelper.commitStorage( xTempSubStorage ) )
				return false;

			// commit substorage to let the renaming take place
			if ( !m_aTestHelper.commitStorage( xTempStorage ) )
				return false;

			// ================================================
			// dispose the storages
			// ================================================

			// dispose lowerest substorage
			if ( !m_aTestHelper.disposeStorage( xTempSubSubStorage ) )
				return false;

			// dispose substorage
			if ( !m_aTestHelper.disposeStorage( xTempSubStorage ) )
				return false;

			// dispose the temporary storage
			if ( !m_aTestHelper.disposeStorage( xTempStorage ) )
				return false;

			// ================================================
			// reopen the storages and check the contents
			// ================================================

			pArgs[1] = new Integer( ElementModes.READ );
			oTempStorage = m_xStorageFactory.createInstanceWithArguments( pArgs );
			xTempStorage = (XStorage) UnoRuntime.queryInterface( XStorage.class, oTempStorage );
			if ( xTempStorage == null )
			{
				m_aTestHelper.Error( "Can't create temporary storage representation!" );
				return false;
			}

			// open the substorages

			xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
															"SubStorage1",
															ElementModes.READ );
			if ( xTempSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}

			// open the lowlevel substorages

			xTempSubSubStorage = m_aTestHelper.openSubStorage( xTempSubStorage,
																"SubSubStorage1",
																ElementModes.READ );
			if ( xTempSubSubStorage == null )
			{
				m_aTestHelper.Error( "Can't create substorage!" );
				return false;
			}

			if ( !m_aTestHelper.checkStorageProperties( xTempSubSubStorage, "MediaType4", false, ElementModes.READ ) )
				return false;

			if ( !m_aTestHelper.checkStorageProperties( xTempSubStorage, "MediaType3", false, ElementModes.READ ) )
				return false;

			if ( !m_aTestHelper.checkStorageProperties( xTempStorage, "MediaType2", true, ElementModes.READ ) )
				return false;

			if ( !m_aTestHelper.checkStream( xTempSubSubStorage, "SubSubStream1", "MediaType1", true, pBytes2 ) )
				return false;

			// the root storage is based on the temporary stream so it can be left undisposed, since it does not lock
			// any resource, later the garbage collector will release the object and it must die by refcount
	
			return true;
		}
		catch( Exception e )
		{
			m_aTestHelper.Error( "Exception: " + e );
			return false;
		}
    } 
}

