#!/bin/sh

backlogtcp='backlog-tcp.data'
backlogim='backlog-im.data'

#Plot queue backlog
set output sprintf("backlog.png")
set title "Queue Backlog vs. Time"
set xlabel "Time (Seconds)"
set ylabel "Queue Backlog"
plot backlogtcp using 3:4 with linespoints title 'Queue backlog for TCP CUBIC', backlogim using 1:3 with linespoints title 'Queue backlog for IM-TCP'
