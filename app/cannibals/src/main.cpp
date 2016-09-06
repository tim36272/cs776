#include "ionlib\log.h"
#include "ionlib\net.h"
#include "ionlib\tree.h"
#include <vector>
#include <fstream>
#include <sstream>

#define BOAT_CAPACITY 2
#define NUM_MISSIONARIES 3
#define NUM_CANNIBALS 3
class RiverState
{
public:
	//the convention is that the fields below answer the question "is the item on the correct side of the river?",
	//that is, the initial state is false and the final state is true
	std::vector<bool> missionary_state;
	std::vector<bool> cannibal_state;
	bool boat_state;
	bool operator==(const RiverState& rhs) const
	{
		if (this->boat_state != rhs.boat_state)
		{
			return false;
		}
		size_t num_missionaries_lhs = std::count(this->missionary_state.begin(), this->missionary_state.end(), false);
		size_t num_canibals_lhs = std::count(this->cannibal_state.begin(), this->cannibal_state.end(), false);
		size_t num_missionaries_rhs = std::count(rhs.missionary_state.begin(), rhs.missionary_state.end(), false);
		size_t num_canibals_rhs = std::count(rhs.cannibal_state.begin(), rhs.cannibal_state.end(), false);
		return (num_missionaries_lhs == num_missionaries_rhs && num_canibals_lhs == num_canibals_rhs);
	}
	friend std::ostream& operator<<(std::ostream& output, RiverState state)
	{
		output << "<";
		for (std::vector<bool>::iterator missionary = state.missionary_state.begin(); missionary != state.missionary_state.end(); ++missionary)
		{
			output << *missionary << ",";
		}
		for (std::vector<bool>::iterator cannibal = state.cannibal_state.begin(); cannibal != state.cannibal_state.end(); ++cannibal)
		{
			output << *cannibal << ",";
		}
		output << state.boat_state;
		output << ">";
		return output;
	}
};
RiverState movePeople(RiverState state, size_t num_missionaries, size_t num_cannibals)
{
	size_t index = 0;
	while (num_missionaries > 0)
	{
		LOGASSERT(index < state.missionary_state.size(), "Attempted to move more missionaries than are available");
		if (state.missionary_state[index] == state.boat_state)
		{
			state.missionary_state[index] = !state.missionary_state[index];
			num_missionaries--;
		}
		++index;
	}
	index = 0;
	while (num_cannibals > 0)
	{
		LOGASSERT(index < state.cannibal_state.size());
		if (state.cannibal_state[index] == state.boat_state)
		{
			state.cannibal_state[index] = !state.cannibal_state[index];
			num_cannibals--;
		}
		++index;
	}
	state.boat_state = !state.boat_state;
	return state;
}
bool IsStateValid(RiverState state)
{
	size_t num_missionaries_origin = std::count(state.missionary_state.begin(), state.missionary_state.end(), false);
	size_t num_canibals_origin = std::count(state.cannibal_state.begin(), state.cannibal_state.end(), false);
	size_t num_missionaries_dest = std::count(state.missionary_state.begin(), state.missionary_state.end(), true);
	size_t num_canibals_dest = std::count(state.cannibal_state.begin(), state.cannibal_state.end(), true);

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
void enumerateAllStates(ion::TreeNode<RiverState>& root, ion::TreeNode<RiverState>* node)
{
	LOGDEBUG("Descending");
	//get the number of cannibals/missionaries that could possibly be moved
	//cap the number of movable people at BOAT_CAPACITY
	RiverState state = node->GetData();
	size_t num_missionaries_movable = min(BOAT_CAPACITY,std::count(state.missionary_state.begin(), state.missionary_state.end(), state.boat_state));
	size_t num_cannibals_movable = min(BOAT_CAPACITY, std::count(state.cannibal_state.begin(), state.cannibal_state.end(), state.boat_state));
	std::vector<ion::TreeNode<RiverState>*> pending_nodes;

	//for node, generate all of its children.
	for (size_t num_missionaries_moved = 0; num_missionaries_moved <= num_missionaries_movable; ++num_missionaries_moved)
	{
		size_t max_cannibals_to_move;
		max_cannibals_to_move = (num_cannibals_movable < num_missionaries_moved) ? 0 : (num_cannibals_movable - num_missionaries_moved);
		for (size_t num_cannibals_moved = 0; num_cannibals_moved <= max_cannibals_to_move; ++num_cannibals_moved)
		{
			//check if there is no one to drive the boat, if so skip this state
			if (num_cannibals_moved == 0 && num_missionaries_moved == 0)
			{
				continue;
			}
			//note that the conditions on getting to this point in the loop guarantees that there are enough people to move and the boat will fit this many people
			//so we can just generate the state and check if it is in the tree already
			RiverState new_state = movePeople(state, num_missionaries_moved, num_cannibals_moved);
			//check if the new_state is valid (i.e. cannibals don't outnumber missionaries)
			bool valid_state = IsStateValid(new_state);
			if (!valid_state)
			{
				continue;
			}
			//check if this state is already in the tree
			std::vector<ion::TreeNode<RiverState>*> path = root.GetPath(new_state);
			if (path.empty())
			{
				//there is no path in the tree which leads to an equivalent state, which means this state is novel
				node->AddLeaf(new_state);
				//enumerate all the children of that state
				pending_nodes.push_back(node->GetLeaf(node->NumLeafs() - 1));
				
			}
		}
	}
	for (std::vector<ion::TreeNode<RiverState>*>::iterator node_it = pending_nodes.begin(); node_it != pending_nodes.end(); ++node_it)
	{
		enumerateAllStates(root, *node_it);
	}
	LOGDEBUG("Ascending");
}
int main(int argc, char* argv[])
{
	/*
	  The ion library is my (Tim Sweet's) general-purpose set of utility
	  functions, such as printing, sockets, etc. I am justing starting to write
	  it, so all of the core functionality for this assignment (i.e. trees,
	  traversal, etc.) has been implemented from scratch just for this
	  assignment. See my commit log at https://github.com/tim36272/ion for
	  proof (in the commit log).
	*/
	ion::LogInit("app");

	//open a file to write the results to
	std::ofstream fout;
	std::stringstream filename;
	filename << NUM_MISSIONARIES << "_missionaries_" << NUM_CANNIBALS << "_cannibals_" << BOAT_CAPACITY << "_seats.txt";
	fout.open(filename.str());
	fout << filename.str() << std::endl;

	//Setup the problem with the requested number of actors
	//Note the convention for true/false is the answer the question
	//	"Is the item on the correct side of the river?"
	RiverState initial_state;
	initial_state.boat_state = false;
	initial_state.cannibal_state.resize(NUM_CANNIBALS, false);
	initial_state.missionary_state.resize(NUM_MISSIONARIES, false);

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
	enumerateAllStates(tree, &tree);
	
	//This prints the complete map of the valid state space.
	std::ofstream file;
	file.open("tree.gv");
	tree.print(file);
	file.close();

	//Generate the goal node so we can search for it
	RiverState goal_state;
	goal_state.boat_state = true;
	goal_state.cannibal_state.resize(NUM_CANNIBALS, true);
	goal_state.missionary_state.resize(NUM_CANNIBALS, true);

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