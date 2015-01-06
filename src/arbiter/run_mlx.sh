#!/bin/bash

mkdir -p log/

sudo ./fast -c 7 -n 3 --no-hpet -d ./librte_pmd_mlx4.so -- -p 1
