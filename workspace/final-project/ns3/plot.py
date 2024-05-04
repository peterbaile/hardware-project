import argparse
import matplotlib.pyplot as plt
import numpy as np
from mlccplot import plot, parser
from mlccplot.defs import Flow
import os

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-f", "--file", required=True, type=str, help="File containing simulation dump.")
arg_parser.add_argument("-o", "--out", required=True, type=str, help="Output file for the graph.")
arg_parser.add_argument("-i", "--interval", required=False, type=float, default=0.05, help="Interval to calculate throughput.")

args = arg_parser.parse_args()

# TODO: pass in the flows into hte object, and globally assign them colors?

if __name__ == "__main__":
    f = open(args.file, "r")
    parser_obj = parser.Parser()
    events = parser_obj.parse_output(f)
    last_time = max([event.time for event in events])
    plotter = plot.Plotter(args.out, args.interval, last_time)
    f.close()
    
    if not os.path.exists(args.out):
        os.mkdir(args.out)
    
    flows = [Flow("10.1.1.1", "9", "10.1.4.1", "9"), Flow("10.1.2.1", "9", "10.1.5.1", "9")]
    #flows = [Flow("10.0.0.1", "9", "10.0.1.1", "9"), Flow("10.0.1.1", "9", "10.0.2.1", "9"), Flow("10.0.2.1", "9", "10.0.3.1", "9"), 
        #Flow("10.0.3.1", "9", "10.0.4.1", "9"), Flow("10.0.4.1", "9", "10.0.0.1", "9")]
    #flows.extend([Flow("10.0.7.1", "9", "10.0.8.1", "9"), Flow("10.0.8.1", "9", "10.0.9.1", "9"), Flow("10.0.9.1", "9", "10.0.10.1", "9"), Flow("10.0.10.1", "9", "10.0.11.1", "9"), Flow("10.0.11.1", "9", "10.0.7.1", "9")])
    #flows = [flows[4]]
    plotter.plot_queue_length(events)
    plotter.plot_throughput(events, flows) 
    plotter.plot_cwnd(events, flows)
    plotter.plot_alpha(events, flows)
    plotter.plot_data_sent(events, flows)
    plotter.plot_link_throughput(events, flows, "0")
    plotter.plot_rtt(events, flows)
