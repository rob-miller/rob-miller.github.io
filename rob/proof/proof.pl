#!/usr/bin/perl -w

#
# proof.pl
#
# generate an html proof sheet referencing thumbnails.
# initial development for Sony cameras DSC-S70 and DSC-F717
#
#  requires:
#   mplayer to to extract jpeg frame from mpeg files
#   transcode to convert Quicktime files to mpeg1
#   ImageMagick (convert)     http://www.imagemagick.org/
#   jhead                     http://www.sentex.net/~mwandel/jhead/
#   mpeg2decode               http://www.mpeg.org/MSSG/
#   Image::Info CPAN module   http://www.cpan.org/modules/index.html
#
#   Original Author R.T. Miller  ( rob at janerob dot com )
#   version 1.0 29 July 2003
#

use Image::Info qw(image_info);

# number of thumbnails across
my $wid=6;

# these are the sizes of the thumbnails embedded in the JPEG images
my $pheight = 120;
my $pwidth  = 160;
my $sizestr = "height=$pheight width=$pwidth";
my $dimstr  = $pwidth."x".$pheight;

my $subdir="";
my $folder="";
my $started = 0;


# check input, write beginning of html file

die "usage: $0 <directory>\n" unless ($#ARGV == 0);
my $dir = $ARGV[0];
die "no directory $dir\n" unless (-d $dir);
open (OUTF,">$dir/proof.html") or die "failed to open $dir/proof.html for writing\n";
writeHead (OUTF, $dir, $ENV{USERNAME});

# look for dcim and mssony subdirs, process each

open (LS0, "ls -1 $dir|") or die "couldn't do ls of $dir\n";
while (<LS0>) {
  if (/dcim/) {
    do_dir ($dir, "dcim");     # JPEGs here (also MPEGs for F717)
  } elsif (/mssony/) {
    do_dir ($dir, "mssony");   # S70 puts MPEGs here
  }
}
close LS0;

# finish and close html file
writeFoot (OUTF);
close OUTF;


###############################

# process folders sithin subdir; F717 may have multiple folders
sub do_dir {
  my $dir = shift;
  my $subdir = shift;

  open (LS1, "ls -1 $dir/$subdir|") or die "couldn't do ls of $dir/$subdir\n";

  while (<LS1>) {
    chomp;
    if (/imcif/) {              # skip e-mail dir for DSC-S70
    } elsif (-d "$dir/$subdir/$_") {
      if ($started) {
	print OUTF "<hr>\n";    # pretty line to separate folder output
      }
      do_proof ($dir, $subdir, $_);
      $started=1;
    }
  }
  close LS1;
}


# convert images to thumbnails, generate main body of html file
sub do_proof {
  my $dir = shift;
  my $subdir = shift;
  my $folder = shift;

  my @jpglist = ();
  my @mpglist = ();

  system ("mkdir $dir/thumbnails-$folder") unless (-d "$dir/thumbnails-$folder");

  # identify files to process, create thumbnails
  open (LS, "ls -1 $dir/$subdir/$folder|") or die "couldn't do ls of $dir/$subdir/$folder\n";
  while (<LS>) { 
    if (/(\S+)\.jpg/) { 
      push @jpglist, "$1.jpg";
      # extract embedded JPEG thumbnails with jhead
      system("/usr/bin/jhead -st $dir/thumbnails-$folder/$1.jpg $dir/$subdir/$folder/$1.jpg");
    } elsif (/(\S+)\.mpg/) {
      push @mpglist, "$1.mpg";
      # create JPEG thumbnail for MPEG files with ImageMagick (which runss mpeg2decode)
      system("/usr/bin/mplayer -really-quiet -ao null -vo jpeg -x 0 -y 0 -frames 1 $dir/$subdir/$folder/$1.mpg");
      system("/usr/bin/convert -size $dimstr 00000001.jpg -resize $dimstr $dir/thumbnails-$folder/$1.jpg");
      system("rm 00000001.jpg");
      #system("/usr/bin/convert -size $dimstr $dir/$subdir/$folder/$1.mpg[1] -resize $dimstr $dir/thumbnails-$folder/$1.jpg");
    } elsif (/(\S+)\.mov/) {
      push @mpglist, "$1.mov";
      system("/usr/bin/transcode -q 0 -i $dir/$subdir/$folder/$1.mov -y ffmpeg,null -F mpeg1 -o $dir/$subdir/$folder/$1.mpg");
      system("mv $dir/$subdir/$folder/$1.mpg.m1v $dir/$subdir/$folder/$1.mpg");
      # create JPEG thumbnail for MPEG files with ImageMagick (which runs mpeg2decode)
      system("/usr/bin/mplayer -really-quiet -ao null -vo jpeg -x 0 -y 0 -frames 1 $dir/$subdir/$folder/$1.mpg");
      system("/usr/bin/convert -size $dimstr 00000001.jpg -resize $dimstr $dir/thumbnails-$folder/$1.jpg");
      system("rm 00000001.jpg");
    }
  }
  close LS;
  
  my $i;
  my $jpgbase=0;
  
  if ($#jpglist > -1) {
    print OUTF "folder $subdir/$folder -- JPEG files\n";
    print OUTF "<table>\n";
  }

  while ($jpgbase <= $#jpglist) {
    my $fname;

    # create a row of $wid thumbnail images

    print OUTF " <tr>\n";
    for ($i=0; $i<$wid; $i++) {
      last if (($jpgbase + $i ) > $#jpglist);
      $fname = "thumbnails-$folder/$jpglist[$jpgbase + $i]";
      print OUTF "  <td>\n   <a href=\"$subdir/$folder/$jpglist[$jpgbase + $i]\"><img src= $fname $sizestr></a>\n  </td>\n";
    }
    print OUTF " </tr>\n";
    

    # create a row of $wid info strings

    print OUTF " <tr>\n";
    for ($i=0; $i<$wid; $i++) {
      last if (($jpgbase + $i ) > $#jpglist);
      $fname = "$dir/$subdir/$folder/$jpglist[$jpgbase + $i]";
      
      # filename without extension

      my $sfname = $jpglist[$jpgbase + $i];
      $sfname =~ s/.jpg//;
      
      my ($picinfo) = image_info($fname);
      
      # photo date and time from Image::Info

      my $datestr = $picinfo->{'DateTimeOriginal'};
      my ($yr,$mo,$dy,$hr,$mn,$sc) = (0,0,0,0,0,0);
      if ($datestr =~ /(\d+):(\d+):(\d+) (\d+):(\d+):(\d+)/) {
	($yr,$mo,$dy,$hr,$mn,$sc) = ($1,$2,$3,$4,$5,$6);
      }
      
      # image resolution from Image::Info

      my ($iw,$ih) = (0,0);
      $ih = $picinfo->{'height'} if (defined $picinfo->{'height'});
      $iw = $picinfo->{'width'} if (defined $picinfo->{'width'});
      
      # print the info string for 1 image
      print OUTF "  <td>$sfname &nbsp &nbsp $dy/$mo $hr:$mn &nbsp &nbsp ",$iw,"x$ih</td>\n";
    }
    print OUTF " </tr>\n";
    
    $jpgbase += $wid;
  }

  if ($#jpglist > -1) {
    print OUTF "</table>\n";
  }
  
  # now process any MPEGs

  return if ($#mpglist <0);

  if ($#jpglist > -1) {
    print OUTF "<hr>\n";
  }
  
  print OUTF "folder $subdir/$folder -- MPEG/MOV files\n";

  my $mpgbase=0;
  
  print OUTF "<table>\n";
  
  while ($mpgbase <= $#mpglist) {
    my $fname;
    print OUTF " <tr>\n";

    # create row of thumbnail images

    for ($i=0; $i<$wid; $i++) {
      last if (($mpgbase + $i ) > $#mpglist);
      $fname = "thumbnails-$folder/$mpglist[$mpgbase + $i]";
      $fname =~ s/.mpg$/.jpg/;
      $fname =~ s/.mov$/.jpg/;
      #    $fname = "$dir/$subdir/$folder/$jpglist[$jpgbase + $i]";
      print OUTF "  <td>\n   <a href=\"$subdir/$folder/$mpglist[$mpgbase + $i]\"><img src= $fname $sizestr>\n  </td>\n";
    }
    print OUTF " </tr>\n";

    # create row of info strings

    print OUTF " <tr>\n";
    for ($i=0; $i<$wid; $i++) {
      last if (($mpgbase + $i ) > $#mpglist);
      $fname = "$dir/$subdir/$folder/$mpglist[$mpgbase + $i]";

      # filename without extension

      my $sfname = $mpglist[$mpgbase + $i];
      $sfname =~ s/.mpg$//;
      $sfname =~ s/.mov$//;
      
      # image resolution, time and framecount from mpeg2decode

      my ($hsize, $vsize, $frames, $timecode) = (0,0,0,"");

      my $fname2 = $fname;
      $fname2 =~ s/.mov$/.mpg/;
      open (MPEG,"/usr/bin/mpeg2decode -v2 -b $fname2 |");
      while (<MPEG>) {
	if (/horizontal_size=(\d+)/) {
	  $hsize = $1;
	} elsif (/vertical_size=(\d+)/) {
	  $vsize = $1;
	} elsif (/picture header/) {
	  $frames++;
	} elsif (/timecode (\S+)/) {
	  $timecode = $1;
	}
      }
      close MPEG;

      # print the info string for 1 MPEG image
      print OUTF "  <td>$sfname &nbsp &nbsp &nbsp $timecode &nbsp &nbsp ",$hsize,"x$vsize</td>\n";
      if ($sfname =~ /^pict/) {
	  system("rm $dir/$subdir/$folder/$sfname.mpg");
	  #print "kill $sfname\n";
      }
    }
    print OUTF " </tr>\n";
    
    $mpgbase += $wid;
  }

  print OUTF "</table>\n";
}


# write beginning of html file

sub writeHead {
  my $outf = shift;
  my $dir = shift;
  my $auth = shift;

print $outf <<EndHeadSec;
<html>
<head>
   <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
   <meta name="Author" content="$ENV{USERNAME}">
   <title>proof sheet for directory $dir</title>
<style type="text/css">
table {font-size: 10}
</style>
</head>

<body>

EndHeadSec

}

# write end of html file
sub writeFoot {
  my $outf = shift;
  print $outf "<br>&nbsp;\n</body>\n</html>\n";
}

