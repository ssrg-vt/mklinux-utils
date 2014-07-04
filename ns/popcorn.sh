#!/bin/sh

echo "switching to poopcorn ns!"
echo 0 > /proc/popcorn
exec bash

