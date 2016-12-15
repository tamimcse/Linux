#!/bin/sh

set print "-"
set terminal png
set autoscale x
set autoscale y
#Set legend
set key inside right top
set datafile missing "-nan"

logfile = ARG1
outPre = ARG2

#Plot Cwnd && SSTH
set output sprintf("%s-cwnd.png",outPre)
set title "Congestion Window vs. Time"
set xlabel "Time (Seconds)"
set ylabel "Congestion window"
plot logfile using 1:7 with linespoints title 'tcp cwnd'
#, logfile using 1:8 with linespoints title 'tcp ssth'

#Plot RTT
set output sprintf("%s-rtt.png",outPre)
set title "RTT vs. Time"
set xlabel "Time (Seconds)"
set ylabel "RTT"
plot logfile using 1:($10/1000) with linespoints title 'tcp rtt'


