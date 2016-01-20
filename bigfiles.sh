#!/bin/bash
files=`du -a`
nfiles=1

if [ $# -eq 1 ]; then
	nfiles=$1
fi

for f in $files; do
	if test -f $f;then 
		echo $f
	fi
done |xargs du -b|sort -nr|sed ${nfiles}q|awk '{ print $2 "\t" $1}'
exit 0
