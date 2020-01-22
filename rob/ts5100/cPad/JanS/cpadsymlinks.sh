#!/bin/bash

sed=/bin/sed
basename=/usr/bin/basename

if [ $# -ne 2 ] ; then
	exit 1
fi

case $1 in
	cdev)	cpad=$2 ;;
	input)	for i in /sys/class/usb/cpad* ; do
			if [ /sys/class/input/$2/device -ef $i/device ] ; then
				cpad=`$basename $i`
				break;
			fi
		done ;;
	*)	exit 1 ;;
esac

if [ $cpad ] ; then
	echo $cpad | sed s/cpad//
	exit
fi

exit 1
