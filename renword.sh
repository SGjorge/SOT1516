#!/bin/bash


if [ $# != 1 ]; then
	echo 'wrong command line: it needs only one parameter' 2>&1 > /dev/stderr
	exit 1
fi

files=`ls`

for f in $files
do
	if grep '^'$1 $f > /dev/null 2>&1 ; then
		mv $f $f.$1
	fi
done

exit 0
