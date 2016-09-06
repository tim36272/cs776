#include "ionlib\log.h"
#include "ionlib\net.h"
#include "ionlib\tree.h"
#include <vector>

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
void enumerateAllStates(ion::TreeNode<RiverState>& root, ion::TreeNode<RiverState>* node)
{
	LOGDEBUG("Descending");
	//get the number of cannibals/missionaries that could possibly be moved
	RiverState state = node->GetData();
	size_t num_missionaries_movable = std::count(state.missionary_state.begin(), state.missionary_state.end(), state.boat_state);
	size_t num_cannibals_movable = std::count(state.cannibal_state.begin(), state.cannibal_state.end(), state.boat_state);

	//cap the number of movable people at BOAT_CAPACITY
	num_missionaries_movable = min(num_missionaries_movable, BOAT_CAPACITY);
	num_cannibals_movable = min(num_cannibals_movable, BOAT_CAPACITY);
	//for node, generate all of its children.
	for (size_t num_missionaries_moved = 0; num_missionaries_moved <= num_missionaries_movable; ++num_missionaries_moved)
	{
		size_t max_cannibals_to_move;
		if (num_cannibals_movable < num_missionaries_moved)
		{
			max_cannibals_to_move = 0;
		} else
		{
			max_cannibals_to_move = (num_cannibals_movable - num_missionaries_moved);
		}
		for (size_t num_cannibals_moved = 0; num_cannibals_moved <= max_cannibals_to_move; ++num_cannibals_moved)
		{
			if (num_cannibals_moved == 0 && num_missionaries_moved == 0)
			{
				//there is no one to drive the boat, skip this state
				continue;
			}
			//note that the conditions on getting to this point in the loop guarantees that there are enough people to move and the boat will fit this many people
			//so we can just generate the state and check if it is in the tree already
			RiverState new_state = movePeople(state, num_missionaries_moved, num_cannibals_moved);
			//check if this state is already in the tree
			std::vector<ion::TreeNode<RiverState>*> path = root.GetPath(new_state);
			if (path.empty())
			{
				LOGDEBUG("Adding: <%d,%d,%d><%d,%d,%d><%d>",
						 int(new_state.missionary_state[0]),
						 int(new_state.missionary_state[1]),
						 int(new_state.missionary_state[2]),
						 int(new_state.cannibal_state[0]),
						 int(new_state.cannibal_state[1]),
						 int(new_state.cannibal_state[2]),
						 new_state.boat_state);
				//there is no path in the tree which leads to an equivalent state, which means this state is novel
				node->AddLeaf(new_state);
				//enumerate all the children of that state
				enumerateAllStates(root, node->GetLeaf(node->NumLeafs() - 1));
			}
		}
	}
	LOGDEBUG("Ascending");
}
int main(int argc, char* argv[])
{
	ion::InitSockets();
	ion::LogInit("app");

	RiverState initial_state;
	initial_state.boat_state = false;
	initial_state.cannibal_state.resize(NUM_CANNIBALS, false);
	initial_state.missionary_state.resize(NUM_MISSIONARIES, false);

	ion::TreeNode<RiverState> tree(initial_state,nullptr);
	enumerateAllStates(tree, &tree);

	//ion::TreeNode<int>::iterator it(ion::DEPTH_FIRST);
	//it.init(tree);
	////ion::algo::traverse(ion::algo::BREADTH_FIRST, tree);
	//while (!it.complete())
	//{
	//	LOGINFO("Tree data: %d", (*it).GetData());
	//	++it;
	//}
	return 0;
}