#!/usr/bin/perl -W
my $val=0;

open LCD, "/proc/acpi/toshiba/lcd" or die "failed opening /proc/acpi/toshiba/lcd for reading \n";

while (<LCD>) {
  if (/brightness:\s+(\d)/) { 
    $val = $1; 
  } 
}

close LCD;

$val++;
$val = 7 if ($val > 7);

open LCD,">/proc/acpi/toshiba/lcd" or die "failed opening /proc/acpi/toshiba/lcd for writing \n";
print LCD "brightness: $val";
close LCD;
