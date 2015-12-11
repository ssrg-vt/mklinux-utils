#!/bin/bash
set -eu -o pipefail

( . "${scripts_dir}/simple_package.sh" scp )
