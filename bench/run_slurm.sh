#!/bin/bash
#SBATCH --job-name=sim
#SBATCH --array=0-13
#SBATCH --output=logs/out_%A_%a.log

PWD_DIR=$(pwd)
IMAGE=champsim:latest
CHAMPSIM=$PWD_DIR/../

module load python/3.12

CONFIG=params/config_${SLURM_ARRAY_TASK_ID}.json

podman run --rm \
    -v $CHAMPSIM:/ChampSim \
    $IMAGE \
    python3 run_sim.py --file $CONFIG
