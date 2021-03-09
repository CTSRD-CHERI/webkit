#!/bin/bash

set -x
set -e

WEBKIT_BUILD_DIR="/local/scratch/bg357/cheri/build/webkit-tier2asm-caps-morello-purecap-build"
REMOTE="root@localhost:14999"
REMOTE_SSH="root@localhost -p 14999"

TEST_FLAGS=""
TEST_FLAGS+=" --jsc-only"
TEST_FLAGS+=" --release"
TEST_FLAGS+=" --no-build"
TEST_FLAGS+=" --quick" # greatly reduce number of configs run
#TEST_FLAGS+=" --no-fail-fast"
#TEST_FLAGS+=" --memory-limited"
TEST_FLAGS+=" --root=${WEBKIT_BUILD_DIR}"
TEST_FLAGS+=" --remote=${REMOTE}"

TEST_FLAGS+=" --no-ftl-jit"
TEST_FLAGS+=" --no-testmasm"
TEST_FLAGS+=" --no-testair"
TEST_FLAGS+=" --no-testb3"
TEST_FLAGS+=" --no-testdfg"
TEST_FLAGS+=" --no-testapi"

ssh ${REMOTE_SSH} "mkdir /root/jsc-temp; echo '{\"tempPath\": \"/root/jsc-temp\"}' > /root/.bencher"
# REQUIRES BASH
~/local/cheri/webkit/Tools/Scripts/run-javascriptcore-tests ${TEST_FLAGS}
