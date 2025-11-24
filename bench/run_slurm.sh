#!/bin/bash
#SBATCH --job-name=sim
#SBATCH --cpus-per-task=14
#SBATCH --mem=32G
#SBATCH --output=logs/out_%A.log

hostname

IMAGE=champsim:latest
BENCH=../bench/
TRACES=../traces/
BIN=../bin/
DPC4=../dpc4/

podman load -i ../champsim_latest.tar

podman run --rm \
    -v $BENCH:/ChampSim/bench \
    -v $TRACES:/ChampSim/traces \
    -v $BIN:/ChampSim/bin \
    -v $DPC4:/ChampSim/dpc4 \
    $IMAGE \
    /bin/bash -c "cd bench && python3 run_sim.py"