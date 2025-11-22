#!/bin/bash
#SBATCH --job-name=sim
#SBATCH --array=0-13
#SBATCH --output=logs/out_%A_%a.log
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G

PWD_DIR=$(pwd)
IMAGE=champsim:latest
CHAMPSIM=$PWD_DIR/../
BENCH=$CHAMPSIM/bench
TRACES=$CHAMPSIM/traces
BIN=$CHAMPSIM/bin
DPC4=$CHAMPSIM/dpc4

echo $PWD_DIR
echo $CHAMPSIM
echo $BENCH
echo $TRACES
echo $BIN
echo $DPC4

module load python/3.12

CONFIG=params/config_${SLURM_ARRAY_TASK_ID}.json

podman load -i $CHAMPSIM/champsim_latest.tar

podman run --rm \
    -v $BENCH:/ChampSim/bench \
    -v $TRACES:/ChampSim/traces \
    -v $BIN:/ChampSim/bin \
    -v $DPC4:/ChampSim/dpc4 \
    $IMAGE \
    /bin/bash -c "cd bench && python3 run_sim.py --file $CONFIG"
