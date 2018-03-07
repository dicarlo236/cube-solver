#!/bin/bash
set -e
for i in {1..100}
do
    ./cube_sequence a &
done
wait
echo "done"
echo "Tested 100000000 serial encodes/decodes"
echo "Tested 100000 solve sequences"

