#!/usr/bin/perl

print "plane\n";
drawme("Data/other/plane.txt");
print "canopy\n";
drawme("Data/other/canopy.txt");

sub drawme {
    my ($file) = @_;
    open(INFILE,"<$file");
    my $i=0;
    while (<INFILE>) {
        if (/(\S+)\s(\S+)/) {
            $x[$i] = $1;
            $y[$i] = $2;
            $i=$i+1;
        }
    }
    close INFILE;
    for ($j=0; $j<$i; $j++) {
        $xx = $x[$j]-32;
        $yy = $y[$j]-10;
        print "{$xx, $yy},\n";
    }
    for ($j=$i-1; $j>=0; $j--) {
        $xx = 32-$x[$j];
        $yy = $y[$j]-10;
        print "{$xx, $yy},\n";
    }
}


