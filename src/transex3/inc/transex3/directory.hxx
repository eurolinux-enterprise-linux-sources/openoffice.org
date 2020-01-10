#include <vector>
#include <algorithm>
#include <rtl/ustring.hxx>
#include <tools/string.hxx>

#ifdef WNT
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <stdio.h>

#ifndef TRANSEX_FILE_HXX
#define TRANSEX_FILE_HXX
#include <transex3/file.hxx>
#endif

namespace transex{

class Directory
{
    private:
    rtl::OUString sDirectoryName;
    rtl::OUString sFullName;
    bool bSkipLinks;

    std::vector<Directory>  aDirVec;
    std::vector<File>       aFileVec;

    public:
    std::vector<Directory>  getSubDirectories()  { return aDirVec;        }
    std::vector<File>       getFiles()           { return aFileVec;       }

    void readDirectory();
    void readDirectory( const rtl::OUString& sFullpath );
    void scanSubDir( int nLevels = 0 );

    rtl::OUString getDirectoryName()            { return sDirectoryName; }
    rtl::OUString getFullName()                 { return sFullName ;     }
    void setSkipLinks( bool is_skipped );

    void dump();
    Directory(){};

    Directory( const rtl::OUString sFullPath );
    Directory( const rtl::OUString sFullPath , const rtl::OUString sEntry ) ;
    Directory( const ByteString sFullPath );

    static bool lessDir ( const Directory& rKey1, const Directory& rKey2 ) ; 
};

}
