#!/bin/bash

# check the arguments
if [ "$#" -ne 1 ]; then
    # set default value of the mask, for 7 cores
    MASK=fe
else
    MASK="$1"
fi

mkdir -p log/

# -m 512 commented out; bring back if needed
sudo build/fast -c $MASK -n 3 --no-hpet -- -p 1

