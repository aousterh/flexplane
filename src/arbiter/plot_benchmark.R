#!/usr/bin/env Rscript
# This script can be run using:
# R < ./plot_benchmark.R <data> --save

# use the ggplot2 library
library(ggplot2)

# read in data
args <- commandArgs()
data <- read.csv(args[2])

theme_set(theme_bw(base_size=12))

data$pps = data$tput * 1000 / (1500*8)
ggplot(data, aes(x=num_cores, y=tput, color=queue_type, shape=bulk)) +
             geom_point() +
             coord_cartesian(ylim=c(0,350)) +
             labs(x="Number of enqueue cores", y="Maximum throughput (Gbits/s)") +
             scale_color_discrete(name="", labels=c("1 queue per enqueue core", "1 queue"))

ggsave("benchmark_throughput.pdf", width=10, height=6)


enq_data <- read.csv("constant_tput_per_enq.csv")
enq_data$core_type = "enqueue"

deq_data <- read.csv("constant_tput_per_deq.csv")
deq_data$core_type = "dequeue"

core_data <- rbind(enq_data, deq_data)
ggplot(core_data, aes(x=num_cores,y=dequeue_waits/60, color=queue_type, shape=core_type)) +
                 geom_point() +
                 coord_cartesian(ylim=c(0,550000000)) +
                 labs(x="Number of enqueue cores", y="Dequeue waits per core per second") +
                 scale_color_discrete(name="Queue setup", labels=c("1 queue per enqueue core", "1 queue")) +
                 scale_shape_discrete(name="Core type")

ggsave("benchmark_dequeue_waits.pdf", width=10, height=6)