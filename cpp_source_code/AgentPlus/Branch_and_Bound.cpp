
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <omp.h>
#include <algorithm>
#include <queue>
#include "AgentPlus.h"
#include "CSVParser.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define N_PAX _MAX_NUMBER_OF_PASSENGERS 
#define N_VEH _MAX_NUMBER_OF_VEHICLES

extern VRP_exchange_data g_VRP_data;
ofstream BBDebugfile;

float g_Global_LowerBound=-99999;
float g_Global_UpperBound=99999;

int g_node_id = 0;

int costMatrix[N_PAX][N_VEH] =
{
	{ 9, 2, 7, 8 },
	{ 6, 4, 3, 7 },
	{ 5, 8, 1, 8 },
};

// state space tree node
class Node
{
public:
	// stores parent node of current node
	// helps in tracing path when answer is found
	Node* parent;
	int node_id; 
	// contains cost for ancestors nodes including current node
	float UBCost;

	// contains least lower bound promising cost
	float BestLBCost;

	// contain worker number
	int VehicleID;
	// contains passsenger ID
	int PassengerID;

	// Boolean array bAssignedPax will contains
	// info about available passenger

	VRP_exchange_data l_vrp_data;

	Node()
	{
		VehicleID = -1;
		// contains passsenger ID
		PassengerID = -1;
	}

	int CalculateCost()
	{
		int cost = 0;

		//
		// add cost of next vehicle


		g_Optimization_Lagrangian_Method_Vehicle_Routing_Problem_Simple_Variables(&l_vrp_data);

		if (l_vrp_data.UBCost < g_Global_UpperBound)
		{
			BBDebugfile << "!!!   Cal: update upper bound from (node UB)" << g_Global_UpperBound << " to a new " << l_vrp_data.UBCost << endl;
			g_Global_UpperBound = l_vrp_data.UBCost;

		}


		BBDebugfile << "   Node " << node_id << " with LB =" << l_vrp_data.LBCost << " UB= " << l_vrp_data.UBCost  << endl;

		bool bFeasibleLBSolution = true;
		for (int p = 1; p < l_vrp_data.V2PAssignmentVector.size(); p++)
		{
			int competing_vehicle_size = l_vrp_data.V2PAssignmentVector[p].output_competting_vehicle_id_vector.size();
			if (competing_vehicle_size >= 2)
			{
				for (int v = 0; v < competing_vehicle_size; v++)
				{
					BBDebugfile << "   Cal: pax " << p << " with competing v[ " << v << "]=" <<l_vrp_data.V2PAssignmentVector[p].output_competting_vehicle_id_vector[v] << endl;

				}

			}

			if (competing_vehicle_size != 1)  //0 or more than 2
				bFeasibleLBSolution = false;


		}

		if (bFeasibleLBSolution)
		{
			BBDebugfile << "   Cal: We have a good feasible LB solution! " << endl;

			//if (l_vrp_data.LBCost < g_Global_UpperBound)
			//{
			//	BBDebugfile << "!!!   Cal: update upper bound from (node LB)" << g_Global_UpperBound << " to " << l_vrp_data.LBCost << endl;
			//	g_Global_UpperBound = l_vrp_data.LBCost;

			//}


		}




		return cost;
	}
};

// Function to allocate a new search tree node
// Here Person p is assigned to vehicle v
Node* newNode(VRP_exchange_data vrp_data, Node* parent)
{
	Node* node = new Node;
	node->node_id = g_node_id++;

	node->l_vrp_data.CopyAssignmentInput(vrp_data.V2PAssignmentVector);

	node->parent = parent;
	node->l_vrp_data = vrp_data;

	node->l_vrp_data.BBNodeNo = node->node_id;


	return node;
}

// Function to calculate the least promising cost
// of node after vehicle v is bAssigned to passenger p.

// Comparison object to be used to order the heap
struct comp
{
	bool operator()(const Node* lhs,
	const Node* rhs) const
	{
		return lhs->l_vrp_data.LBCost > rhs->l_vrp_data.LBCost;
	}
};

// print assignments
void printAssignments(Node *min)
{
	if (min->parent == NULL)
		return;

	printAssignments(min->parent);

	BBDebugfile << "Assign vehicle " << min->VehicleID << " to pax " << min->PassengerID << " with cost " << costMatrix[min->VehicleID][min->PassengerID] << endl;

}

// Finds minimum cost using Branch and Bound.

