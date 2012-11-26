// Matrix addition, CPU version
// gcc matrix_cpu.c -o matrix_cpu -std=c99

#include <stdio.h>

__global__
void add_matrix(float *in, float *in_out)
{
    int index = threadIdx.y * blockDim.x + threadIdx.x;
    if(index < blockDim.x * blockDim.y)
        in_out[index] = in[index] + in_out[index];
    
}

int main()
{
	const int N = 16;

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

    dim3 dimBlock(N,N);
    dim3 dimGrid(1,1);

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
