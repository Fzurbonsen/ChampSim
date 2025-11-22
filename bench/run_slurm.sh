#!/bin/bash
#SBATCH --job-name=sim
#SBATCH --array=0-13
#SBATCH --output=logs/out_%A_%a.log
#SBATCH --mem=8G

PWD_DIR=$(pwd)
IMAGE=champsim:latest
CHAMPSIM=$PWD_DIR/../

module load python/3.12

CONFIG=params/config_${SLURM_ARRAY_TASK_ID}.json

podman load -i $CHAMPSIM/champsim_latest.tar

podman run --rm \
    -v $CHAMPSIM:/ChampSim \
    $IMAGE \
    /bin/bash -c "cd bench && python3 run_sim.py --file $CONFIG"
