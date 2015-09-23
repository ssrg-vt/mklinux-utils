#!/bin/sh
OPT="-ciu"
echo "launching a new namespace $OPT"
./ns_child_exec $OPT ./simple_init
