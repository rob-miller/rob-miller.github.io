Rob-

Here's another handy script, that loads the cPad with Slashdot
headlines.  I cron this to run every 5 minutes and load the default
image one minute earlier (so it's 4 minutes of headlines, and one minute
of default).  You could use this as the basis to cycle various data
sources at regular intervals.

Mace


#!/bin/bash

#
# Update Slashdot headlines on the cPad
#
# Author: Mace Moneta
#
# License: GPL
#
# Requires: ImageMagick (http://www.imagemagick.org)
#           cPad Linux  (http://www.janerob.com/rob/ts5100/index.shtml)
#           links command line browser (you can substitute lynx)
#

CPAD="/tmp/`whoami`_slashpad.xpm"
DATA="/tmp/`whoami`_slashpad.txt"
LOCK="/tmp/cPad.lck"

if [ -f $LOCK ]
then
   exit
fi
/bin/touch $LOCK

/usr/bin/links -source http://slashdot.org/slashdot.xml | \
   /bin/grep '<title>'                                  | \
   /bin/sed -e 's/<title>/* /'                          | \
   /bin/sed -e 's/<\/title>//'                          | \
   /usr/bin/awk '{print $1,$2,$3,$4,$5,$6,$7.$8,$9}'    | \
   /usr/bin/fold -w 22 -s                               | \
   /bin/grep -v "^  $" > $DATA

/usr/bin/X11/convert -font courier-bold -pointsize 18 -draw 'text 0,15
"@'$DATA'"' -size 240x160 /usr/local/bin/blank.xpm -resize 240x160
-monochrome $CPAD

/usr/local/bin/usr_cpad -i $CPAD

/bin/rm -f $DATA
/bin/rm -f $CPAD
/bin/rm -f $LOCK


