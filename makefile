# Version: 1.0
# Place this makefile in the same directory as all of the AQI versions.
# running 'make' will compile all versions of AQI.

all:  AQI-seq AQI-omp AQI-mpi

AQI-seq: AQI-seq.cpp
	g++ -O AQI-seq.cpp -o AQI-seq -std=c++11

AQI-omp: AQI-omp.cpp
	g++ -O -fopenmp AQI-omp.cpp -o AQI-omp -std=c++11

AQI-mpi: AQI-mpi.cpp
	mpicxx AQI-mpi.cpp -o AQI-mpi -std=c++11

clean:
	rm -f $(OBJS) $(TARGET) core
