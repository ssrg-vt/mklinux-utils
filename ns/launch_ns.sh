#!/bin/sh
#c opt is for NEWPOPCORN
#OPT="-cimnpu"
OPT="-cu"
echo "launching a new Popcorn namespace $OPT"
./ns_child_exec $OPT ./simple_init
