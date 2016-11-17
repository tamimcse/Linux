import socket

clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
clientsocket.connect(('172.16.102.1', 8089))
clientsocket.send('hello')
print clientsocket.recv(1024)

