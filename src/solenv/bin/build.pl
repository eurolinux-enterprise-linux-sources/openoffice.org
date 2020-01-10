:
    eval 'exec perl -S $0 ${1+"$@"}'
        if 0;
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
#
# build - build entire project
#
    use Config;
    use POSIX;
    use Cwd qw (cwd);
    use File::Path;
    use File::Temp qw(tmpnam);
    use File::Find;
    use Socket;
    use IO::Socket::INET;
    
    use lib ("$ENV{SOLARENV}/bin/modules");
    use SourceConfig;
    
    my $in_so_env = 0;
    if (defined $ENV{COMMON_ENV_TOOLS}) {
        unshift(@INC, "$ENV{COMMON_ENV_TOOLS}/modules");
        $in_so_env++;
    };
    if (defined $ENV{CWS_WORK_STAMP}) {
        require GenInfoParser; import GenInfoParser;
        require IO::Handle; import IO::Handle;
    };
    my $enable_multiprocessing = 1;
    my $cygwin = 0;
    $cygwin++ if ($^O eq 'cygwin');
    if ($ENV{GUI} eq 'WNT' && !$cygwin) {
        eval { require Win32::Process; import Win32::Process; };
        $enable_multiprocessing = 0 if ($@);
    };
    
    ### for XML file format
    eval { require XMLBuildListParser; import XMLBuildListParser; };
    if (!$@) {
        $enable_xml = 1;
        @modes_array = split('\s' , $ENV{BUILD_TYPE});
    };
#### script id #####

    ( $script_name = $0 ) =~ s/^.*\b(\w+)\.pl$/$1/;

    $id_str = ' $Revision$ ';
    $id_str =~ /Revision:\s+(\S+)\s+\$/
      ? ($script_rev = $1) : ($script_rev = "-");

    print "$script_name -- version: $script_rev\n";

#########################
#                       #
#   Globale Variablen   #
#                       #
#########################

    $modules_number++;
    $perl = "";
    $remove_command = "";
    if ( $^O eq 'MSWin32' ) {
        $perl = "$ENV{PERL}";
        $remove_command = "rmdir /S /Q";
        $nul = '> NULL';
    } else {
        use Cwd 'chdir';
        $perl = 'perl';
        $remove_command = 'rm -rf';
        $nul = '> /dev/null';
    };

    $QuantityToBuild = 0;
# delete $pid when not needed
    %projects_deps_hash = ();   # hash of projects with no dependencies,
                                # that could be built now
    %broken_build = ();         # hash of hashes of the modules,
                                # where build was broken (error occurred)
    %folders_hashes = ();
    %running_children = ();
    $dependencies_hash = 0;
    $cmd_file = '';
    $BuildAllParents = 0;
    $show = 0;
    $checkparents = 0;
    $deliver = 0;
    $pre_custom_job = '';
    $custom_job = '';
    $post_custom_job = '';
    %LocalDepsHash = ();
    %BuildQueue = ();
    %PathHash = ();
    %PlatformHash = ();
    %AliveDependencies = ();
    %global_deps_hash = (); # hash of dependencies of the all modules
    %broken_modules_hashes = ();   # hash of modules hashes, which cannot be built further
    @broken_modules_names = ();   # array of modules, which cannot be built further
    @dmake_args = ();
    %dead_parents = ();
    $CurrentPrj = '';
    $no_projects = 0;
    $only_dependent = 0;
    $build_from = '';
    $build_all_cont = '';
    $build_since = '';
    $dlv_switch = '';
    $child = 0;
    %processes_hash = ();
#    %module_announced = ();
    $prepare = ''; # prepare for following incompatible build
    $ignore = '';
    $html = '';
    @ignored_errors = ();
    %incompatibles = ();
    $only_platform = ''; # the only platform to prepare
    $only_common = ''; # the only common output tree to delete when preparing
    %build_modes = ();
    $maximal_processes = 0; # the max number of the processes run
    %modules_types = (); # modules types ('mod', 'img', 'lnk') hash
    %platforms = (); # platforms available or being working with
    %platforms_to_copy = (); # copy output trees for the platforms when --prepare
    $tmp_dir = get_tmp_dir(); # temp directory for checkout and other actions
#    $dmake_batch = undef;     #
    @possible_build_lists = ('build.lst', 'build.xlist'); # build lists names
    %build_list_paths = (); # build lists names
    %build_lists_hash = (); # hash of arrays $build_lists_hash{$module} = \($path, $xml_list_object) 
    $pre_job = 'announce'; # job to add for not-single module build
    $post_job = '';        # -"-
    %windows_procs = ();
    @warnings = (); # array of warnings to be shown at the end of the process
    @errors = (); # array of errors to be shown at the end of the process
    %html_info = (); # hash containing all necessary info for generating of html page
    %module_by_hash = (); # hash containing all modules names as values and correspondent hashes as keys
    %build_in_progress = (); # hash of modules currently being built 
    %build_is_finished = (); # hash of already built modules
    %modules_with_errors = (); # hash of modules with build errors
    %build_in_progress_shown = ();  # hash of modules being built, 
                                    # and shown last time (to keep order)
    $build_time = time;
    $html_last_updated = 0;
    %jobs_hash = ();
    $html_path = undef;
    $html_file = CorrectPath($ENV{SOLARSRC} . '/' . $ENV{INPATH}. '.build.html');
    $build_finished = 0;
    %had_error = (); # hack for misteriuos windows problems - try run dmake 2 times if first time there was an error
    $mkout = CorrectPath("$ENV{SOLARENV}/bin/mkout.pl");
    %weights_hash = (); # hash contains info about how many modules are dependent from one module
#    %weight_stored = ();
    $grab_output = 1;
    $stop_build_on_error = 0; # for multiprocessing mode: do not build further module if there is an error
    $server_mode = 0;
    $setenv_string = ''; # string for configuration of the client environment
    $ports_string = ''; # string with possible ports for server
    @server_ports = ();
    $socket_obj = undef; # socket object for server
    my %clients_jobs = ();
    my %clients_times = ();
    my $client_timeout = 0; # time for client to build (in sec)... 
                            # The longest time period after that
                            # the server considered as an error/client crash 
    my %lost_client_jobs = (); # hash containing lost jobs
    my %job_jobdir = (); # hash containing job-dir pairs
    my %module_paths = (); # hash with absolute module paths  
    my %active_modules = ();
    my $generate_config = 0;
    my %add_to_config = ();
    my %remove_from_config = ();
    my $clear_config = 0;
### main ###

    get_options();
    
    $html_file = CorrectPath($html_path . '/' . $ENV{INPATH}. '.build.html') if (defined $html_path);
#    my $temp_html_file = CorrectPath($tmp_dir. '/' . $ENV{INPATH}. '.build.html');
    get_build_modes();
    %deliver_env = ();
    if ($prepare) {
        get_platforms(\%platforms);
        @modules_built = ();

        $deliver_env{'BUILD_SOSL'}++;
        $deliver_env{'COMMON_OUTDIR'}++;
        $deliver_env{'GUI'}++;
        $deliver_env{'INPATH'}++;
        $deliver_env{'OFFENV_PATH'}++;
        $deliver_env{'OUTPATH'}++;
        $deliver_env{'L10N_framework'}++;
    };

    if ($generate_config) {
        generate_config_file();
        exit 0;
    }
    $StandDir = get_stand_dir();   # This also sets $CurrentPrj
    get_module_and_buildlist_paths();
    provide_consistency() if (defined $ENV{CWS_WORK_STAMP} && defined($ENV{COMMON_ENV_TOOLS}));

    $deliver_command = $ENV{DELIVER};
    $deliver_command .= ' '. $dlv_switch if ($dlv_switch);
    $ENV{mk_tmp}++;
    %prj_platform = ();
    $check_error_string = '';
    $dmake = '';
#    $dmake_bin = '';
    $dmake_args = '';
    $echo = '';
    $new_line = "\n";

    get_commands();
    unlink ($cmd_file);
    if ($cmd_file) {
        if (open (CMD_FILE, ">>$cmd_file")) {
            select CMD_FILE;
            $echo = 'echo ';
            if ($ENV{GUI} ne 'UNX') {
                $new_line = "echo.\n";
                print "\@$echo off\npushd\n";
            } else {
                $new_line = $echo."\"\"\n";
            };
        } else {
            print_error ("Cannot open file $cmd_file");
        };
#    } elsif ($show) {
#        select STDOUT;
    };

    print $new_line;

    if ($checkparents) {
	    GetParentDeps( $CurrentPrj, \%global_deps_hash );
    } else {
	    BuildAll();
    }
    if (scalar keys %broken_build) {
        cancel_build();
#    } elsif (!$custom_job && $post_custom_job) {
#        do_post_custom_job(CorrectPath($StandDir.$CurrentPrj));
    };
    print_warnings();
    if (scalar keys %active_modules) {
        foreach (keys %dead_parents) {
            delete $dead_parents{$_} if (!defined $active_modules{$_});
        };
    };
    if (scalar keys %dead_parents) {
        my ($DeadPrj);
        print $new_line.$new_line;
        print $echo."WARNING! Project(s):\n";
        foreach $DeadPrj (keys %dead_parents) {
            print $echo."$DeadPrj\n";
        };
        print $new_line;
        print $echo."not found and couldn't be built. Dependencies on that module(s) ignored. Maybe you should correct build lists.\n";
        print $new_line;
        do_exit(1) if ($checkparents);
    };
    if (($ENV{GUI} ne 'UNX') && $cmd_file) {
        print "popd\n";
    };
    $ENV{mk_tmp} = '';
    if ($cmd_file) {
        close CMD_FILE;
        print STDOUT "Script $cmd_file generated\n";
    };
    if ($ignore && scalar @ignored_errors) {
        print STDERR "\nERROR: next directories could not be built:\n";
        foreach (@ignored_errors) {
            print STDERR "\t$_\n";
        };
        print STDERR "\nERROR: please check these directories and build the corresponding module(s) anew!!\n\n";
        do_exit(1);
    };
    do_exit(0);


#########################
#                       #
#      Procedures       #
#                       #
#########################

sub print_warnings {
    if (scalar @warnings) {
        print STDERR "\nWARNING(S):\n";
        print STDERR $_ foreach (@warnings);
    };
};

sub rename_file {
    my ($old_file_name, $new_file_name, $throw_error) = @_;
    
    if(-e $old_file_name) {
        rename($old_file_name, $new_file_name) or system("mv", $old_file_name, $new_file_name);
        if (-e $old_file_name) {
            system("rm -rf $old_file_name") if (!unlink $old_file_name);
        };
    } elsif ($throw_error) {
        print_error("No such file $old_file_name");
    };
};

