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
set xlabel "Time (Seconds)"
set ylabel "Bottleneck queue length (KB)"
#plot backlogtcp using 1:($2/1024) with linespoints title 'TCP CUBIC', backlogim using 1:($2/1024) with linespoints title 'NA-TCP'
plot backlogim using 1:($2/1024) with linespoints title 'NA-TCP'

#Plot Fainess index
set output sprintf("fairness.png")
set xlabel "Time (Seconds)"
set ylabel "Fairness index"
plot "fairness.data" using 1:2 with linespoints title 'TCP CUBIC', "fairness-im.data" using 1:2 with linespoints title 'NA-TCP'
