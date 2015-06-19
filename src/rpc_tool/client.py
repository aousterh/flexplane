import random
import socket
import sys
import time

from stats import *

# max possible to fit in one 1500 byte MTU
response_size = 1448

receive_buffer_size = 1024 * 1024

def setup_socket(server_addr, server_port):
    # set up connections (one for now)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_addr, server_port))

    return client_socket

def run_client(server_addr, server_port):
    avail_socket = setup_socket(server_addr, server_port)
    stats = ConnectionStats(time.time())

    print ConnectionStats.header_names()

    while 1:

        current_socket = avail_socket

        t_before = time.time()
        current_socket.send(str(response_size))

        # wait for reply
        amount_received = 0
        while amount_received < response_size:
            data = current_socket.recv(receive_buffer_size)
            amount_received += len(data)
        t_after = time.time()
        fct = t_after - t_before

        # handle logging
        stats.add_sample(t_after, fct)
        stats.start_new_interval_if_time(t_after)

    for s in sockets:
        s.close()

def main():
    args = sys.argv
    if len(args) < 3:
        print "Usage: python %s server_addr server_port" % str(args[0])
        exit()
    server_addr = args[1]
    server_port = int(args[2])

    run_client(server_addr, server_port)

if __name__ == "__main__":
    main()
