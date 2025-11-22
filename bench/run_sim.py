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


# function to check if the dpc4 config file exists
def build_config_sim(config):
    prefetcher = str(config["simulation"]["prefetcher"])

    path = f"../dpc4/1C.fullBW.{prefetcher}pref.json"

    # check if the config file exists
    if os.path.exists(path):
        return
    
    # build the config file
    shutil.copy("../dpc4/1C.fullBW.nopref.json", path)

    with open(path, "r") as f:
        sim_config = json.load(f)
    
    sim_config["executable_name"] = f"1C.fullBW.{prefetcher}pref"
    sim_config["L2C"]["prefetcher"] = prefetcher

    with open(path, "w") as f:
        json.dump(sim_config, f, indent=2)
    return


# function to run the simulation with the desired config
def compile_sim(config):
    prefetcher = str(config["simulation"]["prefetcher"])

    # check if the binary file exists
    if os.path.exists(f"../bin/1C.fullBW.{prefetcher}pref"):
        return

    # compile the binary
    parent_dir = "../"
    config_file = f"dpc4/1C.fullBW.{prefetcher}pref.json"
    subprocess.run(["./config.sh", config_file], check=True, cwd=parent_dir)
    subprocess.run(["make", "-j4"], check=True, cwd=parent_dir)
    return


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
    build_config_sim(config)
    compile_sim(config)
    run_sim(config)
    return 0


if __name__ == "__main__":
    main()
