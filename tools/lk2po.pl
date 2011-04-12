#!/usr/bin/perl
#
# Import translations from LK8000.
#

use strict;
require Locale::PO;

use vars qw($lk_language);

# XXX hard-coded path, sorry!
$lk_language = '/usr/src/git/LK8000/Common/Data/Language';

# hard-coded list of LK8000 languages
my %languages = (
    GER => 'de',
    FRA => 'fr',
    GRE => 'el',
    CZE => 'cs',
    ESP => 'es',
    HRV => 'hr',
    HUN => 'hu',
    ITA => 'it',
    NED => 'nl',
    POL => 'pl',
    PTB => 'pt_BR',
    POR => 'pt',
    SRB => 'sr',
    SWE => 'sv',
);

sub load_msg($$) {
    my $encoding = shift;
    my $path = shift;
    print STDERR "loading $path\n";
    my @messages;
    local *FILE;
    open FILE, "<:encoding($encoding)", $path or die $!;
    while (<FILE>) {
        $messages[$1] = $2
          if /^_\@M(\d+)_\s+"(.*)"/;
    }
    close FILE;
    return @messages;
}

sub load_menu($$) {
    my $encoding = shift;
    my $path = shift;
    print STDERR "loading $path\n";
    my %labels;
    local *FILE;
    open FILE, "<:encoding($encoding)", $path or die $!;
    my $key = '';
    my $label;
    while (<FILE>) {
        s,^\s+,,;
        s,\s+$,,;

        if (/^(mode|type|data|event)=.*$/) {
            if ($1 eq 'mode') {
                $labels{$key} = $label
                  if length $key and defined $label and length $label;
                $key = '';
                undef $label;
            }

            $key .= $_ . "\n";
        } elsif (/^label=(.+)$/) {
            $label = $1;

            $label =~ s,(\s|\\n)*[\$\&]\(\w+\),,g;
        }
    }

    close FILE;

    return %labels;
}

mkdir 'output/po';

# load the English message file
my @english = load_msg('ASCII', "$lk_language/ENG_MSG.TXT");
my %english_menu = load_menu('ISO-8859-1', "$lk_language/ENG_MENU.TXT");

# now iterate over all languages of LK8000
foreach my $lng (keys %languages) {
    my $iso = $languages{$lng};

    # read the LK8000 translation file
    my @translated = load_msg('UTF-8',
                              "$lk_language/Translations/${lng}_MSG.TXT");
    my %table;
    for (my $i = 0; $i <= $#translated; $i++) {
        my $english = $english[$i];
        next unless defined $english;
        my $translated = $translated[$i];
        next unless defined $translated;
        next unless length $translated;
        next if $english eq $translated;

        # store in table
        $table{$english} = $translated;

        # see if we can merge it with the prefix number stripped
        if ($english =~ /^(\d+)\s+(.*)$/s) {
            my ($e_number, $english2) = ($1, $2);
            if ($translated =~ /^(\d+)\s+(.*)$/s) {
                my ($t_number, $translated2) = ($1, $2);
                $table{$english2} = $translated2 if $e_number == $t_number;
            }
        }

        # now strip trailing whitespace
        if ($english =~ /^(.*?)\s+$/s) {
            my $english2 = $1;
            my $translated2 = $translated;
            $translated2 =~ s,\s+$,,;
            $table{$english2} = $translated2;
        }
    }

    my %translated = load_menu('UTF-8',
                               "$lk_language/Translations/${lng}_MENU.TXT");
    foreach my $key (keys %english_menu) {
        my $english = $english_menu{$key};
        my $translated = $translated{$key};
        next unless defined $translated;
        next if $english eq $translated;

        # store in table
        $table{$english} = $translated;

        # see if we can merge it with the prefix number stripped
        if ($english =~ /^(\d+)\s+(.*)$/s) {
            my ($e_number, $english2) = ($1, $2);
            if ($translated =~ /^(\d+)\s+(.*)$/s) {
                my ($t_number, $translated2) = ($1, $2);
                $table{$english2} = $translated2 if $e_number == $t_number;
            }
        }

        # now strip trailing whitespace
        if ($english =~ /^(.*?)\s+$/s) {
            my $english2 = $1;
            my $translated2 = $translated;
            $translated2 =~ s,\s+$,,;
            $table{$english2} = $translated2;
        }
    }

    my $in = Locale::PO->load_file_asarray("po/${iso}.po");
    $in = Locale::PO->load_file_asarray("po/xcsoar.pot")
      unless defined $in;

    my @out;
    foreach my $x (@$in) {
        # see if the translation is useful and needed
        next if $x->obsolete;
        my $msgid = eval $x->msgid;
        next unless length $msgid;
        my $msgstr = eval $x->msgstr;
        next if length $msgstr and not $x->fuzzy;
        next unless exists $table{$msgid};

        my $z = $table{$msgid};

        # escape special characters
        $z =~ s/\n/\\n/g;
        $z =~ s/\r/\\r/g;

        push @out, new Locale::PO(-msgid => $msgid,
                                  -msgstr => $z);
    }

    # anything new?
    next unless @out;

    # declare the UTF-8 charset
    push @out, new Locale::PO(-msgid => '',
                              -msgstr => "Content-Type: text/plain; charset=UTF-8\\nContent-Transfer-Encoding: 8bit\\n");

    # write temporary file with new translations
    local *FILE;
    open FILE, ">:utf8", "output/po/lk8000-${iso}.po" or die $!;
    foreach my $x (@out) {
        print FILE $x->dump;
    }
    close FILE or die $!;

    # generate PO file if it doesn't exist already
    unless (-f "po/${iso}.po") {
        system("msginit --locale=$iso --no-translator --input=po/xcsoar.pot --output=po/${iso}.po") == 0
          or die;
    }

    # merge the two
    system("msgcat --use-first po/${iso}.po output/po/lk8000-${iso}.po --output-file=po/${iso}.po") == 0
      or die;
}
