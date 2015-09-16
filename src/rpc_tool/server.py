import argparse
import math
import os
import socket
import struct
import sys
import time

from empirical import *

response_dict = {}
DATA_PER_PACKET = 1448
CUSTOM_DATA_SIZE = 4

def generate_response(size, encode_pkts_remaining=False):
    if encode_pkts_remaining:
        # encode the number of remaining packets in the first 4 data bytes
        # of each packet
        num_pkts = int(math.ceil(float(size) / DATA_PER_PACKET))

        pkts = []
        remaining_bytes = size
        print "generating response, encoding pkts remaining"
        while num_pkts > 0:
            # append more custom data and another packet's worth of data
            data_size = min(DATA_PER_PACKET, remaining_bytes)

            # make packet data all the custom data, in case packets aren't
            # perfectly segmented into the expected sizes
            for i in range(data_size / CUSTOM_DATA_SIZE):
                pkts.append(struct.pack('!L', num_pkts))

            remaining_bytes -= data_size
            num_pkts -= 1

        data = ''.join(pkts)
    else:
        # generate random data for the response
        data = os.urandom(size)
    print 'done generating response of size %d' % size
    response_dict[size] = data

def setup_socket(params):
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

    # prepare responses before connecting if a size is specified. this is
    # helpful when responses are large and take a long time to generate
    if params.r_size is not None:
        generate_response(params.r_size, params.pfabric)
    elif params.ecdf is not None:
        empir_rand = EmpiricalRandomVariable(params.ecdf)
        for response_size in empir_rand.get_all_values():
            generate_response(response_size * DATA_PER_PACKET, params.pfabric)

    server_socket, connection, address = setup_socket(params)

    print "connected to port %d at time %f" % (params.server_port, time.time())
    sys.stdout.flush()

    try:
        num_responses = 0
        while True:
            buf = connection.recv(8)
            if len(buf) > 0:
                data = struct.unpack('!LL', buf)
                response_size = int(data[1])

                if response_size not in response_dict:
                    generate_response(response_size, params.pfabric)

                response = response_dict[response_size]
                response_length = len(response_dict[response_size])
                sent_bytes = 0

                while sent_bytes < response_length:
                    sent_bytes += connection.send(response[sent_bytes:])

                num_responses += 1

    except socket.error:
        print "socket error at time %f" % time.time()
        sys.stdout.flush()
        raise

    server_socket.close()

def get_args():
    parser = argparse.ArgumentParser(description="Run an RPC server")
    parser.add_argument('server_addr', metavar='s_addr', type=str,
                        help='the IP address or hostname of this server')
    parser.add_argument('server_port', metavar='s_port', type=int,
                        help='the port of this server')
    parser.add_argument('--r_size', metavar='response_size', type=int,
                        required=False,
                        help='the size of the response (1448 is max for 1 MTU)')
    parser.add_argument('--tos', metavar='tos', type=int,
                        help='tos (Type of Service), now called DSCP')
    parser.add_argument('-pfabric', action="store_const", const=True,
                        required=False, default=False,
                        help='encode remaining flow size for pfabric')
    parser.add_argument('--ecdf', metavar='ecdf_file_name', type=str,
                        help='choose response sizes from an empirical CDF from file')

    return parser.parse_args()

def main():
    args = get_args()

    run_server(args)

if __name__ == "__main__":
    main()
