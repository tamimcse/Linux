gst-launch-1.0 videotestsrc pattern=snow num-buffers=500 ! video/x-raw, framerate=30/1, width=1024, height=680 ! x264enc bitrate=8192  tune="zerolatency" threads=1 ! tcpserversink host=$1 port=8554 &