float findMinCost(int max_number_of_nodes)
{
	g_Global_LowerBound = -99999;
	g_Global_UpperBound = 99999;

	// Create a priority queue to store live nodes of
	// search tree;
	priority_queue<Node*, std::vector<Node*>, comp> priority_queue;

	// initailize heap to dummy node with cost 0
	VRP_exchange_data vrp_data;

	Node* root = newNode(vrp_data, NULL);

	root->l_vrp_data.CopyAssignmentInput(g_VRP_data.V2PAssignmentVector);

	root->CalculateCost();

	// Add dummy node to list of live nodes;
	priority_queue.push(root);

	// Finds a active node with least cost,
	// add its childrens to list of active nodes and
	// finally deletes it from the list.
	int search_count = 0;
	while (!priority_queue.empty())
	{
		// Find a active node with least estimated cost
		Node* min = priority_queue.top();

		BBDebugfile << "Find an active node " << min->node_id << " with a  UB cost = " << min->l_vrp_data.UBCost << " LB cost = " << min->l_vrp_data.LBCost << endl;



		BBDebugfile << "Find an active node " << min->node_id << " with a  UB cost = " << min->l_vrp_data.UBCost << " LB cost = " << min->l_vrp_data.LBCost << endl;
		// The found node is deleted from the list of
		// live nodes
		priority_queue.pop();



		if (search_count >= max_number_of_nodes)
			break;

		search_count++; 
		if (min->l_vrp_data.LBCost > g_Global_UpperBound + 0.00001)
		{
			BBDebugfile << "   cut node " << min->node_id << " as LBCost = " << min->l_vrp_data.LBCost << " > current best upper bound " << g_Global_UpperBound << endl;
			continue;
		}

		if (g_Global_LowerBound < min->l_vrp_data.LBCost)
		{  // update globale lower bound
			g_Global_LowerBound = min->l_vrp_data.LBCost;
			BBDebugfile << "***Update Global LB= to  " << g_Global_LowerBound << "; Global UB = " << g_Global_UpperBound << "gap %% = " << (g_Global_UpperBound - g_Global_LowerBound) / g_Global_LowerBound* 100 << " **** " << endl;

		}
		//branching based competing vehicles

		bool bBranchedFlag = false;
		for (int p = 1; p < min->l_vrp_data.V2PAssignmentVector.size(); p++) // for each p
		{
			int competing_vehicle_size = min->l_vrp_data.V2PAssignmentVector[p].output_competting_vehicle_id_vector.size();
			if (competing_vehicle_size >= 2) // if there are more than 2 competing vehicles
			{
				std::vector <int> prohibited_vehicle_id_vector;

				// first vi inclusive braches 
				for (int vi = 0; vi < competing_vehicle_size; vi++)
				{
					BBDebugfile << "   Branch: pax " << p << " with competing v[ " << vi << "]=" << min->l_vrp_data.V2PAssignmentVector[p].output_competting_vehicle_id_vector[vi] << endl;

					// create a new tree node
					Node* child = newNode(min->l_vrp_data, min);
					int veh_id = min->l_vrp_data.V2PAssignmentVector[p].output_competting_vehicle_id_vector[vi];
					child->VehicleID = veh_id;
					child->PassengerID = p;

					BBDebugfile << "   New Node " << child->node_id << " with vehicle " << child->VehicleID << " to Pax " << child->PassengerID << endl;

					child->l_vrp_data.AddP2VAssignment(p, veh_id);
					prohibited_vehicle_id_vector.push_back(veh_id);

					child->CalculateCost();
					// Add child to list of live nodes;
					priority_queue.push(child);
				}

				// last exclusive branch

				BBDebugfile << "   pax " << p << " with exclusive branches "<< endl;

				Node* child = newNode(min->l_vrp_data, min);
				child->l_vrp_data.AddProhibitedAssignment(p, prohibited_vehicle_id_vector);
				child->CalculateCost();
				// Add child to list of live nodes;
				priority_queue.push(child);

				break;  // branch once and break
			}
			else if (competing_vehicle_size == 0) // if there are 0 vehicle to serve a pax
			{
				BBDebugfile << "   To be branch: pax " << p << " with 0 competing vehicle" << endl;

			
			}
		}
		//	}
		
	}

	return g_Global_UpperBound;
}

int g_Brand_and_Bound()
{
	BBDebugfile.open("BBDebug.txt");
	BBDebugfile << "\nOptimal Cost is " << findMinCost(100) << " Number of Nodes checked = " << g_node_id << endl;;

	BBDebugfile.close();

	return 0;
}

