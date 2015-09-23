#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 16
#define GRID_SIZE 16384
#define false 0
#define true 1

float **heat2;
float **heat1;

int grid_is_steady = 0;
int should_count = 1;

pthread_barrier_t step_barrier;
pthread_barrier_t step_barrier_start;
pthread_barrier_t count_barrier;
pthread_barrier_t count_barrier_done;

int num_threads = 1;

struct calculate_grid_rows_params {
    int start;
    int end;
    int *count;
};

struct count_rows_params {
    int start;
    int end;
    int *count;
};

double when()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

void destroy_grids()
{
	// Put destroy code here
}

int is_fixed_point(int i, int j)
{
	// point is on a boundary
	if (i == 0 || j == 0 || i == GRID_SIZE-1 || j == GRID_SIZE-1)
	{
		return true;
	}

	if (i == 200 && j == 500)
	{
		return true;
	}

	if (i == 400 && (j < 331))
	{
		return true;
	}

	if (i % 20 == 0)
	{
		return true;
	}

	if (j % 20 == 0)
	{
		return true;
	}

	return false;

}

int is_point_steady(int i, int j)
{
	double neighbors_sum = heat2[i-1][j] + heat2[i+1][j] + heat2[i][j+1] + heat2[i][j-1];
	return (heat2[i][j] - neighbors_sum / 4.0) < 0.1;
}

int is_grid_steady()
{

	for (int i = 1; i < GRID_SIZE - 1; i++)
	{
		for (int j = 1; j < GRID_SIZE - 1; j++)
		{
			if (!is_fixed_point(i,j) && !is_point_steady(i,j))
			{
				return false;
			}
		}
	}

	return true;
}

void *set_fixed_rows_cols(void *params)
{
	struct calculate_grid_rows_params *args = params;

	for (int i = args->start; i <= args->end; i++)
	{
		for (int j = 0; j < GRID_SIZE; j++)
		{
			if (i % 20 == 0)
			{
				heat1[i][j] = 100;
				heat2[i][j] = 100;
			}
			if (j % 20 == 0)
			{
				heat1[i][j] = 0;
				heat2[i][j] = 0;
			}
		}
	}
}

void set_fixed_points()
{

	heat1[0][0] = 100;
	heat2[0][0] = 100;
	heat1[GRID_SIZE-1][GRID_SIZE-1] = 100;
	heat2[GRID_SIZE-1][GRID_SIZE-1] = 100;

	heat1[200][500] = 100;
	heat2[200][500] = 100;

	for (int i = 0; i < 331; i++)
	{
		heat1[400][i] = 100;
		heat2[400][i] = 100;
	}

	pthread_t threads[num_threads];
	struct calculate_grid_rows_params params[num_threads];

	for (int i = 0; i < num_threads; i++)
	{

		params[i].start = ((GRID_SIZE / num_threads) * i) + 1;
		params[i].end = (GRID_SIZE / num_threads) * (i + 1);

		if (i == 0)
			params[i].start = 0;

		if (i == num_threads - 1)
			params[i].end = GRID_SIZE - 1;

		pthread_create(&threads[i], NULL, &set_fixed_rows_cols, &params[i]);
	}

	for (int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}

}

void *set_rows_to_50(void *params)
{
	struct calculate_grid_rows_params *args = params;

	for (int i = args->start; i <= args->end; i++)
	{
		for (int j = 0; j < GRID_SIZE; j++)
		{
			heat1[i][j] = 50;
			heat2[i][j] = 50;
		}
	}
}

