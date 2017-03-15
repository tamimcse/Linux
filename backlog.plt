#!/bin/sh

set print "-"
set terminal png
set autoscale x
set autoscale y
set size ratio .5
#Set legend
set key inside right top
set datafile missing "-nan"
set style data line
#set style data linespoints

backlogbbr='backlog-bbr.data'
backlogim='backlog-im.data'
backlogcdg='backlog-cdg.data'

tcp1='BBR'
tcp2='NA-TCP'
tcp3='CDG'

#Plot queue backlog
#set yrange [0:70]
set output sprintf("backlog.png")
set xlabel "Time (Seconds)"
set ylabel "Queuing delay (msec)"
#plot backlogbbr using 1:($2/1024) title tcp1, backlogim using 1:($2/1024) title tcp2, backlogcdg using 1:($2/1024) title tcp3
plot backlogim using 1:($2*8/(1024*1024)) title tcp2, backlogcdg using 1:($2*8/(1024*1024)) title tcp3

#Plot Fainess index
set autoscale y
set key inside right bottom
set output sprintf("fairness.png")
set xlabel "Time (Seconds)"
set ylabel "Fairness index"
#plot "fairness-bbr.data" using 1:2 title tcp1, "fairness-im.data" using 1:2 title tcp2, "fairness-cdg.data" using 1:2 title tcp3
plot "fairness-im.data" using 1:2 title tcp2, "fairness-cdg.data" using 1:2 title tcp3

