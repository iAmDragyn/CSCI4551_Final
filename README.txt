########################################################################################
D E S C R I P T I O N :
########################################################################################
The files included are different implementations for Adaptive Quadrature Integration.
Each is designed with a different method in mind. AQI-seq.cpp is our sequential version,
AQI-omp.cpp is our OpenMP version, and AQI-mpi.cpp is our MPI version.

########################################################################################
H O W   T O   R U N :
########################################################################################
C O M P I L E :
 > Run 'make' in command line all .cpps will be compiled to their respective output files
 $ make (Compiles all .cpps)

R U N :
 > Use included slurm scripts with arguments lower_bound, upper_bound, and error
 $ sbatch AQI-seq_slurm.sh <lower_bound> <upper_bound> <error> (Sequential)
 $ sbatch AQI-omp_slurm.sh <lower_bound> <upper_bound> <error> (OpenMP)
 $ sbatch AQI-mpi_slurm.sh <lower_bound> <upper_bound> <error> (MPI)

########################################################################################
K N O W N   B U G S :
########################################################################################
> The first run immediately after compiling the MPI version will typically return an
  error. All subsequent runs will return outputs as expected.
