#include "ionlib\log.h"
#include "ionlib\genetic_algorithm.h"
#include "ionlib\geometry.h"
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
typedef std::vector<uint32_t> route_t;

typedef struct tsp_s
{
	std::string name;
	std::vector<ion::Point2<double>> cities;
} tsp_t;

class TravelingSalespersonGA : public ion::GeneticAlgorithm<route_t>
{
public:
	TravelingSalespersonGA(size_t num_members, size_t num_citites, double mutation_probability, double crossover_probability, tsp_t tsp) : ion::GeneticAlgorithm<route_t>(num_members, 1, mutation_probability, crossover_probability)
	{
		//according to the problem definition, the salesperson must start at city 1, thus note that all of this class ignores city one except for computing distance
		tsp_ = tsp;
		//setup the members
		if (num_citites > RAND_MAX || num_members > RAND_MAX)
		{
			LOGFATAL("Your chromosome or population size is greater than RAND_MAX, so the random number generator will never select some members");
		}
		for (std::vector<route_t>::iterator member_it = population_.begin(); member_it != population_.end(); ++member_it)
		{
			//subtract 1 because city 1 is the start
			member_it->resize(num_citites - 1);
			for (route_t::iterator city_it = member_it->begin(); city_it != member_it->end(); ++city_it)
			{
				(*city_it)= (uint32_t)((city_it - member_it->begin()) + 1);
			}
			//make several swaps of the cities
			for (route_t::iterator city_it = member_it->begin(); city_it != member_it->end(); ++city_it)
			{
				size_t city_to_swap = ion::randull(0, member_it->size() - 1);
				std::iter_swap(city_it, member_it->begin() + city_to_swap);
			}
		}
		EvaluateMembers();
	}
	virtual void Mutate()
	{
		//randomly select cities to permute
		//we start with the second element because we are doing elite selection
		for (std::vector<route_t>::iterator member_it = population_.begin() + 1; member_it != population_.end(); ++member_it)
		{
			//mutate by swapping cities
			for (route_t::iterator city_it = member_it->begin(); city_it != member_it->end(); ++city_it)
			{
				double random_number = ion::randlf(0.0, 1.0);
				if (random_number < mutation_probability_)
				{
					size_t city_to_swap = ion::randull(0, member_it->size() - 1);
					std::iter_swap(city_it, member_it->begin() + city_to_swap);
				}
			}
		}
	}
	virtual void Select()
	{
		//This implements the PMX selection

		//note that the fitnesses must already be set
		//get the total fitness
		double fitness_sum = std::accumulate(fitness_.begin(), fitness_.end(), 0.0);
		//create a temporary population
		std::vector<route_t> temp_population;
		temp_population.reserve(population_.size());
		//since we are using elite selection, push the elite member
		temp_population.push_back(GetEliteMember());
		//start selecting elements by treating the fitness as cumulative density function
		for (uint32_t member_index = 1; member_index < population_.size(); ++member_index)
		{
			//get a number between 0 and fitness_sum
			double selected_individual = ion::randlf(0.0, fitness_sum);
			//traverse the CDF until selected_individual is found
			std::vector<double>::iterator parent_it;
			uint32_t parent_index = 0;
			for (parent_it = fitness_.begin(); parent_it != fitness_.end(); ++parent_it, ++parent_index)
			{
				selected_individual -= *parent_it;
				if (selected_individual < 0.0000000001)
				{
					break;
				}
			}
			LOGASSERT(selected_individual <= 0.0000000001);
			//now parent_it is the member that is getting propogated to the next generation
			temp_population.push_back(*(population_.begin() + parent_index));
			//if this iteration is an odd number (that is, we have pushed an even number of elements onto the queue) attempt crossover on these two members
			if (member_index % 1 == 1)
			{
				double random_number = ion::randlf(0.0, 1.0);
				if (random_number < crossover_probability_)
				{
					//we will do crossover

					//select two points to crossover at
					uint32_t crossover_begin = (uint32_t)ion::randull(0, (uint32_t)(population_.size()) - 1);
					uint32_t crossover_end   = (uint32_t)ion::randull(crossover_begin + 1, population_.size() - 1);
					//get the last two members pushed onto the temp vector
					std::vector<route_t>::reverse_iterator mate1 = temp_population.rbegin();
					std::vector<route_t>::reverse_iterator mate2 = mate1 + 1;
					//these are the members we will do PMX on
					for (uint32_t crossover_index = crossover_begin; crossover_index <= crossover_end; ++crossover_index)
					{
						//to update mate1, we check what mate 2 has in this position, find that item in mate1, and replace it with what mate1 had there. Similarly for the other mate
						uint32_t city_in_mate2 = (*mate2)[crossover_index]; //2
						uint32_t city_in_mate1 = (*mate1)[crossover_index];
						route_t::iterator mate2_city_in_mate1_it = std::find(mate1->begin(), mate1->end(), city_in_mate2); //[8] ==2
						route_t::iterator mate1_city_in_mate2_it = std::find(mate2->begin(), mate2->end(), city_in_mate1);
						*mate2_city_in_mate1_it = (*mate1)[crossover_index]; //[8] = 5
						*mate1_city_in_mate2_it = (*mate2)[crossover_index];
						(*mate1)[crossover_index] = city_in_mate2; //[3] = 2
						(*mate2)[crossover_index] = city_in_mate1;
					}
				}
			}
		}
		population_.swap(temp_population);
	}
	virtual void EvaluateMembers()
	{
		for (std::vector<route_t>::iterator member_it = population_.begin(); member_it < population_.end(); ++member_it)
		{
#ifdef _DEBUG
			//first, as a debug step, validate all members
			//convert the vector to a set (i.e. container with unique members) and if the lenghts differ there was an issue
			std::set<uint32_t> set(member_it->begin(), member_it->end());
			LOGASSERT(set.size() == member_it->size());
#endif
			ion::Point2<double> last_city = tsp_.cities[0];
			//evaluate their lengths
			double tour_length = 0.0;
			for (route_t::iterator city_it = member_it->begin(); city_it != member_it->end(); ++city_it)
			{
				tour_length += last_city.norm2(tsp_.cities[*city_it]);
			}
			fitness_[member_it - population_.begin()] = std::round(tour_length);
		}

	}
private:
	tsp_t tsp_;
};

