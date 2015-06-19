import os
import socket
import sys

response_dict = {}

def generate_response(size):
    # generate random data for the response
    data = os.urandom(size)
    response_dict[size] = data

def run_server(addr, port):
    # set up a socket
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serversocket.bind((addr, port))
    serversocket.listen(1) # become a server socket, maximum 1 connections

    # accept a connection
    connection, address = serversocket.accept()

    print "connected to port %d" % port

    while True:
        buf = connection.recv(64)
        if len(buf) > 0:
            response_size = int(buf)

            if response_size not in response_dict:
                generate_response(response_size)
            connection.send(response_dict[response_size])

    serversocket.close()

def main():
    args = sys.argv
    if len(args) < 3:
        print "Usage: python %s addr port" % str(args[0])
        exit()
    addr = args[1]
    port = int(args[2])

    run_server(addr, port)

if __name__ == "__main__":
    main()
