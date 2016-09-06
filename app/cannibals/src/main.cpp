/*
Copyright (C) 2016  Tim Sweet

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see <http://www.gnu.org/licenses/>.

*/
#include "ionlib\log.h"
#include "ionlib\tree.h"
#include "ionlib\math.h"
#include <vector>
#include <fstream>
#include <sstream>

//This is used to get command line paramters
typedef struct RiverConfig_e {
	uint32_t num_missionaries;
	uint32_t num_cannibals;
	uint32_t boat_capacity;
} RiverConfig;

/*
RiverState defines one configuration of the missionaries, cannibals, and the boat

Its primary purpose is just to store the member variables, and includes some
other required operators
*/
class RiverState
{
public:
	//the convention is that the fields below answer the question "is the item on the correct side of the river?",
	//that is, the initial state is false and the final state is true
	std::vector<bool> missionaries;
	std::vector<bool> cannibals;
	bool boat_state;
	bool operator==(const RiverState& rhs) const
	{
		if (this->boat_state != rhs.boat_state)
		{
			return false;
		}
		size_t num_missionaries_lhs = std::count(this->missionaries.begin(), this->missionaries.end(), false);
		size_t num_canibals_lhs = std::count(this->cannibals.begin(), this->cannibals.end(), false);
		size_t num_missionaries_rhs = std::count(rhs.missionaries.begin(), rhs.missionaries.end(), false);
		size_t num_canibals_rhs = std::count(rhs.cannibals.begin(), rhs.cannibals.end(), false);
		return (num_missionaries_lhs == num_missionaries_rhs && num_canibals_lhs == num_canibals_rhs);
	}
	friend std::ostream& operator<<(std::ostream& output, RiverState state)
	{
		output << "<";
		for (std::vector<bool>::iterator missionary = state.missionaries.begin(); missionary != state.missionaries.end(); ++missionary)
		{
			output << *missionary;
			if (missionary + 1 != state.missionaries.end())
			{
				output << ",";
			}
		}
		output << "><";
		for (std::vector<bool>::iterator cannibal = state.cannibals.begin(); cannibal != state.cannibals.end(); ++cannibal)
		{
			output << *cannibal;
			if (cannibal + 1 != state.cannibals.end())
			{
				output << ",";
			}
		}
		output << "><" << state.boat_state << ">";
		return output;
	}
};
//This function updates state by moving a number of people to the other side of
//the river. Notice that state includes the position of the boat, so "move" is
//not ambiguous
RiverState MovePeople(RiverState state, size_t num_missionaries, size_t num_cannibals)
{
	size_t index = 0;
	while (num_missionaries > 0)
	{
		LOGASSERT(index < state.missionaries.size(), "Attempted to move more missionaries than are available");
		if (state.missionaries[index] == state.boat_state)
		{
			state.missionaries[index] = !state.missionaries[index];
			num_missionaries--;
		}
		++index;
	}
	index = 0;
	while (num_cannibals > 0)
	{
		LOGASSERT(index < state.cannibals.size(), "Attempted to move more cannibals than are available");
		if (state.cannibals[index] == state.boat_state)
		{
			state.cannibals[index] = !state.cannibals[index];
			num_cannibals--;
		}
		++index;
	}
	state.boat_state = !state.boat_state;
	return state;
}
//This function checks if the cannibals outnumber the missionaries on either
//side of the river
bool IsStateValid(RiverState state)
{
	size_t num_missionaries_origin = std::count(state.missionaries.begin(), state.missionaries.end(), false);
	size_t num_canibals_origin = std::count(state.cannibals.begin(), state.cannibals.end(), false);
	size_t num_missionaries_dest = std::count(state.missionaries.begin(), state.missionaries.end(), true);
	size_t num_canibals_dest = std::count(state.cannibals.begin(), state.cannibals.end(), true);

	bool result = true;
	if (num_missionaries_origin > 0)
	{
		if (num_missionaries_origin < num_canibals_origin)
		{
			result = false;
		}
	}
	if (num_missionaries_dest > 0)
	{
		if (num_missionaries_dest < num_canibals_dest)
		{
			result = false;
		}
	}
	return result;
}
/*
  This function generates a tree of all possible, non-cyclic, valid (i.e. meeting
  the cannibal criteria) states.

  Notice that it does not stop at the goal. This is just so that the complete
  state tree can be printed at the end. It could easily be optimized to stop at
  the goal.

  The basic algorithm here is:
  1. Start at the root
  2. Generate every possible set of moves given:
	* The number of people on the boat's side of the river
	* The boat's capacity
	* The cannibal criteria
  3. For each valid state found, push it into the tree and into a queue
	 (implemented as a vector because why not)
  4. For each element in the queue, recursively call this function

  Note that the function is recursive and doesn't implement any depth-checking.
  It is conceivable that this could recurse forever, but it shouldn't (i.e. I
  tested it)
*/
void enumerateAllStates(ion::TreeNode<RiverState>& root, ion::TreeNode<RiverState>* node, RiverConfig river_config)
{
	RiverState state = node->GetData();
	//get the number of cannibals/missionaries that could possibly be moved
	//cap the number of movable people at river_config.boat_capacity
	size_t num_missionaries_movable = min(river_config.boat_capacity,std::count(state.missionaries.begin(), state.missionaries.end(), state.boat_state));
	size_t num_cannibals_movable = min(river_config.boat_capacity, std::count(state.cannibals.begin(), state.cannibals.end(), state.boat_state));
	
	//This vector stores all the nodes waiting to be expanded
	std::vector<ion::TreeNode<RiverState>*> pending_nodes;

	//Generate all of this node's children.
	for (size_t num_missionaries_moved = 0; num_missionaries_moved <= num_missionaries_movable; ++num_missionaries_moved)
	{
		size_t max_cannibals_to_move;
		max_cannibals_to_move = min(river_config.boat_capacity - num_missionaries_moved, num_cannibals_movable);
		for (size_t num_cannibals_moved = 0; num_cannibals_moved <= max_cannibals_to_move; ++num_cannibals_moved)
		{
			//check if there is no one to drive the boat, if so skip this state
			if (num_cannibals_moved == 0 && num_missionaries_moved == 0)
			{
				continue;
			}
			//Generate the transition from this state to the next
			RiverState new_state = MovePeople(state, num_missionaries_moved, num_cannibals_moved);
			//check if the new_state is valid (i.e. cannibals don't outnumber missionaries)
			bool valid_state = IsStateValid(new_state);
			if (!valid_state)
			{
				continue;
			}
			/*
				Note that the conditions on getting to this point in the loop
				guarantees that:
				  * There are enough people to move
				  * The boat will fit this many people
				  * The state is valid
				So we can just check if it is in the tree already and add it
				
				The next function checks if this state is already in the tree.

				Note the GetPath function returns an empty vector if there is no
				path to this state. So, we don't actually need the path, just
				its length
			*/
			std::vector<ion::TreeNode<RiverState>*> path = root.GetPath(new_state);
			if (path.empty())
			{
				//there is no path in the tree which leads to an equivalent state, which means this state is novel
				node->AddLeaf(new_state);
				//push all the children of that state
				pending_nodes.push_back(node->GetLeaf(node->NumLeafs() - 1));
				
			}
		}
	}
	/*
		The pending_nodes vector now contains all of the child nodes we pushed
		in this function. Recursively calling this function on each node in
		pending_nodes causes us to do a breadth-first search
	*/
	for (std::vector<ion::TreeNode<RiverState>*>::iterator node_it = pending_nodes.begin(); node_it != pending_nodes.end(); ++node_it)
	{
		enumerateAllStates(root, *node_it, river_config);
	}
}
int main(int argc, char* argv[])
{
	/*
	  The ion library is my (Tim Sweet's) general-purpose set of utility
	  functions, such as logging, sockets, etc. All of the core functionality
	  for this assignment (i.e. trees, traversal, etc.) has been implemented
	  from scratch just for this assignment. See my commit log at
	  https://github.com/tim36272/ion for proof (in the commit log).
	*/
	ion::LogInit("cannibals.log");

	if (argc < 4)
	{
		LOGFATAL("Usage: main.exe num_missionaries num_cannibals boat_capacity");
	}
	//get the number of missionaries, cannibals, and boat capacity
	RiverConfig river_config;
	river_config.num_missionaries = (uint32_t)atoi(argv[1]);
	river_config.num_cannibals = (uint32_t)atoi(argv[2]);
	river_config.boat_capacity = (uint32_t)atoi(argv[3]);

	//open a file to write the results to
	std::ofstream fout;
	std::stringstream result_filename;
	result_filename << river_config.num_missionaries << "missionaries_" << river_config.num_cannibals << "cannibals_" << river_config.boat_capacity << "seats.txt";
	fout.open(result_filename.str());
	fout << result_filename.str() << std::endl;

	//Setup the problem with the requested number of actors
	//Note the convention for true/false is the answer the question
	//	"Is the item on the correct side of the river?"
	RiverState initial_state;
	initial_state.boat_state = false;
	initial_state.cannibals.resize(river_config.num_cannibals, false);
	initial_state.missionaries.resize(river_config.num_missionaries, false);

	//Create the head of the tree
	ion::TreeNode<RiverState> tree(initial_state,nullptr);

	/*
		This function generates a tree of the entire valid state space without
		cycles.

		Thus it does not care when it reaches the goal node, it just exhaustively
		searches the space.

		Of course this could be optimized to stop as soon as it reaches the goal,
		but for the purpose of fulfilling question one of the assignment ("Draw a
		diagram of the complete state space") it is necessary to search to
		exhaustion
	*/
	enumerateAllStates(tree, &tree, river_config);
	
	//This prints the complete map of the valid state space.
	std::ofstream file;
	std::stringstream map_filename;
	map_filename << river_config.num_missionaries << "missionaries_" << river_config.num_cannibals << "cannibals_" << river_config.boat_capacity << "seats.gv";
	file.open(map_filename.str());
	tree.print(file);
	file.close();

	//Generate the goal node so we can search for it
	RiverState goal_state;
	goal_state.boat_state = true;
	goal_state.cannibals.resize(river_config.num_cannibals, true);
	goal_state.missionaries.resize(river_config.num_missionaries, true);

	/*
		This function finds the goal node in the tree. Note that it doesn't
		actually matter which way I search I the graph here because it has
		already been generated without cycles, that is, the goal node is unique
		in the already-built graph.
	*/

	ion::TreeNode<RiverState>::iterator it(ion::BREADTH_FIRST);
	it.init(&tree);
	while (!it.complete())
	{
		if ((*it)->GetData() == goal_state)
		{
			//We have found the goal node, just quit this loop so we can go
			//print the transitions. Note that it maintains its postion
			break;
		}
		++it;
	}
	if (it.complete())
	{
		//this means we traversed the entire tree and didn't find the goal. Either
		//there is a bug, or the problem isn't possible (for example the
		//cannibals outnumber the missionaries). Just let the user know.
		fout << "There was no solution to this problem";
		//print it to the log file as well
		LOGERROR("There was no solution to this problem");
	} else
	{
		fout << "Format: <missionaries,...,cannibals,...,boat> where 0=origin, 1=destination" << std::endl;
		//print the path to the goal
		//Note that this function actually searches for the path again. I did
		//it this way so that I could show how the tree is traversed above, but
		//for convenience I just use my path function to get the real path
		std::vector<ion::TreeNode<RiverState>*> path = tree.GetPath(goal_state);
		
		//The path is returned in order from the goal to the root, so traverse it backwards to print it in the logical order
		for (std::vector<ion::TreeNode<RiverState>*>::reverse_iterator node_it = path.rbegin(); node_it != path.rend(); ++node_it)
		{
			fout << (*node_it)->GetData() << std::endl;
		}
	}

	fout.close();
	return 0;
}