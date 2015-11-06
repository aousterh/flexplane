#!/bin/bash

racks=5


sudo cset shield -e -- build/fast -c 6 -n 3 -m 256 --file-prefix first --no-hpet -- -p 1 > run_0_${racks}_racks.txt &
sudo cset shield -e -- build/fast -c 18 -n 3 -m 256 --file-prefix second --no-hpet -- -p 1 > run_1_${racks}_racks.txt &
sudo cset shield -e -- build/fast -c 60 -n 3 -m 256 --file-prefix third --no-hpet -- -p 1 > run_2_${racks}_racks.txt &
sudo cset shield -e -- build/fast -c 180 -n 3 -m 256 --file-prefix fourth --no-hpet -- -p 1 > run_3_${racks}_racks.txt &
sudo cset shield -e -- build/fast -c 600 -n 3 -m 256 --file-prefix fifth --no-hpet -- -p 1 > run_4_${racks}_racks.txt &

