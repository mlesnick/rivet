/**
 * \class	SimplexTree
 * \brief	Stores a bifiltered simplicial complex in a simplex tree structure.
 * \author	Matthew L. Wright
 * \date	February 2014
 * 
 * The SimplexTree class stores a bifiltered simplicial complex in a simplex tree structure.
 * Each node in the simplex tree (implemented by the STNode class) represents one simplex in the bifiltration.
 * Each simplex has a multi-index at which it is born.
 * Implementation is based on a 2012 paper by Boissonnat and Maria.
 */


#ifndef __SimplexTree_H__
#define __SimplexTree_H__

#include <utility>	// std::pair
#include <map>
#include <stdexcept>
#include "point.h"
#include "st_node.h"
#include "map_matrix.h"
#include "index_matrix.h"

struct SimplexData;     //used for return type of SimplexTree::get_simplex_data()
struct DirectSumMatrices;   //used for return type of SimplexTree::get_merge_mxs()

//comparison functor for sorting std::set<STNode*> by REVERSE-LEXICOGRAPHIC multi-grade order
struct NodeComparator
{
    bool operator()(const STNode* left, const STNode* right) const
    {
        if(left->grade_y() < right->grade_y())
            return true;
        if(left->grade_y() == right->grade_y() && left->grade_x() < right->grade_x())
            return true;
        return false;
    }
};

//typedef
typedef std::multiset<STNode*, NodeComparator> SimplexSet;

//now the SimplexTree class
class SimplexTree {
	public:
        SimplexTree(int dim, int v);	//constructor; requires dimension of homology to be computed and verbosity parameter
		
        void build_VR_complex(std::vector<Point> &points, int pt_dim, int max_dim, double max_dist);
                    //builds SimplexTree representing a Vietoris-Rips complex from a vector of points, with certain parameters
		
        void add_simplex(std::vector<int> & vertices, int x, int y);	//adds a simplex (and its faces) to the SimplexTree; multi-grade is (x,y)
		
        void update_global_indexes();			//updates the global indexes of all simplices in this simplex tree
        void update_dim_indexes();              //updates the dimension indexes (reverse-lexicographical multi-grade order) for simplices of dimension (hom_dim-1), hom_dim, and (hom_dim+1)

        int grade_x_position(double value);     //returns the position of "value" in the ordered list of multi-grade first-components, or -1 if not found
        double grade_x_value(int i);            //returns the value at the i^th position in the ordered list of multi-grade first-components
		
        int grade_y_position(double value);     //returns the position of "value" in the ordered list of multi-grade second-components, or -1 if not found
        double grade_y_value(int i);            //returns the value at the i^th position in the ordered list of multi-grade second-components

        MapMatrix* get_boundary_mx(int dim);    //returns a matrix of boundary information for simplices

        DirectSumMatrices get_merge_mxs();      //returns matrices for the merge map [B+C,D], the boundary map B+C, and the multi-grade information

        IndexMatrix* get_index_mx(int dim);     //returns a matrix of column indexes to accompany MapMatrices

        std::vector<int> find_vertices(int gi);	//given a global index, return (a vector containing) the vertices of the simplex
        STNode* find_simplex(std::vector<int>& vertices);   //given a sorted vector of vertex indexes, return a pointer to the node representing the corresponding simplex

        int num_x_grades();                     //returns the number of unique x-coordinates of the multi-grades
        int num_y_grades();                     //returns the number of unique y-coordinates of the multi-grades

        ///// THESE FUNCTIONS MIGHT NEED TO BE UPDATED
            MapMatrix* get_boundary_mx(std::vector<int> coface_global, std::map<int,int> face_order);
                //computes a boundary matrix, using given orders on simplices of dimensions d (cofaces) and d-1 (faces)
                //used in persistence_data.cpp

            SimplexData get_simplex_data(int index);	//returns the multi-grade of the simplex with given global simplex index, as well as the dimension of the simplex

            int get_num_simplices();		//returns the total number of simplices represented in the simplex tree
                //TODO: would it be more efficient to store the total number of simplices???
		
        ///// FUNCTIONS FOR TESTING
            void print();				//prints a representation of the simplex tree
            void test_lists();  //TESTING ONLY

        ///// DEPRECATED FUNCTIONS
            int time_index(double);				//returns the index of a time value, or -1 if not found
            double get_time(int);				//returns a time value, given an index

            int dist_index(double);				//returns the index of a distance value, or -1 if not found
            double get_dist(int);				//returns a distance value, given an index

            int get_num_dists();		//returns the number of unique distance indexes
            int get_num_times();		//returns the number of unique time indexes

		
	private:
        STNode root;		//root node of the simplex tree

        int hom_dim;    //dimension of homology to be computed

        std::vector<double> grade_x_values;     //sorted list of unique birth times, used for creating integer indexes
        std::vector<double> grade_y_values;     //sorted list of unique distances (not greater than max_distance), used for creating integer indexes

        SimplexSet ordered_high_simplices;   //pointers to simplices of dimension (hom_dim + 1) in reverse-lexicographical multi-grade order
        SimplexSet ordered_simplices;        //pointers to simplices of dimension hom_dim in reverse-lexicographical multi-grade order
        SimplexSet ordered_low_simplices;    //pointers to simplices of dimension (hom_dim - 1) in reverse-lexicographical multi-grade order

		
		const int verbosity;	//controls display of output, for debugging
		
        void build_VR_subtree(std::vector<Point> &points, double* distances, STNode &parent, std::vector<int> &parent_indexes, double prev_time, double prev_dist, int cur_dim, int max_dim, int& gic);	//recursive function used in build_VR_complex()
		
        void add_faces(std::vector<int> & vertices, int x, int y);	//recursively adds faces of a simplex to the SimplexTree; WARNING: doesn't update global data structures (time_list, dist_list, or global indexes), so should only be called from add_simplex()
		
		void update_gi_recursively(STNode &node, int &gic); 		//recursively update global indexes of simplices
		
        void build_dim_lists_recursively(STNode &node, int cur_dim);        //recursively build lists to determine dimension indexes

		void find_nodes(STNode &node, int level, std::vector<int> &vec, int time, int dist, int dim);	//recursively search tree for simplices of specified dimension that exist at specified multi-index
		
		void find_vertices_recursively(std::vector<int> &vertices, STNode &node, int key);	//recursively search for a global index and keep track of vertices

        void write_boundary_column(MapMatrix* mat, STNode* sim, int col, int offset);   //writes boundary information for simplex represented by sim in column col of matrix mat; offset allows for block matrices such as B+C

        void print_subtree(STNode&, int);	//recursive function that prints the simplex tree
};

#include "simplex_tree.cpp"

#endif // __SimplexTree_H__
