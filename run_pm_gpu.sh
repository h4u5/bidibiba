#!/bin/bash
#SBATCH -N 512
#SBATCH -C gpu
#SBATCH --gpus-per-node=4
#SBATCH --exclusive
#SBATCH -q regular
#SBATCH -A nstaff_g
#SBATCH -J bidibiba-GPU
#SBATCH --mail-user=tgroves@lbl.gov
#SBATCH --mail-type=ALL
#SBATCH -t 00:15:00

#OpenMP settings:
export OMP_NUM_THREADS=1
export OMP_PLACES=threads
export OMP_PROC_BIND=spread

export MPICH_OFI_NIC_POLICY=NUMA
export MPICH_OFI_NUM_NICS=4


#run the application:
srun --ntasks=2048 --ntasks-per-node=4 --cpu_bind=cores /global/cfs/cdirs/m888/tgroves/network_benchmarking/bidibiba/bidibiba_ss
