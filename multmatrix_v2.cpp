#include <omp.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace std;

double *pha, *phb, *phc;


/**
 * Reads energy consumption in microjoules from the specified file path.
 * @return The energy consumption in microjoules.
 */
long long read_energy_uj() {
    std::ifstream f("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj"); // directory path where the energy_uj file is located.
    long long val; f >> val;
    return val;
}


/**
 * Starts or resets the matrices.
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 */
void startOrResetMatrices(int m_ar, int m_br) {
	pha = (double *)malloc((m_ar * m_ar) * sizeof(double));
	phb = (double *)malloc((m_ar * m_ar) * sizeof(double));
	phc = (double *)calloc((m_ar * m_ar), sizeof(double));

	int i, j, k;

	for(i=0; i<m_ar; i++)
		for(j=0; j<m_ar; j++)
			pha[i*m_ar + j] = (double)1.0;

	for(i=0; i<m_br; i++)
		for(j=0; j<m_br; j++)
			phb[i*m_br + j] = (double)(i+1);
}


/**
 * Frees the matrices.
 */
void freeMatrices() {
	free(pha);
	free(phb);
	free(phc);
}


/**
 * Displays the first row of the result matrix (up to 10 elements).
 * @param m_br Number of columns in the result matrix
 */
void show_result_matrix(int m_br){
	int i, j;
	cout << "Result matrix: " << endl;
	for(i=0; i<1; i++) {
		for(j=0; j<min(10,m_br); j++)
			cout << phc[j] << " ";
	}

	cout << endl;
}


/**
 * Displays the execution time, GFlop/s, and energy consumption for a matrix multiplication operation.
 * @param start The start time of the operation.
 * @param end The end time of the operation.
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 * @param e_before Energy consumption before the operation (in microjoules)
 * @param e_after Energy consumption after the operation (in microjoules)
 */
void display_measurements(double start, double end, int m_ar, int m_br, double e_before, double e_after){
	// Calculate execution time
	double executionTime = (double)(end - start);
	printf("Time: %g seconds\n", executionTime);

	// Calculate gflops
    double gflops = (2.0 * m_ar * m_ar * m_br) / (executionTime * 1e9);
    printf("GFlop/s: %g\n", gflops);

	// Calculate energy consumption
	double joules = (e_after - e_before) / 1e6;
	double watts = joules / executionTime;
    printf("Joules: %.6f\n", joules);
    printf("Watts: %.6f\n", watts);
}


/**
 * Performs standard matrix multiplication using three nested loops (i, j, k order).
 * Accesses matrix B column-by-column, which is not very cache efficient.
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 */
void OnMult(int m_ar, int m_br) {	
	char st[100];
	double temp;
	int i, j, k;

	startOrResetMatrices(m_ar, m_br);
	
	auto e_before = read_energy_uj();
	double start = omp_get_wtime(); // Get start time
	
	for(i=0; i<m_ar; i++) {
		for( j=0; j<m_br; j++) {
			temp = 0;
			for( k=0; k<m_ar; k++) {
				temp += pha[i*m_ar+k] * phb[k*m_br+j];
			}
			phc[i*m_ar+j]=temp;
		}
	}
	
	double end = omp_get_wtime(); // Get end time
	auto e_after = read_energy_uj();

	display_measurements(start, end, m_ar, m_br, e_before, e_after);
	show_result_matrix(m_br);

	freeMatrices();
}


/**  
 * Performs matrix multiplication using row-based access pattern (i, k, j order).
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 */
void OnMultLine(int m_ar, int m_br) {
	int i, j, k;

	startOrResetMatrices(m_ar, m_br);

	auto e_before = read_energy_uj();
	double start = omp_get_wtime(); // Get start time

	for(i=0; i<m_ar; i++) {
		for( k=0; k<m_ar; k++) {
			for(j=0; j<m_br; j++) {	
				phc[i*m_ar+j] += pha[i*m_ar+k] * phb[k*m_br+j];
			}
		}
	}
	
	double end = omp_get_wtime(); // Get end time
	auto e_after = read_energy_uj();

	display_measurements(start, end, m_ar, m_br, e_before, e_after);
	show_result_matrix(m_br);

	freeMatrices();
}


/**
 * Performs parallel matrix multiplication using OpenMP with row-based access pattern.
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 * @param num_threads Number of OpenMP threads to use
 */
void OnMultLineParallel(int m_ar, int m_br, int num_threads) {
	int i, j, k;

	startOrResetMatrices(m_ar, m_br);

	omp_set_num_threads(num_threads);

	auto e_before = read_energy_uj();
	double start = omp_get_wtime(); // Get start time

    #pragma omp parallel for private(i, j, k)
    for(i=0; i<m_ar; i++) {
		for(k=0; k<m_ar; k++) {
			for( j=0; j<m_br; j++) {
				phc[i*m_ar+j] += pha[i*m_ar+k] * phb[k*m_br+j];
			}
		}
	}

	double end = omp_get_wtime(); // Get end time
	auto e_after = read_energy_uj();

	display_measurements(start, end, m_ar, m_br, e_before, e_after);
	show_result_matrix(m_br);

	freeMatrices();
}


/**
 * Performs parallel matrix multiplication using OpenMP and SIMD vectorization.
 * @param m_ar Number of rows/columns of matrix A
 * @param m_br Number of rows/columns of matrix B
 * @param num_threads Number of OpenMP threads to use
 */
void OnMultLineParallelSIMD(int m_ar, int m_br, int num_threads) {
    int i, j, k;

    startOrResetMatrices(m_ar, m_br);

    omp_set_num_threads(num_threads);

	auto e_before = read_energy_uj();
    double start = omp_get_wtime(); // Get start time

	#pragma omp parallel for private(i, k, j)
    for (i = 0; i < m_ar; i++) {
        for (k = 0; k < m_ar; k++) {
            #pragma omp simd
            for (j = 0; j < m_br; j++) {
                phc[i*m_ar+j] += pha[i*m_ar+k] * phb[k*m_br+j];
            }
        }
    }

    double end = omp_get_wtime(); // Get end time
	auto e_after = read_energy_uj();

	display_measurements(start, end, m_ar, m_br, e_before, e_after);
    show_result_matrix(m_br);

	freeMatrices();
}


/**
 * Displays an interactive menu for selecting matrix multiplication method and input dimensions.
 */
int main (int argc, char *argv[]) {
	char c;
	int lin, col, nt=1;
	int op;

	op=1;
	do {
		cout << endl << "1. Multiplication" << endl;
		cout << "2. Line Multiplication" << endl;
		cout << "3. Parallel Line Multiplication" << endl;
		cout << "4. Parallel Line Multiplication with SIMD" << endl;
		cin >> op;
		if (op == 0) break;
		printf("Dimensions: lins cols ? ");
   		cin >> lin >> col;

		switch (op) {
			case 1: 
				OnMult(lin, col);
				break;
			case 2:
				OnMultLine(lin, col);
				break;
            case 3:
                printf("Number of threads? ");
                cin >> nt;
                OnMultLineParallel(lin, col, nt);
                break;
            case 4:
                printf("Number of threads? ");
                cin >> nt;
                OnMultLineParallelSIMD(lin, col, nt);
                break;
			// TODO: Should there be more parallel versions? (Check assignment file)
		}
	} while (op != 0);
}