sub generate_config_file {
    my $source_config = SourceConfig -> new();
    my $source_config_file = $source_config->get_config_file_path();
    my $temp_config_file = File::Temp::tmpnam($ENV{TMP});
    my @config_content_new = ();
    my $addition_message;
    my $removal_message;
    my %present_modules = ();
    if ($source_config_file) {
        open(SOURCE_CONFIG_FILE, $source_config_file);
        my @config_content = <SOURCE_CONFIG_FILE>;
        close SOURCE_CONFIG_FILE;
        my ($module_section, $repository_section);
        foreach (@config_content) {
            $line++;
            if ((!/^\S+/)||(/^\s*#+/)) {
                push(@config_content_new, $_);
                next;
            }
            if (/^\[repositories\]\s*(\s+#)*/) {
                if ($module_section) {
                    $addition_message = add_modules_to_source_config(\%add_to_config, \@config_content_new);
                };
                $module_section = 0;
                $repository_section = 1;
                push(@config_content_new, $_);
                next;
            };
            if (/^\[modules\]\s*(\s+#)*/) {
                $module_section = 1;
                $repository_section = 0;
                push(@config_content_new, $_);
                next;
            };
            if ($module_section && /\s*(\S+)=active\s*(\s+#)*/) {
                if ($clear_config || defined $remove_from_config{$1}) {
                    delete $remove_from_config{$1};
                    $removal_message .= "$1 ";
                } else {
                    push(@config_content_new, $_);
                    if (defined $add_to_config{$1}) {
                        push(@warnings, "Module $1 already activated in $source_config_file\n");
                        delete $add_to_config{$1};
                    }
                };
            } else {
                push(@config_content_new, $_);
            };
        };
                if (keys %add_to_config) {
                    if (!$module_section) {
                        push(@config_content_new, "[modules]\n");
                    };
                    $addition_message = add_modules_to_source_config(\%add_to_config, \@config_content_new);
                };
    } else {
        if ($clear_config || scalar %remove_from_config) {
            print_error('No source config file found');
        };
        $source_config_file = $source_config->get_config_file_default_path();
        push(@config_content_new, "[modules]\n");
        $addition_message = add_modules_to_source_config(\%add_to_config, \@config_content_new);
    };
    die("Cannot open $temp_config_file") if (!open(NEW_CONFIG, ">$temp_config_file"));
    print NEW_CONFIG $_ foreach (@config_content_new);
    close NEW_CONFIG;
    rename_file($temp_config_file, $source_config_file, 1);
    foreach (keys %remove_from_config) {
        push(@warnings, "Module(s) $_ not found in " . $source_config_file . "\n");
    };
    print_warnings();
    print $addition_message if ($addition_message);
    print "Module(s) $removal_message removed from $source_config_file\n" if ($removal_message);
    exit(0);
};

#
# Add modules from the passed hash to the array of config strigns
#
sub add_modules_to_source_config {
    my ($modules_hash_ref, $config_content_new) = @_;
    my $message;
    foreach (keys %$modules_hash_ref) {
        push(@$config_content_new, "$_=active\n");
        $message .= "$_ ";
    };
    if ($message) {
        return 'Module(s) ' .$message . 'are added to the ' . $source_config_file . "\n\n";
    } else {
        return '';
    };
};

#
# procedure retrieves build list path
# (all possibilities are taken into account)
#
sub get_build_list_path {
    my $module = shift;
    my @possible_dirs = ($module, $module. '.lnk', $module. '.link');
    return $build_list_paths{$module} if (defined $build_list_paths{$module});
    foreach (@possible_dirs) {
        my $possible_dir_path = $module_paths{$_}.'/prj/';
        if (-d $possible_dir_path) {
            foreach my $build_list (@possible_build_lists) {
                my $possible_build_list_path = CorrectPath($possible_dir_path . $build_list);
                if (-f $possible_build_list_path) {
                    $build_list_paths{$module} = $possible_build_list_path;
                    return $possible_build_list_path;
                };
            }
            print_error("There's no build list for $module");
        };
    };
    $dead_parents{$module}++;
    $build_list_paths{$module} = CorrectPath(retrieve_build_list($module)) if (!defined $build_list_paths{$module});
    return $build_list_paths{$module};
};
    
#
# Get dependencies hash of the current and all parent projects
#
sub GetParentDeps {
    my (%parents_deps_hash, $module, $parent);
    my $prj_dir = shift;
    my $deps_hash = shift;
    my @UnresolvedParents = get_parents_array($prj_dir);
    $parents_deps_hash{$_}++ foreach (@UnresolvedParents);
    $$deps_hash{$prj_dir} = \%parents_deps_hash;
    while ($module = pop(@UnresolvedParents)) {
        my %parents_deps_hash = ();
        $parents_deps_hash{$_}++ foreach (get_parents_array($module));
        $$deps_hash{$module} = \%parents_deps_hash;
        foreach $Parent (keys %parents_deps_hash) {
            if (!defined($$deps_hash{$Parent})) {
                push (@UnresolvedParents, $Parent);
            };
        };
    };
    check_deps_hash($deps_hash);
};

sub store_weights {
    my $deps_hash = shift;
    foreach (keys %$deps_hash) {
        foreach my $module_deps_hash ($$deps_hash{$_}) {
            foreach my $dependency (keys %$module_deps_hash) {
                $weights_hash{$dependency}++;
            };
        };
    };
};

#
# Build everything that should be built
#
sub BuildAll {
    if ($BuildAllParents) {
        my ($Prj, $PrjDir, $orig_prj);
        GetParentDeps( $CurrentPrj, \%global_deps_hash);
        if (scalar keys %active_modules) {
            $active_modules{$CurrentPrj}++;
            $modules_types{$CurrentPrj} = 'mod';
        };
        modules_classify(keys %global_deps_hash);
        store_weights(\%global_deps_hash);
        if (keys %active_modules && ($build_from || $incompatible)) {
            print_error("There are active module in $source_config_file. Please remove these modules to proceed.\n");
        };
        prepare_build_from(\%global_deps_hash) if ($build_from);
        prepare_incompatible_build(\%global_deps_hash) if ($incompatible);
        if ($build_all_cont || $build_since) {
            print STDERR "There are active module in $source_config_file. Inactive modules will be skipped.\n";
            push (@warnings, "\nThere are active module in $source_config_file. Inactive modules are skipped.\n\n");
            prepare_build_all_cont(\%global_deps_hash);
        };
        $modules_number = scalar keys %global_deps_hash;
        initialize_html_info($_) foreach (keys %global_deps_hash);
        if ($QuantityToBuild) {
            build_multiprocessing();
            return;
        };
        if ($server_mode) {
            run_server();
        };
        while ($Prj = PickPrjToBuild(\%global_deps_hash)) {
            if (!defined $dead_parents{$Prj}) {              
                if (scalar keys %broken_build) {
                    print $echo . "Skipping project $Prj because of error(s)\n";
                    RemoveFromDependencies($Prj, \%global_deps_hash);
                    $build_is_finished{$Prj}++;
                    next;
                };

                $PrjDir = $module_paths{$Prj};
                get_deps_hash($Prj, \%LocalDepsHash);
                my $info_hash = $html_info{$Prj};
                $$info_hash{DIRS} = check_deps_hash(\%LocalDepsHash, $Prj);
                $module_by_hash{\%LocalDepsHash} = $Prj;
                BuildDependent(\%LocalDepsHash);
                print $check_error_string;
            };
            
            RemoveFromDependencies($Prj, \%global_deps_hash);
            $build_is_finished{$Prj}++;
            $no_projects = 0;
        };
    } else {
        store_build_list_content($CurrentPrj);
        get_deps_hash($CurrentPrj, \%LocalDepsHash);
        initialize_html_info($CurrentPrj);
        my $info_hash = $html_info{$CurrentPrj};
        $$info_hash{DIRS} = check_deps_hash(\%LocalDepsHash, $CurrentPrj);
        $module_by_hash{\%LocalDepsHash} = $CurrentPrj;
        if ($server_mode) {
            run_server();
        } else {
            BuildDependent(\%LocalDepsHash);
        };
    };
};

sub initialize_html_info {
    my $module = shift;
    return if (defined $dead_parents{$module});
    $html_info{$module} = { 'DIRS' => [],
                            'ERRORFUL' => [],
                            'SUCCESSFUL' => [], 
                            'BUILD_TIME' => 0};
}

#
# Do job 
#
sub dmake_dir {
    my ($new_BuildDir, $OldBuildDir, $error_code);
    my $BuildDir = shift;
    $jobs_hash{$BuildDir}->{START_TIME} = time();
    $jobs_hash{$BuildDir}->{STATUS} = 'building';
    if ($BuildDir =~ /(\s)/o) {
        $error_code = do_custom_job($BuildDir, \%LocalDepsHash);
    } else {
        html_store_job_info(\%LocalDepsHash, $BuildDir);
        print_error("$BuildDir not found!!\n") if (!-d $BuildDir);
        if (!-d $BuildDir) {
            $new_BuildDir = $BuildDir;
            $new_BuildDir =~ s/_simple//g;
            if ((-d $new_BuildDir)) {
                print("\nTrying $new_BuildDir, $BuildDir not found!!\n");
                $BuildDir = $new_BuildDir;
            } else {
                print_error("\n$BuildDir not found!!\n");
            }
        }
        if ($cmd_file) {
            print "cd $BuildDir\n";
            print $check_error_string;
            print $echo.$BuildDir."\n";
            print "$dmake\n";
            print $check_error_string;
        } else {
            print "$BuildDir\n";
        };
        RemoveFromDependencies($BuildDir, \%LocalDepsHash) if (!$child);
        return if ($cmd_file || $show);
        $error_code = run_job($dmake, $BuildDir);
        html_store_job_info(\%LocalDepsHash, $BuildDir, $error_code) if (!$child);
    };
    if ($error_code && $ignore) {
        push(@ignored_errors, $BuildDir);
        $error_code = 0;
    };
    if ($child) {
        my $oldfh = select STDERR;
        $| = 1;
        select $oldfh;
        $| =1;
        if ($error_code) {
            _exit($error_code >> 8);
        } else {
            _exit($? >> 8) if ($? && ($? != -1));
        };
        _exit(0);
    } elsif ($error_code && ($error_code != -1)) {
        print_error("Error $? occurred while making $BuildDir");
    };
};

#
# Procedure stores information about build list (and)
# build list object in build_lists_hash
#
sub store_build_list_content {
    my $module = shift;
    my $build_list_path = get_build_list_path($module);
    return undef if (!defined $build_list_path);
    return if (!$build_list_path);
    my $xml_list = undef;
    if ($build_list_path =~ /\.xlist$/o) {
        print_error("XMLBuildListParser.pm couldn\'t be found, so XML format for build lists is not enabled") if (!defined $enable_xml);
        $xml_list = XMLBuildListParser->new();
        if (!$xml_list->loadXMLFile($build_list_path)) {
            print_error("Cannot use $build_list_path");
        };
        $build_lists_hash{$module} = $xml_list;
    } else {
        if (open (BUILD_LST, $build_list_path)) {
            my @build_lst = <BUILD_LST>;
            $build_lists_hash{$module} = \@build_lst;
            close BUILD_LST;
            return;
        }
        $dead_parents{$module}++;
    };
}
#
# Get string (list) of parent projects to build
#
sub get_parents_array {
    my $module = shift;
    store_build_list_content($module);
    my $build_list_ref = $build_lists_hash{$module};
    
    if (ref($build_list_ref) eq 'XMLBuildListParser') {
        return $build_list_ref->getModuleDependencies(\@modes_array);
    };
    foreach (@$build_list_ref) {
        if ($_ =~ /#/) {
            if ($`) {
                $_ = $`;
            } else {
                next;
            };
        };
        s/\r\n//;
        if ($_ =~ /\:+\s+/) {
            return pick_for_build_type($');
        };
    };
    return ();
};

#
# get folders' platform infos
#
sub get_prj_platform {
    my $build_list_ref = shift;
    my ($prj_alias, $line);
    foreach(@$build_list_ref) {
        s/\r\n//;
        $line++;
        if ($_ =~ /\snmake\s/) {
            if ($' =~ /\s*-\s+(\w+)[,\S+]*\s+(\S+)/ ) {
                my $platform = $1;
                my $alias = $2;
                print_error ("There is no correct alias set in the line $line!") if ($alias eq 'NULL');
                mark_platform($alias, $platform);
            } else {
                print_error("Misspelling in line: \n$_");
            };
        };
    };
#seek(BUILD_LST, 0, 0);
};

#
# Procedure populate the dependencies hash with
# information from XML build list object
#
sub get_deps_from_object {
    my ($module, $build_list_object, $dependencies_hash) = @_;

    foreach my $dir ($build_list_object->getJobDirectories("make", $ENV{GUI})) {
        $PathHash{$dir} = $module_paths{$module};
        $PathHash{$dir} .= $dir if ($dir ne '/');
        my %deps_hash = ();
        
        foreach my $dep ($build_list_object->getJobDependencies($dir, "make", $ENV{GUI})) {
            $deps_hash{$dep}++;
        };
        $$dependencies_hash{$dir} = \%deps_hash;
    };
};


#
# Getting hashes of all internal dependencies and additional
# information for given project
#
sub get_deps_hash {
    my ($dummy, $module_to_build);
    %DeadDependencies = ();
    $module_to_build = shift;
    my $dependencies_hash = shift;
    if ($custom_job) {
        if ($modules_types{$module_to_build} ne 'lnk') {
            add_prerequisite_job($dependencies_hash, $module_to_build, $pre_custom_job);
            add_prerequisite_job($dependencies_hash, $module_to_build, $pre_job);
            add_dependent_job($dependencies_hash, $module_to_build, $custom_job);
            add_dependent_job($dependencies_hash, $module_to_build, $post_job);
            add_dependent_job($dependencies_hash, $module_to_build, $post_custom_job);
        };
        return;
    };
    if ( defined $modules_types{$module_to_build} && $modules_types{$module_to_build} ne 'mod') {
        add_prerequisite_job($dependencies_hash, $module_to_build, $pre_job);
        return;
    };

    my  $build_list_ref = $build_lists_hash{$module_to_build};
    delete $build_lists_hash{$module_to_build};
    if (ref($build_list_ref) eq 'XMLBuildListParser') {
        get_deps_from_object($module_to_build, $build_list_ref, $dependencies_hash);
    } else {
        get_prj_platform($build_list_ref);
        foreach (@$build_list_ref) {
            if ($_ =~ /#/o) {
                next if (!$`);
                $_ = $`;
            };
            s/\r\n//;
            if ($_ =~ /\s+nmake\s+/o) {
                my ($Platform, $Dependencies, $Dir, $DirAlias);
                my %deps_hash = ();
                $Dependencies = $';
                $dummy = $`;
                $dummy =~ /(\S+)\s+(\S*)/o;
                $Dir = $2;
                $Dependencies =~ /(\w+)/o;
                $Platform = $1;
                $Dependencies = $';
                while ($Dependencies =~ /,(\w+)/o) {
                    $Dependencies = $';
                };
                $Dependencies =~ /\s+(\S+)\s+/o;
                $DirAlias = $1;
                if (!CheckPlatform($Platform)) {
                    next if (defined $PlatformHash{$DirAlias});
                    $DeadDependencies{$DirAlias}++;
                    next;
                };
                delete $DeadDependencies{$DirAlias} if (defined $DeadDependencies{$DirAlias});
                print_error("Directory alias $DirAlias is defined at least twice!! Please, correct build.lst in module $module_to_build") if (defined $$dependencies_hash{$DirAlias});
                $PlatformHash{$DirAlias}++;
                $Dependencies = $';
                print_error("$module_to_build/prj/build.lst has wrongly written dependencies string:\n$_\n") if (!$Dependencies);
                $deps_hash{$_}++ foreach (GetDependenciesArray($Dependencies));
                $$dependencies_hash{$DirAlias} = \%deps_hash;
                $BuildQueue{$DirAlias}++;
                my $local_dir = '';
                if ($Dir =~ /(\\|\/)/o) {
                    $local_dir = $';
                };
                $PathHash{$DirAlias} = CorrectPath($module_paths{$module_to_build} . "/$local_dir");
            } elsif ($_ !~ /^\s*$/ && $_ !~ /^\w*\s/o) {
                chomp;
                push(@errors, $_);
            };
        };
        if (scalar @errors) {
            my $message = "$module_to_build/prj/build.lst has wrongly written string(s):\n";
            $message .= "$_\n" foreach(@errors);
            if ($QuantityToBuild) {
                $broken_build{$module_to_build} = $message;
                $dependencies_hash = undef;
                return;
            } else {
                print_error($message);
            };
        };
        foreach my $alias (keys %DeadDependencies) {
            next if defined $AliveDependencies{$alias};
            if (!IsHashNative($alias)) {
                RemoveFromDependencies($alias, $dependencies_hash);
                delete $DeadDependencies{$alias};
            };
        };
    };
#    check_deps_hash($dependencies_hash);
    resolve_aliases($dependencies_hash, \%PathHash);
    if (!$prepare) {
        add_prerequisite_job($dependencies_hash, $module_to_build, $pre_custom_job);
        add_prerequisite_job($dependencies_hash, $module_to_build, $pre_job);
        add_dependent_job($dependencies_hash, $module_to_build, $custom_job);
        add_dependent_job($dependencies_hash, $module_to_build, $post_job) if ($module_to_build ne $CurrentPrj);
        add_dependent_job($dependencies_hash, $module_to_build, $post_custom_job);
    };
    store_weights($dependencies_hash);
};

#
# procedure adds which is independent from anothers, but anothers are dependent from it
#
sub add_prerequisite_job {
    my ($dependencies_hash, $module, $job) = @_;
    return if (!$job);
    $job = "$module $job";
    foreach (keys %$dependencies_hash) {
        $deps_hash = $$dependencies_hash{$_};
        $$deps_hash{$job}++;
    };
    $$dependencies_hash{$job} = {};
};

#
# procedure adds a job wich is dependent from all already registered jobs
#
sub add_dependent_job {
    # $post_job is dependent from all jobs
    my ($dependencies_hash, $module, $job) = @_;
    return if (!$job);
    my %deps_hash = ();
    $deps_hash{$_}++ foreach (keys %$dependencies_hash);
    $$dependencies_hash{"$module $job"} = \%deps_hash;
};

#
# this procedure converts aliases to absolute paths
#
sub resolve_aliases {
    my ($dependencies_hash, $PathHash) = @_;
    foreach my $dir_alias (keys %$dependencies_hash) {
        my $aliases_hash_ref = $$dependencies_hash{$dir_alias};
        my %paths_hash = ();
        foreach (keys %$aliases_hash_ref) {
            $paths_hash{$$PathHash{$_}}++;
        };
        delete $$dependencies_hash{$dir_alias};
        $$dependencies_hash{$$PathHash{$dir_alias}} = \%paths_hash;
    };
};

#
# mark platform in order to prove if alias has been used according to specs
#
sub mark_platform {
    my $prj_alias = shift;
    if (exists $prj_platform{$prj_alias}) {
        $prj_platform{$prj_alias} = 'all';
    } else {
        $prj_platform{$prj_alias} = shift;
    };
};

#
# Convert path from abstract (with '\' and/or '/' delimiters)
# to system-independent
#
sub CorrectPath {
    $_ = shift;
    if ( ($^O eq 'MSWin32') && (!defined $ENV{SHELL})) {
        s/\//\\/g;
    } else {;
        s/\\/\//g;
    };
    return $_;
};


sub check_dmake {
#print "Checking dmake...";
    if (open(DMAKEVERSION, "dmake -V |")) {
#    if (open(DMAKEVERSION, "dmake -V |")) {
        my @dmake_version = <DMAKEVERSION>;
        close DMAKEVERSION;
#       if ($dmake_version[0] =~ /^dmake\s\-\sCopyright\s\(c\)/) {
#            print " Using version $1\n" if ($dmake_version[0] =~ /Version\s(\d+\.*\d*)/);
#        };
        return;
    };
    my $error_message = 'dmake: Command not found.';
    $error_message .= ' Please rerun bootstrap' if (!defined $ENV{COMMON_ENV_TOOLS});
    print_error($error_message);
};

#
# Get platform-dependent commands
#
sub get_commands {
    my $arg = '';
    # Setting alias for dmake
    $dmake = 'dmake';
    check_dmake();

    if ($cmd_file) {
        if ($ENV{GUI} eq 'UNX') {
            $check_error_string = "if \"\$?\" != \"0\" exit\n";
        } else {
            $check_error_string = "if \"\%?\" != \"0\" quit\n";
        };
    };
    
    $dmake_args = join(' ', 'dmake', @dmake_args);
    
    while ($arg = pop(@dmake_args)) {
        $dmake .= ' '.$arg;
    };
};

#
# Procedure retrieves list of projects to be built from build.lst
#
sub get_stand_dir {
    if (!defined $ENV{GUI}) {
        $ENV{mk_tmp} = '';
        die "No environment set\n";
    };
    my $StandDir;
    if ( defined $ENV{PWD} ) {
	    $StandDir = $ENV{PWD};
	} elsif (defined $ENV{_cwd}) {
		$StandDir = $ENV{_cwd};
	} else {
		$StandDir = cwd();
    };
    my $previous_dir = '';
    do {
        foreach (@possible_build_lists) {# ('build.lst', 'build.xlist');
            if (-e $StandDir . '/prj/'.$_) {
                $CurrentPrj = File::Basename::basename($StandDir);
                $StandDir = File::Basename::dirname($StandDir);
                return $StandDir;
            } elsif ($StandDir eq $previous_dir) {
                $ENV{mk_tmp} = '';
                print_error('Found no project to build');
            };
        };
        $previous_dir = $StandDir;
        $StandDir = File::Basename::dirname(Cwd::realpath($StandDir));
        print_error('Found no project to build') if (!$StandDir);
    }
#    while (chdir '..');
    while (chdir "$StandDir");
};

#
# Picks project which can be built now from hash and then deletes it from hash
#
sub PickPrjToBuild {
    my $DepsHash = shift;
    handle_dead_children(0) if ($QuantityToBuild);
    my $Prj = FindIndepPrj($DepsHash);
    delete $$DepsHash{$Prj};
    generate_html_file();
    return $Prj;
};

#
# Make a decision if the project should be built on this platform
#
sub CheckPlatform {
    my $Platform = shift;
    return 1 if ($Platform eq 'all');
    return 1 if (($ENV{GUI} eq 'WIN') && ($Platform eq 'w'));
    return 1 if (($ENV{GUI} eq 'UNX') && ($Platform eq 'u'));
    return 1 if (($ENV{GUI} eq 'OS2') && ($Platform eq 'p'));
    return 1 if (($ENV{GUI} eq 'WNT') &&
                 (($Platform eq 'w') || ($Platform eq 'n')));
    return 0;
};

#
# Remove project to build ahead from dependencies and make an array
# of all from given project dependent projects
#
sub RemoveFromDependencies {
    my ($ExclPrj, $i, $Prj, $Dependencies);
    $ExclPrj = shift;
    my $ExclPrj_orig = '';
    $ExclPrj_orig = $` if (($ExclPrj =~ /\.lnk$/o) || ($ExclPrj =~ /\.link$/o));
    $Dependencies = shift;
    foreach $Prj (keys %$Dependencies) {
        my $prj_deps_hash = $$Dependencies{$Prj};
        delete $$prj_deps_hash{$ExclPrj} if (defined $$prj_deps_hash{$ExclPrj});
    };
};


#
# Check the hash for consistency
#
sub check_deps_hash {
    my ($deps_hash_ref, $module) = @_;
    my @possible_order;
    my $module_path = $module_paths{$module} if (defined $module);
    return if (!scalar keys %$deps_hash_ref);
    my %deps_hash = %$deps_hash_ref;
    my $consistent;
    foreach $key (keys %$deps_hash_ref) {
        my %values_hash = %{$$deps_hash_ref{$key}};
        $deps_hash{$key} = \%values_hash;
    };
    my $string;
    my $log_name;
    my $build_number = 0;

    do {
        $consistent = '';
        foreach $key (sort keys %deps_hash) {
            $local_deps_ref = $deps_hash{$key};
            if (!scalar keys %$local_deps_ref) {
                if (defined $module) {
                    $build_number++;
                    $string = undef;
                    if ($key =~ /(\s)/o) {
                        $string = $key;
                    } else {
                        if (length($key) == length($module_path)) {
                            $string = './';
                        } else {
                            $string = substr($key, length($module_path) + 1);
                            $string =~ s/\\/\//go;
                        };
                    };
                    $log_name = $string;
                    if ($log_name eq "$module $custom_job") {
                        $log_name = "custom_job";
                    };
                    if ($log_name eq "$module $pre_custom_job") {
                        $log_name = "pre_custom_job";
                    };
                    if ($log_name eq "$module $post_custom_job") {
                        $log_name = "post_custom_job";
                    };
                    $log_name =~ s/\\|\//\./g;
                    $log_name =~ s/\s/_/g;
                    $log_name = $module if ($log_name =~ /^\.+$/);
                    $log_name .= '.txt';
                    push(@possible_order, $key);
                    $jobs_hash{$key} = {    SHORT_NAME => $string,
                                            BUILD_NUMBER => $build_number,
                                            STATUS => 'waiting',
                                            LOG_PATH => $module . "/$ENV{INPATH}/misc/logs/$log_name",
                                            LONG_LOG_PATH => CorrectPath($module_paths{$module} . "/$ENV{INPATH}/misc/logs/$log_name"),
                                            START_TIME => 0,
                                            FINISH_TIME => 0,
                                            CLIENT => '-'
                    };
                };
                RemoveFromDependencies($key, \%deps_hash);
                delete $deps_hash{$key};
                $consistent++;
            };
        };
    } while ($consistent && (scalar keys %deps_hash));
    return \@possible_order if ($consistent);
    print STDERR "Fatal error:";
    foreach (keys %deps_hash) {
        print STDERR "\n\t$_ depends on: ";
        foreach my $i (keys %{$deps_hash{$_}}) {
            print STDERR (' ', $i);
        };
    };
    if ($child) {
        my $oldfh = select STDERR;
        $| = 1;
        _do_exit(1);
    } else {
        print_error("There are dead or circular dependencies\n");
    };
};

#
# Find project with no dependencies left.
#
sub FindIndepPrj {
    my ($Prj, @Prjs, $Dependencies, $i);
    my @candidates = ();
    my $children = children_number();
    return '' if (!$server_mode && $children && ($children >= $QuantityToBuild));
    $Dependencies = shift;
    @Prjs = keys %$Dependencies;
    if ($#Prjs != -1) {
        foreach $Prj (@Prjs) {
            next if (&IsHashNative($Prj));
            my $PrjDeps = $$Dependencies{$Prj};
            push(@candidates, $Prj) if (!scalar keys %$PrjDeps);
        };
        if (scalar @candidates) {
            my $best_candidate = undef;
            my $weight = 0;
            foreach my $candidate (sort @candidates) {
                if (defined $weights_hash{$candidate} && $weights_hash{$candidate} > $weight) {
                    $best_candidate = $candidate;
                    $weight = $weights_hash{$candidate};
                };
            };
            if (defined $best_candidate) {
                return $best_candidate;
            }
            my @sorted_candidates = sort(@candidates);
            return $sorted_candidates[0];
        };
        return '';
    } else {
        $no_projects = 1;
        return '';
    };
};

#
# Check if given entry is HASH-native, that is not a user-defined data
#
sub IsHashNative {
    my $Prj = shift;
    return 1 if ($Prj =~ /^HASH\(0x[\d | a | b | c | d | e | f]{6,}\)/);
    return 0;
};

#
# Getting array of dependencies from the string given
#
sub GetDependenciesArray {
    my ($DepString, @Dependencies, $ParentPrj, $prj, $string);
    @Dependencies = ();
    $DepString = shift;
    $string = $DepString;
    $prj = shift;
    while ($DepString !~ /^NULL/o) {
        print_error("Project $prj has wrongly written dependencies string:\n $string") if (!$DepString);
        $DepString =~ /(\S+)\s*/o;
        $ParentPrj = $1;
        $DepString = $';
        if ($ParentPrj =~ /\.(\w+)$/o) {
            $ParentPrj = $`;
            if (($prj_platform{$ParentPrj} ne $1) &&
                ($prj_platform{$ParentPrj} ne 'all')) {
                print_error ("$ParentPrj\.$1 is a wrongly dependency identifier!\nCheck if it is platform dependent");
            };
            $AliveDependencies{$ParentPrj}++ if (CheckPlatform($1));
            push(@Dependencies, $ParentPrj);
        } else {
            if ((exists($prj_platform{$ParentPrj})) &&
                ($prj_platform{$ParentPrj} ne 'all') ) {
                print_error("$ParentPrj is a wrongly used dependency identifier!\nCheck if it is platform dependent");
            };
            push(@Dependencies, $ParentPrj);
        };
    };
    return @Dependencies;
};


#
# Getting current directory list
#
sub GetDirectoryList {
    my ($Path);
    $Path = shift;
    opendir(CurrentDirList, $Path);
    @DirectoryList = readdir(CurrentDirList);
    closedir(CurrentDirList);
    return @DirectoryList;
};

sub print_error {
    my $message = shift;
    my $force = shift;
    $modules_number -= scalar keys %global_deps_hash;
    $modules_number -= 1;
    print STDERR "\nERROR: $message\n";
    $ENV{mk_tmp} = '';
    if ($cmd_file) {
        close CMD_FILE;
        unlink ($cmd_file);
    };
    if (!$child) {
        $ENV{mk_tmp} = '';
        close CMD_FILE if ($cmd_file);
        unlink ($cmd_file);
        do_exit(1);
    };
    do_exit(1) if (defined $force);
};

sub usage {
    print STDERR "\nbuild\n";
    print STDERR "Syntax:    build    [--all|-a[:prj_name]]|[--from|-f prj_name1[:prj_name2] [prj_name3 [...]]]|[--since|-c prj_name] [--with_branches|-b]|[--prepare|-p][:platform] [--dontchekoutmissingmodules]] [--deliver|-d [--dlv_switch deliver_switch]]] [-P processes|--server [--setenvstring \"string\"] [--client_timeout MIN] [--port port1[:port2:...:portN]]] [--show|-s] [--help|-h] [--file|-F] [--ignore|-i] [--version|-V] [--mode|-m OOo[,SO[,EXT]] [--html [--html_path html_file_path] [--dontgraboutput]] [--pre_job=pre_job_sring] [--job=job_string|-j] [--post_job=post_job_sring] [--stoponerror] [--genconf [--removeall|--clear|--remove|--add module1,module2[,...,moduleN]]]\n";
    print STDERR "Example1:    build --from sfx2\n";
    print STDERR "                     - build all projects dependent from sfx2, starting with sfx2, finishing with the current module\n";
    print STDERR "Example2:    build --all:sfx2\n";
    print STDERR "                     - the same as --all, but skip all projects that have been already built when using \"--all\" switch before sfx2\n";
    print STDERR "Example3:    build --all --server\n";
    print STDERR "                     - build all projects in server mode, use first available port from default range 7890-7894 (running clients required!!)\n";
    print STDERR "Example4(for unixes):\n";
    print STDERR "             build --all --pre_job=echo\\ Starting\\ job\\ in\\ \\\$PWD --job=some_script.sh --post_job=echo\\ Job\\ in\\ \\\$PWD\\ is\\ made\n";
    print STDERR "                     - go through all projects, echo \"Starting job in \$PWD\" in each module, execute script some_script.sh, and finally echo \"Job in \$PWD is made\"\n";
    print STDERR "\nSwitches:\n";
    print STDERR "        --all        - build all projects from very beginning till current one\n";
    print STDERR "        --from       - build all projects dependent from the specified (including it) till current one\n";
    print STDERR "        --mode OOo   - build only projects needed for OpenOffice.org\n";
    print STDERR "        --prepare    - clear all projects for incompatible build from prj_name till current one [for platform] (cws version)\n";
    print STDERR "        --with_branches- build all projects in neighbour branches and current branch starting from actual project\n";
    print STDERR "        --since      - build all projects beginning from the specified till current one (the same as \"--all:prj_name\", but skipping prj_name)\n";
    print STDERR "        --checkmodules      - check if all required parent projects are availlable\n";
    print STDERR "        --show       - show what is going to be built\n";
    print STDERR "        --file       - generate command file file_name\n";
    print STDERR "        --deliver    - only deliver, no build (usable for \'-all\' and \'-from\' keys)\n";
    print STDERR "        -P           - start multiprocessing build, with number of processes passed\n";
    print STDERR "        --server     - start build in server mode (clients required)\n";
    print STDERR "          --setenvstring  - string for configuration of the client environment\n";
    print STDERR "          --port          - set server port, default is 7890. You may pass several ports, the server will be started on the first available\n";
    print STDERR "                            otherwise the server will be started on first available port from the default range 7890-7894\n";
    print STDERR "          --client_timeout  - time frame after which the client/job is considered to be lost. Default is 120 min\n";
    print STDERR "        --dlv_switch - use deliver with the switch specified\n";
    print STDERR "        --help       - print help info\n";
    print STDERR "        --ignore     - force tool to ignore errors\n";
    print STDERR "        --html       - generate html page with build status\n";
    print STDERR "                       file named $ENV{INPATH}.build.html will be generated in $ENV{SOLARSRC}\n";
    print STDERR "          --html_path      - set html page path\n";
    print STDERR "          --dontgraboutput - do not grab console output when generating html page\n";
    print STDERR "        --genconf    - generate/modify workspace configuration file\n";
    print STDERR "          --add            - add active module(s) to configuration file\n";
    print STDERR "          --remove         - removeactive  modules(s) from configuration file\n";
    print STDERR "          --removeall|--clear          - remove all active modules(s) from configuration file\n";

    print STDERR "        --stoponerror      - stop build when error occurs (for mp builds)\n";
    print STDERR "        --dontchekoutmissingmodules - do not chekout missing modules when running prepare (links still will be broken)\n";
    print STDERR "   Custom jobs:\n";
    print STDERR "        --job=job_string        - execute custom job in (each) module. job_string is a shell script/command to be executed instead of regular dmake jobs\n";
    print STDERR "        --pre_job=pre_job_string        - execute preliminary job in (each) module. pre_job_string is a shell script/command to be executed before regular job in the module\n";
    print STDERR "        --post_job=job_string        - execute a postprocess job in (each) module. post_job_string is a shell script/command to be executed after regular job in the module\n";
    print STDERR "Default:             - build current project\n";
    print STDERR "Unknown switches passed to dmake\n";
};

#
# Get all options passed
#
sub get_options {
    my ($arg, $dont_grab_output);
    while ($arg = shift @ARGV) {
        $arg =~ /^-P$/            and $QuantityToBuild = shift @ARGV     and next;
        $arg =~ /^-P(\d+)$/            and $QuantityToBuild = $1 and next;
        $arg =~ /^--all$/        and $BuildAllParents = 1             and next;
        $arg =~ /^-a$/        and $BuildAllParents = 1             and next;
        $arg =~ /^--show$/        and $show = 1                         and next;
        $arg =~ /^--checkmodules$/       and $checkparents = 1 and $ignore = 1 and next;
        $arg =~ /^-s$/        and $show = 1                         and next;
        $arg =~ /^--deliver$/    and $deliver = 1                     and next;
        $arg =~ /^(--job=)/       and $custom_job = $' and next;
        $arg =~ /^(--pre_job=)/       and $pre_custom_job = $' and next;
        $arg =~ /^(--post_job=)/       and $post_custom_job = $' and next;
        $arg =~ /^-d$/    and $deliver = 1                     and next;
        $arg =~ /^--dlv_switch$/    and $dlv_switch = shift @ARGV    and next;
        $arg =~ /^--file$/        and $cmd_file = shift @ARGV             and next;
        $arg =~ /^-F$/        and $cmd_file = shift @ARGV             and next;

        $arg =~ /^--with_branches$/        and $BuildAllParents = 1
                                and $build_from = shift @ARGV         and next;
        $arg =~ /^-b$/        and $BuildAllParents = 1
                                and $build_from = shift @ARGV         and next;

        $arg =~ /^--all:(\S+)$/ and $BuildAllParents = 1
                                and $build_all_cont = $1            and next;
        $arg =~ /^-a:(\S+)$/ and $BuildAllParents = 1
                                and $build_all_cont = $1            and next;
        if ($arg =~ /^--from$/ || $arg =~ /^-f$/) {
                                    $BuildAllParents = 1;
                                    get_incomp_projects();
                                    next;
        };
        $arg =~ /^--prepare$/    and $prepare = 1 and next;
        $arg =~ /^-p$/            and $prepare = 1 and next;
        $arg =~ /^--prepare:/    and $prepare = 1 and $only_platform = $' and next;
        $arg =~ /^-p:/            and $prepare = 1 and $only_platform = $' and next;
        $arg =~ /^--since$/        and $BuildAllParents = 1
                                and $build_since = shift @ARGV         and next;
        $arg =~ /^-c$/        and $BuildAllParents = 1
                                and $build_since = shift @ARGV         and next;
        $arg =~ /^-s$/            and $BuildAllParents = 1
                                and $build_since = shift @ARGV         and next;
        $arg =~ /^--help$/        and usage()                            and do_exit(0);
        $arg =~ /^-h$/        and usage()                            and do_exit(0);
        $arg =~ /^--ignore$/        and $ignore = 1                            and next;
        $arg =~ /^--genconf$/        and $generate_config = 1                  and next;
        if ($arg =~ /^--add$/)      {
                                        get_list_of_modules(\%add_to_config);
                                        next;
        };
        if ($arg =~ /^--remove$/)   {
                                        get_list_of_modules(\%remove_from_config);
                                        next;
        };
        ($arg =~ /^--clear$/ || $arg =~ /^--removeall$/)  and $clear_config = 1 and next;
        $arg =~ /^--html$/        and $html = 1                            and next;
        $arg =~ /^--dontgraboutput$/        and $dont_grab_output = 1      and next;
        $arg =~ /^--html_path$/ and $html_path = shift @ARGV  and next;
        $arg =~ /^-i$/        and $ignore = 1                            and next;
        $arg =~ /^--server$/        and $server_mode = 1                      and next;
        $arg =~ /^--client_timeout$/ and $client_timeout = (shift @ARGV)*60  and next;
        $arg =~ /^--setenvstring$/            and $setenv_string =  shift @ARGV         and next;
        $arg =~ /^--port$/            and $ports_string =  shift @ARGV         and next;
        $arg =~ /^--version$/   and do_exit(0);
        $arg =~ /^-V$/          and do_exit(0);
        $arg =~ /^-m$/            and get_modes()         and next;
        $arg =~ /^--mode$/        and get_modes()         and next;
        $arg =~ /^--stoponerror$/        and $stop_build_on_error = 1         and next;
        if ($arg =~ /^--$/) {
            push (@dmake_args, get_dmake_args()) if (!$custom_job);
            next;
        };
        push (@dmake_args, $arg);
    };
    if (!$html) {
        print_error("\"--html_path\" switch is used only with \"--html\"") if ($html_path);
        print_error("\"--dontgraboutput\" switch is used only with \"--html\"") if ($dont_grab_output);
    };
    $grab_output = 0 if ($dont_grab_output);
    print_error('Switches --with_branches and --all collision') if ($build_from && $build_all_cont);
#    print_error('Please prepare the workspace on one of UNIX platforms') if ($prepare && ($ENV{GUI} ne 'UNX'));
    print_error('Switches --with_branches and --since collision') if ($build_from && $build_since);
    if ($show) {
        $QuantityToBuild = 0;
        $cmd_file = '';
    };
    print_error('Switches --job and --deliver collision') if ($custom_job && $deliver);
    $custom_job = 'deliver' if $deliver; 
    $post_job = 'deliver' if (!$custom_job);
    $incompatible = scalar keys %incompatibles;
    if ($prepare) {
        print_error("--prepare is for use with --from switch only!\n") if (!$incompatible);
    };
    if ($QuantityToBuild) {
        if ($ignore && !$html) {
            print_error("Cannot ignore errors in multiprocessing build");
        };
        if (!$enable_multiprocessing) {
            print_error("Cannot load Win32::Process module for multiprocessing build");
        };
        if ($server_mode) {
            print_error("Switches -P and --server collision");
        };
    } elsif ($stop_build_on_error) {
        print_error("Switche --stoponerror is only for multiprocessing builds");
    };
    if ($server_mode) {
        $html++;
        $client_timeout = 60 * 60 * 2 if (!$client_timeout);
    } else {
        print_error("--ports switch is for server mode only!!") if ($ports_string);
        print_error("--setenvstring switch is for server mode only!!") if ($setenv_string);
        print_error("--client_timeout switch is for server mode only!!") if ($client_timeout);
    };

    if (!$generate_config) {
        my $error_message = ' switch(es) should be used only with "--genconf"';
        print_error('"--removeall" ("--clear")' . $error_message) if ($clear_config);
        if ((scalar %add_to_config) || (scalar %remove_from_config)) {
            print_error('"--add" or/and "--remove"' . $error_message);
        };
    } elsif ((!scalar %add_to_config) && !$clear_config && (!scalar %remove_from_config)){
        print_error('Please supply necessary switch for "--genconf" (--add|--remove|--removeall)');
    };

    if ($only_platform) {
        $only_common = 'common';
        $only_common .= '.pro' if ($only_platform =~ /\.pro$/);
    };
    # Default build modes(for OpenOffice.org)
    $ENV{BUILD_TYPE} = 'OOo EXT' if (!defined $ENV{BUILD_TYPE});
    @ARGV = @dmake_args;
};

sub get_module_and_buildlist_paths {
    my $source_config = SourceConfig -> new($StandDir);
    my $source_config_file = $source_config->get_config_file_path();
    $active_modules{$_}++ foreach ($source_config->get_active_modules());
    my %active_modules_copy = %active_modules;
    foreach ($source_config->get_all_modules()) {
        delete $active_modules_copy{$_} if defined($active_modules_copy{$_});
        $module_paths{$_} = $source_config->get_module_path($_);
        $build_list_paths{$_} = $source_config->get_module_build_list($_)
    }
    $dead_parents{$_}++ foreach (keys %active_modules_copy);
};


sub get_dmake_args {
    my $arg;
    my @job_args = (); 
    while ($arg = shift @ARGV) {
        next if ($arg =~ /^--$/);
        push (@job_args, $arg);
    };
    return @job_args;
};

#
# get all options without '-'
#
sub get_switch_options {
    my $string = '';
    my $option = '';
    while ($option = shift @ARGV) {
        if (!($option =~ /^-+/)) {
            $string .= '-' . $option;
            $string .= ' ';
        } else {
            unshift(@ARGV, $option);
            last;
        };
    };
    $string =~ s/\s$//;
    return $string;
};

#
# cancel build when one of children has error exit code
#
sub cancel_build {
#    close_server_socket();
    $modules_number -= scalar keys %global_deps_hash;
    my $broken_modules_number = scalar @broken_modules_names;
    if ($broken_modules_number) {
        $modules_number -= $broken_modules_number;
        print "\n";
        print $broken_modules_number;
        print " module(s): ";
        foreach (@broken_modules_names) {
            print "\n\t$_";
#            RemoveFromDependencies($_, \%global_deps_hash);
        };
        print "\nneed(s) to be rebuilt\n\nReason(s):\n\n";
        foreach (keys %broken_build) {
            print "ERROR: error " . $broken_build{$_} . " occurred while making $_\n";
        };
        print "\nAttention: if you build and deliver the above module(s) you may prolongue your the build issuing command \"build --from @broken_modules_names\"\n";
    } else {
#        if ($ENV{GUI} eq 'WNT') {
            while (children_number()) {
                handle_dead_children(1);
            }
            foreach (keys %broken_build) {
                print "ERROR: error " . $broken_build{$_} . " occurred while making $_\n";
            };
#        } else {
#            kill 9 => -$$;
#        };
    };
    print "\n";
    do_exit(1);
};

#
# Function for storing error in multiprocessing AllParents build
#
sub store_error {
    my ($pid, $error_code) = @_;
    return 0 if (!$error_code);
    my $child_nick = $processes_hash{$pid};
    if ($ENV{GUI} eq 'WNT') {
        if (!defined $had_error{$child_nick}) {
            $had_error{$child_nick}++;
            return 1;
        };
    };
    $broken_modules_hashes{$folders_hashes{$child_nick}}++;
    $broken_build{$child_nick} = $error_code;
    if ($stop_build_on_error) {
        clear_from_child($pid);
        # Let all children finish their work 
        while (children_number()) {
            handle_dead_children(1);
        };
        cancel_build();
    };
    return 0;
};

#
# child handler (clears (or stores info about) the terminated child)
#
sub handle_dead_children {
    my $running_children = children_number();
    return if (!$running_children);
    my $force_wait = shift;
    my $try_once_more = 0;
    do {
        my $pid = 0;
        if ($ENV{GUI} eq 'WNT' && !$cygwin) {
            foreach $pid (keys %processes_hash) {
                my $exit_code  = undef;
                my $proc_obj = $windows_procs{$pid};
                $proc_obj->GetExitCode($exit_code);
                if ( $exit_code != 259 ) {
                    $try_once_more  = store_error($pid, $exit_code);
                    delete $windows_procs{$pid};
                    if ($try_once_more) {
                        give_second_chance($pid);
                    } else {
                        clear_from_child($pid);
                    };
                };
            };
            sleep 1 if (children_number() >= $QuantityToBuild || ($force_wait && ($running_children == children_number())));
        } else {
            if (children_number() >= $QuantityToBuild ||
                    ($force_wait && ($running_children == children_number()))) {
                $pid = wait();
            } else {
                $pid = waitpid( -1, &WNOHANG);
            };
            if ($pid > 0) {
                $try_once_more = store_error($pid, $?);
                if ($try_once_more) {
                    give_second_chance($pid);
                } else {
                    clear_from_child($pid);
                };
            };
        };
    } while(children_number() >= $QuantityToBuild);
};

sub give_second_chance {
    my $pid = shift;
    # A malicious hack for misterious windows problems - try 2 times 
    # to run dmake in the same directory if errors occurs
    my $child_nick = $processes_hash{$pid};
    $running_children{$folders_hashes{$child_nick}}--;
    delete $processes_hash{$pid};
    start_child($child_nick, $folders_hashes{$child_nick});
};

sub clear_from_child {
    my $pid = shift;
    my $child_nick = $processes_hash{$pid};
    my $error_code = 0;
    if (defined $broken_build{$child_nick}) {
        $error_code = $broken_build{$child_nick};
    } else {
        RemoveFromDependencies($child_nick,
                            $folders_hashes{$child_nick});
    };
    my $module = $module_by_hash{$folders_hashes{$child_nick}};
    html_store_job_info($folders_hashes{$child_nick}, $child_nick, $error_code);
    $running_children{$folders_hashes{$child_nick}}--;
    delete $processes_hash{$pid};
    $only_dependent = 0;
    print 'Running processes: ' . children_number() . "\n";
};

#
# Build the entire project according to queue of dependencies
#
sub BuildDependent {
    $dependencies_hash = shift;
    my $pid = 0;
    my $child_nick = '';
    $running_children{$dependencies_hash} = 0 if (!defined $running_children{$dependencies_hash});
    while ($child_nick = PickPrjToBuild($dependencies_hash)) {
        if (($QuantityToBuild)) { # multiprocessing not for $BuildAllParents (-all etc)!!
            do {
                handle_dead_children(0);
                if (defined $broken_modules_hashes{$dependencies_hash} && !$ignore) {
                    return if ($BuildAllParents);
                    last;
                };
                # start current child & all
                # that could be started now
                start_child($child_nick, $dependencies_hash) if ($child_nick);
                $child_nick = PickPrjToBuild($dependencies_hash);
                if (!$child_nick) {
                    return if ($BuildAllParents);
                    handle_dead_children(1) if (!$no_projects);
                };
            } while (!$no_projects);
            return if ($BuildAllParents);
            while (children_number()) {
                handle_dead_children(1);
            };
#            if (defined $last_module) {
#                $build_is_finished{$last_module}++ if (!defined $modules_with_errors{$last_module});
#            };

            if (defined $broken_modules_hashes{$dependencies_hash}) {
                cancel_build();
            }
            mp_success_exit();
        } else {
            dmake_dir($child_nick);
        };
        $child_nick = '';
    };
};

sub children_number {
    return scalar keys %processes_hash;
};

sub start_child {
    my ($job_dir, $dependencies_hash) = @_;
    $jobs_hash{$job_dir}->{START_TIME} = time();
    $jobs_hash{$job_dir}->{STATUS} = 'building';
    if ($job_dir =~ /(\s)/o) {
        my $error_code = undef;
        $error_code = do_custom_job($job_dir, $dependencies_hash);
        return;
    };
    html_store_job_info($dependencies_hash, $job_dir);
    my $pid = undef;
    my $children_running;
    my $oldfh = select STDOUT;
    $| = 1;
    if ($ENV{GUI} eq 'WNT' && !$cygwin) {
        print "$job_dir\n";
        my $process_obj = undef;
        my $rc = Win32::Process::Create($process_obj, $dmake_bin,
                                    $dmake_args, 
			                        0, 0, #NORMAL_PRIORITY_CLASS,
                                    $job_dir);
#        my $rc = Win32::Process::Create($process_obj, $_4nt_exe,
#                                    "/c $dmake_batch", 
#			                        0, NORMAL_PRIORITY_CLASS,
#                                    $job_dir);
        print_error("Cannot start child process") if (!$rc);
        $pid = $process_obj->GetProcessID();
        $windows_procs{$pid} = $process_obj;
    } else {
        if ($pid = fork) { # parent
        } elsif (defined $pid) { # child
            select $oldfh;
            $child = 1;
            dmake_dir($job_dir);
            do_exit(1);
        };
    };
    select $oldfh;
    $processes_hash{$pid} = $job_dir;
    $children_running = children_number();
    print 'Running processes: ', $children_running, "\n";
    $maximal_processes = $children_running if ($children_running > $maximal_processes);
    $folders_hashes{$job_dir} = $dependencies_hash;
    $running_children{$dependencies_hash}++;
};

#
# Build everything that should be built multiprocessing version
#
sub build_multiprocessing {
    my $Prj;
    my @build_queue = ();        # array, containing queue of projects
                                # to build
    do {
        while ($Prj = PickPrjToBuild(\%global_deps_hash)) {
            push @build_queue, $Prj;
            $projects_deps_hash{$Prj} = {};
            get_deps_hash($Prj, $projects_deps_hash{$Prj});
            my $info_hash = $html_info{$Prj};
            $$info_hash{DIRS} = check_deps_hash($projects_deps_hash{$Prj}, $Prj);
            $module_by_hash{$projects_deps_hash{$Prj}} = $Prj;
        };
        if (!$Prj || !defined $projects_deps_hash{$Prj}) {
            cancel_build() if (!scalar @build_queue && !children_number());
            handle_dead_children(1);
        }
        build_actual_queue(\@build_queue);
    } while (scalar (keys %global_deps_hash));
    # Let the last module be built till the end
    while (scalar @build_queue) {
        build_actual_queue(\@build_queue);
        handle_dead_children(1);
    };
    # Let all children finish their work 
    while (children_number()) {
        handle_dead_children(1);
    };
    cancel_build() if (scalar keys %broken_build);
    mp_success_exit();
};

sub mp_success_exit {
#    close_server_socket();
#    if (!$custom_job && $post_custom_job) {
#        do_post_custom_job(CorrectPath($StandDir.$CurrentPrj));
#    };
    print "\nMultiprocessing build is finished\n";
    print "Maximal number of processes run: $maximal_processes\n";
    do_exit(0);
};

#
# Here the built queue is built as long as possible
#
sub build_actual_queue {
    my $build_queue = shift;
    my $i = 0;
    do {
        while ($i <= (scalar(@$build_queue) - 1)) {
            $Prj = $$build_queue[$i];
            if (defined $broken_modules_hashes{$projects_deps_hash{$Prj}} && !$ignore) {
                push (@broken_modules_names, $Prj);
                splice (@$build_queue, $i, 1);
                next;
            };
            $only_dependent = 0;
            $no_projects = 0;
            BuildDependent($projects_deps_hash{$Prj});
            handle_dead_children(0);
            if ($no_projects &&
                !$running_children{$projects_deps_hash{$Prj}}) {
                if (!defined $broken_modules_hashes{$projects_deps_hash{$Prj}} || $ignore)
                {
                    RemoveFromDependencies($Prj, \%global_deps_hash);
                    $build_is_finished{$Prj}++;
                    splice (@$build_queue, $i, 1);
                    next;
                };
            };
            $i++;
        };
        $i = 0;
    } while (!are_all_dependent($build_queue));
};

sub run_job {
    my ($job, $path, $registered_name) = @_;
    my $job_to_do = $job;
    if ( $show ) {
        print "$job_to_do\n";
        return 0;
    }
    $job_to_do = $deliver_command if ($job eq 'deliver');
    $registered_name = $path if (!defined $registered_name);
    chdir $path;
    getcwd();
    
    if ($html) {
        my $log_file = $jobs_hash{$registered_name}->{LONG_LOG_PATH};
        my $log_dir = File::Basename::dirname($log_file);
        if (!-d $log_dir) {
             system("$perl $mkout");
        };
        $error_code = system ("$job_to_do > $log_file 2>&1");
        if (!$grab_output && -f $log_file) {
            system("cat $log_file");
        };
    } else {
        $error_code = system ("$job_to_do");
    };
    return $error_code;
};

sub do_custom_job {
    my ($module_job, $dependencies_hash) = @_;
    $module_job =~ /(\s)/o;
    my $module = $`;
    my $job = $';
    html_store_job_info($dependencies_hash, $module_job);
    my $error_code = 0;
    if ($job eq $pre_job) {
        announce_module($module);
#        html_store_job_info($dependencies_hash, $job_dir);
        RemoveFromDependencies($module_job, $dependencies_hash);
    } else {
        $error_code = run_job($job, $module_paths{$module}, $module_job);
        if ($error_code) {
            # give windows one more chance 
            if ($ENV{GUI} eq 'WNT') {
                $error_code = run_job($job, $module_paths{$module}, $module_job);
            };
        };
        if ($error_code) {
            $broken_modules_hashes{$dependencies_hash}++;
            $broken_build{$module} = $error_code;
        } else {
            RemoveFromDependencies($module_job, $dependencies_hash);
        };
    };
    html_store_job_info($dependencies_hash, $module_job, $error_code);
    return $error_code;
};

#
# Print announcement for module just started
#
sub announce_module {
    my $Prj = shift;
    $build_in_progress{$Prj}++;
    print_announce($Prj);
};

sub print_announce {
    my $Prj = shift;
    my $prj_type = '';
    $prj_type = $modules_types{$Prj} if (defined $modules_types{$Prj});
    my $text;
    if ($prj_type eq 'lnk') {
        if (scalar keys %active_modules && (!defined $active_modules{$Prj})) {
            $text = "Skipping module $Prj\n";
        } else {
            $text = "Skipping link to $Prj\n";
        };
        $build_is_finished{$Prj}++;
    } elsif ($prj_type eq 'img') {
#        return if (defined $module_announced{$`});
        $text = "Skipping incomplete $Prj\n";
        $build_is_finished{$Prj}++;
    } elsif ($custom_job) {
        $text = "Running custom job \"$custom_job\" in module $Prj\n";
    } else {
        $text = "Building module $Prj\n";
    };
    print $echo . "=============\n";
    print $echo . $text;
};

sub are_all_dependent {
    my $build_queue = shift;
    my $folder = '';
    foreach my $prj (@$build_queue) {
        $folder = FindIndepPrj($projects_deps_hash{$prj});
        return '' if ($folder);
    };
    return '1';
};


#
# Procedure defines if the local directory is a
# complete module, an image or a link
# return values: lnk link
#                img incomplete (image)
#                mod complete (module)
#
sub modules_classify {
    my @modules = @_;
    foreach my $module (sort @modules) {
        if (!defined $module_paths{$module}) {
            $modules_types{$module} = 'img';
            next;
        };
        if (( $module_paths{$module} =~ /\.lnk$/) || ($module_paths{$module} =~ /\.link$/)
                || (scalar keys %active_modules && (!defined $active_modules{$module}))) {
            $modules_types{$module} = 'lnk';
            next;
        };
        $modules_types{$module} = 'mod';
    };
};

#
# This procedure provides consistency for cws
# and optimized build (ie in case of -with_branches, -all:prj_name
# and -since switches)
#
sub provide_consistency {
    check_dir();
    foreach my $module_ref (\$build_from, \$build_all_cont, \$build_since) {
        if ($$module_ref) {
            return if (defined $module_paths{$$module_ref});
            print_error("Cannot find module '$$module_ref'", 9);
            return;
        };
    };
};

#
# Get the workspace list ('stand.lst'), either from 'localini'
# or, if this is not possible, from 'globalini.
# (Heiner's proprietary :)
#
sub get_workspace_lst
{
    my $home;
    if ( $^O eq 'MSWin32' ) {
        $home = $ENV{TEMP};
    }
    else {
        $home = $ENV{HOME};
    }
    my $inifile = "$home/localini/stand.lst";
    if (-f $inifile) {
        return $inifile;
#    } else {
#        $inifile = get_globalini() . "/stand.lst";
#        return $inifile if (-f $inifile);
    };
    return '';
}

#
# Procedure clears up module for incompatible build
#
sub ensure_clear_module {
    my $module = shift;
    if ($modules_types{$module} eq 'mod') {
         clear_module($module);
         return;
    };
    if ($modules_types{$module} eq 'lnk' && (File::Basename::basename($module_paths{$module}) ne $module)) {
        if(rename($module_paths{$module}, File::Basename::dirname($module_paths{$module}) ."/$module")) {
            $module_paths{$module} = File::Basename::dirname($module_paths{$module}) ."/$module";
            clear_module($module); 
        } else {
            print_error("Cannot rename link to $module. Please rename it manually");
        };
    };
};

#
# Procedure removes output tree from the module (without common trees)
#
sub clear_module {
    my $module = shift;
    print "Removing module's $module output trees...\n";
    print "\n" and return if ($show);
    opendir DIRHANDLE, $module_paths{$module};
    my @dir_content = readdir(DIRHANDLE);
    closedir(DIRHANDLE);
    foreach (@dir_content) {
        next if (/^\.+$/);
        my $dir = CorrectPath($module_paths{$module}.'/'.$_);
        if ((!-d $dir.'/.svn') && is_output_tree($dir)) {
            #print "I would delete $dir\n";
            rmtree("$dir", 0, 1);
            if (-d $dir) {
                system("$remove_command $dir");
                if (-d $dir) {
                    push(@warnings, "Cannot delete $dir");
#print_error("Cannot delete $dir");
                } else {
                    print STDERR (">>> Removed $dir by force\n");
                };
            };
        };
    };
};

#
# Figure out if the directory is an output tree
#
sub is_output_tree {
    my $dir = shift;
    $dir =~ /([\w\d\.]+)$/;
    $_ = $1;
    return '1' if (defined $platforms{$_});
    if ($only_common) {
        return '1' if ($_ eq $only_common);
    } else {
        if (scalar keys %platforms < scalar keys %platforms_to_copy) {
            return '';
        };
        return '1' if (/^common$/);
        return '1' if (/^common\.pro$/);
    };
    return '';
};

sub get_tmp_dir {
    my $tmp_dir;
    if( defined($ENV{TMPDIR}) ) {
       $tmp_dir = $ENV{TMPDIR} . '/';
    } elsif( defined($ENV{TMP}) ) {
       $tmp_dir = $ENV{TMP} . '/';
    } else {
       $tmp_dir = '/tmp/';
    }
    
    return File::Temp::tempdir(DIR =>$tmp_dir);
    
#    $tmp_dir .= $$ while (-e $tmp_dir);
#    $tmp_dir = CorrectPath($tmp_dir);
#    eval {mkpath($tmp_dir)};
#    print_error("Cannot create temporary directory in $tmp_dir") if ($@);
#    return $tmp_dir;
};


sub retrieve_build_list {
    my $module = shift;
    my $old_fh = select(STDOUT);
    
    # Try to get global depencies from solver's build.lst if such exists
    my $solver_inc_dir = "$ENV{SOLARVER}/common";
    $solver_inc_dir .= $ENV{PROEXT} if (defined $ENV{PROEXT});
    $solver_inc_dir .= '/inc';
    $solver_inc_dir .= $ENV{UPDMINOREXT} if (defined $ENV{UPDMINOREXT});
    $solver_inc_dir .= "/$module";
    $solver_inc_dir = CorrectPath($solver_inc_dir);
    $dead_parents{$module}++;
    print "Fetching dependencies for module $module from solver...";
    foreach (@possible_build_lists) {
        my $possible_build_lst = "$solver_inc_dir/$_";
        if (-e $possible_build_lst) {
            print " ok\n";
            select($old_fh);
            return $possible_build_lst;
        };
    }
    print " failed\n";

    if (!defined $dead_parents{$module}) {
        print "WARNING: Cannot figure out CWS for $module. Forgot to set CWS?\n";
    }
    select($old_fh);
    return undef;
};

sub fix_permissions {
     my $file = $File::Find::name;
     return unless -f $file;
     chmod '0664', $file;
};

#
# Removes projects which it is not necessary to build
# in incompatible build
#
sub prepare_incompatible_build {
    my ($prj, $deps_hash, @missing_modules);
    $deps_hash = shift;
    foreach (keys %incompatibles) {
        my $incomp_prj = $_;
        if (!defined $$deps_hash{$_}) {
            $incomp_prj .= '.lnk' if ($module_paths{$module} =~ /\.lnk$/);
            $incomp_prj .= '.link' if ($module_paths{$module} =~ /\.link$/);
        }
        delete $incompatibles{$_};
        $incompatibles{$incomp_prj} = $$deps_hash{$incomp_prj};
        delete $$deps_hash{$incomp_prj};
    }
    while ($prj = PickPrjToBuild($deps_hash)) {
        RemoveFromDependencies($prj, $deps_hash);
        RemoveFromDependencies($prj, \%incompatibles);
    };
    foreach (keys %incompatibles) {
        $$deps_hash{$_} = $incompatibles{$_};
    };
    if ($build_all_cont) {
        prepare_build_all_cont($deps_hash);
        delete $$deps_hash{$build_all_cont};
    };
    @modules_built = keys %$deps_hash;
    clear_delivered() if ($prepare);
    my $old_output_tree = '';
    foreach $prj (sort keys %$deps_hash) {
        if ($prepare) {
            ensure_clear_module($prj);
        } else {
            next if ($show);
            if ($modules_types{$prj} ne 'mod') {
                push(@missing_modules, $prj);
            } elsif (-d $module_paths{$prj}. '/'. $ENV{INPATH}) {
                $old_output_tree++;
            };
        };
    };
    if (scalar @missing_modules) {
        my $warning_string = 'Following modules are inconsistent/missing: ' . "@missing_modules";
        push(@warnings, $warning_string);
    };
    if ($build_all_cont) {
        $$deps_hash{$build_all_cont} = ();
        $build_all_cont = '';
    };
    if ($old_output_tree) {
        push(@warnings, 'Some module(s) contain old output tree(s)!');
    };
    if (scalar @warnings) {
        print "WARNING(S):\n";
        print STDERR "$_\n" foreach (@warnings);
        print "\nATTENTION: If you are performing an incompatible build, please break the build with Ctrl+C and prepare the workspace with \"--prepare\" switch!\n\n" if (!$prepare);
        sleep(10);
    };
    if ($prepare) {
    print "\nPreparation finished";
        if (scalar @warnings) {
            print " with WARNINGS!!\n\n";
        } else {print " successfully\n\n";}
    }
    do_exit(0) if ($prepare);
};

#
# Removes projects which it is not necessary to build
# with -with_branches switch
#
sub prepare_build_from {
    my ($prj, $deps_hash);
    $deps_hash = shift;
    my %from_deps_hash = ();   # hash of dependencies of the -from project
    GetParentDeps($build_from, \%from_deps_hash);
    foreach $prj (keys %from_deps_hash) {
        delete $$deps_hash{$prj};
        RemoveFromDependencies($prj, $deps_hash);
    };
};

#
# Removes projects which it is not necessary to build
# with --all:prj_name or --since switch
#
sub prepare_build_all_cont {
    my ($prj, $deps_hash, $border_prj);
    $deps_hash = shift;
    $border_prj = $build_all_cont if ($build_all_cont);
    $border_prj = $build_since if ($build_since);
    while ($prj = PickPrjToBuild($deps_hash)) {
        $orig_prj = '';
        $orig_prj = $` if ($prj =~ /\.lnk$/o);
        $orig_prj = $` if ($prj =~ /\.link$/o);
        if (($border_prj ne $prj) &&
            ($border_prj ne $orig_prj)) {
            RemoveFromDependencies($prj, $deps_hash);
            next;
        } else {
            if ($build_all_cont) {
                $$deps_hash{$prj} = ();
            } else {
                RemoveFromDependencies($prj, $deps_hash);
            };
            return;
        };
    };
};

sub get_modes {
    my $option = '';
    while ($option = shift @ARGV) {
        if ($option =~ /^-+/) {
            unshift(@ARGV, $option);
            return;
        } else {
            if ($option =~ /,/) {
                $build_modes{$`}++;
                unshift(@ARGV, $') if ($');
            } else {$build_modes{$option}++;};
        };
    };
    $build_modes{$option}++;
};

sub get_list_of_modules {
    my $option = '';
    my $hash_ref = shift;
    while ($option = shift @ARGV) {
        if ($option =~ /^-+/) {
            unshift(@ARGV, $option);
            return;
        } else {
            if ($option =~ /,/) {
                foreach (split /,/, $option) {
                    next if (!$_);
                    $$hash_ref{$_}++;
                };
            } else {
                $$hash_ref{$option}++;
            };
        };
    };
    if (!scalar %$hash_ref) {
        print_error('No module list supplied!!');
    };
};

sub get_incomp_projects {
    my $option = '';
    while ($option = shift @ARGV) {
        if ($option =~ /^-+/) {
            unshift(@ARGV, $option);
            return;
        } else {
            if ($option =~ /(:)/) {
                $option = $`;
                print_error("-from switch collision") if ($build_all_cont);
                $build_all_cont = $';
            };
            $incompatibles{$option}++;
        };
    };
};


sub get_platforms {
    my $platforms_ref = shift;
    if ($only_platform) {
        foreach (split(',', $only_platform)) {
            $$platforms_ref{$_}++;
        }
        $platforms_ref = \%platforms_to_copy;
    };

    my $workspace_lst = get_workspace_lst();
    if ($workspace_lst) {
        my $workspace_db = GenInfoParser->new();
        my $success = $workspace_db->load_list($workspace_lst);
        if ( !$success ) {
            print_error("Can't load workspace list '$workspace_lst'.", 4);
        }
        my $access_path = $ENV{WORK_STAMP} . '/Environments';
        my @platforms_available = $workspace_db->get_keys($access_path);
        my $solver = $ENV{SOLARVERSION};
        foreach (@platforms_available) {
            my $s_path = $solver . '/' .  $_;
            $$platforms_ref{$_}++ if (-d $s_path);
        };
    };
    
    if (!scalar keys %platforms) {
        # An Auses wish - fallback to INPATH for new platforms
        if (defined $ENV{INPATH}) { 
            $$platforms_ref{$ENV{INPATH}}++;
        } else {
            print_error("There is no platform found!!") ;
        };
    };
};

#
# This procedure clears solver from delivered
# by the modules to be build
#
sub clear_delivered {
    my $message = 'Clearing up delivered';
    my %backup_vars;
    my $deliver_delete_switches = '-delete';
    if (scalar keys %platforms < scalar keys %platforms_to_copy) {
        $message .= ' without common trees';
        $deliver_delete_switches .= ' -dontdeletecommon';
        $only_common = '';
    };
    print "$message\n";
    
    foreach my $platform (keys %platforms) {
        print "\nRemoving delivered for $platform\n";
        my %solar_vars = ();
        read_ssolar_vars($platform, \%solar_vars);
        if (scalar keys %solar_vars) {
            foreach (keys %solar_vars) {
                if (!defined $backup_vars{$_}) {
                    $backup_vars{$_} = $ENV{$_};
                };
                $ENV{$_} = $solar_vars{$_};
            };
        };
        my $undeliver = "$deliver_command $deliver_delete_switches $nul";
#        my $current_dir = getcwd();
        foreach my $module (sort @modules_built) {
            if (chdir($module_paths{$module})) {
                push(@warnings, "Could not remove delivered files from the module $module. Your build can become inconsistent.\n");
            } else {
                print "Removing delivered from module $module\n";
                next if ($show);
                if (system($undeliver)) {
                    $ENV{$_} = $backup_vars{$_} foreach (keys %backup_vars);
                    print_error("Cannot run: $undeliver");
                }
            };
        };
#        chdir $current_dir;
#        getcwd();
    };
    $ENV{$_} = $backup_vars{$_} foreach (keys %backup_vars);
};

#
# Run setsolar for given platform and
# write all variables needed in %solar_vars hash
#
sub read_ssolar_vars {
    my ($setsolar, $tmp_file);
    $setsolar = $ENV{ENV_ROOT} . '/etools/setsolar.pl';
    my ($platform, $solar_vars) = @_;
    if ( $^O eq 'MSWin32' ) {
        $tmp_file = $ENV{TEMP} . "\\solar.env.$$.tmp";
    } else {
        $setsolar = '/net/jumbo2.germany/buildenv/r/etools/setsolar.pl' if ! -e $setsolar;
        $tmp_file = $ENV{HOME} . "/.solar.env.$$.tmp";
    };
    if (!-e $setsolar) {
        print STDERR "There is no setsolar found. Falling back to current platform settings\n";
        return;
    }
    my $pro = "";
    if ($platform =~ /\.pro$/) {
        $pro = "-pro";
        $platform = $`;
    };
	
    my ($verswitch, $source_root, $cwsname);
	$verswitch = "-ver $ENV{UPDMINOR}" if (defined $ENV{UPDMINOR});
    $source_root = '-sourceroot' if (defined $ENV{SOURCE_ROOT_USED});
    $cws_name = "-cwsname $ENV{CWS_WORK_STAMP}" if (defined $ENV{CWS_WORK_STAMP});
    
    my $param = "-$ENV{WORK_STAMP} $verswitch $source_root $cws_name $pro $platform";
    my $ss_command = "$perl $setsolar -file $tmp_file $param $nul";
    if (system($ss_command)) {
        unlink $tmp_file;
        print_error("Cannot run command:\n$ss_command");
    };
    get_solar_vars($solar_vars, $tmp_file);
};

#
# read variables to hash
#
sub get_solar_vars {
    my ($solar_vars, $file) = @_;
    my ($var, $value);
    open SOLARTABLE, "<$file" or die "can�t open solarfile $file";
    while(<SOLARTABLE>) {
        s/\r\n//o;
        next if(!/^\w+\s+(\w+)/o);
        next if (!defined $deliver_env{$1});
        $var = $1;
        if ( $^O eq 'MSWin32' ) {
            my $string_tail = $';
            $string_tail =~ /=(\S+)$/o;
            $value = $1;
        } else {
            /\'(\S+)\'$/o;
            $value = $1;
        };
        $$solar_vars{$var} = $value;
    };
    close SOLARTABLE;
    unlink $file;
}

#
# Procedure renames <module>.lnk (.link) into <module>
#
sub get_current_module {
    my $module_name = shift;
    my $link_name = $module_name . '.lnk';
    $link_name .= '.link' if (-e $StandDir.$module_name . '.link');
    chdir $StandDir;
    getcwd();
    print "\nBreaking link to module $module_name";
    my $result = rename $link_name, $module_name;
    if ( ! $result ) {
        print_error("Cannot rename $module_name: $!\n");
    }
    if ( $CurrentPrj eq $link_name) {
        $CurrentPrj = $module_name;
    }
    chdir $module_name;
    getcwd();
};

sub check_dir {
    my $start_dir = getcwd();
    my @dir_entries = split(/[\\\/]/, $start_dir);
    my $current_module = $dir_entries[$#dir_entries];
    $current_module = $` if (($current_module =~ /(\.lnk)$/) || ($current_module =~ /(\.link)$/));
    my $link_name = $ENV{SOLARSRC}.'/'.$current_module.$1;
    if ( $^O eq 'MSWin32' ) {
        $start_dir =~ s/\\/\//go;
        $link_name =~ s/\\/\//go;
        if (lc($start_dir) eq lc($link_name)) {
            get_current_module($current_module);
        };
    } elsif ((-l $link_name) && (chdir $link_name)) {
        if ($start_dir eq getcwd()) {
            # we're dealing with link => fallback to SOLARSRC under UNIX
            $StandDir = $ENV{SOLARSRC}.'/';
            get_current_module($current_module);
            return;
        } else {
            chdir $start_dir;
            getcwd();
        };
    };
};

#
# Store all available build modi in %build_modes
#
sub get_build_modes {
    return if (scalar keys %build_modes);
    if (defined $ENV{BUILD_TYPE}) {
        if ($ENV{BUILD_TYPE} =~ /\s+/o) {
            my @build_modes = split (/\s+/, $ENV{BUILD_TYPE});
            $build_modes{$_}++ foreach (@build_modes);
        } else {
            $build_modes{$ENV{BUILD_TYPE}}++;
        };
        return;
    };
};

#
# pick only the modules, that should be built for
# build types from %build_modes
#
sub pick_for_build_type {
    my $modules = shift;
    my @mod_array = split(/\s+/, $modules);
    print_error("Wrongly written dependencies string:\n $modules\n") if ($mod_array[$#mod_array] ne 'NULL');
    pop @mod_array;
    my @modules_to_build;
    foreach (@mod_array) {
        if (/(\w+):(\S+)/o) {
            push(@modules_to_build, $2) if (defined $build_modes{$1});
            next;
        };
        push(@modules_to_build, $_);
    };
    return @modules_to_build;
};

sub do_exit {
#    close_server_socket();
    my $exit_code = shift;
    $build_finished++;
    generate_html_file(1);
    if ( $^O eq 'os2' )
    {
        # perl 5.10 returns 'resource busy' for rmtree
        rmdir(CorrectPath($tmp_dir)) if ($tmp_dir);
    }
    rmtree(CorrectPath($tmp_dir), 1, 0) if ($tmp_dir);
    exit($exit_code);
};

#
# Procedure sorts module in user-frendly order
#
sub sort_modules_appearance {
    foreach (keys %dead_parents) {
        delete $build_is_finished{$_} if (defined $build_is_finished{$_});
        delete $build_in_progress{$_} if (defined $build_in_progress{$_});
    };
    foreach (keys %build_is_finished) {
        delete $build_in_progress{$_} if (defined $build_in_progress{$_});
        delete $build_in_progress_shown{$_} if (defined $build_in_progress_shown{$_});
    }; 
    @modules_order = sort keys %modules_with_errors;
    foreach (keys %modules_with_errors) {
        delete $build_in_progress{$_} if (defined $build_in_progress{$_});
        delete $build_is_finished{$_} if (defined $build_is_finished{$_});
        delete $build_in_progress_shown{$_} if (defined $build_in_progress_shown{$_});
    };
    $build_in_progress_shown{$_}++ foreach (keys %build_in_progress);
    push(@modules_order, $_) foreach (sort keys %build_in_progress_shown);
    push(@modules_order, $_) foreach (sort keys %build_is_finished);
    foreach(sort keys %html_info) {
        next if (defined $build_is_finished{$_} || defined $build_in_progress{$_} || defined $modules_with_errors{$_});
        push(@modules_order, $_);
    };
    return @modules_order;
};

sub generate_html_file {
    return if (!$html);
    my $force_update = shift;
    $html_last_updated = time;
    my @modules_order = sort_modules_appearance();
    my ($successes_percent, $errors_percent) = get_progress_percentage(scalar keys %html_info, scalar keys %build_is_finished, scalar keys %modules_with_errors);
    my $build_duration = get_time_line(time - $build_time);
    my $temp_html_file = File::Temp::tmpnam($tmp_dir);
    my $title;
    $title = $ENV{CWS_WORK_STAMP} . ': ' if (defined $ENV{CWS_WORK_STAMP});
    $title .= $ENV{INPATH};
    die("Cannot open $temp_html_file") if (!open(HTML, ">$temp_html_file"));
    print HTML '<html><head>';
    print HTML '<TITLE id=MainTitle>' . $title . '</TITLE>';
    print HTML '<script type="text/javascript">' . "\n";
    print HTML 'initFrames();' . "\n";
    print HTML 'var IntervalID;' . "\n";
    print HTML 'function loadFrame_0() {' . "\n";
    print HTML 'document.write("<html>");' . "\n";
    print HTML 'document.write("<head>");' . "\n";
    print HTML 'document.write("</head>");' . "\n";
    print HTML 'document.write("<body>");' . "\n";
    if ($build_finished) {
        print HTML 'document.write("<h3 align=center style=\"color:red\">Build process is finished</h3>");' . "\n";
        print HTML '        top.frames[0].clearInterval(top.frames[0].IntervalID);' . "\n";
    };
    if ($BuildAllParents) {
        print HTML 'document.write("<table valign=top cellpadding=0 hspace=0 vspace=0 cellspacing=0 border=0>");' . "\n";
        print HTML 'document.write("    <tr>");' . "\n";
        print HTML 'document.write("        <td><a id=ErroneousModules href=\"javascript:top.Error(\'\', \'';
        print HTML join('<br>', sort keys %modules_with_errors);
        print HTML '\', \'\')\"); title=\"';
        print HTML scalar keys %modules_with_errors;
        print HTML ' module(s) with errors\">Total Progress:</a></td>");' . "\n";
        print HTML 'document.write("        <td>");' . "\n";
        print HTML 'document.write("            <table width=100px valign=top cellpadding=0 hspace=0 vspace=0 cellspacing=0 border=0>");' . "\n";
        print HTML 'document.write("                <tr>");' . "\n";
        print HTML 'document.write("                    <td height=20px width=';
        print HTML $successes_percent + $errors_percent;
        if (scalar keys %modules_with_errors) {
            print HTML '% bgcolor=red valign=top></td>");' . "\n";
        } else {
            print HTML '% bgcolor=#25A528 valign=top></td>");' . "\n";
        };
        print HTML 'document.write("                    <td width=';
        print HTML 100 - ($successes_percent + $errors_percent);
        print HTML '% bgcolor=lightgrey valign=top></td>");' . "\n";
        print HTML 'document.write("                </tr>");' . "\n";
        print HTML 'document.write("            </table>");' . "\n";
        print HTML 'document.write("        </td>");' . "\n";
        print HTML 'document.write("        <td align=right>&nbsp Build time: ' . $build_duration .'</td>");' . "\n";
        print HTML 'document.write("    </tr>");' . "\n";
        print HTML 'document.write("</table>");' . "\n";
    };

    print HTML 'document.write("<table width=100% bgcolor=white>");' . "\n";
    print HTML 'document.write("    <tr>");' . "\n";
    print HTML 'document.write("        <td width=30% align=\"center\"><strong style=\"color:blue\">Module</strong></td>");' . "\n";
    print HTML 'document.write("        <td width=* align=\"center\"><strong style=\"color:blue\">Status</strong></td>");' . "\n";
    print HTML 'document.write("        <td width=15% align=\"center\"><strong style=\"color:blue\">CPU Time</strong></td>");' . "\n";
    print HTML 'document.write("    </tr>");' . "\n";

    foreach (@modules_order) {
        next if ($modules_types{$_} eq 'lnk');
        next if (scalar keys %active_modules && (!defined $active_modules{$_}));
        my ($errors_info_line, $dirs_info_line, $errors_number, $successes_percent, $errors_percent, $time) = get_html_info($_);
#<one module> 
        print HTML 'document.write("    <tr>");' . "\n";
        print HTML 'document.write("        <td width=*>");' . "\n";

        if (defined $dirs_info_line) {
            print HTML 'document.write("            <a id=';
            print HTML $_;
            print HTML ' href=\"javascript:top.Error(\'';
            print HTML $_ , '\', ' ;
            print HTML $errors_info_line;
            print HTML ',';
            print HTML $dirs_info_line;
            print HTML ')\"); title=\"';
            print HTML $errors_number;
            print HTML ' error(s)\">', $_, '</a>");' . "\n";
        } else {
            print HTML 'document.write("<em style=color:gray>' . $_ . '</em>");';
        };


        print HTML 'document.write("        </td>");' . "\n";
        print HTML 'document.write("        <td>");' . "\n";
        print HTML 'document.write("            <table width=100% valign=top cellpadding=0 hspace=0 vspace=0 cellspacing=0 border=0>");' . "\n";
        print HTML 'document.write("                <tr>");' . "\n";
        print HTML 'document.write("                    <td height=15* width=';
                
        print HTML $successes_percent + $errors_percent;
        if ($errors_number) {
            print HTML '% bgcolor=red valign=top></td>");' . "\n";
        } else {
            print HTML '% bgcolor=#25A528 valign=top></td>");' . "\n";
        };
        print HTML 'document.write("                    <td width=';
                
        print HTML 100 - ($successes_percent + $errors_percent);
        print HTML '% bgcolor=lightgrey valign=top></td>");' . "\n";
        print HTML 'document.write("                </tr>");' . "\n";
        print HTML 'document.write("            </table>");' . "\n";
        print HTML 'document.write("        </td>");' . "\n";
        print HTML 'document.write("        <td align=\"center\">', $time, '</td>");' . "\n";
        print HTML 'document.write("    </tr>");' . "\n";
# </one module>
    } 
    print HTML 'document.write("</table>");' . "\n";
    print HTML 'document.write("</body>");' . "\n";
    print HTML 'document.write("</html>");' . "\n";
    print HTML 'document.close();' . "\n";
    print HTML 'refreshInfoFrames();' . "\n";
    print HTML '}' . "\n";
    
    print HTML 'function refreshInfoFrames() {        ' . "\n";
    print HTML '    var ModuleNameObj = top.innerFrame.frames[2].document.getElementById("ModuleErrors");' . "\n";
    print HTML '    if (ModuleNameObj != null) {' . "\n";
    print HTML '        var ModuleName = ModuleNameObj.getAttribute(\'name\');' . "\n";
    print HTML '        var ModuleHref = top.innerFrame.frames[0].document.getElementById(ModuleName).getAttribute(\'href\');' . "\n";
    print HTML '         eval(ModuleHref);' . "\n";
    print HTML '    } else if (top.innerFrame.frames[2].document.getElementById("ErroneousModules") != null) {' . "\n";
    print HTML '        var ModuleHref = top.innerFrame.frames[0].document.getElementById("ErroneousModules").getAttribute(\'href\');' . "\n";
    print HTML '        eval(ModuleHref);' . "\n";
    print HTML '        if (top.innerFrame.frames[1].document.getElementById("ModuleJobs") != null) {' . "\n";
    print HTML '            var ModuleName = top.innerFrame.frames[1].document.getElementById("ModuleJobs").getAttribute(\'name\');' . "\n";
    print HTML '            ModuleHref = top.innerFrame.frames[0].document.getElementById(ModuleName).getAttribute(\'href\');' . "\n";
    print HTML '            var HrefString = ModuleHref.toString();' . "\n";
    print HTML '            var RefEntries = HrefString.split(",");' . "\n";
    print HTML '            var RefreshParams = new Array();' . "\n";
    print HTML '            for (i = 0; i < RefEntries.length; i++) {' . "\n";
    print HTML '                RefreshParams[i] = RefEntries[i].substring(RefEntries[i].indexOf("\'") + 1, RefEntries[i].lastIndexOf("\'"));' . "\n";
    print HTML '            };' . "\n";
    print HTML '            FillFrame_1(RefreshParams[0], RefreshParams[1], RefreshParams[2]);' . "\n";
    print HTML '        }' . "\n";
    print HTML '    };' . "\n";
    print HTML '}' . "\n";
    print HTML 'function loadFrame_1() {' . "\n";
    print HTML '    document.write("<h3 align=center>Jobs</h3>");' . "\n";
    print HTML '    document.write("Click on the project of interest");' . "\n";
    print HTML '    document.close();' . "\n";
    print HTML '}' . "\n";
    print HTML 'function loadFrame_2() {' . "\n";
    print HTML '    document.write("<tr bgcolor=lightgrey<td><h3>Errors</h3></pre></td></tr>");' . "\n";
    print HTML '    document.write("Click on the project of interest");' . "\n";
    print HTML '    document.close();' . "\n";
    print HTML '}    function getStatusInnerHTML(Status) {        var StatusInnerHtml;' . "\n";
    print HTML '    if (Status == "success") {' . "\n";            
    print HTML '        StatusInnerHtml = "<em style=color:green>";' . "\n";
    print HTML '    } else if (Status == "building") {' . "\n";
    print HTML '        StatusInnerHtml = "<em style=color:blue>";' . "\n";
    print HTML '    } else if (Status == "error") {' . "\n";
    print HTML '        StatusInnerHtml = "<em style=color:red>";' . "\n";
    print HTML '    } else {' . "\n";
    print HTML '        StatusInnerHtml = "<em style=color:gray>";' . "\n";
    print HTML '    };' . "\n";
    print HTML '    StatusInnerHtml += Status + "</em>";' . "\n";
    print HTML '    return StatusInnerHtml;' . "\n";
    print HTML '}    ' . "\n";
    print HTML 'function ShowLog(LogFilePath) {' . "\n";
	if (defined $html_path) {
		print HTML '    top.innerFrame.frames[2].document.location.replace("file://"+LogFilePath);' . "\n";
	} else {
		print HTML '    top.innerFrame.frames[2].document.location.replace(LogFilePath);' . "\n";
	}
    print HTML '    top.innerFrame.frames[2].document.close();' . "\n";
    print HTML '};' . "\n";
    print HTML 'function FillFrame_1(Module, Message1, Message2) {' . "\n";
    print HTML '    var FullUpdate = 1;' . "\n";
    print HTML '    if (top.innerFrame.frames[1].document.getElementById("ModuleJobs") != null) {' . "\n";
    print HTML '        var ModuleName = top.innerFrame.frames[1].document.getElementById("ModuleJobs").getAttribute(\'name\');' . "\n";
    print HTML '        if (Module == ModuleName) FullUpdate = 0;' . "\n";
    print HTML '    }' . "\n";
    print HTML '    if (FullUpdate) {' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("<h3 align=center>Jobs in module " + Module + ":</h3>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("<table id=ModuleJobs  name=" + Module + " width=100% bgcolor=white>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("    <tr>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("        <td width=* align=center><strong style=color:blue>Status</strong></td>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("        <td width=* align=center><strong style=color:blue>Job</strong></td>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("        <td width=* align=center><strong style=color:blue>Start Time</strong></td>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("        <td width=* align=center><strong style=color:blue>Finish Time</strong></td>");' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("        <td width=* align=center><strong style=color:blue>Client</strong></td>");' . "\n" if ($server_mode);
    print HTML '        top.innerFrame.frames[1].document.write("    </tr>");' . "\n";
    print HTML '        var dir_info_strings = Message2.split("<br><br>");' . "\n";
    print HTML '        for (i = 0; i < dir_info_strings.length; i++) {' . "\n";
    print HTML '            var dir_info_array = dir_info_strings[i].split("<br>");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("    <tr status=" + dir_info_array[0] + ">");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("        <td align=center>");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write(               getStatusInnerHTML(dir_info_array[0]) + "&nbsp");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("        </td>");' . "\n";
    print HTML '            if (dir_info_array[4] == "@") {' . "\n";
    print HTML '                top.innerFrame.frames[1].document.write("        <td style=white-space:nowrap>" + dir_info_array[1] + "</td>");' . "\n";
    print HTML '            } else {' . "\n";
    print HTML '                top.innerFrame.frames[1].document.write("        <td><a href=\"javascript:top.ShowLog(\'" + dir_info_array[4] + "\')\"); title=\"Show Log\">" + dir_info_array[1] + "</a></td>");' . "\n";
    print HTML '            };' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("        <td align=center>" + dir_info_array[2] + "</td>");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("        <td align=center>" + dir_info_array[3] + "</td>");' . "\n";
    print HTML '            top.innerFrame.frames[1].document.write("        <td align=center>" + dir_info_array[5] + "</td>");' . "\n" if ($server_mode);
    print HTML '            top.innerFrame.frames[1].document.write("    </tr>");' . "\n";
    print HTML '        };' . "\n";
    print HTML '        top.innerFrame.frames[1].document.write("</table>");' . "\n";
    print HTML '    } else {' . "\n";
    print HTML '        var dir_info_strings = Message2.split("<br><br>");' . "\n";
    print HTML '        var ModuleRows = top.innerFrame.frames[1].document.getElementById("ModuleJobs").rows;' . "\n";
    print HTML '        for (i = 0; i < dir_info_strings.length; i++) {' . "\n";
    print HTML '            var dir_info_array = dir_info_strings[i].split("<br>");' . "\n";
    print HTML '            var OldStatus = ModuleRows[i + 1].getAttribute(\'status\');' . "\n";
    print HTML '            if(dir_info_array[0] != OldStatus) {' . "\n";
    print HTML '                var DirectoryInfos = ModuleRows[i + 1].cells;' . "\n";
    print HTML '                DirectoryInfos[0].innerHTML = getStatusInnerHTML(dir_info_array[0]) + "&nbsp";' . "\n";
    print HTML '                if (dir_info_array[4] != "@") {' . "\n";
    print HTML '                    DirectoryInfos[1].innerHTML = "<a href=\"javascript:top.ShowLog(\'" + dir_info_array[4] + "\')\"); title=\"Show Log\">" + dir_info_array[1] + "</a>";' . "\n";
    print HTML '                };' . "\n";
    print HTML '                DirectoryInfos[2].innerHTML = dir_info_array[2];' . "\n";
    print HTML '                DirectoryInfos[3].innerHTML = dir_info_array[3];' . "\n";
    print HTML '                DirectoryInfos[4].innerHTML = dir_info_array[5];' . "\n" if ($server_mode);
    print HTML '            };' . "\n";
    print HTML '        };' . "\n";
    print HTML '    };' . "\n";
    print HTML '    top.innerFrame.frames[1].document.close();' . "\n";
    print HTML '};' . "\n";
    print HTML 'function Error(Module, Message1, Message2) {' . "\n";
    print HTML '    if (Module == \'\') {' . "\n";
    print HTML '        if (Message1 != \'\') {' . "\n";
    print HTML '            var erroneous_modules = Message1.split("<br>");' . "\n";
    print HTML '            var ErrorNumber = erroneous_modules.length;' . "\n";

    print HTML '            top.innerFrame.frames[2].document.write("<h3 id=ErroneousModules errors=" + erroneous_modules.length + ">Modules with errors:</h3>");' . "\n";
    print HTML '            for (i = 0; i < ErrorNumber; i++) {' . "\n";
    print HTML '                var ModuleObj = top.innerFrame.frames[0].document.getElementById(erroneous_modules[i]);' . "\n";
    print HTML '                top.innerFrame.frames[2].document.write("<a href=\"");' . "\n";
    print HTML '                top.innerFrame.frames[2].document.write(ModuleObj.getAttribute(\'href\'));' . "\n";
    print HTML '                top.innerFrame.frames[2].document.write("\"); title=\"");' . "\n";
    print HTML '                top.innerFrame.frames[2].document.write("\">" + erroneous_modules[i] + "</a>&nbsp ");' . "\n";
    print HTML '            };' . "\n";
    print HTML '            top.innerFrame.frames[2].document.close();' . "\n";
    print HTML '        };' . "\n";
    print HTML '    } else {' . "\n";
    print HTML '        var ModuleNameObj = top.innerFrame.frames[2].document.getElementById("ModuleErrors");' . "\n";
    print HTML '        var OldErrors = null;' . "\n";
    print HTML '        var ErrorNumber = Message1.split("<br>").length;' . "\n";
    print HTML '        if ((ModuleNameObj != null) && (Module == ModuleNameObj.getAttribute(\'name\')) ) {' . "\n";
    print HTML '            OldErrors = ModuleNameObj.getAttribute(\'errors\');' . "\n";
    print HTML '        }' . "\n";
    print HTML '        if ((OldErrors == null) || (OldErrors != ErrorNumber)) {' . "\n";
    print HTML '            top.innerFrame.frames[2].document.write("<h3 id=ModuleErrors errors=" + ErrorNumber + " name=\"" + Module + "\">Errors in module " + Module + ":</h3>");' . "\n";
    print HTML '            top.innerFrame.frames[2].document.write(Message1);' . "\n";
    print HTML '            top.innerFrame.frames[2].document.close();' . "\n";
    print HTML '        }' . "\n";
    print HTML '        FillFrame_1(Module, Message1, Message2);' . "\n";
    print HTML '    }' . "\n";
    print HTML '}' . "\n";
    print HTML 'function updateInnerFrame() {' . "\n";
    print HTML '     top.innerFrame.frames[0].document.location.reload();' . "\n";
    print HTML '     refreshInfoFrames();' . "\n";
    print HTML '};' . "\n\n";
    
    print HTML 'function setRefreshRate() {' . "\n";
    print HTML '    RefreshRate = document.Formular.rate.value;' . "\n";
    print HTML '    if (!isNaN(RefreshRate * 1)) {' . "\n";
    print HTML '        top.frames[0].clearInterval(IntervalID);' . "\n";
    print HTML '        IntervalID = top.frames[0].setInterval("updateInnerFrame()", RefreshRate * 1000);' . "\n";
    print HTML '    };' . "\n";
    print HTML '};' . "\n";

    print HTML 'function initFrames() {' . "\n";
    print HTML '    var urlquery = location.href.split("?");' . "\n";
    print HTML '    if (urlquery.length == 1) {' . "\n";
    print HTML '        document.write("<html><head><TITLE id=MainTitle>' . $ENV{INPATH} .'</TITLE>");' . "\n";
    print HTML '        document.write("    <frameset rows=\"12%,88%\">");' . "\n";
    print HTML '        document.write("        <frame name=\"topFrame\" src=\"" + urlquery + "?initTop\"/>");' . "\n";
    print HTML '        document.write("        <frame name=\"innerFrame\" src=\"" + urlquery + "?initInnerPage\"/>");' . "\n";
    print HTML '        document.write("    </frameset>");' . "\n";
    print HTML '        document.write("</head></html>");' . "\n";
    print HTML '    } else if (urlquery[1].substring(0,7) == "initTop") {' . "\n";
    print HTML '        var urlquerycontent = urlquery[1].split("=");' . "\n";
    print HTML '        var UpdateRate = 10' . "\n";
    print HTML '        if (urlquerycontent.length > 2) {' . "\n";
    print HTML '            if (isNaN(urlquerycontent[2] * 1)) {' . "\n";
    print HTML '                alert(urlquerycontent[2] + " is not a number. Ignored.");' . "\n";
    print HTML '            } else {' . "\n";
    print HTML '                UpdateRate = urlquerycontent[2];' . "\n";
    print HTML '            };' . "\n";
    print HTML '        };' . "\n";
    print HTML '        document.write("<html><body>");' . "\n";
    print HTML '        document.write("<h3 align=center>Build process progress status</h3>");' . "\n";
    print HTML '        document.write("<div align=\"right\">");' . "\n";
    print HTML '        document.write("    <table border=\"0\"> <tr>");' . "\n";
    print HTML '        document.write("<td>Refresh rate(sec):</td>");' . "\n";
    print HTML '        document.write("<th>");' . "\n";
    print HTML '        document.write("<FORM name=\"Formular\" onsubmit=\"setRefreshRate()\">");' . "\n";
    print HTML '        document.write("<input type=\"hidden\" name=\"initTop\" value=\"\"/>");' . "\n";
    print HTML '        document.write("<input type=\"text\" id=\"RateValue\" name=\"rate\" autocomplete=\"off\" value=\"" + UpdateRate + "\" size=\"1\"/>");' . "\n";
    print HTML '        document.write("<input type=\"submit\" value=\"OK\">");' . "\n";
    print HTML '        document.write("</FORM>");' . "\n";
    print HTML '        document.write("</th></tr></table>");' . "\n";
    print HTML '        document.write("</div>");' . "\n";
    print HTML '        document.write("    </frameset>");' . "\n";
    print HTML '        document.write("</body></html>");' . "\n";
    print HTML '        top.frames[0].clearInterval(IntervalID);' . "\n";
    print HTML '        IntervalID = top.frames[0].setInterval("updateInnerFrame()", UpdateRate * 1000);' . "\n";
    print HTML '    } else if (urlquery[1] == "initInnerPage") {' . "\n";
    print HTML '        document.write("<html><head>");' . "\n";
    print HTML '        document.write(\'    <frameset rows="80%,20%\">\');' . "\n";
    print HTML '        document.write(\'        <frameset cols="70%,30%">\');' . "\n";
    print HTML '        document.write(\'            <frame src="\');' . "\n";
    print HTML '        document.write(urlquery[0]);' . "\n";
    print HTML '        document.write(\'?initFrame0"/>\');' . "\n";
    print HTML '        document.write(\'            <frame src="\');' . "\n";
    print HTML '        document.write(urlquery[0]);' . "\n";
    print HTML '        document.write(\'?initFrame1"/>\');' . "\n";
    print HTML '        document.write(\'        </frameset>\');' . "\n";
    print HTML '        document.write(\'            <frame src="\');' . "\n";
    print HTML '        document.write(urlquery[0]);' . "\n";
    print HTML '        document.write(\'?initFrame2"/>\');' . "\n";
    print HTML '        document.write(\'    </frameset>\');' . "\n";
    print HTML '        document.write("</head></html>");' . "\n";
    print HTML '    } else {' . "\n";
    print HTML '        if (urlquery[1] == "initFrame0" ) {' . "\n";
    print HTML '            loadFrame_0();' . "\n";
    print HTML '        } else if (urlquery[1] == "initFrame1" ) {          ' . "\n";
    print HTML '            loadFrame_1();' . "\n";
    print HTML '        } else if (urlquery[1] == "initFrame2" ) {' . "\n";
    print HTML '            loadFrame_2();' . "\n";
    print HTML '        }' . "\n";
    print HTML '    };' . "\n";
    print HTML '};' . "\n";
    print HTML '</script><noscript>Your browser doesn\'t support JavaScript!</noscript></head></html>' . "\n";
    close HTML;
    rename_file($temp_html_file, $html_file);
};

sub get_local_time_line {
    my $epoch_time = shift;
    my $local_time_line;
    my @time_array;
    if ($epoch_time) {
        @time_array = localtime($epoch_time);
        $local_time_line = sprintf("%02d:%02d:%02d", $time_array[2], $time_array[1], $time_array[0]);
    } else {
        $local_time_line = '-';
    };
    return $local_time_line;
};

sub get_dirs_info_line {
    my $job = shift;
    my $dirs_info_line = $jobs_hash{$job}->{STATUS} . '<br>';
    my @time_array;
    my $log_path_string;
    $dirs_info_line .= $jobs_hash{$job}->{SHORT_NAME} . '<br>';
    $dirs_info_line .= get_local_time_line($jobs_hash{$job}->{START_TIME}) . '<br>';
    $dirs_info_line .= get_local_time_line($jobs_hash{$job}->{FINISH_TIME}) . '<br>';
    if ($jobs_hash{$job}->{STATUS} eq 'waiting' || (!-f $jobs_hash{$job}->{LONG_LOG_PATH})) {
        $dirs_info_line .= '@';
    } else {
        if (defined $html_path) {
            $log_path_string = $jobs_hash{$job}->{LONG_LOG_PATH};
        } else {
            $log_path_string = $jobs_hash{$job}->{LOG_PATH};
        };
        $log_path_string =~ s/\\/\//g;
        $dirs_info_line .= $log_path_string;
    };
    $dirs_info_line .= '<br>';
    $dirs_info_line .= $jobs_hash{$job}->{CLIENT} . '<br>' if ($server_mode);
    return $dirs_info_line;
};

sub get_html_info {
    my $module = shift;
    my $module_info_hash = $html_info{$module};
    my $dirs = $$module_info_hash{DIRS};
    my $dirs_number = scalar @$dirs;
    my $dirs_info_line = '\'';
    if ($dirs_number) {
        my %dirs_sorted_by_order = ();
        foreach (@$dirs) {
            $dirs_sorted_by_order{$jobs_hash{$_}->{BUILD_NUMBER}} = $_;
        }
        foreach (sort {$a <=> $b} keys %dirs_sorted_by_order) {
            $dirs_info_line .= get_dirs_info_line($dirs_sorted_by_order{$_}) . '<br>';
        }
    } else {
        return(undef, undef, 0, 0, 0, '-');
#        $dirs_info_line .= 'No information available yet';
    };
    $dirs_info_line =~ s/(<br>)*$//o;
    $dirs_info_line .= '\'';
    $dirs = $$module_info_hash{SUCCESSFUL};
    my $successful_number = scalar @$dirs;
    $dirs = $$module_info_hash{ERRORFUL};
    my $errorful_number = scalar @$dirs;
    my $errors_info_line = '\'';
    if ($errorful_number) {
        $errors_info_line .= $_ . '<br>' foreach (@$dirs);
    } else {
        $errors_info_line .= 'No errors';
    };
    $errors_info_line .= '\'';
#    if (defined $full_info) {
    my $time_line = get_time_line($$module_info_hash{BUILD_TIME});
        my ($successes_percent, $errors_percent) = get_progress_percentage($dirs_number, $successful_number, $errorful_number); 
        return($errors_info_line, $dirs_info_line, $errorful_number, $successes_percent, $errors_percent, $time_line);
#    } else {
#        return($errors_info_line, $dirs_info_line, $errorful_number);
#    };
};

sub get_time_line {
    use integer;
    my $seconds = shift;
    my $hours = $seconds/3600;
    my $minits = ($seconds/60)%60;
    $seconds -= ($hours*3600 + $minits*60);
    return(sprintf("%02d\:%02d\:%02d" , $hours, $minits, $seconds));
};

sub get_progress_percentage {
    use integer;
    my ($dirs_number, $successful_number, $errorful_number) = @_;
    return (0 ,0) if (!$dirs_number);
    my $errors_percent = ($errorful_number * 100)/ $dirs_number;
    my $successes_percent;
    if ($dirs_number == ($successful_number + $errorful_number)) {
        $successes_percent = 100 - $errors_percent;
    } else {
        $successes_percent = ($successful_number * 100)/ $dirs_number;
    };
    return ($successes_percent, $errors_percent);
};

#
# This procedure stores the dmake result in %html_info
#
sub html_store_job_info {
    return if (!$html);
    my ($deps_hash, $build_dir, $error_code) = @_;
    my $force_update = 0;
    if ($build_dir =~ /(\s)/o && (defined $error_code)) {
        $force_update++ if (!children_number());
    }
    my $module = $module_by_hash{$deps_hash};
    my $module_info_hash = $html_info{$module};
    my $dmake_array;
    if (defined $error_code) {
        $jobs_hash{$build_dir}->{FINISH_TIME} = time();
        $$module_info_hash{BUILD_TIME} += $jobs_hash{$build_dir}->{FINISH_TIME} - $jobs_hash{$build_dir}->{START_TIME};
        if ($error_code) {
            $jobs_hash{$build_dir}->{STATUS} = 'error';
            $dmake_array = $$module_info_hash{ERRORFUL};
            $build_dir =~ s/\\/\//g;
            $modules_with_errors{$module}++;
        } else {
            $jobs_hash{$build_dir}->{STATUS} = 'success';
            $dmake_array = $$module_info_hash{SUCCESSFUL};
        };
        push (@$dmake_array, $build_dir);
    };
};

sub start_server_on_port {
    my $port = shift;
    if ($ENV{GUI} eq 'WNT') {
        $socket_obj = new IO::Socket::INET (#LocalAddr => hostname(),
                                  LocalPort => $port,
                                  Proto     => 'tcp',
                                  Listen    => 100); # 100 clients can be on queue, I think it is enough
    } else {
        $socket_obj = new IO::Socket::INET (#LocalAddr => hostname(),
                                  LocalPort => $port,
                                  Proto     => 'tcp',
                                  ReuseAddr     => 1,
                                  Listen    => 100); # 100 clients can be on queue, I think it is enough
    };
    return('Cannot create socket object') if (!defined $socket_obj);
    my $timeout = $socket_obj->timeout($client_timeout);
    $socket_obj->autoflush(1);
    print "SERVER started on port $port\n";
    return 0;
};

sub accept_connection {
    my $new_socket_obj = undef;
    do {
        $new_socket_obj = $socket_obj->accept();
        if (!$new_socket_obj) {
            print "Timeout on incoming connection\n";
            check_client_jobs();
        };
    } while (!$new_socket_obj);
    return $new_socket_obj;
};

sub check_client_jobs {
    foreach (keys %clients_times) {
        if (time - $clients_times{$_} > $client_timeout) {
            print "Client's $_ Job: \"$clients_jobs{$_}\" apparently got lost...\n"; 
            print "Scheduling for rebuild...\n";
            print "You might need to check the $_\n";
            $lost_client_jobs{$clients_jobs{$_}}++;
            delete $processes_hash{$_};
            delete $clients_jobs{$_};
            delete $clients_times{$_};
#        } else {
#            print time - $clients_times{$_} . "\n";
        };
    };
};

sub run_server {
    my @build_queue = ();        # array, containing queue of projects
                                # to build
    # use port 7890 as default
    my $default_port = 7890;
    if ($ports_string) {
        @server_ports = split( /:/, $ports_string);
    } else {
        @server_ports = ($default_port .. $default_port + 4);
    };
    my $error = 0;
    if (scalar @server_ports) {
        foreach (@server_ports) {
            $error = start_server_on_port($_);
            if ($error) {
                print STDERR "port $_: $error\n";
            } else {
#                $SIG{KILL} = \&stop_server;
#                $SIG{INT} = \&stop_server;
#                $SIG{TERM} = \&stop_server;
#                $SIG{QUIT} = \&stop_server;
                last;
            };
        };
    };
    print_error('It is impossible to start server on port(s): ' . "@server_ports\n") if ($error);

    my $client_addr;
    my $job_string_base = get_job_string_base();
    my $new_socket_obj;
     while ($new_socket_obj = accept_connection()) {
        check_client_jobs();
    	# find out who connected
    	my $client_ipnum = $new_socket_obj->peerhost();
        my $client_host = gethostbyaddr(inet_aton($client_ipnum), AF_INET);
    	# print who is connected
    	# send them a message, close connection
        my $client_message = <$new_socket_obj>;
        chomp $client_message;
        my @client_data = split(/ /, $client_message);
        my %client_hash = ();
        foreach (@client_data) {
            /(=)/;
            $client_hash{$`} = $';
        }
        my $pid = $client_hash{pid} . '@' . $client_host;
        if (defined $client_hash{platform}) {
            if ($client_hash{platform} ne $ENV{OUTPATH} || (defined $client_hash{osname} && ($^O ne $client_hash{osname}))) {
                print $new_socket_obj "Wrong platform";
                close($new_socket_obj);
                next;
            };
        } else {
            if ($client_hash{result} eq "0") {
#                print "$clients_jobs{$pid} succedded on $pid\n";
            } else {
                print "Error $client_hash{result}\n";
                if (store_error($pid, $client_hash{result})) {
                    print $new_socket_obj $job_string_base . $clients_jobs{$pid};
                    close($new_socket_obj);
                    $clients_times{$pid} = time;
                    next;
                };
            };
            delete $clients_times{$pid};
            clear_from_child($pid);
            delete $clients_jobs{$pid};
            print 'Running processes: ', children_number(), "\n";
            # Actually, next 3 strings are only for even distribution 
            # of clients if there are more than one build server running
    	    print $new_socket_obj 'No job';
            close($new_socket_obj);
            next;
        };
        my $job_string;
        my @lost_jobs = keys %lost_client_jobs;
        if (scalar @lost_jobs) {
            $job_string = $lost_jobs[0];
            delete $lost_client_jobs{$lost_jobs[0]};
        } else {
#            $job_string = get_job_string(\@build_queue, $pid);
            $job_string = get_job_string(\@build_queue);
        };
        if ($job_string) {
            my $job_dir = $job_jobdir{$job_string};
            $processes_hash{$pid} = $job_dir;
            $jobs_hash{$job_dir}->{CLIENT} = $pid;
            print "$pid got $job_dir\n";
    	    print $new_socket_obj $job_string_base . $job_string;
            $clients_jobs{$pid} = $job_string;
            $clients_times{$pid} = time;
            $children_running = children_number();
            print 'Running processes: ', $children_running, "\n";
            $maximal_processes = $children_running if ($children_running > $maximal_processes);
        } else {
    	    print $new_socket_obj 'No job';
        };
        close($new_socket_obj);
    };
};

#
# Procedure returns the part of the job string that is similar for all clients
#
sub get_job_string_base {
    if ($setenv_string) {
        return "setenv_string=$setenv_string ";
    };
	my $job_string_base = "server_pid=$$ setsolar_cmd=$ENV{SETSOLAR_CMD} ";
    $job_string_base .= "source_root=$ENV{SOURCE_ROOT} " if (defined $ENV{SOURCE_ROOT});
    $job_string_base .= "updater=$ENV{UPDATER} " if (defined $ENV{UPDATER});
    return $job_string_base;
};

sub get_job_string {
    my $build_queue = shift;
	my $job = $dmake;
    my ($job_dir, $dependencies_hash);
    if ($BuildAllParents) {
        fill_modules_queue($build_queue);
        do {
            ($job_dir, $dependencies_hash) = pick_jobdir($build_queue);
            return '' if (!$job_dir);
            $jobs_hash{$job_dir}->{START_TIME} = time();
            $jobs_hash{$job_dir}->{STATUS} = 'building';
            if ($job_dir =~ /(\s)$pre_job/o) {
                do_custom_job($job_dir, $dependencies_hash);
                $job_dir = '';
            };
        } while (!$job_dir);
    } else {
        $dependencies_hash = \%LocalDepsHash;
        do {
            $job_dir = PickPrjToBuild(\%LocalDepsHash);
            if (!$job_dir && !children_number()) {
                cancel_build() if (scalar keys %broken_build);
                mp_success_exit();
            };
            return '' if (!$job_dir);
            $jobs_hash{$job_dir}->{START_TIME} = time();
            $jobs_hash{$job_dir}->{STATUS} = 'building';
            if ($job_dir =~ /(\s)$pre_job/o) {
#                if ($' eq $pre_job) {
                    do_custom_job($job_dir, $dependencies_hash);
                    $job_dir = '';
#                }
            };
        } while (!$job_dir);
    };
    $running_children{$dependencies_hash}++;
    $folders_hashes{$job_dir} = $dependencies_hash;
    my $log_file = $jobs_hash{$job_dir}->{LONG_LOG_PATH};
    my $full_job_dir = $job_dir;
    if ($job_dir =~ /(\s)/o) {
        $job = $';
        $job = $deliver_command if ($job eq $post_job);
        $full_job_dir = $module_paths{$`};
    }
    my $log_dir = File::Basename::dirname($log_file);
    if (!-d $log_dir) {
        chdir $full_job_dir;
        getcwd();
        system("$perl $mkout");
    };
    my $job_string = "job_dir=$full_job_dir job=$job log=$log_file";
    $job_jobdir{$job_string} = $job_dir;
    return $job_string;
};

sub pick_jobdir {
    my $build_queue = shift;
    my $i = 0;
    foreach (@$build_queue) {
        $Prj = $$build_queue[$i];
        my $prj_deps_hash = $projects_deps_hash{$Prj};
        if (defined $broken_modules_hashes{$prj_deps_hash} && !$ignore) {
            push (@broken_modules_names, $Prj);
            splice (@$build_queue, $i, 1);
            next;
        };
        $only_dependent = 0;
        $no_projects = 0;
        $running_children{$prj_deps_hash} = 0 if (!defined $running_children{$prj_deps_hash});
        $child_nick = PickPrjToBuild($prj_deps_hash);
        if ($child_nick) {
            return ($child_nick, $prj_deps_hash);
        }
        if ($no_projects && !$running_children{$prj_deps_hash}) {
            if (!defined $broken_modules_hashes{$prj_deps_hash} || $ignore)
            {
                RemoveFromDependencies($Prj, \%global_deps_hash);
                $build_is_finished{$Prj}++;
                splice (@$build_queue, $i, 1);
                next;
            };
        };
        $i++;
    };
};

sub fill_modules_queue {
    my $build_queue = shift;
    my $Prj;
    while ($Prj = PickPrjToBuild(\%global_deps_hash)) {
        push @$build_queue, $Prj;
        $projects_deps_hash{$Prj} = {};
        get_deps_hash($Prj, $projects_deps_hash{$Prj});
        my $info_hash = $html_info{$Prj};
        $$info_hash{DIRS} = check_deps_hash($projects_deps_hash{$Prj}, $Prj);
        $module_by_hash{$projects_deps_hash{$Prj}} = $Prj;
    };
    if (!$Prj && !children_number() && (!scalar @$build_queue)) {
        cancel_build() if (scalar keys %broken_build);
        mp_success_exit();
    };
};
