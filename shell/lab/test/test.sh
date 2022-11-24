
#! /bin/bash
cd ..
make
mytsh_result="./test/tshmy.txt"
tshref_result="./test/tshref.txt"
truncate $mytsh_result --size 0 
truncate $tshref_result --size 0 
echo -e "testing...\n"

for i in {1..16}; do
    t=""
    if [ $i -ge 10 ]; then t="$i"
    else t="0$i"
    fi
    # echo -e "./sdriver.pl  -t trace$t.txt -s ../shell/tsh -a \"-p\"" >> result
    ./test/sdriver.pl  -t ./trace/trace$t.txt -s ./tsh -a "-p" >> "$mytsh_result"
    ./test/sdriver.pl  -t ./trace/trace$t.txt -s ./tshref -a "-p" >> $tshref_result
done

diff $tshref_result $mytsh_result > ./test/tshdiff.txt
echo -e "end...\n"