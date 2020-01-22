#!/usr/bin/perl -w

my $dir=".";

my $i=0;
my @opts=();

while (defined $ARGV[$i]) {
  if ($ARGV[$i] =~ /^-(\S+)/) {
    if ($1 eq '?') {
      print "options:\n";
      print "   -c    clear: remove index.html and thumbnail subdirs before continuing\n";
      print "   -x    exit after processing preceding options (use e.g. with -c)\n";
      print "   -?    this help\n";
      die "\n";
    }
    push @opts,$1;
  } elsif ($ARGV[$i] =~ /(\S+)/) {
    $dir = $1;
  }
  $i++;
}

foreach $opt (@opts) {
  if ($opt eq 'c') {
    my $rslt=0; 
    if ($rslt = system("rm -f $dir/index.html")) {
      die "while removing $dir/index.html\n";
    }
    if (system("rm -fr $dir/*/thumbnails-*")) {
      die "$! while removing $dir/*/thumbnails-*\n";
    }
  } elsif ($opt eq 'x') {
    exit;
  } else {
    die "option $opt not recognised, try -?\n";
  }
}


$dir = $ARGV[0] if (defined $ARGV[0]);


if (-e "$dir/index.html") {
  die "please remove $dir/index.html first, or use -c\n";
}


open (LS,"ls -1 $dir |") or die "unable to ls $dir\n";

open (NDX,">$dir/index.html") or die "unable to open $dir/index.html\n";

writeHead(NDX);

while (<LS>) {
  chomp;
  if ((-d "$dir/$_/dcim") or (-d "$dir/$_/mssony")) {
    my ($mpgs, $jpgs, $movs) = (0,0,0);
    my $curr = $_;
    system("/usr/local/bin/proof.pl $_");
    if (open(JPC,"ls -1 $dir/$curr/*/*/*.jpg 2>/dev/null | grep -v imcif | wc -l |")) {
      while (<JPC>) {
	if (/(\d+)/) {
	  $jpgs = $1;
	}
      }
      close JPC;
    }
    if (open(MPC,"ls -1 $dir/$curr/*/*/*.mpg 2>/dev/null | wc -l |")) {
      while (<MPC>) {
	if (/(\d+)/) {
	  $mpgs = $1;
	}
      }
      close MPC;
    }
    if (open(MOC,"ls -1 $dir/$curr/*/*/*.mov 2>/dev/null | wc -l |")) {
      while (<MOC>) {
	if (/(\d+)/) {
	  $movs = $1;
	}
      }
      close MOC;
    }

    print NDX "<a href=\"$curr/proof.html\">$curr</a> &nbsp $jpgs JPEGs &nbsp $mpgs MPEGs &nbsp $movs MOVs<br>\n"
  }
}

close LS;

writeFoot(NDX);
close NDX;


##########

# write beginning of html file

sub writeHead {
  my $outf = shift;
  my $auth = $ENV{USERNAME};

print $outf <<EndHeadSec;
<html>
<head>
   <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
   <meta name="Author" content="$ENV{USERNAME}">
   <title></title>
</head>

<body>

EndHeadSec

}

# write end of html file
sub writeFoot {
  my $outf = shift;
  print $outf "<br>&nbsp;\n</body>\n</html>\n";
}
