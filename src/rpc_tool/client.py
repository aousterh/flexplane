import argparse
import random
import socket
import struct
import sys
import time

from empirical import *
from stats import *

HIGHEST_PRIORITY = 0
receive_buffer_size = 1024 * 1024

def setup_socket(params):
    # set up connections (one for now)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # repeatedly try to connect until it succeeds
    while 1:
        try:
            client_socket.connect((params.server_addr, params.server_port))
            break
        except socket.error:
            pass

    # set TOS on socket if applicable
    if params.tos != None:
        try:
            # lower two bits reserved for ECN
            client_socket.setsockopt(socket.IPPROTO_IP, socket.IP_TOS,
                                     params.tos << 2)
        except:
            "WARNING: failed to set TOS correctly"

    return client_socket

def run_client(params):
    print params

    # read in cdf values from file if necessary
    if params.ecdf is not None:
        empir_rand = EmpiricalRandomVariable(params.ecdf)

    client_socket = setup_socket(params)

    stats = ConnectionStats(time.time())
    print ConnectionStats.header_names()

    current_time = time.time()

    # Poisson distribution with a mean time between requests of
    # 1 / params.qps
    next_send_time = current_time + random.expovariate(params.qps)

    num_requests = 0
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

        # choose response size
        if params.ecdf:
            # value is in MTUs - we want bytes
            response_size = empir_rand.get_value() * 1448
        else:
            response_size = params.r_size

        t_before = time.time()

        # requests are only one packet long, so they get highest priority
        sys.stderr.write("about to send request %d\n" % num_requests)
        client_socket.send(struct.pack('!LL', HIGHEST_PRIORITY, response_size))
        sys.stderr.write("sent request %d\n" % num_requests)

        # wait for reply
        amount_received = 0
        while amount_received < response_size:
            data = client_socket.recv(receive_buffer_size)
            amount_received += len(data)
        sys.stderr.write("received response %d\n" % num_requests)
        num_requests += 1
        t_after = time.time()
        fct = t_after - t_before

        # handle logging
        stats.add_sample(t_after, fct, too_late)
        stats.start_new_interval_if_time(t_after)

    client_socket.close()

def get_args():
    parser = argparse.ArgumentParser(description="Run an RPC client")
    parser.add_argument('server_addr', metavar='s_addr', type=str,
                        help='the IP address or hostname of the server')
    parser.add_argument('server_port', metavar='s_port', type=int,
                        help='the port of the server')
    parser.add_argument('qps', metavar='qps', type=float,
                        help='the target queries per second')
    parser.add_argument('--r_size', metavar='response_size', type=int,
                        help='the size of the response (1448 is max for 1 MTU)',
                        required=False)
    parser.add_argument('--tos', metavar='tos', type=int,
                        help='tos (Type of Service), now called DSCP')
    parser.add_argument('--ecdf', metavar='ecdf_file_name', type=str,
                        help='choose response sizes from an empirical CDF from file')

    return parser.parse_args()

def main():
    args = get_args()

    run_client(args)

if __name__ == "__main__":
    main()
