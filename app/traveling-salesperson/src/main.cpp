#include "ionlib\log.h"
#include "ionlib\genetic_algorithm.h"
#include "ionlib\geometry.h"
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <sstream>
#include <map>
#include <time.h>
#include <signal.h>
#define MIDPOINT_MUTATION
//#define RANK_PROPORTIONAL_SELECTION
#define FITNESS_PROPORTIONAL_SELECTION
typedef std::vector<uint32_t> route_t;

void SignalHandler(int signal)
{
	printf("Signal %d", signal);
}

typedef struct tsp_s
{
	std::string name;
	std::vector<ion::Point2<double>> cities;
	route_t optimal_route;
} tsp_t;

class TravelingSalespersonGA : public ion::GeneticAlgorithm<route_t>
{
public:
	TravelingSalespersonGA(size_t num_members, size_t num_citites, double mutation_probability, double crossover_probability, tsp_t tsp) : ion::GeneticAlgorithm<route_t>(num_members, 1, mutation_probability, crossover_probability)
	{
		//according to the problem definition, the salesperson must start at city 1, thus note that all of this class ignores city one except for computing distance
		tsp_ = tsp;
		optimal_length_ = 0.0;
		optimal_fitness_ = 1.0;
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
		if (tsp.optimal_route.size() > 0)
		{
			optimal_length_ = GetRouteLength(tsp.optimal_route);
			optimal_fitness_ = 1.0 / std::round(optimal_length_);
		}
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
				//note this line consumes about 66% of the CPU time, I could optimize it to make the program run much faster
				double random_number = ion::randlf(0.0, 1.0);
				if (random_number < mutation_probability_)
				{
#ifndef MIDPOINT_MUTATION
					size_t city_to_swap = ion::randull(0, member_it->size() - 1);
					std::iter_swap(city_it, member_it->begin() + city_to_swap);
#elif defined(MIDPOINT_MUTATION)
					//find the city closest to the midpoint between these neighbors
					uint32_t neighbor_left, neighbor_right;
					neighbor_left = *city_it;
					if (member_it->end() - city_it == 1)
					{
						neighbor_right = 0;
					} else
					{
						neighbor_right = *(city_it + 1);
					}
					//instead of blindly selecting the closest city to the midpoint, probabilistically select a nearby city by partitioning the space in half repeatedly
					random_number = ion::randlf(0.0, 1.0);
					//subdivide the space until that number is found
					double partition = 0.5;
					size_t partition_iteration = 0;
					random_number -= partition;
					//don't allow the city to stay the same
					while (random_number > 0 && partition_iteration < (member_it->size()-3))
					{
						partition = partition / 2.0;
						random_number -= partition;
						partition_iteration++;
					}
					//now find the partition_iteration'th closest city
					ion::Point2<double> left_location, right_location;
					left_location = tsp_.cities[neighbor_left];
					right_location = tsp_.cities[neighbor_right];


					std::multimap<double, uint64_t> city_distance;
					for(uint32_t city_index = 1; city_index < member_it->size(); ++city_index) {
						if (city_index == neighbor_left || city_index == neighbor_right)
						{
							continue;
						}
						//compute the distance between these cities
						double distance = left_location.distance(tsp_.cities[city_index]) + right_location.distance(tsp_.cities[city_index]);
						city_distance.insert(std::pair<double, uint64_t>(distance, city_index));
					}
					//get the n'th element
					std::map<double, uint64_t>::iterator nearby_city = city_distance.begin();
					while (partition_iteration != 0)
					{
						partition_iteration--;
						nearby_city++;
					}
					//find this city in the route
					route_t::iterator city_to_swap_1 = std::find(member_it->begin(), member_it->end(), nearby_city->second);
					route_t::iterator city_to_swap_2;
					//swap the left city with this city, unless this is the last city
					if (neighbor_right == 0)
					{
						city_to_swap_2 = city_it;
					} else
					{
						city_to_swap_2 = city_it + 1;
					}
					std::iter_swap(city_to_swap_1, city_to_swap_2);


#else
#error No mutation method selected
#endif
				}
			}
		}
	}
	virtual void Select()
	{
		//This implements the PMX selection

		//note that the fitnesses must already be set
		//get the total fitness
#if defined(FITNESS_PROPORTIONAL_SELECTION)
		double fitness_sum = std::accumulate(fitness_.begin(), fitness_.end(), 0.0);
#elif defined(RANK_PROPORTIONAL_SELECTION)
		//compute the maximum rank sum
		size_t rank_sum = 0;
		size_t num_cities = population_[0].size();
		size_t index = num_cities;
		while (index != 0)
		{
			rank_sum += index/**index*/;
			index--;
		}
		//make a map of fitnesses (maps are ordered)
		std::multimap<double, uint32_t> fitness_map;
		for (std::vector<double>::iterator fitness_it = fitness_.begin(); fitness_it != fitness_.end(); ++fitness_it)
		{
			fitness_map.insert(std::pair<double,uint32_t>(*fitness_it, (uint32_t)(fitness_it - fitness_.begin())));
		}
#else
#error No selection method enabled
#endif
		//create a temporary population
		std::vector<route_t> temp_population;
		temp_population.reserve(population_.size());
		//since we are using elite selection, push the elite member
		temp_population.push_back(GetEliteMember());
		//start selecting elements by treating the fitness as cumulative density function
		for (uint32_t member_index = 1; member_index < population_.size(); ++member_index)
		{
#if defined(FITNESS_PROPORTIONAL_SELECTION)
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
#elif defined(RANK_PROPORTIONAL_SELECTION)
			int32_t selected_offset = (int32_t)ion::randull(1, rank_sum);
			//LOGDEBUG("Selected offset: %d", selected_offset);
			size_t rank_index = 0;
			std::multimap<double, uint32_t>::reverse_iterator selected_pair = fitness_map.rbegin();
			while (selected_offset > 1)
			{
				selected_offset -= (int32_t)((num_cities - rank_index)/**(num_cities - rank_index)*/);
				rank_index++;
				selected_pair++;
			}
			//hack because I can't figure out why it sometimes gets to the rend element
			uint32_t parent_index;
			if (selected_pair == fitness_map.rend())
			{
				LOGERROR("Using defualt parent due to rend bug");
				parent_index = 0;
			} else
			{
				//now selected_pair has the city to select
				parent_index = selected_pair->second;
			}
#else
#error No selection method enabled
#endif
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
		//hack sometimes temp population doesn't have enough items in it, so add more
		while (temp_population.size() < population_.size())
		{
			LOGERROR("Apply population missize hack");
			temp_population.push_back(*(temp_population.begin()));
		}
		population_.swap(temp_population);
		
	}
	double GetRouteLength(route_t member)
	{
#ifdef _DEBUG
		//first, as a debug step, validate all members
		//convert the vector to a set (i.e. container with unique members) and if the lenghts differ there was an issue
		std::set<uint32_t> set(member.begin(), member.end());
		LOGASSERT(set.size() == member.size());
#endif
		ion::Point2<double> last_city = tsp_.cities[0];
		//evaluate their lengths
		double tour_length = 0.0;
		for (route_t::iterator city_it = member.begin(); city_it != member.end(); ++city_it)
		{
			if (*city_it > tsp_.cities.size())
			{
				//hack because I can't figure this out
				LOGERROR("Injecting bad fitness");
				return 999999999.0;
			}
			ion::Point2<double> city_coord = tsp_.cities[*city_it];
			tour_length += std::round(last_city.distance(city_coord));
			last_city = city_coord;
		}
		//the tour ends at city 1
		tour_length += std::round(last_city.distance(tsp_.cities[0]));
		return tour_length;
	}
	virtual void EvaluateMembers()
	{
		for (std::vector<route_t>::iterator member_it = population_.begin(); member_it < population_.end(); ++member_it)
		{
			double tour_length = GetRouteLength(*member_it);

			//I use the 1/distance method to compute fitness knowing that the tour length will never be 0
			fitness_[member_it - population_.begin()] = 1.0 / tour_length;
		}

	}
	double optimal_length_;
	double optimal_fitness_;
private:
	tsp_t tsp_;
};

