#!/bin/bash
set -eu -o pipefail

target="${image_root}/root/"
cp -r "./replica1" "${target}/"
cp -r "./replica2" "${target}/"
