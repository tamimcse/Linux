gst-launch-1.0  tcpclientsrc host=$1 port=8554 ! h264parse ! avdec_h264 ! xvimagesink &
