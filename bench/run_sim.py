#
# run_sim.py
# Script to run the bench locally
# Author: Frederic zur Bonsen
# E-Mail: fzurbonsen@ethz.ch
#

import argparse, os, shutil, subprocess, multiprocessing, re, csv, sys, time

# config
host_path = "../"
config_path = "dpc4/"
trace_path = "traces/"
bin_path = "bin/"
traces = [
    "GAP/bc-0.trace.gz",
    "GAP/bc-12.trace.gz",
    "GAP/bfs-10.trace.gz",
    "GAP/bfs-14.trace.gz",
    "GAP/cc-13.trace.gz",
    "GAP/cc-14.trace.gz",
    "GAP/cc-5.trace.gz",
    "GAP/sssp-10.trace.gz",
    "GAP/sssp-14.trace.gz",
    "charlie/charlie_0000.champsim.gz",
    "charlie/charlie_0001.champsim.gz",
    "charlie/charlie_0002.champsim.gz",
    "charlie/charlie_0003.champsim.gz",
    "charlie/charlie_0004.champsim.gz"
]
out_path = "out/" # this is not relative to the host_path but relative to the script location
csv_path = "csv/" # this is not relative to the host_path but relative to the script location
csv_file = "test.csv"
warmup_instr = int(1e4) # should be 1e7
sim_instr = int(5e4) # should be 5e7
config_core = "1C"
config_bandwidth = "fullBW"
config_prefetcher = "ghb_stride"
# config_prefetcher = "no"

# global ui state
ui_state = {}

# lock
print_lock = multiprocessing.Lock()


# init UI
def ui_init():
    global ui_state
    global traces

    for trace in traces:
        ui_state[trace] = "none"

# ui utility
def clear_screen():
    sys.stdout.write("\033[2J\033[H")
    sys.stdout.flush()

def move_cursor_top():
    sys.stdout.write("\033[H")
    sys.stdout.flush()

# print ui
def ui_print():
    global ui_state
    clear_screen()
    move_cursor_top()

    print("=== Simulation Status ===")
    print("Trace                                 | Status")
    print("---------------------------------------+--------------------")

    for trace in traces:
        print(f"{trace:40} | {ui_state[trace]}")
    return


# function to compile the simulation
def compile_sim():
    global host_path
    global config_path
    global config_prefetcher
    global config_core
    global config_bandwidth
    global bin_path

    config_file = f"{config_core}.{config_bandwidth}.{config_prefetcher}pref.json"
    bin_file = f"{config_core}.{config_bandwidth}.{config_prefetcher}pref"

    # check if the config file exists
    if not os.path.exists(host_path + config_path + config_file):
        print("Error: config file does not exist!")
        print(host_path + config_path + config_file)
        exit(1)
    
    # chech if the binary already exists
    if os.path.exists(host_path + bin_path + bin_file):
        print("Binary already exists!")
        return bin_file

    # compile the simulation
    subprocess.run(["./config.sh", config_path + config_file], check=True, cwd=host_path)
    subprocess.run(["make", "-j"], check=True, cwd=host_path)
    return bin_file


# function to parse the sim output to return the simulation stats
def parse_stdout(stdout):
    stats = {}

    # IPC:
    ipc_match = re.search(
        r"CPU 0 cumulative IPC: ([\d\.]+) instructions: (\d+) cycles: (\d+)", stdout
    )
    if ipc_match:
        stats["ipc"] = float(ipc_match.group(1))
        stats["instructions"] = int(ipc_match.group(2))
        stats["cycles"] = int(ipc_match.group(3))

    return stats



# function to run the simulation
def run_sim(trace, bin_file):
    global host_path
    global bin_path
    global trace_path
    global out_path
    global warmup_instr
    global sim_instr
    global ui_state
    
    # build arguments
    args = [
        bin_path + bin_file,
        f"--warmup-instructions={warmup_instr}",
        f"--simulation-instructions={sim_instr}",
        trace_path + trace
    ]

    # run simulation
    print(f"Running: {' '.join(args)}")
    # with print_lock:
    #     ui_state[trace] = "Running"
    #     ui_print()

    result = subprocess.run(
        args,
        cwd=host_path,
        capture_output=True,
        text=True,
        timeout=60 * 60
    )

    print(f"Terminated: {' '.join(args)}")
    # with print_lock:
    #     ui_state[trace] = "Terminated"
    #     ui_print()

    # retrieve relevant stats
    stats = parse_stdout(result.stdout)

    return {
        "trace": trace,
        "args": args,
        "stdout": result.stdout,
        "stderr": result.stderr,
        "stats": stats
    }
    

# function to run all traces in parallel
def threaded_run(traces, bin_file):

    # prepare tasks
    tasks = [(trace, bin_file) for trace in traces]

    # let python figure out how many threads to use
    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        results = pool.starmap(run_sim, tasks)

    return results


# function to write to .csv file
def write_to_csv(results):
    global csv_path
    global csv_file

    file = csv_path + csv_file

    with open(file, "w", newline="") as f:
        writer = csv.writer(f)

        # set header
        writer.writerow(["trace", "IPC", "instructions", "cycles"])

        # iterate over all results
        for result in results:
            stats = result["stats"]
            # build row
            row = [
                result["trace"],
                stats["ipc"],
                stats["instructions"],
                stats["cycles"]
            ]
            # write the row into the file
            writer.writerow(row)
    return


# main function
def main():
    global traces
    bin_file = compile_sim()

    # init ui
    ui_init()

    results = threaded_run(traces, bin_file)

    for r in results:
        print(f"Finished: {r['trace']}")
        # print("---STDOUT---")
        # print(f"{r['stdout']}")
        # print("---STDERR---")
        # print(f"{r['stderr']}")
        print(f"{r['stats']}")
    
    write_to_csv(results)
    return


if __name__ == "__main__":
    main()