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

package installer::converter;

use installer::globals;

#############################
# Converter
#############################

sub convert_array_to_hash
{
	my ($arrayref) = @_;  
	
	my %newhash = ();

	for ( my $i = 0; $i <= $#{$arrayref}; $i++ )
	{
		my $line = ${$arrayref}[$i];
		
		if ( $line =~ /^\s*([\w-]+?)\s+(.*?)\s*$/ )
		{
			my $key = $1;
			my $value = $2;
			$newhash{$key} = $value;
		}
	}

	return \%newhash;
}

sub convert_hash_into_array
{
	my ($hashref) = @_;
	
	my @array = ();
	my $key;
	
	foreach $key (keys %{$hashref})
	{
		my $value = $hashref->{$key};
		my $input = "$key = $value\n";
		push(@array ,$input);
	}

	return \@array
}

#############################################################################
# Converting a string list with separator $listseparator 
# into an array
#############################################################################

sub convert_stringlist_into_array_without_linebreak_and_quotes
{
	my ( $includestringref, $listseparator ) = @_;
	
	my @newarray = ();
	my $first;
	my $last = ${$includestringref};

	while ( $last =~ /^\s*(.+?)\Q$listseparator\E(.+)\s*$/)	# "$" for minimal matching
	{
		$first = $1;
		$last = $2;	
		$first =~ s/\"//g;
		push(@newarray, $first);
	}	

	$last =~ s/\"//g;
	push(@newarray, $last);	
	
	return \@newarray;
}

#############################################################################
# Converting a string list with separator $listseparator 
# into an array
#############################################################################

sub convert_stringlist_into_array
{
	my ( $includestringref, $listseparator ) = @_;
	
	my @newarray = ();
	my $first;
	my $last = ${$includestringref};

	while ( $last =~ /^\s*(.+?)\Q$listseparator\E(.+)\s*$/)	# "$" for minimal matching
	{
		$first = $1;
		$last = $2;	
		# Problem with two directly following listseparators. For example a path with two ";;" directly behind each other
		$first =~ s/^$listseparator//;
		push(@newarray, "$first\n");
	}	

	push(@newarray, "$last\n");	
	
	return \@newarray;
}

#############################################################################
# Converting a string list with separator $listseparator 
# into an array
#############################################################################

sub convert_stringlist_into_array_without_newline
{
	my ( $includestringref, $listseparator ) = @_;
	
	my @newarray = ();
	my $first;
	my $last = ${$includestringref};

	while ( $last =~ /^\s*(.+?)\Q$listseparator\E(.+)\s*$/)	# "$" for minimal matching
	{
		$first = $1;
		$last = $2;	
		push(@newarray, "$first");
	}	

	push(@newarray, "$last");	
	
	return \@newarray;
}

#############################################################################
# Converting a string list with separator $listseparator 
# into a hash with values 1.
#############################################################################

sub convert_stringlist_into_hash
{
	my ( $includestringref, $listseparator ) = @_;
	
	my %newhash = ();
	my $first;
	my $last = ${$includestringref};

	while ( $last =~ /^\s*(.+?)\Q$listseparator\E(.+)\s*$/)	# "$" for minimal matching
	{
		$first = $1;
		$last = $2;	
		$newhash{$first} = 1;
	}	

	$newhash{$last} = 1;
	
	return \%newhash;
}

#############################################################################
# Converting a string list with separator $listseparator 
# into an array
#############################################################################

sub convert_whitespace_stringlist_into_array
{
	my ( $includestringref ) = @_;
	
	my @newarray = ();
	my $first;
	my $last = ${$includestringref};

	while ( $last =~ /^\s*(\S+?)\s+(\S+)\s*$/)	# "$" for minimal matching
	{
		$first = $1;
		$last = $2;	
		push(@newarray, "$first\n");
	}	

	push(@newarray, "$last\n");	
	
	return \@newarray;
}

#############################################################################
# Converting an array into a comma separated string 
#############################################################################

sub convert_array_to_comma_separated_string
{
	my ( $arrayref ) = @_;

	my $newstring = "";
	
	for ( my $i = 0; $i <= $#{$arrayref}; $i++ )
	{
		my $arrayentry = ${$arrayref}[$i];
		$arrayentry =~ s/\s*$//;
		$newstring = $newstring . $arrayentry . ",";
	}
	
	$newstring =~ s/\,\s*$//;

	return $newstring;
}

#############################################################################
# Converting an array into a space separated string 
#############################################################################

sub convert_array_to_space_separated_string
{
	my ( $arrayref ) = @_;

	my $newstring = "";
	
	for ( my $i = 0; $i <= $#{$arrayref}; $i++ )
	{
		my $arrayentry = ${$arrayref}[$i];
		$arrayentry =~ s/\s*$//;
		$newstring = $newstring . $arrayentry . " ";
	}
	
	$newstring =~ s/\s*$//;

	return $newstring;
}

#############################################################################
# The file name contains for some files "/". If this programs runs on 
# a windows platform, this has to be converted to "\".
#############################################################################

sub convert_slash_to_backslash
{
	my ($filesarrayref) = @_;

	for ( my $i = 0; $i <= $#{$filesarrayref}; $i++ )
	{
		my $onefile = ${$filesarrayref}[$i];
		if ( $onefile->{'Name'} ) { $onefile->{'Name'} =~ s/\//\\/g; }
	}
}

############################################################################
# Creating a copy of an existing file object
# No converter
############################################################################

sub copy_item_object
{
	my ($olditemhashref, $newitemhashref) = @_;
	
	foreach $key (keys %{$olditemhashref})
	{
		my $value = $olditemhashref->{$key};
		$newitemhashref->{$key} = $value;
	}
}

#################################################################
# Windows pathes must not contain the following structure:
# c:\dirA\dirB\..\dirC
# This has to be exchanged to 
# c:\dirA\dirC
#################################################################

sub make_path_conform
{
	my ( $path ) = @_;

	my $oldpath = $path;
	
	while ( $path =~ /(^.*)(\Q$installer::globals::separator\E.*?[^\.])(\Q$installer::globals::separator\E\.\.)(\Q$installer::globals::separator\E.*$)/ )
	{
		my $part1 = $1;
		my $part2 = $4;

		# $2 must not end with a "." ! Problem with "..\.."

		$path = $part1 . $part2;		
	}

	return $path;	
}

#################################################################
# Copying an item collector
# A reference to an array consisting of references to hashes.
#################################################################

sub copy_collector
{
	my ( $oldcollector ) = @_;

	my @newcollector = ();

	for ( my $i = 0; $i <= $#{$oldcollector}; $i++ )
	{
		my %newhash = ();
		my $key;
	
		foreach $key (keys %{${$oldcollector}[$i]})
		{
			$newhash{$key} = ${$oldcollector}[$i]->{$key};
		}
				
		push(@newcollector, \%newhash);
	}

	return \@newcollector;
}

#################################################################
# Copying an array
#################################################################

sub copy_array_from_references
{
	my ( $arrayref ) = @_;
	
	my @newarray = ();

	for ( my $i = 0; $i <= $#{$arrayref}; $i++ )
	{		
		push(@newarray, ${$arrayref}[$i]);
	}

	return \@newarray;
}

###########################################################
# Copying a hash
###########################################################

sub copy_hash_from_references
{
	my ($hashref) = @_;
	
	my %newhash = ();
	my $key;
	
	foreach $key (keys %{$hashref})
	{
		$newhash{$key} = $hashref->{$key};
	}

	return \%newhash;
}

#################################################################
# Combining two arrays, first wins
#################################################################

sub combine_arrays_from_references_first_win
{
	my ( $arrayref1, $arrayref2 ) = @_;

	my $hashref1 = convert_array_to_hash($arrayref1);
	my $hashref2 = convert_array_to_hash($arrayref2);
	my %commonhash = ();	
	my @newarray = ();

	# starting with second hash
	foreach my $key ( keys %{$hashref2} ) { $commonhash{$key} = $hashref2->{$key}; }
	# overwriting with first hash
	foreach my $key ( keys %{$hashref1} ) { $commonhash{$key} = $hashref1->{$key}; }
	
	# Creating the new array
	foreach my $key ( keys %commonhash ) { push(@newarray, "$key $commonhash{$key}\n"); }

	return \@newarray;
}

#################################################################
# Combining two arrays
#################################################################

sub combine_arrays_from_references
{
	my ( $arrayref1, $arrayref2 ) = @_;
	
	my @newarray = ();

	for ( my $i = 0; $i <= $#{$arrayref1}; $i++ )
	{		
		push(@newarray, ${$arrayref1}[$i]);
	}

	for ( my $i = 0; $i <= $#{$arrayref2}; $i++ )
	{		
		push(@newarray, ${$arrayref2}[$i]);
	}

	return \@newarray;
}

#################################################################
# Returning the current ending number of a directory
#################################################################

sub get_number_from_directory
{
	my ( $directory ) = @_;
	
	my $number = 0;
	
	if ( $directory =~ /\_(\d+)\s*$/ )
	{
		$number = $1;
	}

	return $number;
}

#################################################################
# Replacing separators, that are included into quotes
#################################################################

sub replace_masked_separator
{
	my ($string, $separator, $replacementstring) = @_;
	
	$string =~ s/\\\Q$separator\E/$replacementstring/g;
	
	return $string;
}

#################################################################
# Resolving separators, that were replaced
# in function mask_separator_in_quotes
#################################################################

sub resolve_masked_separator
{
	my ($arrayref, $separator, $replacementstring) = @_;
	
	for ( my $i = 0; $i <= $#{$arrayref}; $i++ )
	{
		${$arrayref}[$i] =~ s/$replacementstring/$separator/g
	}
}

1;
