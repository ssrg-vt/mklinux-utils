#!/bin/sh

echo "activating poopcorn ns!"
echo 0 > /proc/popcorn_namespace
exec bash

