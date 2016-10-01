#include <iostream>
#include <float.h>
#include <string.h>
#include <time.h>
using namespace std;

double eval(int *pj);

#define VECTOR_LENGTH 150
void mutate(int vec[VECTOR_LENGTH])
{
	//select a random bit to flip
	int random_value = rand();
	//clamp the value to the range [0,100)
	if (random_value < 0)
	{
		random_value *= -1;
	}
	random_value %= VECTOR_LENGTH;
	//flip that bit
	vec[random_value] = 1 - vec[random_value];
}

int main(int argc, char* argv[])
{
	int best_vec[VECTOR_LENGTH];
	int vec[VECTOR_LENGTH];

	int i;
	srand((uint32_t)time(NULL));
	for (i = 0; i < VECTOR_LENGTH; i++)
	{
		best_vec[i] = abs(rand() % 2);
	}

	double fitness = eval(best_vec);
	double best_fitness = fitness;
	uint32_t iterations = 0;
	double max_fitness = 64;
	while (best_fitness < max_fitness)
	{
		memcpy(vec, best_vec, sizeof(int) * VECTOR_LENGTH);
		mutate(vec);
		fitness = eval(vec);
		if (fitness >= best_fitness)
		{
			memcpy(best_vec, vec, sizeof(int) * VECTOR_LENGTH);
			best_fitness = fitness;
		}
		++iterations;
		if (iterations % 10 == 0)
		{
			cout << "Best fitness as of iteration "<< iterations <<":" << best_fitness << " best vec:{";
			for (uint32_t index = 0; index < VECTOR_LENGTH; ++index)
			{
				cout << best_vec[index];
				if (index != VECTOR_LENGTH-1)
				{
					cout << ",";
				}
			}
			cout << "}" << endl;
		}
	}
	cout << "The hill climber found a fitness of " << fitness << " in "<<iterations <<" iterations" <<endl;

}

