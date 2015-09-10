import random
import sys

# Gets random variable samples from an empirical CDF, read in from a file.

class EmpiricalRandomVariable():
    def __init__(self, filename):
        # read in data from file
        data_in = file(filename)
        lines = data_in.read().split('\n')

        # store cdf in an easy-to-parse way
        self.cum_probs = []
        self.values = []
        for l in lines:
            fields = l.split(' ')
            if len(fields) < 3:
                continue

            self.cum_probs.append(float(fields[2]))
            self.values.append(int(fields[0]))

        print self.cum_probs
        print self.values

    def get_value(self):
        r = random.random()
        
        for i in range(len(self.cum_probs)):
            if r <= self.cum_probs[i]:
                return self.values[i]

def main():
    args = sys.argv

    if len(args) < 2:
        print 'Usage: python %s cdf_file_name' % str(args[0])
        exit()

    in_filename = args[1]
    
    rand_var = EmpiricalRandomVariable(in_filename)

    # Test that it obeys the distribution (assuming the CDF_search.txt input
    # file)
    bucket_vals = [6, 13, 19, 33, 53, 133, 667, 1333, 3333, 6667, 20000]
    bucket_counts = [0 for x in range(11)]
    for i in range(100000):
        v = rand_var.get_value()
        for j in range(11):
            if bucket_vals[j] == v:
                bucket_counts[j] += 1
                break

    bucket_counts = [(x * 0.00001) for x in bucket_counts]
    print bucket_counts

if __name__ == "__main__":
    main()
