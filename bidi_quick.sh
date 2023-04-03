#!/bin/bash
#SBATCH -N 192
#SBATCH -q regular
#SBATCH -J bidibiba-quick
#SBATCH --mail-user=tgroves@lbl.gov
#SBATCH --mail-type=ALL
#SBATCH -t 00:15:00

#OpenMP settings:
export OMP_NUM_THREADS=1
export OMP_PLACES=threads
export OMP_PROC_BIND=spread


#run the application:
srun --ntasks=384 --ntasks-per-node=2 -G 4 --cpu_bind=cores /global/cfs/cdirs/m888/tgroves/network_benchmarking/bidibiba/bidibiba_ss
