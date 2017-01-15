#!/usr/bin/python

import os
import gi
import sys, getopt
gi.require_version('Gst', '1.0')
gi.require_version('Gtk', '3.0')
from gi.repository import Gst, GObject, Gtk

host = sys.argv[1]
#pipeline_str = 'videotestsrc pattern=snow num-buffers=1800 ! video/x-raw, framerate=30/1, width=512, height=340 ! x264enc bitrate=512 ! tcpserversink host=' + host + ' port=8554'
#pipeline = Gst.parse_launch(pipeline)

GObject.threads_init()
Gst.init(None)

pipeline = Gst.Pipeline() 

src = Gst.ElementFactory.make("videotestsrc","src")
src.set_property('pattern', 'snow')
src.set_property('num-buffers', 1800)

cfilter = Gst.ElementFactory.make("capsfilter", "cfilter")
caps = Gst.Caps.from_string("video/x-raw, framerate=30/1, width=512, height=340")
cfilter.set_property("caps", caps)

enc = Gst.ElementFactory.make("x264enc","enc")
enc.set_property('bitrate', 512)

svr = Gst.ElementFactory.make("tcpserversink","svr")
svr.set_property('host', host)
svr.set_property('port', 8554)

pipeline.add(src)
pipeline.add(cfilter)
pipeline.add(enc)
pipeline.add(svr)

src.link(cfilter)
cfilter.link(enc)
enc.link(svr)

if (not pipeline or not src or not cfilter or not enc or not svr):
    print('Not all elements could be created.')
    exit(-1)

#print "Hello"
pipeline.set_state(Gst.State.PLAYING)
Gtk.main()

