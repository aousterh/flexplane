import argparse
import os
import socket

response_dict = {}

def generate_response(size):
    # generate random data for the response
    data = os.urandom(size)
    response_dict[size] = data

def run_server(params):
    # prepare a response before connecting if a size is specified. this is
    # helpful when responses are large and take a long time to generate
    if params.r_size is not None:
        generate_response(params.r_size)

    # set up a socket
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serversocket.bind((params.server_addr, params.server_port))
    serversocket.listen(1) # become a server socket, maximum 1 connections

    # accept a connection
    connection, address = serversocket.accept()

    print "connected to port %d" % params.server_port

    while True:
        buf = connection.recv(64)
        if len(buf) > 0:
            response_size = int(buf)

            if response_size not in response_dict:
                generate_response(response_size)
            connection.send(response_dict[response_size])

    serversocket.close()

def get_args():
    parser = argparse.ArgumentParser(description="Run an RPC server")
    parser.add_argument('server_addr', metavar='s_addr', type=str,
                        help='the IP address or hostname of this server')
    parser.add_argument('server_port', metavar='s_port', type=int,
                        help='the port of this server')
    parser.add_argument('--r_size', metavar='r_size', type=int, required=False,
                        help='the size of the response (1448 is max for 1 MTU)')

    return parser.parse_args()

def main():
    args = get_args()

    run_server(args)

if __name__ == "__main__":
    main()
