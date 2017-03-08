#!/bin/sh

set print "-"
set terminal png
set autoscale x
set autoscale y
#Set legend
set key inside right top
set datafile missing "-nan"
set style data line
#set style data linespoints

backlogtcp='backlog-tcp.data'
backlogim='backlog-im.data'

tcp1='BBR'
tcp2='NA-TCP'

#Plot queue backlog
#set yrange [0:70]
set output sprintf("backlog.png")
set xlabel "Time (Seconds)"
set ylabel "Bottleneck queue length (KB)"
plot backlogtcp using 1:($2/1024) title tcp1, backlogim using 1:($2/1024) title tcp2
#plot backlogim using 1:($2/1024) title 'NA-TCP'

#Plot Fainess index
set autoscale y
set key inside right bottom
set output sprintf("fairness.png")
set xlabel "Time (Seconds)"
set ylabel "Fairness index"
plot "fairness.data" using 1:2 title tcp1, "fairness-im.data" using 1:2 title tcp2
