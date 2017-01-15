#!/usr/bin/python

import os
import gi
import sys, getopt
gi.require_version('Gst', '1.0')
gi.require_version('Gtk', '3.0')
from gi.repository import Gst, GObject, Gtk

host = sys.argv[1]
pipeline = 'videotestsrc pattern=snow num-buffers=1800 ! video/x-raw, framerate=30/1, width=512, height=340 ! x264enc bitrate=512 ! tcpserversink host=' + host + ' port=8554'

GObject.threads_init()
Gst.init(None)

#pipeline = gst.Pipeline() 

pipeline = Gst.parse_launch(pipeline)
pipeline.set_state(Gst.State.PLAYING)

Gtk.main()

