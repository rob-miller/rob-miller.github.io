#!/bin/bash

awk=/usr/bin/awk
grep=/bin/grep
basename=/usr/bin/basename
proc=/proc/driver/cpad

if [ $# -ne 2 ] ; then
	exit 1
fi

if [ ! -e $proc ] ; then
	exit 1
fi

case $1 in
	cdev)	proc_entry="cdev minor" ;;
	fb)	proc_entry="fb nr." ;;
	input)	proc_entry="input dev" ;;
	*)	exit 1 ;;
esac

for i in $proc/* ; do
	if echo "`$awk -F: "/$proc_entry/{ print \\\$2 }" $i`" | $grep $2 \
							> /dev/null ; then
		echo `$basename $i`
		exit
	fi
done
exit 1
