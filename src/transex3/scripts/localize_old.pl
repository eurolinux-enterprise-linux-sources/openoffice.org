:
eval 'exec perl -wS $0 ${1+"$@"}'
    if 0;

    
#*************************************************************************
#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
# 
# Copyright 2008 by Sun Microsystems, Inc.
#
# OpenOffice.org - a multi-platform office productivity suite
#
# $RCSfile: localize.pl,v $
#
# $Revision: 1.18 $
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
   
use strict;
use Getopt::Long;
use IO::Handle;
use File::Find;
use File::Temp;
use File::Copy;
use File::Glob qw(:glob csh_glob);
use Cwd;

# ver 1.1
#
#### module lookup 
#use lib ("$ENV{SOLARENV}/bin/modules", "$ENV{COMMON_ENV_TOOLS}/modules");

#### module lookup
# OOo conform
my @lib_dirs;
BEGIN {
    if ( !defined($ENV{SOLARENV}) ) {
        die "No environment found (environment variable SOLARENV is undefined)";
    }
    push(@lib_dirs, "$ENV{SOLARENV}/bin/modules");
    push(@lib_dirs, "$ENV{COMMON_ENV_TOOLS}/modules") if defined($ENV{COMMON_ENV_TOOLS});
}
use lib (@lib_dirs);

#### globals ####
my $sdffile = '';
my $no_sort = '';
my $outputfile = '';
my $mode = '';
my $bVerbose="0";
my $srcpath = '';
my $WIN;
my $languages;
#my %sl_modules;     # Contains all modules where en-US and de is source language
my $use_default_date = '0';

         #         (                           leftpart                                                     )            (           rightpart                    )    
         #            prj      file      dummy     type       gid       lid      helpid    pform     width      lang       text    helptext  qhelptext   title    timestamp
my $sdf_regex  = "((([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*))\t([^\t]*)\t(([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t)([^\t]*))";
my $file_types = "(src|hrc|xcs|xcu|lng|ulf|xrm|xhp|xcd|xgf|xxl|xrb)";
# Always use this date to prevent cvs conflicts
my $default_date = "2002-02-02 02:02:02";

#### main ####
parse_options();

if ( defined $ENV{USE_SHELL} && $ENV{USE_SHELL} eq '4nt' ) {
   $WIN = 'TRUE';
}
 else {
   $WIN = ''; 
}

#%sl_modules = fetch_sourcelanguage_dirlist();


if   ( $mode eq "merge"    )    {   
    merge_gsicheck();
    splitfile( $sdffile );      
    unlink $sdffile;             # remove temp file!
}
elsif( $mode eq "extract"  )    {   
    collectfiles( $outputfile );
}
else                            {  
    usage();                    
}

exit(0);

