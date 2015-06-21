
# seconds per logging interval
LOGGING_INTERVAL = 1

class ConnectionStats():
    """A class for recording stats (FCT, qps, etc.) about a connection
    over an interval of time"""
    def __init__(self, start_time):
        self.start_time = start_time
        self.end_time = start_time + LOGGING_INTERVAL
        self.queries = 0
        self.FCTs = [] # in microseconds

    def add_sample(self, time_now, fct):
        self.queries += 1
        self.FCTs.append(fct * 1000 * 1000)

    def start_new_interval_if_time(self, time_now):
        if time_now < self.end_time:
            return

        # print previous interval
        duration = time_now - self.start_time
        qps = self.queries / duration
        mean_fct = sum(self.FCTs) / len(self.FCTs)
        min_fct = min(self.FCTs)
        max_fct = max(self.FCTs)

        fields = [self.start_time, time_now, duration, qps, mean_fct, min_fct,
                  max_fct]
        fields_str = [str(x) for x in fields]
        print ",".join(fields_str)

        # start new interval
        self.start_time = time_now
        self.end_time += LOGGING_INTERVAL
        self.queries = 0
        self.FCTs = []

    @staticmethod
    def header_names():
        return "start_time,end_time,interval_duration,qps,mean_fct,min_fct,max_fct"
