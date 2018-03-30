#!/bin/bash
# Copyright (c) 2008-2009 by Ian Cahoon <icahoon@gmail.com>
# All rights reserved.

export PYTHONPATH="$(dirname $PWD)"

test_dir=$PWD
tests="$(/bin/ls -1 ${test_dir:-.}/*_test 2>/dev/null)"
tests="$tests $(/bin/ls -1 ${test_dir:-.}/*_test.sh  2>/dev/null)"
tests="$tests $(/bin/ls -1 ${test_dir:-.}/*_test.py  2>/dev/null)"

result=0
column="\\033[35G"
date_color="\\033[1;33m"
success="\\033[1;32m"
failure="\\033[1;31m"
clear="\\033[0;39m"

echo -e "\nRunning Automated Tests\t $date_color$(date)$clear\n"

for t in $tests; do
    echo -en "\t${t##*/}$column"

    if [ -r $t.in.txt ]; then
        $t < $t.in.txt > $t.out.txt 2>&1
    else
        $t > $t.out.txt 2>&1
    fi
    if [ $? -ne 0 ]; then
        echo -e "${failure}failed${clear}"; (( result = result + 1 ))
    else
        echo -e "${success}success${clear}"
    fi

    summary="$(grep "^test summary:" $t.out.txt)"
    echo -e "\t${summary}\n"
done

echo -en "\nCompleted Tests$column"

if [ $result -ne 0 ]; then
    if [ $result -ne 1 ]; then
        echo -e "$result ${failure}failures${clear}\n"
    else
        echo -e "$result ${failure}failure${clear}\n"
    fi
else
    echo -e "${success}success${clear}\n"
fi
exit $result
