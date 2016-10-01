#include "ionlib\log.h"
#include "ionlib\net.h"
#include "ionlib\genetic_algorithm.h"
#include <fstream>

void ion::EvaluateMembers(ion::GeneticAlgorithm* algo)
{
	for (std::vector<std::vector<bool>>::iterator member_it = algo->population_.begin(); member_it != algo->population_.end(); ++member_it)
	{
		double fitness = 0.0;
		for (std::vector<bool>::iterator gene_it = member_it->begin(); gene_it != member_it->end(); ++gene_it)
		{
			if (*gene_it)
			{
				fitness += 1.0 / member_it->size();
			}
		}
		algo->fitness_[member_it - algo->population_.begin()] = fitness;
	}
}

int main(int argc, char* argv[])
{
	ion::Error result = ion::InitSockets();
	ion::LogInit("genetic_algorithm");
	//open a file for logging results
	std::ofstream fout;
	fout.open("ga.log");
	fout << "Generation,Min,Max,Mean";
	ion::GeneticAlgorithm algo(100,100,0.001,0.67);
	uint32_t generation;
	for (generation = 0; generation < 5000 && algo.GetMaxFitness() < 0.99999999;  ++generation)
	{
		LOGINFO("Generation %d, Min fitness: %lf, Max fitness: %lf, Mean fitness: %lf", generation,algo.GetMinFitness(), algo.GetMaxFitness(), algo.GetAverageFitness());
		fout << generation << "," << algo.GetMinFitness() << "," << algo.GetMaxFitness() << "," << algo.GetAverageFitness() << std::endl;
		algo.NextGeneration();
	}
	LOGINFO("Final: Generation %d, Min fitness: %lf, Max fitness: %lf, Mean fitness: %lf", generation, algo.GetMinFitness(), algo.GetMaxFitness(), algo.GetAverageFitness());
	return 0;
}