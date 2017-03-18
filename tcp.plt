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


bbrlog = ARG1
imlog = ARG2
cdglog = ARG3
outPre = ARG4

tcp1='BBR'
tcp2='NA-TCP'
tcp3='CDG'

#Plot Cwnd && SSTH
#set output sprintf("%s-cwnd.png",outPre)
#set xlabel "Time (Seconds)"
#set ylabel "Congestion window size"
#plot bbrlog using 1:7 title tcp1, imlog using 1:7 title tcp2, cdglog using 1:7 title tcp3
#plot imlog using 1:7 title tcp2, cdglog using 1:7 title tcp3


#Plot RTT
set output sprintf("%s-rtt.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "SRTT (milliseconds)"
set yrange [130:*]
#plot bbrlog using 1:($10/1000) title tcp1, imlog using 1:($10/1000) title tcp2, cdglog using 1:($10/1000) title tcp3
plot imlog using 1:($10/1000) title tcp2, cdglog using 1:($10/1000) title tcp3

#Plot throughput
set output sprintf("%s-throughput.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "Throughput (kbps)"
set autoscale y
#plot bbrlog using 1:($12 /1024) title tcp1, imlog using 1:($12 /1024) title tcp2, cdglog using 1:($12 /1024) title tcp3
plot imlog using 1:($12 * 8 /1024) title tcp2, cdglog using 1:($12 * 8 /1024) title tcp3

#Plot Jitter
#set yrange [0:500]
#set output sprintf("%s-jitter.png",outPre)
#set xlabel "Time (Seconds)"
#set ylabel "Jitter (milliseconds)"
#plot tcplog using 1:($13/1000) title tcp1, imlog using 1:($13/1000) title tcp2

#Plot Cwnd bytes
#set autoscale y
#set output sprintf("%s-cwnd-bytes.png",outPre)
#set xlabel "Time (Seconds)"
#set ylabel "Congestion window size (kb)"
#plot bbrlog using 1:14 title tcp1, imlog using 1:14 title tcp2, cdglog using 1:14 title tcp3
#plot imlog using 1:14 title tcp2, cdglog using 1:14 title tcp3


#Plot log(throughput/delay)
#set autoscale x
#set autoscale y
#set output sprintf("%s-power.png",outPre)
#set xlabel "Time (Seconds)"
#set ylabel "log(throughput/delay)"
#plot tcplog using 1:(log10(($12 * 8)/($10/1000))) title tcp1, imlog using 1:(log10(($12 * 8)/($10/1000))) title tcp2

