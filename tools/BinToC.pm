package BinToC;

sub binary_to_c($$$$$) {
    my ($input_file_path,
        $output_src_fh,
        $array_var_name,
        $end_ptr_var_name,
        $size_var_name) = @_;
    
    my $input_file_size = -s $input_file_path;
    
    print $output_src_fh "#include <stddef.h>\n";
    print $output_src_fh "#include <stdint.h>\n";
    print $output_src_fh "asm(\n";
    print $output_src_fh "\".p2align 3\\n\"\n";
    print $output_src_fh "\".data\\n\"\n";;
    print $output_src_fh "\"$array_var_name:\\n\"\n";
    print $output_src_fh "\"_$array_var_name:\\n\"\n";
    print $output_src_fh "\".globl $array_var_name\\n\"\n";
    print $output_src_fh "\".globl _$array_var_name\\n\"\n";
    print $output_src_fh "\".incbin \\\"$input_file_path\\\"\\n\"\n";
    print $output_src_fh ");\n";
    print $output_src_fh "extern const uint8_t $array_var_name\[\];\n";
    print $output_src_fh "const uint8_t * const $end_ptr_var_name\n";
    print $output_src_fh "    = $array_var_name + $input_file_size;\n";
    print $output_src_fh "const size_t $size_var_name\n";
    print $output_src_fh "    = $input_file_size;\n";
    print $output_src_fh "\n";
}

1;