#########################################################
sub splitfile{

    my $lastFile        = '';
    my $currentFile     = '';
    my $cur_sdffile     = '';
    my $last_sdffile    = '';
    my $delim;
    my $badDelim;
    my $start           = 'TRUE';
    my %index  = ();
    my %block;

    STDOUT->autoflush( 1 );

    #print STDOUT "Open File $sdffile\n";
    open MYFILE , "< $sdffile"
    or die "Can't open '$sdffile'\n";
    
    while( <MYFILE>){
         if( /$sdf_regex/ ){
            my $line           = defined $_ ? $_ : '';
            my $prj            = defined $3 ? $3 : '';
            my $file           = defined $4 ? $4 : '';
            my $type           = defined $6 ? $6 : '';
            my $gid            = defined $7 ? $7 : '';
            my $lid            = defined $8 ? $8 : '';
            my $lang           = defined $12 ? $12 : '';
            my $plattform      = defined $10 ? $10 : '';
            my $helpid         = defined $9 ? $9 : ''; 
           
            next if( $prj eq "binfilter" );     # Don't merge strings into binfilter module
	        chomp( $line );
            $currentFile  = $srcpath . '\\' . $prj . '\\' . $file;
            if ( $WIN ) { $currentFile  =~ s/\//\\/g; }
            else        { $currentFile  =~ s/\\/\//g; }
 
            $cur_sdffile = $currentFile;
            #if( $cur_sdffile =~ /\.$file_types[\s]*$/ ){
                if( $WIN ) { $cur_sdffile =~ s/\\[^\\]*\.$file_types[\s]*$/\\localize.sdf/; }
                else       { $cur_sdffile =~ s/\/[^\/]*\.$file_types[\s]*$/\/localize.sdf/; }
            #} 
             
            # Set default date
            if( $line =~ /(.*)\t[^\t\$]*$/ ){
                $line = $1."\t".$default_date;
            }
     
            if( $start ){
                $start='';
                $lastFile = $currentFile; # ?
                $last_sdffile = $cur_sdffile;
            }    

            if( $lang eq "en-US" ){}
            elsif( $cur_sdffile eq $last_sdffile )
            { 
                $block{ "$prj\t$file\t$type\t$gid\t$lid\t$helpid\t$plattform\t$lang" } =  $line ;
            }
            else
            {
                writesdf( $lastFile , \%block );
                $lastFile = $currentFile; #?
                $last_sdffile = $cur_sdffile;
                %block = ();
                #if( ! $lang eq "en-US"  ) {  
                $block{ "$prj\t$file\t$type\t$gid\t$lid\t$helpid\t$plattform\t$lang" } =  $line ; 
                #}

            }
        } #else { print STDOUT "splitfile REGEX kaputt\n";}

    }
    writesdf( $lastFile , \%block );
    %block = ();
    close( MYFILE );

}
#########################################################

#sub fetch_sourcelanguage_dirlist
#{
#
#    my $working_path = getcwd();
#    my %sl_dirlist;
#       
#    chdir $srcpath;         
#    my @all_dirs = csh_glob( "*" );
#    
#    foreach my $file ( @all_dirs )
#    {
#        if( -d $file )                                             
#        {
#            my $module = $file;
#            $file .= "/prj/l10n";                  
#            $file =~ s/\//\\/ , if( $WIN ) ;
#        
#            if( -f $file )                                        # Test file <module>/prj/l10n
#            {
#                $sl_dirlist{ $module } = 1; 
#                if( $bVerbose eq "1" ) { print STDOUT "$module: de and en-US source language detected\n"; }
#            }
#        }
#    }
#    
#    chdir $working_path;
#    
#    return %sl_dirlist;
#}

#sub has_two_sourcelanguages
#{
#    my $module          = shift;
#    return defined $sl_modules{ $module } ;
#}
sub writesdf{
        
    my $lastFile        = shift;
    my $blockhash_ref   = shift;
    my $localizeFile    = $lastFile;
    my %index=();

    if( $localizeFile =~ /\.$file_types[\s]*$/ ){
        if( $WIN ) { $localizeFile =~ s/\\[^\\]*\.$file_types[\s]*$/\\localize.sdf/; }
        else       { $localizeFile =~ s/\/[^\/]*\.$file_types[\s]*$/\/localize.sdf/; }
        }else {
            print STDERR "Strange filetype found '$localizeFile'\n";
            return;
        }
        if( open DESTFILE , "< $localizeFile" ){

        #or die "Can't open/create '\$localizeFile'";  

        #### Build hash
        while(<DESTFILE>){
         if( /$sdf_regex/ ){
            my $line           = defined $_ ? $_ : '';
            my $prj            = defined $3 ? $3 : '';
            my $file           = defined $4 ? $4 : '';
            my $type           = defined $6 ? $6 : '';
            my $gid            = defined $7 ? $7 : '';
            my $lid            = defined $8 ? $8 : '';
            my $lang           = defined $12 ? $12 : '';
            my $plattform      = defined $10 ? $10 : ''; 
            my $helpid         = defined $9 ? $9 : '';
            
            chomp( $line );    
            $index{ "$prj\t$file\t$type\t$gid\t$lid\t$helpid\t$plattform\t$lang" } =  $line ;
            
         } #else { print STDOUT "writesdf REGEX kaputt $_\n";}

        }      
        close( DESTFILE );
    }
    #### Copy new strings
    my @mykeys = keys( %{ $blockhash_ref } );
    my $isDirty = "FALSE";
    foreach my $key( @mykeys ){
        if( ! defined $index{ $key } ){
            # Add new entry
            $index{ $key }  = $blockhash_ref->{ $key} ;
            $isDirty = "TRUE";
        }elsif( $index{ $key } ne $blockhash_ref->{ $key } ){
            # Overwrite old entry
            $index{ $key } = $blockhash_ref->{ $key };
            $isDirty = "TRUE";
        }else {
        }
    }

    #### Write file  

    if( !$bVerbose ){ print STDOUT "."; }
    if( $isDirty eq "TRUE" ){
        if( $bVerbose ){ print STDOUT "$localizeFile\n"; }
        if( open DESTFILE , "+> $localizeFile" ){
            print DESTFILE get_license_header();
            @mykeys = sort keys( %index );
            foreach my $key( @mykeys ){
                print DESTFILE ( $index{ $key } , "\n" );
            }
            close DESTFILE;
         }else {
            print STDOUT "WARNING: File $localizeFile is not writable , try to merge ...\n";
            my ( $TMPFILE , $tmpfile ) = File::Temp::tempfile();
            if( open DESTFILE , "+> $tmpfile " ){
                @mykeys = keys( %index );
                foreach my $key( @mykeys ){
                    print DESTFILE ( $index{ $key } , "\n" );
                }
                close DESTFILE;
                if( move( $localizeFile , $localizeFile.".backup" ) ){
                    if( copy( $tmpfile , $localizeFile ) ){ 
                        unlink $localizeFile.".backup";
                    } else { print STDERR "Can't open/create '$localizeFile', original file is renamed to  $localizeFile.backup\n"; }
                } else { print STDERR "Can't open/create '$localizeFile'\n"; }
            }else{
                print STDERR "WARNING: Can't open/create '$localizeFile'\n";  
            }
            unlink $tmpfile;
        }
    }
#    if( $no_sort eq '' ){
#        sort_outfile( $localizeFile );
#    }
}

sub get_license_header{
    return 
"#\n".
"#    ####    ###     #   #   ###   #####    #####  ####   #####  #####  \n".
"#    #   #  #   #    ##  #  #   #    #      #      #   #    #      #    \n".
"#    #   #  #   #    # # #  #   #    #      ###    #   #    #      #    \n".
"#    #   #  #   #    #  ##  #   #    #      #      #   #    #      #    \n".
"#    ####    ###     #   #   ###     #      #####  ####   #####    #    \n".
"#\n".
"#    DO NOT EDIT! This file will be overwritten by localisation process\n".
"#\n".
"#*************************************************************************\n".
"#\n".
"# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.\n".
"# \n".
"# Copyright 2008 by Sun Microsystems, Inc.\n".
"#\n".
"# OpenOffice.org - a multi-platform office productivity suite\n".
"#\n".
"# \$RCSfile:".
"localize.pl,v \$\n".
"#\n".
"# \$Revision: ".
"1.17.4.1 \$\n".
"#\n".
"# This file is part of OpenOffice.org.\n".
"#\n".
"# OpenOffice.org is free software: you can redistribute it and/or modify\n".
"# it under the terms of the GNU Lesser General Public License version 3\n".
"# only, as published by the Free Software Foundation.\n".
"#\n".
"# OpenOffice.org is distributed in the hope that it will be useful,\n".
"# but WITHOUT ANY WARRANTY; without even the implied warranty of\n".
"# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n".
"# GNU Lesser General Public License version 3 for more details\n".
"# (a copy is included in the LICENSE file that accompanied this code).\n".
"#\n".
"# You should have received a copy of the GNU Lesser General Public License\n".
"# version 3 along with OpenOffice.org.  If not, see\n".
"# <http://www.openoffice.org/license.html>\n".
"# for a copy of the LGPLv3 License.\n".
"#\n".
"#*************************************************************************\n";
}
######## Check input sdf file and use only the correct part
sub merge_gsicheck{
    my $command = '';
    my ( $TMPHANDLE , $tmpfile ) = File::Temp::tempfile();
    my ( $TMPHANDLE2 , $tmpfile2 ) = File::Temp::tempfile();
    close ( $TMPHANDLE );
    close ( $TMPHANDLE2 );
   
    unlink $tmpfile2;
    my $output2 = `cat $sdffile | sort > $tmpfile2`;
    my $rc2 = $? << 8;
    if( $rc2 ne  0 ){
        printf("ERROR: Failed -> cat $sdffile | sort > $tmpfile2\n$output2\n");
        exit( -1 );
    }
    
#    if( $ENV{WRAPCMD} ){
#        $command = "$ENV{WRAPCMD} gsicheck";
#    }else{
#        $command = "gsicheck";
#    }
#    my $errfile = $tmpfile.".err";
#    $command .= " -k -c -wcf $tmpfile -wef $errfile -l \"\" $tmpfile2";
#    my $output = `$command`;
#    my $rc = $? << 8;
#    if ( $output ne "" ){
#        print STDOUT "### gsicheck ###\n";
#        print STDOUT "### The file $errfile have been written containing the errors in your sdf file. Those lines will not be merged: ###\n\n";
#        print STDOUT "$output\n";
#        print STDOUT "################\n";
#       
#    }else{
#        # Remove the 0 Byte file
#        unlink $errfile;
#    }
    $sdffile = $tmpfile2;
#    unlink $tmpfile2;
}
#########################################################
sub collectfiles{
    print STDOUT "### Localize\n"; 
    my @sdfparticles;
    my $localizehash_ref;  
    my ( $bAll , $bUseLocalize, $langhash_ref , $bHasSourceLanguage , $bFakeEnglish ) = parseLanguages();
    
    # Enable autoflush on STDOUT
    # $| = 1; 
    STDOUT->autoflush( 1 );

    ### Search sdf particles
    print STDOUT "### Searching sdf particles\n";
    my $working_path = getcwd();
    chdir $srcpath;         
    find sub { 
        my $file = $File::Find::name;
        if( -f && $file =~ /.*localize.sdf$/ ) {
            push   @sdfparticles , $file; 
            if( $bVerbose eq "1" ) { print STDOUT "$file\n"; }
            else { print ".";  }
    
         }
    } , getcwd() ;#"."; #$srcpath;
    chdir $working_path;

    my $nFound  = $#sdfparticles +1; 
    print "\n    $nFound files found !\n";

    my ( $LOCALIZEPARTICLE , $localizeSDF ) = File::Temp::tempfile();
    close( $LOCALIZEPARTICLE ); 

    my ( $ALLPARTICLES_MERGED , $particleSDF_merged )     = File::Temp::tempfile();
    close( $ALLPARTICLES_MERGED ); 
    my ( $LOCALIZE_LOG , $my_localize_log ) = File::Temp::tempfile();
    close( $LOCALIZE_LOG );
    
    ## Get the localize de,en-US extract
    if( $bAll || $bUseLocalize ){
        print "### Fetching source language strings\n";
        my $command = "";
        my $args    = "";
        
        if( $ENV{WRAPCMD} ){
            $command = "$ENV{WRAPCMD} localize_sl";
        }else{
            $command = "localize_sl";
        }

        # -e
        # if ( -x $command ){
        if( $command ){
            if( !$bVerbose  ){ $args .= " -QQ -skip_links "; }
            $args .= " -e -f $localizeSDF -l ";
            my $bFlag="";
            if( $bAll ) {$args .= " en-US";}
            else{
              my @list;
              foreach my $isokey ( keys( %{ $langhash_ref } ) ){
                push @list , $isokey;
                if( $langhash_ref->{ $isokey } ne "" ){
                    push @list , $langhash_ref->{ $isokey };
                } 
              }
              remove_duplicates( \@list );
              foreach my $isokey ( @list ){
                switch :{
                    #( $isokey=~ /^de$/i  ) 
                    #    && do{
                    #            if( $bFlag eq "TRUE" ){ $args .= ",de"; }
                    #            else  {     
                    #                $args .=  "de";  $bFlag = "TRUE"; 
                    #             }
                    #          };
                        ( $isokey=~ /^en-US$/i  ) 
                        && do{  
                                if( $bFlag eq "TRUE" ){ $args .= ",en-US"; }
                                else {     
                                    $args .= "en-US";  $bFlag = "TRUE"; 
                                 }
                              };
                                        
                    } #switch
                } #foreach
              } # if 
        } # if
#        if ( !$bVerbose ){ 
#            if ( $WIN eq "TRUE" ) { $args .= " > $my_localize_log";  }
#            else                  { $args .= " >& $my_localize_log"; }
#        }
        if ( $bVerbose ) { print STDOUT $command.$args."\n"; }
        
        my $rc = system( $command.$args );

        #my $output = `$command.$args`;
        #my $rc = $? << 8;
        
        if( $rc < 0 ){    print STDERR "ERROR: localize rc = $rc\n"; exit( -1 ); }
        ( $localizehash_ref )  = read_file( $localizeSDF , $langhash_ref );
     
    }
    ## Get sdf particles
   open ALLPARTICLES_MERGED , "+>> $particleSDF_merged" 
    or die "Can't open $particleSDF_merged"; 
    
    ## Fill fackback hash
    my( $fallbackhashhash_ref ) = fetch_fallback( \@sdfparticles , $localizeSDF ,  $langhash_ref );
#    my( $fallbackhashhash_ref ) = fetch_fallback( \@sdfparticles , $localizeSDF , $langhash_ref );
    my %block;
    my $cur_fallback;
    if( !$bAll) {
        foreach my $cur_lang ( keys( %{ $langhash_ref } ) ){
            #print STDOUT "DBG: G1 cur_lang=$cur_lang\n";
            $cur_fallback = $langhash_ref->{ $cur_lang };
            if( $cur_fallback ne "" ){
                # Insert fallback strings
                #print STDOUT "DBG: Renaming $cur_fallback to $cur_lang in fallbackhash\n";
                rename_language(  $fallbackhashhash_ref ,  $cur_fallback , $cur_lang );
            }
            foreach my $currentfile ( @sdfparticles ){
                if ( open MYFILE , "< $currentfile" ) {
                    while(<MYFILE>){
                        if( /$sdf_regex/ ){
                            my $line           = defined $_ ? $_ : '';
                            my $prj            = defined $3 ? $3 : '';
                            my $file           = defined $4 ? $4 : '';
                            my $type           = defined $6 ? $6 : '';
                            my $gid            = defined $7 ? $7 : '';
                            my $lid            = defined $8 ? $8 : '';
                            my $lang           = defined $12 ? $12 : '';
                            my $plattform      = defined $10 ? $10 : ''; 
                            my $helpid         = defined $9 ? $9 : '';
                            
                            chomp( $line );

                            if ( $lang eq $cur_lang ){
                                # Overwrite fallback strings with collected strings
                                #if( ( !has_two_sourcelanguages( $cur_lang) && $cur_lang eq "de" ) || $cur_lang ne "en-US" ){
                                     $fallbackhashhash_ref->{ $cur_lang }{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } =  $line ;
                                     #}

                            }
                        }
                    }
                }else { print STDERR "WARNING: Can't open file $currentfile"; }
            }

            foreach my $line ( keys( %{$fallbackhashhash_ref->{ $cur_lang } } )) {
                if( #$cur_lang ne "de" && 
                    $cur_lang ne "en-US" ){
                    print ALLPARTICLES_MERGED ( $fallbackhashhash_ref->{ $cur_lang }{ $line }, "\n" );
                }
             }
        } 
    } else {
        foreach my $currentfile ( @sdfparticles ){
            if ( open MYFILE , "< $currentfile" ) {
                while( <MYFILE> ){
                    print ALLPARTICLES_MERGED ( $_, "\n" );  # recheck de / en-US !
                }
            }
            else { print STDERR "WARNING: Can't open file $currentfile"; }
        }
    }
    close ALLPARTICLES_MERGED;
    
    
    # Hash of array
    my %output;
    my @order;
    
    ## Join both
    if( $outputfile ){
        if( open DESTFILE , "+> $outputfile" ){
            if( !open LOCALIZEPARTICLE ,  "< $localizeSDF" ) { print STDERR "ERROR: Can't open file $localizeSDF\n"; } 
            if( !open ALLPARTICLES_MERGED , "< $particleSDF_merged" ) { print STDERR "ERROR: Can't open file $particleSDF_merged\n"; }
                
            # Insert localize
            my $extract_date="";
            while ( <LOCALIZEPARTICLE> ){
                if( /$sdf_regex/ ){
                    my $leftpart       = defined $2 ? $2 : '';
                    my $lang           = defined $12 ? $12 : '';
                    my $rightpart      = defined $13 ? $13 : '';
                    my $timestamp      = defined $18 ? $18 : '';

                    my $prj            = defined $3 ? $3 : '';
                    my $file           = defined $4 ? $4 : '';
                    my $type           = defined $6 ? $6 : '';
                    my $gid            = defined $7 ? $7 : '';
                    my $lid            = defined $8 ? $8 : '';
                    #my $lang           = defined $12 ? $12 : '';
                    my $plattform      = defined $10 ? $10 : ''; 
                    my $helpid         = defined $9 ? $9 : '';
 
                    
                    if( $use_default_date )
                    {
                        $extract_date = "$default_date\n" ; 
                    }
                    elsif( $extract_date eq "" ) {  
                        $extract_date = $timestamp ; 
                        $extract_date =~ tr/\r\n//d;
                        $extract_date .= "\n";
                    }

                    if( $bAll ){ print DESTFILE $leftpart."\t".$lang."\t".$rightpart.$extract_date ; }
                    else {
                        foreach my $sLang ( keys( %{ $langhash_ref } ) ){
                            if( $sLang=~ /all/i )                       {  
                                push @{ $output{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } } ,  $leftpart."\t".$lang."\t".$rightpart.$extract_date ;
                                #print DESTFILE $leftpart."\t".$lang."\t".$rightpart.$extract_date;  
                            }
                            #if( $sLang eq "de" && $lang eq "de" )       {  
                            #    push @{ $output{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } } ,  $leftpart."\t".$lang."\t".$rightpart.$extract_date ;
                                #print DESTFILE $leftpart."\t".$lang."\t".$rightpart.$extract_date;  
                                #}
                            if( $sLang eq "en-US" && $lang eq "en-US" ) {  
                                push @order , $prj.$gid.$lid.$file.$type.$plattform.$helpid;
                                if( !$bFakeEnglish ){ push @{ $output{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } } ,  $leftpart."\t".$lang."\t".$rightpart.$extract_date ; }
                                #print DESTFILE $leftpart."\t".$lang."\t".$rightpart.$extract_date;  
                            }

                        }
                    }
                }
            }
            # Insert particles
            while ( <ALLPARTICLES_MERGED> ){
                if( /$sdf_regex/ ){
                    my $leftpart       = defined $2 ? $2 : '';
                    my $prj            = defined $3 ? $3 : '';
                    my $lang           = defined $12 ? $12 : '';
                    my $rightpart      = defined $13 ? $13 : '';
                    my $timestamp      = defined $18 ? $18 : '';

                    #my $prj            = defined $3 ? $3 : '';
                    my $file           = defined $4 ? $4 : '';
                    my $type           = defined $6 ? $6 : '';
                    my $gid            = defined $7 ? $7 : '';
                    my $lid            = defined $8 ? $8 : '';
                    #my $lang           = defined $12 ? $12 : '';
                    my $plattform      = defined $10 ? $10 : ''; 
                    my $helpid         = defined $9 ? $9 : '';
 
                    
                    if( $use_default_date )
                    {
                        $extract_date = "$default_date\n" ; 
                    }
                    elsif( $extract_date eq "" ) 
                    { 
                        $extract_date = $timestamp; 
                    }

                    if( ! ( $prj =~ /binfilter/i ) ) { 
                        push @{ $output{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } } , $leftpart."\t".$lang."\t".$rightpart.$extract_date ; 
                        #print DESTFILE $leftpart."\t".$lang."\t".$rightpart.$extract_date ; 
                    }
                 }
            }

            # Write!
            foreach my $curkey ( @order ){
                foreach my $curlist ( $output{ $curkey } ){
                    foreach my $line ( @{$curlist} ){
                        print DESTFILE $line;
                    }
                }
            } 
        
        }else { print STDERR "Can't open $outputfile";}
    }
    close DESTFILE;
    close LOCALIZEPARTICLE;
    close ALLPARTICLES_MERGED;
    
    #print STDOUT "DBG: \$localizeSDF $localizeSDF \$particleSDF_merged $particleSDF_merged\n";    
    unlink $localizeSDF , $particleSDF_merged ,  $my_localize_log;
    
    #sort_outfile( $outputfile );
    #remove_obsolete( $outputfile ) , if $bHasSourceLanguage ne "";
    }

#########################################################
sub remove_obsolete{
    my $outfile = shift;
    my @lines;
    my $enusleftpart;
    my @good_lines;
    
    print STDOUT "### Removing obsolete strings\n";

    # Kick out all strings without en-US reference
    if ( open ( SORTEDFILE , "< $outfile" ) ){
        while( <SORTEDFILE> ){
            if( /$sdf_regex/ ){
                my $line           = defined $_ ? $_ : '';
                my $language       = defined $12 ? $12 : '';
                my $prj            = defined $3 ? $3 : '';
                my $file           = defined $4 ? $4 : '';
                my $type           = defined $6 ? $6 : '';
                my $gid            = defined $7 ? $7 : '';
                my $lid            = defined $8 ? $8 : '';
                my $plattform      = defined $10 ? $10 : ''; 
                my $helpid         = defined $9 ? $9 : '';
                
                my $leftpart = $prj.$gid.$lid.$file.$type.$plattform.$helpid;
                
                if( $language eq "en-US" ){                 # source string found, 1. entry
                    $enusleftpart = $leftpart;
                    push @good_lines , $line;
                }else{
                    if( !defined $enusleftpart or !defined $leftpart ){
                        print STDERR "BADLINE: $line\n";
                        print STDERR "\$enusleftpart = $enusleftpart\n";
                        print STDERR "\$leftpart = $leftpart\n";
                    }
                    if( $enusleftpart eq $leftpart ){   # matching language
                        push @good_lines , $line;
                    }
                    #else{
                    #    print STDERR "OUT:  \$enusleftpart=$enusleftpart \$leftpart=$leftpart \$line=$line\n";
                    #}
                }            
            }
        }    
        close SORTEDFILE;
    } else { print STDERR "ERROR: Can't open file $outfile\n";}
    
    # Write file
    if ( open ( SORTEDFILE , "> $outfile" ) ){
        foreach my $newline ( @good_lines ) {
            print SORTEDFILE $newline;
        }
        close SORTEDFILE;
    } else { print STDERR "ERROR: Can't open file $outfile\n";}

}
#########################################################
sub sort_outfile{
        my $outfile = shift;
        print STDOUT "### Sorting ... $outfile ...";
        my @lines;
        my @sorted_lines;
        
        
        #if ( open ( SORTEDFILE , "< $outputfile" ) ){
        if ( open ( SORTEDFILE , "< $outfile" ) ){
            my $line;
            while ( <SORTEDFILE> ){
                $line = $_;
                if( $line =~ /^[^\#]/ ){ 
                    push @lines , $line; 
                }
            }
            close SORTEDFILE;
            @sorted_lines = sort {
                my $xa_lang          = "";      
                my $xa_left_part    = "";
                my $xa_right_part    = "";
                my $xa_timestamp     = "";
                my $xb_lang          = "";      
                my $xb_left_part    = "";
                my $xb_right_part    = "";
                my $xb_timestamp     = "";
                my $xa               = "";
                my $xb               = "";
                my @alist;
                my @blist;
                
                if( $a=~ /$sdf_regex/ ){
                    $xa_left_part       = defined $2 ? $2 : '';
                    $xa_lang           = defined $12 ? $12 : '';
                    $xa_right_part     = defined $13 ? $13 : '';
                    $xa_left_part = remove_last_column( $xa_left_part );

                }
                if( $b=~ /$sdf_regex/ ){
                    $xb_left_part       = defined $2 ? $2 : '';
                    $xb_lang           = defined $12 ? $12 : '';
                    $xb_right_part     = defined $13 ? $13 : '';
                    $xb_left_part = remove_last_column( $xb_left_part );


                } 
                if( (  $xa_left_part cmp $xb_left_part ) == 0 ){         # Left part equal
                     if( ( $xa_lang cmp $xb_lang ) == 0 ){               # Lang equal
                         return ( $xa_right_part cmp $xb_right_part );   # Right part compare
                    }
                    elsif( $xa_lang eq "en-US" ) { return -1; }        # en-US wins
                    elsif( $xb_lang eq "en-US" ) { return 1;  }        # en-US wins
                    else { return $xa_lang cmp $xb_lang; }             # lang compare
                }
                else { 
                    return $xa_left_part cmp $xb_left_part;        # Left part compare
                }
            } @lines;
            
            if ( open ( SORTEDFILE , "> $outfile" ) ){
                print SORTEDFILE get_license_header();
                foreach my $newline ( @sorted_lines ) {
                    print SORTEDFILE $newline;
                    #print STDOUT $newline;
                }            
            } 
            close SORTEDFILE;
        } else { print STDERR "WARNING: Can't open file $outfile\n";}
	print "done\n";

}
#########################################################
sub remove_last_column{
    my $string                  = shift;
    my @alist = split ( "\t" , $string );
    pop @alist;
    return join( "\t" , @alist );
}

#########################################################
sub rename_language{
    my $fallbackhashhash_ref    = shift; 
    my $cur_fallback            = shift;
    my $cur_lang                = shift;
    my $line;
      
    foreach my $key( keys ( %{ $fallbackhashhash_ref->{ $cur_fallback } } ) ){
        $line = $fallbackhashhash_ref->{ $cur_fallback }{ $key };
        if( $line =~ /$sdf_regex/ ){
            my $leftpart       = defined $2 ? $2 : '';
            my $lang           = defined $12 ? $12 : '';
            my $rightpart      = defined $13 ? $13 : '';
                
            $fallbackhashhash_ref->{ $cur_lang }{ $key } = $leftpart."\t".$cur_lang."\t".$rightpart;
        }
    }
}

############################################################
sub remove_duplicates{
    my $list_ref    = shift;
    my %tmphash;
    foreach my $key ( @{ $list_ref } ){ $tmphash{ $key } = '' ; }
    @{$list_ref} = keys( %tmphash );
}

##############################################################
sub fetch_fallback{
    my $sdfparticleslist_ref   = shift;
    my $localizeSDF            = shift;
    my $langhash_ref           = shift; 
    my %fallbackhashhash;
    my $cur_lang;
    my @langlist;
    
    foreach my $key ( keys ( %{ $langhash_ref } ) ){
        $cur_lang = $langhash_ref->{ $key };
        if ( $cur_lang ne "" ) {
            push @langlist , $cur_lang;
        }
    }
    remove_duplicates( \@langlist ); 
    foreach  $cur_lang ( @langlist ){ 
        if( $cur_lang eq "en-US" ){
            read_fallbacks_from_source( $localizeSDF , $cur_lang , \%fallbackhashhash );
        }
    }
    
    # remove de / en-US
    my @tmplist;
    foreach $cur_lang( @langlist ){
        if(  $cur_lang ne "en-US" ){
           push @tmplist , $cur_lang;
 
        }
    }
    @langlist = @tmplist;
    if ( $#langlist +1 ){ 
        read_fallbacks_from_particles( $sdfparticleslist_ref , \@langlist , \%fallbackhashhash ); 

    }
    return (\%fallbackhashhash);
}

#########################################################
sub write_file{
  
    my $localizeFile = shift;
    my $index_ref    = shift;
    
    if( open DESTFILE , "+> $localizeFile" ){
        foreach my $key( %{ $index_ref } ){
            print DESTFILE ($index_ref->{ $key }, "\n" );
        }
        close DESTFILE;
    }else {
      print STDERR "Can't open/create '$localizeFile'";  
    }
}

#########################################################
sub read_file{
    
    my $sdffile         = shift;
    my $langhash_ref    = shift;
    my %block           = ();
    
    open MYFILE , "< $sdffile"
        or die "Can't open '$sdffile'\n";
        while( <MYFILE>){
          if( /$sdf_regex/ ){
            my $line           = defined $_ ? $_ : '';
            my $prj            = defined $3 ? $3 : '';
            my $file           = defined $4 ? $4 : '';
            my $type           = defined $6 ? $6 : '';
            my $gid            = defined $7 ? $7 : '';
            my $lid            = defined $8 ? $8 : '';
            my $plattform      = defined $10 ? $10 : '';
            my $lang           = defined $12 ? $12 : '';
            my $helpid         = defined $9 ? $9 : '';
            
            foreach my $isolang ( keys ( %{ $langhash_ref } ) ){
                if( $isolang=~ /$lang/i || $isolang=~ /all/i ) { $block{$prj.$gid.$lid.$file.$type.$plattform.$helpid } =  $line ; }
            }
        }
    }
    return (\%block);
}

#########################################################
sub read_fallbacks_from_particles{
    
    my $sdfparticleslist_ref    = shift;
    my $isolanglist_ref         = shift;
    my $fallbackhashhash_ref    = shift;
    my $block_ref;
    foreach my $currentfile ( @{ $sdfparticleslist_ref } ){
        if ( open MYFILE , "< $currentfile" ) {
            while(<MYFILE>){
                if( /$sdf_regex/ ){
                    my $line           = defined $_ ? $_ : '';
                    my $prj            = defined $3 ? $3 : '';
                    my $file           = defined $4 ? $4 : '';
                    my $type           = defined $6 ? $6 : '';
                    my $gid            = defined $7 ? $7 : '';
                    my $lid            = defined $8 ? $8 : '';
                    my $lang           = defined $12 ? $12 : '';
                    my $plattform      = defined $10 ? $10 : ''; 
                    my $helpid         = defined $9 ? $9 : '';
                          
                    chomp( $line );
             
                    foreach my $isolang ( @{$isolanglist_ref}  ){
                        if( $isolang=~ /$lang/i ) { 
                            $fallbackhashhash_ref->{ $isolang }{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } =  $line ; 
                        }
                    }
                }
            }
       }else { print STDERR "WARNING: Can't open file $currentfile"; }
    }
}

#########################################################
sub read_fallbacks_from_source{
    
    my $sdffile                 = shift;
    my $isolang                 = shift;
    my $fallbackhashhash_ref    = shift;
    my $block_ref;
    # read fallback for single file 
    open MYFILE , "< $sdffile"
        or die "Can't open '$sdffile'\n";
    
    while( <MYFILE>){
          if( /$sdf_regex/ ){
            my $line           = defined $_ ? $_ : '';
            my $prj            = defined $3 ? $3 : '';
            my $file           = defined $4 ? $4 : '';
            my $type           = defined $6 ? $6 : '';
            my $gid            = defined $7 ? $7 : '';
            my $lid            = defined $8 ? $8 : '';
            my $helpid         = defined $9 ? $9 : '';
            my $lang           = defined $12 ? $12 : '';
            my $plattform      = defined $10 ? $10 : '';
            
            chomp( $line ); 
            if( $isolang=~ /$lang/i ) { $fallbackhashhash_ref->{ $isolang }{ $prj.$gid.$lid.$file.$type.$plattform.$helpid } =  $line ; 
            }
        }
    }
}

#########################################################
sub parseLanguages{

    my $bAll;
    my $bUseLocalize;
    my $bHasSourceLanguage="";
    my $bFakeEnglish="";
    my %langhash;
    my $iso="";
    my $fallback="";
  
    #### -l all
    if(   $languages=~ /all/ ){
        $bAll = "TRUE";
        $bHasSourceLanguage = "TRUE";
    }
    ### -l fr=de,de
    elsif( $languages=~ /.*,.*/ ){
        my @tmpstr =  split "," , $languages;
        for my $lang ( @tmpstr ){
            if( $lang=~ /([a-zA-Z]{2,3}(-[a-zA-Z\-]*)*)(=([a-zA-Z]{2,3}(-[a-zA-Z\-]*)*))?/ ){
                $iso        = $1;
                $fallback   = $4;
                
                if( ( $iso && $iso=~ /(en-US)/i )  || ( $fallback && $fallback=~ /(en-US)/i ) ) {
                    $bUseLocalize = "TRUE";
                }
                if( ( $iso && $iso=~ /(en-US)/i ) ) {
                    $bHasSourceLanguage = "TRUE";
                } 
             if( $fallback ) { $langhash{ $iso } = $fallback;   }
             else            { $langhash{ $iso } = "";          }
            }
        }
    }
    ### -l de
    else{
        if( $languages=~ /([a-zA-Z]{2,3}(-[a-zA-Z\-]*)*)(=([a-zA-Z]{2,3}(-[a-zA-Z\-]*)*))?/ ){
            $iso        = $1;
            $fallback   = $4;

            if( ( $iso && $iso=~ /(en-US)/i )  || ( $fallback && $fallback=~ /(en-US)/i ) ) {
                $bUseLocalize = "TRUE";

            }
            if( ( $iso && $iso=~ /(en-US)/i )  ) {
                $bHasSourceLanguage = "TRUE";
            } 

             if( $fallback ) { $langhash{ $iso } = $fallback;   }
             else            { $langhash{ $iso } = "";          }
        }    
    }
    # HACK en-US always needed!
    if( !$bHasSourceLanguage ){
        #$bHasSourceLanguage = "TRUE";
        $bUseLocalize = "TRUE";
        $bFakeEnglish = "TRUE";
        $langhash{ "en-US" } = "";
    }
    return ( $bAll ,  $bUseLocalize , \%langhash , $bHasSourceLanguage, $bFakeEnglish);
}

#########################################################
sub parse_options{
 
    my $help;
    my $merge;
    my $extract;
    my $success = GetOptions('f=s' => \$sdffile , 'l=s' => \$languages , 's=s' => \$srcpath ,  'h' => \$help , 'v' => \$bVerbose , 
                             'm' => \$merge , 'e' => \$extract , 'x' => \$no_sort , 'd' => \$use_default_date );
    $outputfile = $sdffile;
    
    #print STDOUT "DBG: lang = $languages\n";
    if( !$srcpath ){
        #$srcpath = "$ENV{SRC_ROOT}";
        if( !$srcpath ){
	        print STDERR "No path to the source root found!\n\n";
	        usage();
            exit(1);
        }
    }
    if( $help || !$success || $#ARGV > 1 || ( !$sdffile ) ){
        usage();
        exit(1);
    }
    if( $merge && $sdffile && ! ( -r $sdffile)){
        print STDERR "Can't open file '$sdffile'\n";
        exit(1);    
    }
    if( !( $languages=~ /[a-zA-Z]{2,3}(-[a-zA-Z\-]*)*(=[a-zA-Z]{2,3}(-[a-zA-Z\-]*)*)?(,[a-zA-Z]{2,3}(-[a-zA-Z\-]*)*(=[a-zA-Z]{2,3}(-[a-zA-Z\-]*)*)?)*/ ) ){
        print STDERR "Please check the -l iso code\n";
        exit(1);
    }
    if( ( !$merge && !$extract ) || ( $merge && $extract ) ){ usage();exit( -1 );}
    if( $extract ){ $mode = "extract"; }
    else          { $mode = "merge";   }
}

#########################################################
sub usage{

    print STDERR "Usage: localize.pl\n";
    print STDERR "Split or collect SDF files\n";
    print STDERR "           merge: -m -f <sdffile>    -l l1[=f1][,l2[=f2]][...] [ -s <sourceroot> ]\n";
    print STDERR "         extract: -e -f <outputfile> -l <lang> [ -s <sourceroot> ] [-d]\n";
    print STDERR "Options:\n";
    print STDERR "    -h              help\n";
    print STDERR "    -m              Merge mode\n";
    print STDERR "    -e              Extract mode\n"; 
    print STDERR "    -f <sdffile>    To split a big SDF file into particles\n";
    print STDERR "       <outputfile> To collect and join all particles to one big file\n";
    print STDERR "    -s <sourceroot> Path to the modules, if no \$SRC_ROOT is set\n";
    print STDERR "    -l ( all | <isocode> | <isocode>=fallback ) comma seperated languages\n";
    print STDERR "    -d              Use default date in extracted sdf file\n";
    print STDERR "    -v              Verbose\n";
    print STDERR "\nExample:\n";
    print STDERR "\nlocalize -e -l en-US,pt-BR=en-US -f my.sdf\n( Extract en-US and pt-BR with en-US fallback )\n";
    print STDERR "\nlocalize -m -l cs -f my.sdf\n( Merge cs translation into the sourcecode ) \n";
}

#            my $line           = defined $_ ? $_ : '';
#            my $leftpart       = defined $2 ? $2 : '';
#            my $prj            = defined $3 ? $3 : '';
#            my $file           = defined $4 ? $4 : '';
#            my $dummy          = defined $5 ? $5 : '';
#            my $type           = defined $6 ? $6 : '';
#            my $gid            = defined $7 ? $7 : '';
#            my $lid            = defined $8 ? $8 : '';
#            my $helpid         = defined $9 ? $9 : '';
#            my $plattform      = defined $10 ? $10 : '';
#            my $width          = defined $11 ? $11 : '';
#            my $lang           = defined $12 ? $12 : '';
#            my $rightpart      = defined $13 ? $13 : '';
#            my $text           = defined $14 ? $14 : '';
#            my $helptext       = defined $15 ? $15 : '';
#            my $quickhelptext  = defined $16 ? $16 : '';
#            my $title          = defined $17 ? $17 : '';
#            my $timestamp      = defined $18 ? $18 : '';
 
