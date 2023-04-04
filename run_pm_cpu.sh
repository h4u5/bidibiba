#!/bin/bash
#SBATCH --nodes=2048
#SBATCH -C cpu
#SBATCH -q regular
#SBATCH -A nstaff
#SBATCH -J bidibiba-cpu
#SBATCH --mail-user=tgroves@lbl.gov
#SBATCH --mail-type=ALL
#SBATCH -t 00:15:00

#OpenMP settings:
export OMP_NUM_THREADS=1
export OMP_PLACES=threads
export OMP_PROC_BIND=spread


#run the application:
srun --ntasks=2048 --ntasks-per-node=1 --cpu-bind=map_cpu:0 --cpu-freq=high /global/cfs/cdirs/m888/tgroves/network_benchmarking/bidibiba/bidibiba_ss
