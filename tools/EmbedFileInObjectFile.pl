#!/usr/bin/perl -w

# This script does the same what GNU Binutil's "ld -b binary" does, but
# in a more portable way. It creates a small assembler program with an
# ".incbin" directive to embed the file and assembles it.

use strict;
use lib './tools';
use File::Basename;
use EmbedFileInObjectFile;

my $input;
my $output;
my $as;

($input, $output, $as) = @ARGV;

my($input_file, $input_dir, $input_ext) = fileparse($input);

my $sym_name_prefix = "_binary_${input_file}${input_ext}";
$sym_name_prefix =~ s,\.,_,g;

EmbedFileInObjectFile::embed_file_in_object_file($input, $output,
        $sym_name_prefix."_start", $sym_name_prefix."_end",
        $sym_name_prefix."_size", $as);

