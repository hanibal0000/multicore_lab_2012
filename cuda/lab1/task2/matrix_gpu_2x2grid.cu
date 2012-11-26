// Matrix addition, CPU version
// gcc matrix_cpu.c -o matrix_cpu -std=c99

#include <stdio.h>

#define MATRIX_SIZE 16
#define GRID_DIM 2

__global__
void add_matrix(float *in, float *in_out)
{
    //number of blocks * length of one block line
    int line_length = gridDim.x * blockDim.x;
    //height of a block * position of block + position of thread in block
    int line_number = blockDim.y  * blockIdx.y + threadIdx.y;
    //length of a block * position of block + position of thread
    int line_pos = blockDim.x * blockIdx.x + threadIdx;

    int index = (line_number - 1) * line_length + line_pos;

    if(index < (gridDim.x * blockDim.x) * (gridDim.y * blockDim.y))
        in_out[index] = in[index] + in_out[index];
    
}

int main()
{
    
	const int N = MATRIX_SIZE;

	float *a = new float[N*N];
	float *b = new float[N*N];
	float *cu_in, *cu_out;
    size_t size = N*N*sizeof(float);

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			a[i+j*N] = 10 + i;
			b[i+j*N] = (float)j / N;
		}

    cudaMalloc((void **)&cu_in, size); 
    cudaMalloc((void **)&cu_out, size); 
    cudaMemcpy(cu_in, a, size, cudaMemcpyHostToDevice);
    cudaMemcpy(cu_out, b, size, cudaMemcpyHostToDevice);

    dim3 dimBlock(N/GRID_DIM,N/GRID_DIM);
    dim3 dimGrid(GRID_DIM,GRID_DIM);

    add_matrix<<<dimGrid, dimBlock>>>(cu_in, cu_out);

    cudaMemcpy(b, cu_out, size, cudaMemcpyDeviceToHost);
	
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			printf("%0.2f ", b[i+j*N]);
		}
		printf("\n");
	}
}
