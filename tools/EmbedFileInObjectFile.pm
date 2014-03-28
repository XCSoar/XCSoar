package EmbedFileInObjectFile;

sub embed_file_in_object_file($$$$$$) {
    my ($input, $output, $start_sym_name, $end_sym_name, $size_sym_name, $as) = @_;

    my $input_file_size = -s $input;

    my $asm = "";
    $asm .= ".p2align 3\n";
    $asm .= ".data\n";
    $asm .= ".globl ${start_sym_name}\n";
    $asm .= "${start_sym_name}:\n";
    $asm .= ".incbin \"${input}\"\n";
    $asm .= ".globl ${end_sym_name}\n";
    $asm .= ".set ${end_sym_name}, \.\n";
    $asm .= ".globl ${size_sym_name}\n";
    $asm .= ".set ${size_sym_name}, ${input_file_size}\n";
    $asm .= ".globl _${start_sym_name}\n";
    $asm .= ".set _${start_sym_name}, ${start_sym_name}\n";
    $asm .= ".globl _${end_sym_name}\n";
    $asm .= ".set _${end_sym_name}, ${end_sym_name}\n";
    $asm .= ".globl _${size_sym_name}\\n";
    $asm .= ".set _${size_sym_name}\, ${size_sym_name}\\n";

    my $asm_echo = $asm;
    $asm_echo =~ s,\n,\\n,g;
    $asm_echo =~ s,\",\\",g;

    system("printf \"${asm_echo}\" | ${as} -o ${output}"
              ) == 0 or die;
}

1;

