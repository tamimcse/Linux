sudo python fair.py
sudo gnuplot -p -c tcp.plt h1-bbr.data h1-im.data h1-cdg.data h1
sudo gnuplot -p -c tcp.plt h3-bbr.data h3-im.data h3-cdg.data h3
sudo gnuplot -p -c tcp.plt h5-bbr.data h5-im.data h5-cdg.data h5
sudo gnuplot backlog.plt

