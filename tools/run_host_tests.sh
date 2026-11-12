#!/bin/bash
# both host suites at the given levels (default -O1 -O2)
cd "$(dirname "$0")"
for fl in ${@:--O1 -O2}; do
  F="-DHOST $fl -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-builtin-declaration-mismatch -Wno-misleading-indentation"
  rm -f host_test host_sb_test
  gcc $F -o host_test spooky.c host_test.c -lm 2>/tmp/c1.txt && ./host_test 2>/tmp/e1.txt | grep -E 'ALL'; grep -a -o 'Assertion.\{0,200\}' /tmp/e1.txt
  gcc $F -o host_sb_test spooky.c host_sb_test.c -lm 2>/tmp/c2.txt && ./host_sb_test 2>/tmp/e2.txt | grep -E 'ALL'; grep -a -o 'Assertion.\{0,200\}' /tmp/e2.txt
  grep -c "error" /tmp/c1.txt /tmp/c2.txt
done
