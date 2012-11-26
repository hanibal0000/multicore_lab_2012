// Simple CUDA example by Ingemar Ragnemalm 2009. Simplest possible?
// Assigns every element in an array with its index.

// nvcc simple.cu -L /usr/local/cuda/lib -lcudart -o simple

#include <stdio.h>
#include <math.c>

const int N = 16; 
const int blocksize = 16; 

__global__ 
void simple(float *c) 
{
	c[threadIdx.x] = sqrt(c[threadIdx.x]);
}

int main()
{
	float *res = new float[N];	
    float *orig = new float[N];
    float *test = new float[N];
	float *cd;
	const int size = N*sizeof(float);
	
	cudaMalloc( (void**)&cd, size );
    int i;
    elem = 4;
    for(i = 0; i<N; i++)
    {
        orig[i] = elem;
        test[i] = sqrt(elem);
        elem = pow(elem, 2);
    }

    cudaMemcpy( cd, orig, size, cudaMemcpyHostToDevice );
	dim3 dimBlock( blocksize, 1 );
	dim3 dimGrid( 1, 1 );
	simple<<<dimGrid, dimBlock>>>(cd);
	cudaThreadSynchronize();
	cudaMemcpy( res, cd, size, cudaMemcpyDeviceToHost ); 
	cudaFree( cd );
	
	for (i = 0; i < N; i++)
		printf("Input: %f, CPU: %f, CUDA: %f ", orig[i], test[i], res[i]);
	printf("\n");
	delete[] c;
	printf("done\n");
	return EXIT_SUCCESS;
}
