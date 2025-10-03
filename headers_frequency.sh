#!/bin/bash
fd -e cpp -e hpp src |
    xargs rg '^#include' |
    rg -o '#include\s+[<"].+[>"]' |
    sort | uniq -c | sort -rn
