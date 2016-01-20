#!/bin/bash
npass=2 

if [ $# -lt 4 ]; then
	echo 'wrong command line: it needs almost four files' 2>&1 > /dev/stderr
	exit 1
fi

names=`awk '{print $1}' $* | sort -u`
for student in $names; do
	npstudent=`sort $* | uniq -c | grep ' '$student' si$' | awk '{print $1}'`
	if [ $npstudent -ge $npass ] > /dev/null 2>&1; then
		echo $student si
	else
		echo $student no
	fi
done

