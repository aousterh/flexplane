#!/bin/bash

# compile one arbiter of each type for running experiments

# make arbiter that runs RED
make clean && make EMU_RTR_FLAGS=-DRED -j5
mv build/fast build/fast_red

# make arbiter that runs DCTCP
make clean && make EMU_RTR_FLAGS=-DDCTCP -j5
mv build/fast build/fast_dctcp

# make arbiter that runs drop tail
make clean && make EMU_RTR_FLAGS=-DDROP_TAIL -j5
cp build/fast build/fast_drop_tail
