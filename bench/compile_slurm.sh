#!/bin/bash
#SBATCH --job-name=compile
#SBATCH --output=logs/out_compile.log
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G

cd ..
CHAMPSIM=$(pwd)
cd bench

IMAGE=champsim:latest
BENCH=$CHAMPSIM/bench
TRACES=$CHAMPSIM/traces
BIN=$CHAMPSIM/bin
DPC4=$CHAMPSIM/dpc4

echo $CHAMPSIM
echo $BENCH
echo $TRACES
echo $BIN
echo $DPC4

module load python/3.12

CONFIG=params/config_0.json

podman load -i $CHAMPSIM/champsim_latest.tar

podman run --rm \
    -v $BENCH:/ChampSim/bench \
    -v $TRACES:/ChampSim/traces \
    -v $BIN:/ChampSim/bin \
    -v $DPC4:/ChampSim/dpc4 \
    $IMAGE \
    /bin/bash -c "cd bench && python3 compile_sim.py --file $CONFIG"
