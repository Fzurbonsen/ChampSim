#
# build_csv.py
# This is a python script to parse the outputs from ChampSim and compile it into a csv file
# Author: Frederic zur Bonsen
# E-Mail: fzurbonsen@ethz.ch
#

import argparse, os, csv, re


# function to read an output.log file
def read_file(file):

    # read file as lines
    with open(file, "r") as f:
        lines = f.readlines()

    # parse all lines
    for line in lines:
        if "CPU 0 runs traces/" in line:
            match = re.search(r"CPU 0 runs traces/(.+)", line)
            if match:
                trace = match.group(1).strip()
        if "CPU 0 cumulative IPC:" in line:
            match = re.search(r"cumulative IPC:\s*([0-9.]+)", line)
            if match:
                IPC = float(match.group(1))
                break

    return trace, IPC


# function to read a direcotry of output.log files
def read_out_dir(path):
    log_files = []

    # find all file with .log
    for name in os.listdir(path):
        if name.endswith(".log"):
            log_files.append(name)

    traces = []
    IPCs = []

    for file in log_files:
        trace, IPC= read_file(path +  "/" + file)
        traces.append(trace)
        IPCs.append(IPC)

    return traces, IPCs


# function to write to CSV file
def write_to_csv(file, traces, IPCs):

    # open file for writing
    with open(file, "w", newline="") as f:
        writer = csv.writer(f)

        # write header
        writer.writerow(["trace", "IPC"])
        
        # write data
        for trace, IPC in zip(traces, IPCs):
            writer.writerow([trace, IPC])
    return
    

# main function
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--input", type=str, required=True,
                        help="Input directory to read from")
    parser.add_argument("--output", type=str, required=True,
                        help="Output file name [csv]")
    
    args = parser.parse_args()
    traces, IPCs = read_out_dir(args.input)
    write_to_csv(args.output, traces, IPCs)
    return 0

    


if __name__ == "__main__":
    main()