tsp_t ReadTspInput(std::string filename)
{
	std::ifstream fin;
	tsp_t tsp;
	fin.open(filename);
	//read the header
	std::string dummy;
	fin >> dummy >> tsp.name;
	while (fin.good())
	{
		fin >> dummy;
		if (dummy == "NODE_COORD_SECTION")
		{
			break;
		}
	}
	//now we are in the node section
	while (fin.good())
	{
		ion::Point2<double> point;
		//read id
		fin >> dummy;
		//note the text "EOF" is literally at the end of the file, not to be confused with the EOF character which will come right after that
		if (dummy == "EOF")
		{
			break;
		}
		fin >> point.x1_;
		fin >> point.x2_;
		tsp.cities.push_back(point);
	}
	return tsp;
}

int main(int argc, char* argv[])
{
	ion::LogInit("app");
	if (argc < 2)
	{
		LOGINFO("Usage: traveling-salesperson-win-x64-Debug.exe input_file.tsp");
		return -1;
	}
	tsp_t tsp = ReadTspInput(argv[1]);
	
	uint32_t generation = 0;
	double max_fitness[5000] = { 0 };
	double min_fitness[5000] = { 0 };
	double avg_fitness[5000] = { 0 };
	double num_evals[5000] = { 0 };
	double num_hits[5000] = { 0 };
	for (uint32_t trial = 0; trial < 30; ++trial)
	{
		TravelingSalespersonGA ga(10, tsp.cities.size(), 0.0001, 0.67, tsp);
		max_fitness[generation] += ga.GetMaxFitness();
		min_fitness[generation] += ga.GetMinFitness();
		avg_fitness[generation] += ga.GetAverageFitness();
		num_evals[generation] += ga.GetNumEvals();
		num_hits[generation]++;
		for (generation = 1; ga.GetMaxFitness() < 0.99999999 && generation < 5000; ++generation)
		{
			ga.NextGeneration();
			max_fitness[generation] += ga.GetMaxFitness();
			min_fitness[generation] += ga.GetMinFitness();
			avg_fitness[generation] += ga.GetAverageFitness();
			num_evals[generation] += ga.GetNumEvals();
			num_hits[generation]++;
		}
	}
	return 0;
}