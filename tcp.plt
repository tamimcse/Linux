#!/bin/sh

set print "-"
set terminal png
set autoscale x
set autoscale y
#Set legend
set key inside right top
set datafile missing "-nan"

tcplog = ARG1
imlog = ARG2
outPre = ARG3

#Plot Cwnd && SSTH
set output sprintf("%s-cwnd.png",outPre)
set title "Congestion Window vs. Time"
set xlabel "Time (Seconds)"
set ylabel "Congestion window"
plot tcplog using 1:7 with linespoints title 'TCP CUBIC Cwnd', imlog using 1:7 with linespoints title 'IM-TCP Cwnd'
#plot tcplog using 1:7 with linespoints title 'TCP CUBIC Cwnd'
#plot imlog using 1:7 with linespoints title 'IM-TCP Cwnd'

#Plot RTT
set output sprintf("%s-rtt.png",outPre)
set title "RTT vs. Time"
set xlabel "Time (Seconds)"
set ylabel "SRTT (milliseconds)"
plot tcplog using 1:($10/1000) with linespoints title 'TCP CUBIC RTT', imlog using 1:($10/1000) with linespoints title 'IM-TCP RTT'
#plot tcplog using 1:($10/1000) with linespoints title 'TCP CUBIC RTT'
#plot imlog using 1:($10/1000) with linespoints title 'IM-TCP RTT'


#Plot rate delivered
set output sprintf("%s-rate.png",outPre)
set title "Sending Rate vs. Time"
set xlabel "Time (Seconds)"
set ylabel "Sending Rate"
plot tcplog using 1:12 with linespoints title 'TCP CUBIC Rate', imlog using 1:12 with linespoints title 'IM-TCP Rate'
#plot tcplog using 1:12 with linespoints title 'TCP CUBIC Rate'
#plot imlog using 1:12 with linespoints title 'IM-TCP Rate'
