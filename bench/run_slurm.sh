#!/bin/bash
#SBATCH --job-name=sim
#SBATCH --array=0-13

CONFIG=params/config${SLURM_ARRAY_TASK_ID}.json

python run_sim.py --file $CONFIG