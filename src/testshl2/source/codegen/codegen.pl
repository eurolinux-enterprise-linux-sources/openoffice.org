eval 'exec perl -wS $0 ${1+"$@"}'
    if 0;

use strict;
use Cwd;

# #/usr/bin/perl

# @lines
# push(@lines, $line)    append
# pop(@lines)            remove last
# shift(@lines)          remove at first
# unshift(@lines, $line) insert at first
# $lines[-1]             get last
# foreach $line (@lines)

# $a eq $b compares strings
# $a == $b compares digits

# split operator
# ($vorname, $nachname, $email) = split (/\s+/, $person);

my $sCurrentPackage = "";
my $sCurrentClass = "";
my @sMethodNames;
my @sClassNameStack;
my @sFilenameStack;
my $sCurrentFilename;
my $nNoAdditionalAnyMore = 0;
my $bShowDemo = 1;
my $sSrcExt = ".cxx";

my $cwd = getcwd();

sub generateMakefileEntry(*$$);

# ------------------------------------------------------------------------------
sub createFilename
{
    my $sPackageName = shift;
    my $sFilenameCounter = "$sPackageName$sSrcExt";
    my $sFilename = "$sPackageName";
    my $nCount = 0;
    while ( -e $sFilenameCounter)
    {
        $nCount ++;
        $sFilename = "$sPackageName" . "_$nCount";
        $sFilenameCounter = "$sFilename$sSrcExt";
    }
    push(@sFilenameStack, $sFilename);
    
    $sCurrentFilename = $sFilenameCounter;
    return $sFilenameCounter;
}
# ------------------------------------------------------------------------------

sub generateNewPackage(*$)
{
    local *CPPFILE = shift;
    my $sPackageName = shift;

    my $sFilename = createFilename($sPackageName);
    open(CPPFILE, ">$sFilename") || die "can't create source file";
    print CPPFILE "// autogenerated file with codegen.pl\n";
    print CPPFILE "\n";
    print CPPFILE "#include <cppunit/simpleheader.hxx>\n";
    print CPPFILE "\n";
    print CPPFILE "namespace $sPackageName\n";
    print CPPFILE "{\n";
}

