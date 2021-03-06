// ------------------------------------------------------------------
// A D A P T I V E   Q U A D R A T U R E   I N T E G R A T I O N
//
// M P I
// ------------------------------------------------------------------
// Version: 1.1
// Developed by: Alex Verkest, Quang Pham, Fernando Guardado-Perez
//  Description: Sequential implementation of AQI that takes bounds
//               and error threshold as command line arguments and
//               produces an integration of an equation hardcoded
//               within the function() function.
//         Run:  $ make
//                  or
//               $ g++ -O -fopenmp AQI-mpi.cpp -o AQI-mpi -std=c++11
//               $ sbatch AQI-mpi_slurm.sh <lower_bound> <upper_bound> <error>
// ------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stack>

using namespace std;

// F U N C T I O N :
// ------------------------------------------------------------------
// Description: Contains hardcoded f(x); the desired equation to
//              be integrated. To change f(x): edit the return
//              statement (uses c++ syntax).
//       Input: x: Float value of x to be computed by equation f(x)
//      Output: The calculated value of f(x).
// ------------------------------------------------------------------
float function(float x) {
  // return 0.005 * pow(x, 2);                         // f(x) = 1/200x^2
  // return (0.25 * x) + 4;                            // f(x) = 1/4x + 4
  // return fabs(20 * cos(x)) / 4;                     // f(x) = | 20cos(x) |/4
  return fabs( (5 * x) * ( cos(6*x) * sin(x) ) ) / 20;  // f(x) = | 5x( (cos(6x) + sin(x) ) | / 20

  // f(x) = |10 * ( cos(x^x) / (2 ^ ((x^x - pi/2) / 5))) + 4 |
}

// T R A P E Z O I D A L   A C T U A L :
// ------------------------------------------------------------------
// Description: Produces the definite integral of above function
//              using the bounds passed as parameters.
//       Input: lower_bound: Lower bound of definite integral
//              upper_bound: Upper bound of definite integral
//      Output: The calculated definite integral of function().
//      Source: https://www.geeksforgeeks.org/trapezoidal-rule-for-approximate-value-of-definite-integral/
// ------------------------------------------------------------------
float trapezoidal_actual(float lower_bound, float upper_bound) {
  int interval = 1000; // higher interval = more accurate
  float grid = (upper_bound - lower_bound) / interval;
  // Computing sum of first and last terms in function()
  float s = function(lower_bound) + function(upper_bound);
  // Adding middle terms in above formula
  for (int i = 1; i < interval; i++)
      s += 2 * function(lower_bound + i * grid);

  return (grid / 2) * s;
}

// T R A P E Z O I D A L   R U L E :
// ------------------------------------------------------------------
// Description: Produces area under f(x) using the trapezoidal rule
//              and bounds passed as parameters.
//       Input: a: Lower bound of trapezoid
//              b: Upper bound of trapezoid
//      Output: Area of trapezoid under the graphed function f(x).
// ------------------------------------------------------------------
float trapezoidal_rule(float a, float b) {
  return 0.5 * (b - a) * (function(a) + function(b));
}

// C H E C K   E R R O R :
// ------------------------------------------------------------------
// Description: Determines if delta of actual and calcuted integral
//              exceeds user error threshold.
//       Input: calculated: calculated integral of f(x) using AQI
//              actual: actual integral of f(x) generated by
//                      trapezoidal_actual()
//              error: user decided error threshold
//      Output: True: if the difference between actual and calculated
//              is less than error.
//              False: if the difference is more than the error.
// ------------------------------------------------------------------
bool check_error(float calculated, float actual, float error) {
  if (fabs(actual - calculated) > error)
    return false;
  else
    return true;
}

// A D A P T I V E   Q U A D R A T U R E :
// ------------------------------------------------------------------
// Description: Recursive function to calculate integral using AQI.
//       Input: lower: Lower bound of definite integral
//              upper: Upper bound of definite integral
//              error: User generated error threshold
//      Output: Integration generated AQI that passed check_error().
// ------------------------------------------------------------------
float adaptive_quadrature(float lower, float upper, float error) {
  float trapezoidal_area = trapezoidal_rule(lower, upper);
  float actual_integration = trapezoidal_actual(lower, upper);
  float aqi_integration = 0.0;

  // Condition: if trapezoidal_area doesn't pass check_error() split
  //            bounds in half and use recursion to find integral
  //            that passes check_error().
  if (!check_error(trapezoidal_area, actual_integration, error)) {
    float midpoint = (upper + lower) / 2;
    aqi_integration += adaptive_quadrature(lower, midpoint, error);
    aqi_integration += adaptive_quadrature(midpoint, upper, error);
  }
  else // if trapezoidal_area passes check_error(), add it AQI
    aqi_integration += trapezoidal_area;

  return aqi_integration;
}

