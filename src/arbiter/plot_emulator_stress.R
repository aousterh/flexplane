#!/usr/bin/env Rscript
# This script can be run using:
# R < ./plot_emulator_stress.R <data> --save

# use the ggplot2 library
library(ggplot2)
library(reshape2)
library(plyr)

# read in data
args <- commandArgs()
data <- read.csv(args[2])

theme_set(theme_bw(base_size=12))
data$r_factor = ifelse(data$type == "in_rack", 1,
              ifelse(data$type == "biased", (1 + 2 / (data$racks - 1)),
              ifelse(data$type == "uniform", (3 * data$racks - 2) / data$racks,
              1)))
data$rtr_tput = data$ep_tput * data$r_factor
head(data)

biased_data = data[data$type == "biased",]
biased_long <- melt(biased_data, id.vars=c("racks", "cores"),
            measure.vars=c("ep_tput", "rtr_tput"),
            variable.name="tput_type", value.name="tput")

head(biased_long)

summary <- ddply(biased_long, c("racks", "cores", "tput_type"), summarise,
        mean_tput = mean(tput), tput_range = max(tput) - min(tput),
        max_deviation = max(abs(tput - mean_tput)))
summary

ggplot(summary, aes(x=cores, y=mean_tput, color=tput_type, shape=tput_type)) +
                geom_point() +
                coord_cartesian(ylim=c(0,400)) +
                labs(x="Number of cores", y="Maximum total throughput (Gbits/s)") +
                theme(legend.key = element_blank(), legend.position=c(0.8,0.2)) +
                scale_color_discrete(name="", labels=c("Endpoint throughput", "Total router throughput")) +
                scale_shape_discrete(name="", labels=c("Endpoint throughput", "Total router throughput"))

ggsave("emulator_throughput.pdf", width=6, height=3.5)
