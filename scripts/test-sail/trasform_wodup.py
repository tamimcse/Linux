#This scripts reads actual-routes.txt and converts all the next-hops with veth0-31 
from subprocess import call

out = open("demofile1.txt", "a")

ip_list = []
with open("SAIL_FIB.txt") as f:
    for line in f:
	arr = line.replace("\r\n", "").split("\t")
	ip = arr[0].split("/")[0]
	if ip not in ip_list :
	    ip_list.append(ip)	
	    fib_index = int(arr[1]) % 32
	    out.write("{0}\tveth{1}\r\n".format(arr[0], fib_index))

