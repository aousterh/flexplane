#!/usr/bin/env Rscript
# This script can be run using:
# R < ./plot_emulator_stress.R <data> --save

# use the ggplot2 library
library(ggplot2)

# read in data
args <- commandArgs()
data <- read.csv(args[2])

theme_set(theme_bw(base_size=12))

data$r_factor = ifelse(data$type == "in_rack", 1,
              ifelse(data$type == "biased", (1 + 2 / (data$racks - 1)),
              (3 * data$racks - 2) / data$racks))
data$rtr_tput = data$endpoint_tput * data$r_factor
data

ggplot(data, aes(x=cores, y=rtr_tput, color=type)) +
             geom_point() +
             coord_cartesian(ylim=c(0,600)) +
             labs(x="Number of cores", y="Maximum total router throughput (Gbits/s)")

ggsave("emulator_throughput.pdf", width=10, height=6)
