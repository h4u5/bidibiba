#!/bin/bash
#SBATCH -N 9304
#SBATCH -C knl
#SBATCH -q regular
#SBATCH -J bidibiba-large
#SBATCH --mail-user=tgroves@lbl.gov
#SBATCH --mail-type=ALL
#SBATCH -t 01:00:00

#OpenMP settings:
export OMP_NUM_THREADS=1
export OMP_PLACES=threads
export OMP_PROC_BIND=spread


#run the application:
srun -n 9304 -c 272 --cpu_bind=cores $HOME/Aries/Benchmarks/BIDIBIBA/bidibiba
