import argparse
import random
import socket
import time

from stats import *

receive_buffer_size = 1024 * 1024

def setup_socket(server_addr, server_port):
    # set up connections (one for now)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_addr, server_port))

    return client_socket

def run_client(params):
    avail_socket = setup_socket(params.server_addr, params.server_port)
    stats = ConnectionStats(time.time())

    print params

    print ConnectionStats.header_names()

    current_time = time.time()

    # Poisson distribution with a mean time between requests of
    # 1 / params.qps
    next_send_time = current_time + random.expovariate(params.qps)

    while 1:
        too_late = 0
        if current_time > next_send_time:
            too_late = 1
        else:
            current_time = time.time()
            if current_time > next_send_time:
                too_late = 1
            else:
                # early! spin until next send time
                while current_time < next_send_time:
                    current_time = time.time()

        # update next sent time
        next_send_time += random.expovariate(params.qps)

        current_socket = avail_socket

        t_before = time.time()
        current_socket.send(str(params.response_size))

        # wait for reply
        amount_received = 0
        while amount_received < params.response_size:
            data = current_socket.recv(receive_buffer_size)
            amount_received += len(data)
        t_after = time.time()
        fct = t_after - t_before

        # handle logging
        stats.add_sample(t_after, fct, too_late)
        stats.start_new_interval_if_time(t_after)

    for s in sockets:
        s.close()

def get_args():
    parser = argparse.ArgumentParser(description="Run an RPC client")
    parser.add_argument('server_addr', metavar='s_addr', type=str,
                        help='the IP address or hostname of the server')
    parser.add_argument('server_port', metavar='s_port', type=int,
                        help='the port of the server')
    parser.add_argument('qps', metavar='qps', type=int,
                        help='the target queries per second')
    parser.add_argument('response_size', metavar='r_size', type=int,
                        help='the size of the response (1448 is max for 1 MTU)')

    return parser.parse_args()

def main():
    args = get_args()

    run_client(args)

if __name__ == "__main__":
    main()
