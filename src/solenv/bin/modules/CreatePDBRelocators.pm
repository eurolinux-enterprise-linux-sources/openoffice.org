#*************************************************************************
#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# Copyright 2000, 2010 Oracle and/or its affiliates.
#
# OpenOffice.org - a multi-platform office productivity suite
#
# This file is part of OpenOffice.org.
#
# OpenOffice.org is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# only, as published by the Free Software Foundation.
#
# OpenOffice.org is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU Lesser General Public License
# version 3 along with OpenOffice.org.  If not, see
# <http://www.openoffice.org/license.html>
# for a copy of the LGPLv3 License.
#
#*************************************************************************

#*************************************************************************
#
# createPDBRelocators - create for pdb relocator files
#                       PDB relocator files are used to find debug infos
#                       for analysis of creash reports
#
# usage: create_pdb_relocators($inpath, $milestone, $pre);
#
#************************************************************************* 

package CreatePDBRelocators;

use strict;
use File::Basename;

sub create_pdb_relocators
{
    my $inpath   = shift;
    my $milestone    = shift;
    my $pre      = shift;
    
    my $solarversion = $ENV{SOLARVERSION};
    if ( !$solarversion ) {
	    print STDERR "can't determine SOLARVERSION.\n";
        return undef;
    }
    
    my $o = $ENV{SRC_ROOT};
    if ( !$o ) {
	    print STDERR "can't determine SOLAR_SRC.\n";
        return undef;
    }

    my $root_dir = "$solarversion/$inpath";
    
    # sanitize path
    $root_dir =~ s/\\/\//g;
    $o =~ s/\\/\//g;
	my $pdb_dir = $root_dir . "/pdb.$pre$milestone";
	my $pdb_so_dir = $root_dir . "/pdb.$pre$milestone/so";

    # create pdb directories if necessary
    if ( ! -d $pdb_dir ) {
	    if ( !mkdir($pdb_dir, 0775) ) {
		    print STDERR "can't create directory '$pdb_dir'\n";
                return undef;
        }
    }
    if ( ! -d $pdb_so_dir ) {
	    if ( !mkdir($pdb_so_dir, 0775) ) {
		    print STDERR "can't create directory '$pdb_so_dir'\n";
                return undef;
        }
    }

    # collect files
    my @pdb_files;
    collect_files( $o, $inpath, \@pdb_files);

    foreach (@pdb_files) {
        my $relocator = basename($_) . ".location";
        /$o\/(.*)/i;
        
        my $src_location = $1;
        
        my $location = "";
        my $target = "";
        if ( $src_location =~ /\/so\// )
        {
            $location = "../../../src.$milestone/" . $src_location;
            $target = "$pdb_dir/so/$relocator";
        }
        else
        {
            $location = "../../src.$milestone/" . $src_location;
            $target = "$pdb_dir/$relocator";
        }

        if ( !open(RELOCATOR, ">$target") ) {
            print STDERR "can't write file '$target'\n";
            return undef;
        }
        print RELOCATOR "$location\n";
        close(RELOCATOR);
    }
    return 1;
}

sub collect_files
{
    my ($srcdir, $platform, $filesref) = @_; 
    my $template = "$srcdir/*/$platform";
	if ( $ENV{GUI} eq "WNT" ) {
        # collect all pdb files on o:
        # regular glob does not work with two wildcard on WNT
        my @bin    = glob("$template/bin/*.pdb");
        my @bin_so = glob("$template/bin/so/*.pdb");
        # we are only interested in pdb files which are accompanied by
        # .exe or .dll which the same name
        foreach (@bin, @bin_so) {
            my $dir  = dirname($_);
            my $base = basename($_, ".pdb");
            my $exe = "$dir/$base.exe";
            my $dll = "$dir/$base.dll";
            if ( -e $exe || -e $dll ) {
                push(@$filesref, $_);
            }
        }
    }
    else {
        # collect all shared libraries on o:
        my @lib = glob("$template/lib/*.so*");
        my @lib_so = glob("$template/lib/so/*.so*");
        my @mac_lib = glob("$template/lib/*.dylib*");
        my @mac_lib_so = glob("$template/lib/so/*.dylib*");
        # collect all binary executables on o:
        my @bin = find_binary_execs("$template/bin");
        my @bin_so = find_binary_execs("$template/bin/so");
        @$filesref = (@lib, @lib_so, @mac_lib, @mac_lib_so, @bin, @bin_so);
    }
    return 1;
}

sub find_binary_execs
{
    my $path = shift;
    my @files = glob("$path/*");
    my @execs = grep(-x $_, @files);
    my @elf_files = grep(`file $_` =~ /ELF/, @execs);
    my @MachO_files = grep(`file $_` =~ /Mach\-O/, @execs);
    return ( @elf_files, @MachO_files );
}

1; # required