// M A S T E R :
// ------------------------------------------------------------------
//
// ------------------------------------------------------------------
float master(float lower, float upper, float error, int numProcesses, int myProcessID) {
	struct fStruct {float data[3];};
	MPI_Status status;
	float values[3] = {lower,upper, 0};
	fStruct temp = {{values[0], values[1], values[2]}};
	double result = 0;

	int remaining = 1;
	stack<fStruct> tasks;
	tasks.push(temp);
	bool busy[numProcesses - 1];
	bool isBusy = false;

	while (tasks.size() > 0 || isBusy){
		MPI_Recv(values, 3, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		busy[status.MPI_SOURCE - 1] = false;

		if(status.MPI_TAG == 1) {
			temp = {{values[0], values[1], values[2]}};
			tasks.push(temp);
			MPI_Recv(values, 3, MPI_FLOAT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
			temp = {{values[0], values[1], values[2]}};
			tasks.push(temp);
		}
		if (status.MPI_TAG == 2) {
			result += values[2];
		}

		for(int i = 0; i < numProcesses - 1; i++){
			if(!tasks.empty() && !busy[i]){
				temp = tasks.top();
				tasks.pop();
				values[0] = temp.data[0];
				values[1] = temp.data[1];
				values[2] = temp.data[2];
				MPI_Send(values, 3, MPI_FLOAT, i + 1, 0, MPI_COMM_WORLD);
				busy[i] = true;
			}
		}

		isBusy = false;
		for(int i = 0; i < numProcesses - 1; i++){
				if(isBusy || busy[i])
					isBusy = true;
			}
	}
	for(int i = 0; i < numProcesses - 1; i++){
		MPI_Send(values, 3, MPI_FLOAT, i + 1, 3, MPI_COMM_WORLD);
	}

	return result;
}

// S L A V E :
// ------------------------------------------------------------------
//
// ------------------------------------------------------------------
void slave(int myProcessID, float error) {
	float values[3] = {0,0,0};
	MPI_Status status;

	MPI_Send(values, 3, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

	while(1){
		MPI_Recv(values, 3, MPI_FLOAT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		if (status.MPI_TAG == 3) {
			break;
		} else {

			float lower = values[0];
			float upper = values[1];
			float trapezoidal_area = trapezoidal_rule(lower, upper);
			float actual_integration = trapezoidal_actual(lower, upper);
			float aqi_integration = 0.0;

			if (!check_error(trapezoidal_area, actual_integration, error)) {
				float midpoint = (upper + lower) / 2;
				values[0] = lower;
				values[1] = midpoint;
				MPI_Send(values, 3, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
				values[0] = midpoint;
				values[1] = upper;
				MPI_Send(values, 3, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);

			} else {
				values[2] = trapezoidal_area;
				MPI_Send(values, 3, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
			}
		}
	}

	return;
}
// M A I N :
// ------------------------------------------------------------------
// Description: Main driver of program. Takes command line arguments
//       Input: argv[1]: lower limit of integral
//              argv[2]: upper limit of integral
//              argv[3]: error threshold
//      Output: Printed and formatted AQI definite integration of
//              function()
// ------------------------------------------------------------------
int main(int argc, char *argv[]) {
	int n, numProc, myProcID;
	float result;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numProc);
	MPI_Comm_rank(MPI_COMM_WORLD, &myProcID);

  // Command line error message
  if (argc < 4) {
    cout << "You may be missing some arguments.\n";
    cout << "Try: <AQI> <lower limit> <upper limit> <error>\n";
    cout << ":)\n";
    MPI_Finalize();
    exit(1);
  }

  if (numProc < 2){
	  cout << "Must have 2 or more nodes";
	  MPI_Finalize();
	  exit(1);
  }

  //Get start time
  double runtime = MPI_Wtime();

  float lower = stof(argv[1]);
  float upper = stof(argv[2]);
  float error = stof(argv[3]);

  if (myProcID == 0){
	  result = master(lower, upper, error, numProc, myProcID);
  } else {
	  slave(myProcID, error);
  }

  if (myProcID == 0){
    runtime = MPI_Wtime() - runtime;
	  // Formatted Ouput
	  printf("##################################\n");
	  printf(" Adaptive Quadrature Integration: \n");
	  printf("##################################\n");
	  printf(" • Integral: ∫ %s dx \n", "<copy function here>");
	  printf(" •   Bounds: %4.2f, %4.2f \n", lower, upper);
	  printf(" •    Error: %4.2f \n", error);
	  // printf(" •   Actual: %4.2f \n", trapezoidal_actual(lower, upper));
	  printf(" •      AQI: %4.2f \n", result);
    cout << " •  Runtime: "	<< setiosflags(ios::fixed)
                        << setprecision(4) << runtime << " seconds\n";
  }

  MPI_Finalize();
  return(0);
}
