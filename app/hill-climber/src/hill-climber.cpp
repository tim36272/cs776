#include <iostream>
#include <float.h>
#include <string.h>
#include <time.h>
using namespace std;

double eval(int *pj);

#define USE_DIFFICULT_EVAL
#if defined(USE_DIFFICULT_EVAL)
double eval(int vec[100])
{
	//simple algorithm: the hill climber assumes that the dimensions are independent, that is, that
	//changing a bit and seeing an increased fitness means that that bit should be changed.
	
	//However, an eval is proposed which effectively exploits the fact that the order in which it
	//searches the space is important (even though no memory is introduced into the eval function,
	//it would still be a better decision to try the string (1,0,0,...0) first than (0,0,0,...,1)).
	//Thus a trivial hill climber *could* be written to climb this space if it processes the string
	//from index 0 to index 99, but the implemented hill climber is unable to (read as: is
	//incredibly unlikely to) find a good result

	//Put more simply: when a bit is changed, all of the bits after it may need to be flipped,
	//thus there are local maximum everywhere in this space
	
	//Also note that a perfect string is: (1,0,0,0,...,0) (there are several perfect strings, all of which start with a 1)
	uint32_t num_ones=1;
	double fitness = 0.0;
	for (uint32_t index = 0; index < 100; ++index)
	{
		if (vec[index] == num_ones % 2)
		{
			fitness += 1.0;
		}
		if (vec[index] == 1)
		{
			num_ones++;
		}
	}
	return fitness;
}
#endif

void mutate(int vec[100])
{
	//select a random bit to flip
	int random_value = rand();
	//clamp the value to the range [0,100)
	if (random_value < 0)
	{
		random_value *= -1;
	}
	random_value %= 100;
	//flip that bit
	vec[random_value] = 1 - vec[random_value];
}

int main(int argc, char* argv[])
{
	int best_vec[100];
	int vec[100];

	int i;
	srand((uint32_t)time(NULL));
	for (i = 0; i < 100; i++)
	{
		best_vec[i] = abs(rand() % 2);
	}

	double fitness = eval(best_vec);
	double best_fitness = fitness;
	uint32_t iterations = 0;
	double max_fitness = 100;
	while (best_fitness < max_fitness)
	{
		memcpy(vec, best_vec, sizeof(int) * 100);
		mutate(vec);
		fitness = eval(vec);
		if (fitness >= best_fitness)
		{
			memcpy(best_vec, vec, sizeof(int) * 100);
			best_fitness = fitness;
		}
		++iterations;
		if (iterations % 500000 == 0)
		{
			cout << "Best fitness as of iteration "<< iterations <<":" << best_fitness << " best vec:{";
			for (uint32_t index = 0; index < 100; ++index)
			{
				cout << best_vec[index];
				if (index != 99)
				{
					cout << ",";
				}
			}
			cout << "}" << endl;
		}
	}
	cout << "The hill climber found a fitness of " << fitness << " in "<<iterations <<" iterations" <<endl;

}

