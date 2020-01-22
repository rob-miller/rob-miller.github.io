#!/usr/bin/perl -w
#use Image::Info qw(image_info);
use Image::ExifTool 'ImageInfo';
my $imgMatch = '(img|IMG)_(\d{4}).(jpg|JPG|cr2|CR2)';
#my $imgTarg  = '$1_$2.jpg';
my $maxSize  = int(4.3*1024*1024*1024);
my $dirMatch = '^d(\d+)$';
my $noLocal  = 0;
my $testOnly = 0;

my $dfltDest="/data2/photos-dvd";
my $dst = $dfltDest;
my $src="";

my $dbg=0;


my $first="";
my $last ="";
my $firstSrc="";
my $lastSrc ="";

sub usage {
    print "$0 [-opt] <src dir> [<dest dir>]\n";
    print "   copies all of <src dir> to yr-month-day[_n] dir in current dir\n";
    print "   copies files matching $imgMatch under <src dir> \n";
    print "    to d<n> subdirs under <dest dir> to max size of $maxSize bytes\n";
    print "   default <dest dir> is $dfltDest\n";
    print "   options\n";
    print "      -n  :  omit copy to yr-month-day[_n] dir in current dir\n";
    print "      -t  :  test only - report progress, warn of not copied files\n";

    exit;
}

sub pixDate {
    my $pix = shift;
    #my $pinfor = image_info($pix);
    my $pinfor = ImageInfo($pix);
    return $pinfor->{'DateTimeOriginal'};
}

### main

my $usedSpace=0;
my $dirCnt=1;

while ($#ARGV > -1) {
    my $arg = shift;

    if ($arg =~ /^-(\w)$/) {
	my $opt = $1;
	if ($opt eq 'n') {
	    $noLocal=1;
	} elsif ($opt eq 't') {
	    $testOnly=1;
	} else {
	    usage;
	}
    } elsif ($src eq "") {
	$src = $arg;
    } elsif ($dst != $dfltDest) {
	$dst = $arg;
    } else {
	usage;
    }
    
}

usage unless (-d $src);

unless ($noLocal) {
    my @tim = localtime(time);
    my $yr= $tim[5]+1900;
    my $mo= $tim[4]+1;
    my $dy= $tim[3];

    my $dstr = "$yr-$mo-$dy";
    my $cnt=0;
    while (-d $dstr) {
	$cnt++;
	$dstr = "$yr-$mo-$dy"."_$cnt";
    }

    print "creating local dir $dstr\n" if ($dbg);
    unless ($testOnly) {
	die "unable to create dest dir $dstr : $!\n" 
	    unless system("mkdir $dstr") ==0;

	system("cp -a $src/* $dstr/") ==0
	    or die "unable to copy $src to $dstr: $!\n";
    }
    
}

if (! -d $dst) {
    print "create targ dir and first subdir\n" if ($dbg);
    unless ($testOnly) {
	die "unable to create dest dir $dst : $!\n" 
	    unless system("mkdir $dst") ==0;
	die "unable to create dest dir $dst/d$dirCnt : $!\n" 
	    unless system("mkdir $dst/d$dirCnt") ==0;
    }
} else {
    my $haveDir=0;
    my $currSize=0;
    print "targ dir exists\n" if ($dbg);
    open DLST, "ls -1 $dst |" 
	or die "unable to exec ls on $dst: $!\n";
    while (<DLST>) {
	chomp;
	if (/$dirMatch/) {
	    $dirCnt = $1;
	    $haveDir=1;
	    printf "targ subdir %3s : ",$_;
	    open DLST2, "ls -1 $dst/$_ |" or die "unable to exec ls on $_: $!\n";

	    while (<DLST2>) {
		if (/$imgMatch/) {
		    if ($first eq "") {
			$first = "$1_$2.$3";
		    } else {
			$last = "$1_$2.$3";
		    }
		}
	    }
	    close DLST2;

	    open DU, "du -s $dst/d$dirCnt |" 
		or die "unable to exec du on $dst/d$dirCnt: $!\n";
	    $currSize=0;
	    while (<DU>) {
		if (/^(\d+)\s+(\S+)$/) {
		    $currSize = $1 * 1024;
		}
	    }
	    close DU;

	    print "$currSize bytes  $first ";
	    print pixDate("$dst/d$dirCnt/$first");
	    print " .. $last ";
	    print pixDate("$dst/d$dirCnt/$last");
	    print "\n";

	    $first="";
	    $last ="";
	} else {
	    print "no targ subdir match on $_\n"  if ($dbg);
	}
    }
    close DLST;
    if ($haveDir) {
	if ($currSize >= $maxSize) {
	    $dirCnt++;
	    print "targ subdir full : $currSize \n" if ($dbg);
	    print "creating new subdir $dst/d$dirCnt \n" if ($dbg);
	    unless ($testOnly) {
		die "unable to create dest dir $dst/d$dirCnt : $!\n" 
		    unless system("mkdir $dst/d$dirCnt") ==0;
	    }
	} else {
	    $usedSpace = $currSize;
	    print "targ subdir ok : $currSize \n" if ($dbg);
	}
    } else {
	print "creating new subdir $dst/d$dirCnt \n" if ($dbg);
	unless ($testOnly) {
	    die "unable to create dest dir $dst/d$dirCnt : $!\n" 
		unless system("mkdir $dst/d$dirCnt") ==0;
	}
    }
}


