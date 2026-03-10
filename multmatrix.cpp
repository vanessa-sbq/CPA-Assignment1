//#include <omp.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <cstdlib>

#include <omp.h>
#include "argparse.hpp"

using namespace std;

#define SYSTEMTIME clock_t

 
void OnMult(int dimensions, double *pha, double *phb, double *phc)
{
	
	SYSTEMTIME Time1, Time2;
	
	char st[100];
	double temp;
	int i, j, k;

    Time1 = clock();

	for(i=0; i<dimensions; i++)
	{	for( j=0; j<dimensions; j++)
		{	temp = 0;
			for( k=0; k<dimensions; k++)
			{	
				temp += pha[i*dimensions+k] * phb[k*dimensions+j];
			}
			phc[i*dimensions+j]=temp;
		}
	}


    Time2 = clock();
	sprintf(st, "Time: %3.3f seconds\n", (double)(Time2 - Time1) / CLOCKS_PER_SEC);
	cout << st;

	cout << "Result matrix: " << endl;
	for(i=0; i<1; i++)
	{	for(j=0; j<min(10,dimensions); j++)
			cout << phc[j] << " ";
	}
	cout << endl;
}


void OnMultLine(int dimensions, double *pha, double *phb, double *phc)
{
    for (int i = 0; i < dimensions; i ++) {
        for (int k = 0; k < dimensions; k ++) {
            for (int j = 0; j < dimensions; j ++) {
                phc[i*dimensions+j] += pha[i*dimensions+k] * phb[k*dimensions+j];
            }
        }
    }
}

void parallel_multiplication(int dimensions, double *pha, double *phb, double *phc) {
    #pragma omp parallel for
    for (int i = 0; i < dimensions; i ++) {
        for (int k = 0; k < dimensions; k ++) {
            for (int j = 0; j < dimensions; j++) {
                phc[i*dimensions+j] += pha[i*dimensions+k] * phb[k*dimensions+j];
            }
        }
    }
}

int main (int argc, char *argv[])
{
    argparse::ArgumentParser argparser {argv[0]};
    argparser.add_argument("--dimensions", "-d")
        .help("Matrix size")
        .required()
        .nargs(1)
        .scan<'i', int>()
        ;

    argparser.add_argument("--n-threads", "-nt")
        .help("Number of threads")
        .nargs(1)
        .scan<'i', int>()
        ;

    argparser.add_argument("--strategy", "-st")
        .help("Matrix multiplication strategy used (1: naive, 2: line multiplication, 3: parallel)")
        .default_value(3)
        .nargs(1)
        .scan<'i', int>()
        ;

    argparser.parse_args(argc, argv);

	int dimensions = argparser.get<int>("--dimensions");
    std::optional<int> num_threads = argparser.present<int>("--n-threads");
    if (num_threads.has_value())
        omp_set_num_threads(num_threads.value());
    int strategy = argparser.get<int>("--strategy");

    double *pha = (double *)malloc((dimensions * dimensions) * sizeof(double));
    double *phb = (double *)malloc((dimensions * dimensions) * sizeof(double));
    double *phc = (double *)malloc((dimensions * dimensions) * sizeof(double));

	for(int i=0; i<dimensions; i++)
		for(int j=0; j<dimensions; j++)
			pha[i*dimensions + j] = (double)1.0;

	for(int i=0; i<dimensions; i++)
		for(int j=0; j<dimensions; j++)
			phb[i*dimensions + j] = (double)(i+1);

    for(int i=0; i<dimensions; i++)
		for(int j=0; j<dimensions; j++)
			phc[i*dimensions + j] = 0.0;

    switch (strategy){
        case 1: 
            OnMult(dimensions, pha, phb, phc);
            break;
        case 2:
            OnMultLine(dimensions, pha, phb, phc);
            break;
        case 3:
            parallel_multiplication(dimensions, pha, phb, phc);
            break;
        default:
            cerr << "Invalid strategy\n";
            exit(1);
    }

    free(pha);
    free(phb);
    free(phc);
}