void init_grids()
{

	heat1 = malloc(sizeof(float*)*GRID_SIZE);
	heat2 = malloc(sizeof(float*)*GRID_SIZE);

	for (int i = 0; i < GRID_SIZE; i++)
	{
		heat1[i] = (float*)malloc(sizeof(float)*GRID_SIZE);
		heat2[i] = (float*)malloc(sizeof(float)*GRID_SIZE);
	}

	pthread_t threads[num_threads];
	struct calculate_grid_rows_params params[num_threads];

	for (int i = 0; i < num_threads; i++)
	{

		params[i].start = ((GRID_SIZE / num_threads) * i) + 1;
		params[i].end = (GRID_SIZE / num_threads) * (i + 1);

		if (i == 0)
			params[i].start = 0;

		if (i == num_threads - 1)
			params[i].end = GRID_SIZE - 1;

		pthread_create(&threads[i], NULL, &set_rows_to_50, &params[i]);
	}

	for (int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	for (int i = 0; i < GRID_SIZE; i++)
	{

		// Top Row
		heat1[0][i] = 0;
		heat2[0][i] = 0;

		// Side Columns
		heat1[i][0] = 0;
		heat2[i][0] = 0;
		heat1[i][GRID_SIZE-1] = 0;
		heat2[i][GRID_SIZE-1] = 0;

		// Bottom Row
		heat1[GRID_SIZE-1][i] = 100;
		heat2[GRID_SIZE-1][i] = 100;
		
	}
	
	set_fixed_points();

}

void *count_rows(void *params)
{
	struct count_rows_params *args = params;


	while(should_count)
	{
		pthread_barrier_wait(&count_barrier);
		(*args->count) = 0;
		for (int i = args->start; i <= args->end; i++)
		{
			for (int j = 1; j < GRID_SIZE - 1; j++)
			{
				if (heat2[i][j] > 50)
				{
					(*args->count)++;
				}
			}
		}
		pthread_barrier_wait(&count_barrier_done);
	}


}

void *calculate_grid_rows(void *params)
{
	struct calculate_grid_rows_params *args = params;

	while (!grid_is_steady)
	{
		for (int i = args->start; i <= args->end; i++)
		{
			for (int j = 1; j < GRID_SIZE - 1; j++)
			{
				if (!is_fixed_point(i,j))
				{
					heat1[i][j] = ((heat2[i+1][j] + heat2[i-1][j] + heat2[i][j+1] + heat2[i][j-1]) + 4 * heat2[i][j]) / 8.0;
				}
			}
		}
		pthread_barrier_wait(&step_barrier);
		pthread_barrier_wait(&step_barrier_start);
	}

}

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		printf("%s\n", "Invalid number of arguments!");
		return 1;
	}

	num_threads = atoi(argv[1]);

	double start_time = when();
	init_grids();

	int count = 0;
	int above_50 = 0;
	int is_steady = is_grid_steady();
	int counts[num_threads];

	pthread_t threads[num_threads];
	struct calculate_grid_rows_params params[num_threads];

	pthread_barrier_init(&step_barrier,NULL,num_threads + 1);
	pthread_barrier_init(&step_barrier_start,NULL,num_threads + 1);
	pthread_barrier_init(&count_barrier,NULL,num_threads + 1);
	pthread_barrier_init(&count_barrier_done,NULL,num_threads + 1);

	for (int i = 0; i < num_threads; i++)
	{

		params[i].start = ((GRID_SIZE / num_threads) * i) + 1;
		params[i].end = (GRID_SIZE / num_threads) * (i + 1);
		params[i].count = &counts[i];

		if (i == 0)
			params[i].start = 1;

		if (i == num_threads - 1)
			params[i].end = GRID_SIZE - 1;

		// Create threads that calculate new values
		pthread_create(&threads[i], NULL, &calculate_grid_rows, &params[i]);
		
		// Create threads that count above 50
		pthread_create(&threads[i], NULL, &count_rows, &params[i]);
	}


	do
	{
		pthread_barrier_wait(&step_barrier);

		float **temp = heat1;
		heat1 = heat2;
		heat2 = temp;
		
		is_steady = is_grid_steady();
		grid_is_steady = is_steady;

		pthread_barrier_wait(&count_barrier);
		pthread_barrier_wait(&count_barrier_done);

		above_50 = 0;
		for (int i = 0; i < num_threads; i++)
		{
			above_50 += counts[i];
		}

		printf("[%d] %d above 50\n", count, above_50);
		count++;
		pthread_barrier_wait(&step_barrier_start);

	}
	while (!is_steady);

	should_count = false;
	pthread_barrier_wait(&count_barrier);
	pthread_barrier_wait(&count_barrier_done);

	above_50 = 0;
	for (int i = 0; i < num_threads; i++)
	{
		above_50 += counts[i];
	}

	pthread_barrier_destroy(&step_barrier);
	pthread_barrier_destroy(&step_barrier_start);
	pthread_barrier_destroy(&count_barrier);
	pthread_barrier_destroy(&count_barrier_done);

	printf("Reached steady state\n\tIterations:\t%d\n\tTime:\t\t%f\n\tAbove 50:\t%d\n\tThreads:\t\t%d\n", count, when() - start_time, above_50, num_threads);

	destroy_grids();

	return 0;

}