open FLST, "find $src |" or die "unable to exec find on $src: $!\n";

while (<FLST>) {
    chomp;
    if (/$imgMatch/) {
	my $size = (-s $_);
	#print "file $_ size $size\n";
	#print "copy $_ to $1_$2.$3\n";

	if ($usedSpace + $size < $maxSize) {
	    if (-s "$dst/d$dirCnt/$1_$2.$3") {
		open DIFF, "diff --brief $_ $dst/d$dirCnt/$1_$2.$3 |"
		    or die "unable to exec diff on $_ and $dst/d$dirCnt/$1_$2.$3 : $!\n";
		while (<DIFF>) {
		    if (/\S+/) {
			die "not copying $_ over different file $dst/d$dirCnt/$1_$2.$3\n";
		    }
		}
		close DIFF;
	    } else {
		unless ($testOnly) {
		    system("cp -a $_ $dst/d$dirCnt") ==0 
			or die "error copying $_ to $dst/d$dirCnt : $!\n";
		}
		$usedSpace += $size;
	    }
	} else {
	    printf "finished    %3s : ","d$dirCnt";
	    print "$usedSpace bytes  $first ";
	    #?print pixDate("$firstSrc") ;
	    print pixDate("$dst/d$dirCnt/$first") ;
	    print " .. $last ";
	    #?print pixDate("$lastSrc");
	    print pixDate("$dst/d$dirCnt/$last");
	    print "\n";

# size $usedSpace   $first .. $last\n" if ($dbg);
	    $first="";
	    $last ="";
	    $dirCnt++;
	    $usedSpace=0;
	    print "creating new subdir $dst/d$dirCnt \n" if ($dbg);
	    unless ($testOnly) {
		die "unable to create dest dir $dst/d$dirCnt : $!\n" 
		    unless system("mkdir $dst/d$dirCnt") ==0;
		system("cp -a $_ $dst/d$dirCnt") ==0 
		    or die "error copying $_ to $dst/d$dirCnt : $!\n";
	    }
	    $usedSpace += $size;
	}    

	# will mess up if just 1 img fills dir
	if ($first eq "") {
	    $first = "$1_$2.$3";
	    $firstSrc=$_;
	} else {
	    $last = "$1_$2.$3";
	    $lastSrc=$_;
	}

    } elsif (-f $_ and -s $_) {
	print "no match on $_ - not copied\n";
    }
}

printf "final dir   %3s : ", "d$dirCnt";
print "$usedSpace bytes  $first ";
#?print pixDate("$firstSrc/$first");
print pixDate("$dst/d$dirCnt/$first") ;
print " .. $last ";
#?print pixDate("$lastSrc/$last");
print pixDate("$dst/d$dirCnt/$last") ;
print "\n";

# size $usedSpace  $first .. $last\n" if ($dbg);
