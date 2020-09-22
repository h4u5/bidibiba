#!/bin/bash
#SBATCH -N 386
#SBATCH -C knl
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
srun -n 386 -c 272 --cpu_bind=cores $HOME/Aries/Benchmarks/BIDIBIBA/bidibiba
