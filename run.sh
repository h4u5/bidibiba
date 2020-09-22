#!/bin/bash
#SBATCH -N 1920
#SBATCH -C knl,quad,cache
#SBATCH -p regular
#SBATCH -J bidibiba
#SBATCH --mail-user=tgroves@lbl.gov
#SBATCH --mail-type=ALL
#SBATCH -t 01:00:00

#run the application:
srun -n 385 -c 272 --cpu_bind=cores /global/homes/t/tgroves/sandbox/bidibiba/bidibiba
sleep 1
srun -n 576 -c 272 --cpu_bind=cores /global/homes/t/tgroves/sandbox/bidibiba/bidibiba
sleep 1
srun -n 768 -c 272 --cpu_bind=cores /global/homes/t/tgroves/sandbox/bidibiba/bidibiba
sleep 1
srun -n 1152 -c 272 --cpu_bind=cores /global/homes/t/tgroves/sandbox/bidibiba/bidibiba
sleep 1
srun -n 1920 -c 272 --cpu_bind=cores /global/homes/t/tgroves/sandbox/bidibiba/bidibiba
sleep 1

