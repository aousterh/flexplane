import argparse
import math
import os
import socket
import struct

response_dict = {}
DATA_PER_PACKET = 1448
CUSTOM_DATA_SIZE = 4

def generate_response(size, encode_pkts_remaining=False):
    if encode_pkts_remaining:
        # encode the number of remaining packets in the first 4 data bytes
        # of each packet
        num_pkts = math.ceil(float(size) / (DATA_PER_PACKET - CUSTOM_DATA_SIZE))
        data = ""
        remaining_bytes = size
        print "generating response, encoding pkts remaining"
        while num_pkts >= 0:
            # append more custom data and another packet's worth of random data
            random_data_size = min(DATA_PER_PACKET - CUSTOM_DATA_SIZE,
                                   remaining_bytes)
            data = (data + struct.pack('!L', num_pkts) +
                    os.urandom(random_data_size))

            remaining_bytes -= random_data_size
            num_pkts -= 1
    else:
        # generate random data for the response
        data = os.urandom(size)

    response_dict[size] = data

def setup_socket(params):
    # prepare a response before connecting if a size is specified. this is
    # helpful when responses are large and take a long time to generate
    if params.r_size is not None:
        generate_response(params.r_size)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((params.server_addr, params.server_port))

    # set TOS on socket if applicable. must be done before accepting a
    # connection.
    if params.tos != None:
        try:
            # lower two bits reserved for ECN
            server_socket.setsockopt(socket.IPPROTO_IP, socket.IP_TOS,
                                     params.tos << 2)
            print "set tos"
        except:
            print "WARNING: failed to set TOS correctly"

    # accept a connection
    server_socket.listen(1) # become a server socket, maximum 1 connections
    connection, address = server_socket.accept()

    return (server_socket, connection, address)

def run_server(params):
    print params

    server_socket, connection, address = setup_socket(params)

    print "connected to port %d" % params.server_port

    while True:
        buf = connection.recv(64)
        if len(buf) > 0:
            response_size = int(buf)

            if response_size not in response_dict:
                generate_response(response_size, params.pfabric)
            connection.send(response_dict[response_size])

    server_socket.close()

def get_args():
    parser = argparse.ArgumentParser(description="Run an RPC server")
    parser.add_argument('server_addr', metavar='s_addr', type=str,
                        help='the IP address or hostname of this server')
    parser.add_argument('server_port', metavar='s_port', type=int,
                        help='the port of this server')
    parser.add_argument('--r_size', metavar='r_size', type=int, required=False,
                        help='the size of the response (1448 is max for 1 MTU)')
    parser.add_argument('--tos', metavar='tos', type=int,
                        help='tos (Type of Service), now called DSCP')
    parser.add_argument('-pfabric', action="store_const", const=True,
                        required=False, default=False,
                        help='encode remaining flow size for pfabric')

    return parser.parse_args()

def main():
    args = get_args()

    run_server(args)

if __name__ == "__main__":
    main()
