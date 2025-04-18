#!/usr/bin/perl -w

# This script generates a C file to embed a a binary resource.
# The result is very similar to what GNU Binutil's "ld -b binary"
# generates.

use strict;
use lib './tools';
use File::Basename;
use BinToC;

my $input_file_path;
my $output_src_file_path;

($input_file_path, $output_src_file_path) = @ARGV;

my($input_file_name, $input_file_dir, $input_file_ext)
  = fileparse $input_file_path;

my $array_name = "${input_file_name}${input_file_ext}";
$array_name =~ s,\.,_,g;

open my $output_src_fh, '>', $output_src_file_path
  or die "Could not source open output file $output_src_file_path!\n";

BinToC::binary_to_c($input_file_path,
                    $output_src_fh,
                    $array_name,
                    $array_name."_end",
                    $array_name."_size");
close $output_src_fh;
