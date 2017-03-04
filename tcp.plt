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
set xlabel "Time (Seconds)"
set ylabel "Congestion window size"
plot tcplog using 1:7 with linespoints title 'TCP CUBIC', imlog using 1:7 with linespoints title 'NA-TCP'
#plot tcplog using 1:7 with linespoints title 'TCP CUBIC Cwnd'
#plot imlog using 1:7 with linespoints title 'IM-TCP Cwnd'

#Plot RTT
set output sprintf("%s-rtt.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "SRTT (milliseconds)"
plot tcplog using 1:($10/1000) with linespoints title 'TCP CUBIC', imlog using 1:($10/1000) with linespoints title 'NA-TCP RTT'
#plot tcplog using 1:($10/1000) with linespoints title 'TCP CUBIC RTT'
#plot imlog using 1:($10/1000) with linespoints title 'IM-TCP RTT'


#Plot throughput
set output sprintf("%s-throughput.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "Throughput (KB/s)"
plot tcplog using 1:($12 /1024) with linespoints title 'TCP CUBIC', imlog using 1:($12 /1024) with linespoints title 'NA-TCP'
#plot tcplog using 1:($12 * 8 /1024) with linespoints title 'TCP CUBIC Rate'
#plot imlog using 1:($12 * 8 /1024) with linespoints title 'IM-TCP Rate'

#Plot Jitter
set yrange [0:500]
set output sprintf("%s-jitter.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "Jitter (milliseconds)"
plot tcplog using 1:($13/1000) with linespoints title 'TCP CUBIC', imlog using 1:($13/1000) with linespoints title 'NA-TCP'
#plot tcplog using 1:($13/1000) with linespoints title 'TCP CUBIC Jitter'
#plot imlog using 1:($13/1000) with linespoints title 'IM-TCP Jitter'

#Plot Cwnd bytes
set autoscale y
set output sprintf("%s-cwnd-bytes.png",outPre)
set xlabel "Time (Seconds)"
set ylabel "Congestion window size (kb)"
#plot tcplog using 1:14 with linespoints title 'TCP CUBIC', imlog using 1:14 with linespoints title 'NA-TCP'
plot imlog using 1:($14*8/1024) with linespoints title 'NA-TCP'

#Plot log(throughput/delay)
#set autoscale x
#set autoscale y
#set output sprintf("%s-power.png",outPre)
#set xlabel "Time (Seconds)"
#set ylabel "log(throughput/delay)"
#plot tcplog using 1:(log10(($12 * 8)/($10/1000))) with linespoints title 'TCP CUBIC', imlog using 1:(log10(($12 * 8)/($10/1000))) with linespoints title 'IM-TCP'
