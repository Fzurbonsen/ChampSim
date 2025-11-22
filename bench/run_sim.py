#
#   run_sim.py
#   This is a python script to run ChampSim with paramters from a parameters file.
#   Author: Frederic zur Bonsen
#   E-Mail: fzurbonsen@ethz.ch
#

import argparse, json, os, shutil, subprocess


# function to read the input parameters form the input file
def read_input_parameters(file):
    with open(file, "r") as f:
        config = json.load(f)
    return config


# function to run the sim
def run_sim(config):
    trace = str(config["files"]["trace"])
    out = str(config["files"]["out"])
    prefetcher = str(config["simulation"]["prefetcher"])
    warmup_instr = int(config["simulation"]["warmup-instructions"])
    sim_instr = int(config["simulation"]["simulation-instructions"])

    parent_dir = "../"
    binary = f"./bin/1C.fullBW.{prefetcher}pref"
    warmup = f"--warmup-instructions={warmup_instr}"
    simulation = f"--simulation-instructions={sim_instr}"
    trace_path = f"traces/{trace}"
    result = subprocess.run(
        [binary, warmup, simulation, trace_path],
        cwd=parent_dir,
        capture_output=True,
        text=True
    )
    stdout = result.stdout
    stderr = result.stderr

    # check if the simulation has thrown an error
    if stderr:
        print("---STDERR---")
        print(stderr)
        exit(1)
    
    with open(out, "w") as f:
        f.write(stdout)
    return


# main function
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--file", type=str, required=True,
                        help="Paramater file to run the simulation [.config]")

    args = parser.parse_args()

    config = read_input_parameters(args.file)
    run_sim(config)
    return 0


if __name__ == "__main__":
    main()
