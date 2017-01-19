#!/bin/sh

set print "-"
set terminal png
set autoscale x
set autoscale y
#Set legend
set key inside right top
set datafile missing "-nan"

backlogtcp='backlog-tcp.data'
backlogim='backlog-im.data'

#Plot queue backlog
set output sprintf("backlog.png")
set title "Queue Backlog vs. Time"
set xlabel "Time (Seconds)"
set ylabel "Queue Backlog (KB)"
plot backlogtcp using 1:($2/(8*1024)) with linespoints title 'Queue backlog for TCP CUBIC', backlogim using 1:($2/(8*1024)) with linespoints title 'Queue backlog for IM-TCP'