# ------------------------------------------------------------------------------
sub generateNewClass
{
    my $sClassName = shift;
    print CPPFILE "\n";
    print CPPFILE "class $sClassName : public CppUnit::TestFixture\n";
    print CPPFILE "{\n";
    print CPPFILE "public:\n";
}
# ------------------------------------------------------------------------------
sub closeMethods
{
    # due to the fact, that this is a function based code, not object based
    # we have to do some hacks, to prevent us from creating wrong code
    if ($sCurrentClass =~ /^$/ || $sCurrentPackage =~ /^$/ )
    {
        return;
    }

    # here we create the methods
    # first the setUp() and tearDown()
    print CPPFILE "    // initialise your test code values here.\n"; 
    print CPPFILE "    void setUp()\n";
    print CPPFILE "    {\n";
    print CPPFILE "    }\n";
    print CPPFILE "\n";
    print CPPFILE "    void tearDown()\n";
    print CPPFILE "    {\n";
    print CPPFILE "    }\n";
    print CPPFILE "\n";
    
    print CPPFILE "    // insert your test code here.\n"; 

    my $sMethod;
    if ($#sMethodNames >= 0)
    {
        # all found methods
        foreach $sMethod (@sMethodNames)
        {
            print CPPFILE "    void $sMethod()\n";
            print CPPFILE "    {\n";
            if ($bShowDemo == 1)
            {
                print CPPFILE "        // this is demonstration code\n";
                print CPPFILE "        // CPPUNIT_ASSERT_MESSAGE(\"a message\", 1 == 1);\n";
                $bShowDemo = 0;
            }
            print CPPFILE "        CPPUNIT_ASSERT_STUB();\n";
            print CPPFILE "    }\n";
            print CPPFILE "\n";
        }
    }
    else
    {
        # if no methods found, create at least one
        print CPPFILE "    // this is only demonstration code\n";
        print CPPFILE "    void EmptyMethod()\n";
        print CPPFILE "    {\n";
        print CPPFILE "           // CPPUNIT_ASSERT_MESSAGE(\"a message\", 1 == 1);\n";
        print CPPFILE "       CPPUNIT_ASSERT_STUB();\n";
        print CPPFILE "    }\n";
        print CPPFILE "\n";
    }

    # create the autoregister code
    print CPPFILE "    // Change the following lines only, if you add, remove or rename \n";
    print CPPFILE "    // member functions of the current class, \n";
    print CPPFILE "    // because these macros are need by auto register mechanism.\n";
    print CPPFILE "\n";

    print CPPFILE "    CPPUNIT_TEST_SUITE($sCurrentClass);\n";
    push(@sClassNameStack, $sCurrentClass);

    my $nCount = 0;
    if ($#sMethodNames >= 0)
    {
        foreach $sMethod (@sMethodNames)
        {
            print CPPFILE "    CPPUNIT_TEST($sMethod);\n";
            $nCount ++;
        }
        # empty the method list
        my $i;
        for ($i = 0;$i < $nCount;$i++)
        {
            pop(@sMethodNames);
        }
    }
    else
    {
        print CPPFILE "    CPPUNIT_TEST(EmptyMethod);\n";
    }
    print CPPFILE "    CPPUNIT_TEST_SUITE_END();\n";
}

# ------------------------------------------------------------------------------
sub closeClass
{
    # my $sClassName = shift;
    if ($sCurrentClass =~ /^$/)
    {
        return;
    }
    print CPPFILE "}; // class $sCurrentClass\n";
    print CPPFILE "\n";
    $sCurrentClass = "";
}
# ------------------------------------------------------------------------------

sub closePackage
{
    # my $sPackageName = shift;
    if ($sCurrentPackage =~ /^$/)
    {
        return;
    }
    # create the autoregister code
    print CPPFILE "// -----------------------------------------------------------------------------\n";
    my $nCount = 0;
    my $sClassName;
    foreach $sClassName (@sClassNameStack)
    {
        print CPPFILE "CPPUNIT_TEST_SUITE_NAMED_REGISTRATION($sCurrentPackage" . "::" . "$sClassName, \"$sCurrentPackage\");\n";
        $nCount ++;
    }
    # empty the method list
    my $i;
    for ($i = 0;$i < $nCount;$i++)
    {
        pop(@sClassNameStack);
    }

    print CPPFILE "} // namespace $sCurrentPackage\n";
    print CPPFILE "\n";

    print CPPFILE "
// -----------------------------------------------------------------------------

// this macro creates an empty function, which will called by the RegisterAllFunctions()
// to let the user the possibility to also register some functions by hand.
";
    # if the variable nNoAdditionalAnyMore is set to one, we insert a remark before
    # the macro NOADDITIONAL; to prevent us from linker problems.
    if ($nNoAdditionalAnyMore == 1)
    {
        print CPPFILE "// ";
    }
    print CPPFILE "NOADDITIONAL;\n";
    print CPPFILE "\n";
    
    close(CPPFILE);

    $nNoAdditionalAnyMore = 1;

    $sCurrentPackage = "";
}

# ------------------------------------------------------------------------------

# sub generateCppSource
# {
#     my $sPackageName = shift;
#     my $sClassName = shift;
#     my $sMethodName = shift;
# 
#     
#     open(CPPFILE, ">$sFilename") || die "can't create cxx source file";
#     print CPPFILE "// autogenerated file\n";
#     close(CPPFILE);
# }
# ------------------------------------------------------------------------------
sub walkThroughJobFile
{
    my $filename = shift;

    open(FILE, $filename) || die "can't open $filename\n";

    print "start jobfile interpreter.\n";
    my $line;
    while($line = <FILE>)
    {
        chomp($line);
        # DOS Hack grrrr...
        while ($line =~ /
$/)
        {
            $line = substr($line, 0, -1);
        }

        if ($line =~ /^\#/ || $line =~ /^$/)
        {
            # remark or empty line
        }
        else
        {
            if ($line =~ /^\w+/) # must start with a word character
            {
                # print "$line\n";
                my $sPackageName = "";
                my $sClassName = "";
                my $sMethodName = "";
                my @names;
                @names = split(/\./, $line);
                if (exists $names[0])
                {
                    $sPackageName = $names[0];
                }
                if (exists $names[1])
                {
                    $sClassName = $names[1];
                }
                if (exists $names[2])
                {
                    $sMethodName = $names[2];
                }

                if ($sClassName =~ /^$/)
                {
                    print "error: in $line, no class name exist. Build no code.\n";
                }
                # test if it also works without methods
                # elsif ($sMethodName =~ /^$/)
                # {
                #         print "error: in $line, no method name exist.\n";
                # }
                else
                {
                    if ($sMethodName =~ /^$/)
                    {
                        print "warning: in $line, no method name exist, create only one member function.\n";
                    }

                    print "build code for: ${sPackageName}.${sClassName}.${sMethodName}\n";

                    if ($sCurrentPackage ne $sPackageName)
                    {
                        closeMethods();
                        closeClass();
                        closePackage();

                        $sCurrentPackage = $sPackageName;
                        generateNewPackage(CPPFILE, $sPackageName);
                    }
                    if ($sCurrentClass ne $sClassName)
                    {
                        closeMethods();
                        closeClass();
                        $sCurrentClass = $sClassName;
                        generateNewClass($sClassName);
                    }
                    if ($sMethodName)
                    {
                        push(@sMethodNames, $sMethodName);
                    }
                }
            }
        }
    }
    
    closeMethods();
    closeClass();
    closePackage();
    close(FILE);

    print "done.\n\nThe following files have been created in the current directory:\n";
    
    my $sFilename;
    foreach $sFilename (@sFilenameStack)
    {
        print "  ${sFilename}${sSrcExt}\n";
    }
    print "\n";
}

# ------------------------------------------------------------------------------
sub checkMakefileNumber
{
    my $sTargetName = shift;
    
    # This function gives back the lowest number for SHL\dTARGET
    open(MAKEFILE, "makefile.mk") || return -1;

    my $line;
    my $nNumber;
    my @aNumbers;
    my $i;
    for($i = 1;$i < 10;$i++)
    {
        $aNumbers[$i] = 0;
    }

    my $nReplacePos = -1;
    while($line = <MAKEFILE>)
    {
        chomp($line);
        
        if ($line =~ /^SHL(\d)TARGET/)
        {
            $aNumbers[$1] = 1;
            $line =~ /^SHL(\d)TARGET=\s*(\S+)\s*/;
            
            print "Target: $2\n";
            if ($sTargetName eq $2)
            {
                print "info: Targetname already exist.\n";
                $nReplacePos = $1;
            }
        }
    }
    close(MAKEFILE);

    my $nFirstFree = 0;
    for($i = 1;$i < 10;$i++)
    {
        if ($aNumbers[$i] == 0)
        {
            $nFirstFree = $i;
            last;
        }
    }
    return $nFirstFree, $nReplacePos;
}
# ------------------------------------------------------------------------------
sub createNewMakefile($$$)
{
    my $sNewMakefileName = shift;
    my $sTargetName = shift;
    my $nNumber = shift;
    local *MAKEFILE;

    # this function split a makefile into two parts,
    open(MAKEFILE, "makefile.mk") || return;
    my @lines = <MAKEFILE>;
    close(MAKEFILE);

    # print "info: Makefile has $#lines lines.\n";

    # search a point, where to insert the new makefile part.
    my $nTargetMK = -1;
    my $i;
    for ($i = $#lines; $i > 0; $i--)
    {
        if ($lines[$i] =~ /\.INCLUDE.*target.mk\s$/)
        {
            $nTargetMK = $i;
            last;
        }
    }
    if ($nTargetMK > 0)
    {
        # print "info: target.mk found, is in line $nTargetMK\n";
        
        # print "@lines[0]";
        # print "@lines[1]";
        # print "@lines[2]";
        # print "@lines[$nTargetMK - 2]";
        # print "@lines[$nTargetMK - 1]";
        # print "@lines[$nTargetMK]";

        local *OUT_MAKEFILE;
        open(OUT_MAKEFILE, ">$sNewMakefileName") || return;
        for ($i = 0;$i < ($nTargetMK - 2); $i++)
        {
            print OUT_MAKEFILE $lines[$i];
        }

        generateMakefileEntry(OUT_MAKEFILE, $sTargetName, $nNumber);

        for ($i = ($nTargetMK - 2);$i <= $#lines; $i++)
        {
            print OUT_MAKEFILE $lines[$i];
        }
        close(OUT_MAKEFILE);
    }
}
# ------------------------------------------------------------------------------

sub generateMakefileEntry(*$$)
{
    # my MAKEFILE = shift;
    local *_MAKEFILE = shift;
    my $sTargetName = shift;
    my $nNumber = shift;

    # open(_MAKEFILE, ">makefile.add") || die "can't open makefile.add";

    print _MAKEFILE "# BEGIN ----------------------------------------------------------------\n";
    print _MAKEFILE "# auto generated Target:$sTargetName by codegen.pl \n";
    print _MAKEFILE "SHL${nNumber}OBJS= ";
    my $sFilename;
    foreach $sFilename (@sFilenameStack)
    {
        print _MAKEFILE " \\\n";
        print _MAKEFILE "       \$(SLO)\$/$sFilename.obj";
    }
    print _MAKEFILE "\n\n";

    # targetname
    print _MAKEFILE "SHL${nNumber}TARGET= $sTargetName\n";
    # additional libraries
    print _MAKEFILE "SHL${nNumber}STDLIBS=\\\n";
    print _MAKEFILE "   \$(SALLIB) \\\n";
    # LLA: added by sb 18th jun 2003 (announced)
    print _MAKEFILE "   \$(CPPUNITLIB) \n";
    # link static cppunit library
    # print _MAKEFILE ".IF \"\$(GUI)\" == \"WNT\"\n";
    # print _MAKEFILE "SHL${nNumber}STDLIBS+=   \$(SOLARLIBDIR)\$/cppunit.lib\n";
    # print _MAKEFILE ".ENDIF\n";
    # print _MAKEFILE ".IF \"\$(GUI)\" == \"UNX\"\n";
    # print _MAKEFILE "SHL${nNumber}STDLIBS+=\$(SOLARLIBDIR)\$/libcppunit\$(DLLPOSTFIX).a\n";
    # print _MAKEFILE ".ENDIF\n";
    print _MAKEFILE "\n";
    print _MAKEFILE "SHL${nNumber}IMPLIB= i\$(SHL${nNumber}TARGET)\n";
    print _MAKEFILE "# SHL${nNumber}DEF=    \$(MISC)\$/\$(SHL${nNumber}TARGET).def\n";
    print _MAKEFILE "\n";
    # DEF name
    print _MAKEFILE "DEF${nNumber}NAME    =\$(SHL${nNumber}TARGET)\n";
    print _MAKEFILE "# DEF${nNumber}EXPORTFILE= export.exp\n";
    print _MAKEFILE "SHL${nNumber}VERSIONMAP= export.map\n";
    print _MAKEFILE "# auto generated Target:$sTargetName\n";
    print _MAKEFILE "# END ------------------------------------------------------------------\n\n";

    # close(_MAKEFILE);
}

# ------------------------------------------------------------------------------
sub usage
{
    print "usage:\ncodegen.pl joblist [targetname]\n";
    print "\n(c) Sun Microsystems Inc. 2003\n";
    print "
This is a testshl2 codegenerator which creates compilable C++ source files
with stub functions for all given test routines from the jobfile.
Also generate a makefile entry which is insert in the makefile.new
if a makefile.mk already exist or this tool creates a makefile.add. Which
has to add into a new makefile.mk by hand.
Also generate a export.map file, if no one exist.
";
    exit(1);
}
# -------------------------------- main function --------------------------------

sub main
{
    if ($#ARGV < 0)
    {
        usage();
    }
    my $jobfile = $ARGV[0];
    my $sTargetName;
    if ($#ARGV < 2)
    {
        # remove .sce
        $sTargetName = $jobfile;
        $sTargetName =~ s/\.\w*//;
    }
    else
    {
        $sTargetName = $ARGV[1];
    }
    print "Test code generator\n\n";
    
    if (! -e $jobfile)
    {
        print "error: given jobfile '$jobfile' doesn't exist.\n";
        exit(1);
    }
    walkThroughJobFile($jobfile);

    # generate makefile
    
    if (-e "makefile.mk")
    {
        my $n;
        my $nReplacePos;
        ($n, $nReplacePos) = checkMakefileNumber($sTargetName);
        # $n == -1 no makefile
        # $n == 0  no free number
        # $n 1..9 ok.
        if ($n > 0)
        {
            my $sNewMakefileName = "makefile.new";
            print "Makefile: Add the 'SHL${n}TARGET' to the file '$sNewMakefileName'\n";
            createNewMakefile($sNewMakefileName, $sTargetName, $n);
        }
    }
    else
    {
        print "warning: No makefile.mk found, please add the content of makefile.add to a makefile.mk file\n";
        local *MAKEFILE;
        open(MAKEFILE, ">makefile.add");
        generateMakefileEntry(MAKEFILE, $sTargetName, 1);
        close(MAKEFILE);
    }
    print "\n";
    # this is the old export.exp method
    # if (! -e "export.exp")
    # {
    #     print "info: create export.exp file\n";
    #     open(EXPORTEXP, ">export.exp") || die "can't create export.exp";
    #     print EXPORTEXP "registerAllTestFunction\n";
    #     close(EXPORTEXP);
    # }
    # else
    # {
    #     print "The file 'export.exp' file already exist, please make sure that it contains the entry 'registerAllTestFunction'.\n";
    # }
    if (! -e "export.map")
    {
        local *EXPORTMAP;
        print "info: create export.map file\n";
        open(EXPORTMAP, ">export.map") || die "can't create export.map";
        print EXPORTMAP "UDK_3.0 {\n";
        print EXPORTMAP "    global:\n";
        print EXPORTMAP "        registerAllTestFunction;\n";
        print EXPORTMAP "\n";
        print EXPORTMAP "    local:\n";
        print EXPORTMAP "        *;\n";
        print EXPORTMAP "};\n";
        close(EXPORTMAP);
    }
    else
    {
        print "The file 'export.map' file already exist, please make sure that it contains the entry 'registerAllTestFunction'.\n";
    }
}

# ------------------------------------------------------------------------------

main();
