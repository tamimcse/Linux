from subprocess import call

with open("routes.1.txt") as f:
    for line in f:
        # Do something with 'line'
	arr = line.replace("\r\n", "").split("\t")
    	print arr
	call('sudo ip route del {0}'.format(arr[0]), shell=True)
