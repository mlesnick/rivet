//test program
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include <algorithm>

#include "point.h"
#include "simplex_tree.h"
#include "st_node.h"
#include "map_matrix.h"
#include "multi_betti.h"

using namespace std;


int main(int argc, char* argv[])
{	
	//verbose: true for more output; false for less output
	const bool verbose = true;
	
	//check for name of data file
	if(argc == 1)
	{
		cout << "USAGE: run <filename>\n";
		return 1;
	}
	
	//integer dimension of data
	int dimension;	
	
	//maximum dimension of simplices in Vietoris-Rips complex
	int max_dim;
	
	//maximum distance for edges in Vietoris-Rips complex
	double max_dist;
	
	//create vector for points
	vector<Point> points;
		
	//read points from file
	if(verbose) { cout << "READING FILE:\n"; }
	string line;
	ifstream myfile(argv[1]);
	if(myfile.is_open())
	{
		//get dimension of the points from the first line of the file
		getline(myfile,line);
		stringstream(line) >> dimension;
		if(verbose) { cout << "  dimension of data: " << dimension << "\n"; }
		
		//get maximum dimension of simplices in Vietoris-Rips complex
		getline(myfile,line);
		stringstream(line) >> max_dim;
		if(verbose) { cout << "  maximum dimension of simplices: " << max_dim << "\n"; }
		
		//get maximum distance for edges in Vietoris-Rips complex
		getline(myfile,line);
		stringstream(line) >> max_dist;
		if(verbose) { cout << "  maximum distance: " << max_dist << "\n"; }
		
		//get points
		while( getline(myfile,line) )
		{
			//parse current point from string
			istringstream iss(line);
			double* n = new double[dimension];				//DO I NEED TO delete THESE LATER???
			for(int i=0; i<dimension; i++)
			{
				iss >> n[i];	//extract the next double from the string
			}
			double t;	//time of birth for this point
			iss >> t; 
			
			Point p (n, t);
			
			//add current point to the vector
			points.push_back(p);
		}
		myfile.close();
		if(verbose) { cout << "  read " << points.size() << " points; input finished\n"; }
	}
	else
	{
		cout << "Error: Unable to open file " << argv[1] << ".\n";
		return 1;
	}
	
	//sort the points
	if(verbose) { cout << "SORTING POINTS BY BIRTH TIME\n"; }
	sort(points.begin(), points.end());
	
	
	//test points vector
	if(verbose) {
		cout << "TESTING VECTOR:\n";
		for(int i=0; i<points.size(); i++)
		{
			Point p = points.at(i);
			double *m = p.get_coords();
			cout << "  point " << i << ": (";
			for(int i=0; i<dimension; i++)
			{
				cout << m[i];
				if(i<dimension-1) { cout << ", "; }
			}
			cout << ") born at time " << p.get_birth() << "\n";		
		}
		cout << "  found " << points.size() << " points\n";
	}
	
	
	//build the filtration
	if(verbose) { cout << "BUILDING FILTRATION\n"; }
	SimplexTree simplex_tree(points, dimension, max_dim+1, max_dist);
	
	//print simplex tree
	cout << "TESTING SIMPLEX TREE:\n";
	simplex_tree.print();	
	
	//TEST: 
	cout << "  vertex lists for each of the " << simplex_tree.get_num_simplices() << " simplices:\n";
	for(int i=0; i < simplex_tree.get_num_simplices(); i++)
	{
		cout << "    simplex " << i << ": ";
		vector<int> vert = simplex_tree.find_vertices(i);
		for(int j=0; j<vert.size(); j++)
			cout << vert[j] << ", ";
		cout << "\n";
	}
	
	

	//compute xi_0 and xi_1
	cout << "COMPUTING xi_0 AND xi_1:\n";
	int dim = 1;	//dimension of homology
	MultiBetti mb(&simplex_tree, dim);
	
	cout << "  VALUES OF xi_0(time, dist):\n";
	cout << "        dist 0  1  2\n    -----------------\n";
	for(int i=0; i<simplex_tree.get_num_times(); i++)
	{
		cout << "    time " << i << " | ";
		for(int j=0; j<simplex_tree.get_num_dists(); j++)
		{
			if(i==0 || j==0)
				cout << "x  ";
			else
				cout << mb.xi0(i,j) << "  ";
		}
		cout << "\n";
	}
	cout << "\n  VALUES OF xi_1(time, dist):\n";
	cout << "        dist 0  1  2\n    -----------------\n";
	for(int i=0; i<simplex_tree.get_num_times(); i++)
	{
		cout << "    time " << i << " | ";
		for(int j=0; j<simplex_tree.get_num_dists(); j++)
		{
			if(i==0 || j==0)
				cout << "x  ";
			else
				cout << mb.xi1(i,j) << "  ";
		}
		cout << "\n";
	}
	cout << "\n";
	
	
	
	//end
	cout << "Done.\n\n";
}//end main()



