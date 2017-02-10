sudo python fair.py
sudo gnuplot -p -c tcp.plt h1.data h1-im.data h1
sudo gnuplot -p -c tcp.plt h3.data h3-im.data h3
sudo gnuplot -p -c tcp.plt h5.data h5-im.data h5
sudo gnuplot backlog.plt