tsp_t ReadTspInput(std::string tsp_filename, std::string optimal_filename)
{
	std::ifstream fin;
	tsp_t tsp;
	fin.open(tsp_filename);
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
	fin.close();
	if (optimal_filename != "")
	{
		fin.open(optimal_filename);
		while (fin.good())
		{
			fin >> dummy;
			if (dummy == "TOUR_SECTION")
			{
				break;
			}
		}
		while (fin.good())
		{
			int64_t city_id;
			fin >> city_id;
			if (city_id == -1)
			{
				break;
			}
			//my cities are 0-indexed
			tsp.optimal_route.push_back((uint32_t)city_id-1);
		}
	}
	return tsp;
}

void ExecuteGa(tsp_t tsp, size_t population_size, double mutation_rate, double crossover_rate)
{
	std::ofstream fout;
	uint32_t generation = 0;
	static double max_fitness[500000] = { 0 };
	static double min_fitness[500000] = { 0 };
	static double avg_fitness[500000] = { 0 };
	static double num_evals[500000] = { 0 };
	static double num_hits[500000] = { 0 };
	std::stringstream filename;
	filename << "TSP_" << tsp.name << "_pop" << population_size << "_mut" << mutation_rate << "_xover" << crossover_rate << ".csv";
	fout.open(filename.str());
	fout << "Generation,Min,Max,Mean,Evals" << std::endl;

	for (uint32_t trial = 0; trial < 30; ++trial)
	{
		LOGINFO("Starting trial %u", trial);
		TravelingSalespersonGA ga(population_size, tsp.cities.size(), mutation_rate, crossover_rate, tsp);
		LOGINFO("The optimal fitness is %lf, the optimal length is %lf", ga.optimal_fitness_, ga.optimal_length_);
		max_fitness[generation] += ga.GetMaxFitness();
		min_fitness[generation] += ga.GetMinFitness();
		avg_fitness[generation] += ga.GetAverageFitness();
		num_evals[generation] += ga.GetNumEvals();
		num_hits[generation]++;
		for (generation = 1; ga.GetMaxFitness() < ga.optimal_fitness_ && generation < 50000; ++generation)
		{
			ga.NextGeneration();
			max_fitness[generation] += ga.GetMaxFitness();
			min_fitness[generation] += ga.GetMinFitness();
			avg_fitness[generation] += ga.GetAverageFitness();
			num_evals[generation] += ga.GetNumEvals();
			num_hits[generation]++;
			if (generation % 2000 == 0)
			{
				LOGINFO("Generation %u, shortest path: %lf", generation, 1.0 / ga.GetMaxFitness());
				std::stringstream path;
				path << "Shortest path: ";
				route_t elite_member = ga.GetEliteMember();
				for (route_t::iterator city_it = elite_member.begin(); city_it != elite_member.end(); ++city_it)
				{
					//add one to the city ID because the files are 1-indexed
					path << (*city_it) + 1 << ", ";
				}
				LOGDEBUG("%s", path.str().c_str());
			}
		}
		LOGINFO("Final result: after %u generations the shortest path is: %lf", generation, 1.0 / ga.GetMaxFitness());
		std::stringstream path;
		path << "Trial "<<trial<<" Shortest path: ";
		route_t elite_member = ga.GetEliteMember();
		for (route_t::iterator city_it = elite_member.begin(); city_it != elite_member.end(); ++city_it)
		{
			//add one to the city ID because the files are 1-indexed
			path << (*city_it) + 1 << ", ";
		}
		LOGDEBUG("%s", path.str().c_str());
		fout.flush();
		fout << "Trial "<<trial << " final result: after " << generation << " generations the shortest path is: " << 1.0 / ga.GetMaxFitness() << std::endl;
		fout.flush();
		fout << path.str() << std::endl;
		fout.flush();
		fout << "Trial "<<trial<<" begin summary section" << std::endl;
		//scale all of the computed values
		for (uint32_t generation_index = 0; generation_index < 50000; ++generation_index)
		{
			if (num_hits[generation_index] == 0)
			{
				break;
			}
			max_fitness[generation_index] /= num_hits[generation_index];
			min_fitness[generation_index] /= num_hits[generation_index];
			avg_fitness[generation_index] /= num_hits[generation_index];
			num_evals[generation_index] /= num_hits[generation_index];
			fout.flush();
			fout << generation_index << "," << 1.0/min_fitness[generation_index] << "," << 1.0/max_fitness[generation_index] << "," << 1.0/avg_fitness[generation_index] << "," << num_evals[generation_index] << std::endl;
			fout.flush();
		}
		fout << "End summary section" << std::endl;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: traveling-salesperson-win-x64-Debug.exe input_file.tsp optimal_file.tsp");
		fflush(stdout);
		return -1;
	}
	std::string optimal_filename;
	uint32_t population_choice;
	double mutation_choice;
	double crossover_choice;
	if (argc == 6)
	{
		optimal_filename = argv[2];
		population_choice = atoi(argv[3]);
		mutation_choice = atof(argv[4]);
		crossover_choice = atof(argv[5]);
	} else
	{
		population_choice = atoi(argv[2]);
		mutation_choice = atof(argv[3]);
		crossover_choice = atof(argv[4]);
	}

	tsp_t tsp = ReadTspInput(argv[1], optimal_filename);
	if (tsp.cities.size() == 0)
	{
		LOGFATAL("Failed to load TSP info");
	}
	std::stringstream log_name;
	log_name << "TSP_" << tsp.name <<"p"<<population_choice<<"x"<<crossover_choice<<"m"<<mutation_choice<<".log";
	ion::LogInit(log_name.str().c_str());

	typedef void(*SignalHandlerPointer)(int);

	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGSEGV, SignalHandler);
	
	std::srand((uint32_t)time(NULL)+1);

	uint32_t population_set[3] = { 50, 100, 150 };
	double mutation_set[4] = { 0.0001, 0.001, 0.01 , 0.1};
	double crossover_set[3] = { 0.2, 0.67, 0.99 };
	//for (uint32_t pop_choice = 0; pop_choice < 3; ++pop_choice)
	//{
	//	for (uint32_t mutation_choice = 0; mutation_choice < 4; ++mutation_choice)
	//	{
	//		for (uint32_t crossover_choice = 0; crossover_choice < 3; ++crossover_choice)
	//		{
	ExecuteGa(tsp, population_choice, mutation_choice, crossover_choice);
	LOGINFO("Completed pop %d, mutation %d, crossover %d", population_choice, mutation_choice, crossover_choice);
	//		}
	//	}
	//}
	return 0;
}