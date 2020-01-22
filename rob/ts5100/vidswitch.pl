#!/usr/bin/perl -W
my $lcd=0;
my $crt=0;
my $tv=0;
my $tmp=0;

open VIDEO, "/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for reading \n";

while (<VIDEO>) {
  if (/lcd_out:\s+(\d)/) { 
    $lcd = $1; 
  } elsif (/crt_out:\s+(\d)/) { 
    $crt = $1; 
  } elsif (/tv_out:\s+(\d)/) { 
    $tv = $1; 
  }
}

close VIDEO;

$tmp = $tv;
$tv = $crt;
$crt = $lcd;
$lcd = $tmp;

if ( ! $lcd and ! $crt and ! $tv ) {
  $lcd = 1;
}

open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( ! $lcd ) {
  print VIDEO "lcd_out: 0";
}

close VIDEO;
open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( ! $crt ) {
  print VIDEO "crt_out: 0";
}

close VIDEO;
open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( ! $tv ) {
  print VIDEO "tv_out: 0";
}

close VIDEO;
open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( $lcd ) {
  print VIDEO "lcd_out: 1";
}

close VIDEO;
open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( $crt ) {
  print VIDEO "crt_out: 1";
}

close VIDEO;
open VIDEO,">/proc/acpi/toshiba/video" or die "failed opening /proc/acpi/toshiba/video for writing \n";

if ( $tv ) {
  print VIDEO "tv_out: 1";
}

close VIDEO;
