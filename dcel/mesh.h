/**
 * \class	Mesh
 * \brief	Stores and manipulates the DCEL decomposition of the affine Grassmannian.
 * \author	Matthew L. Wright
 * \date	March 2014
 */

#ifndef __DCEL_Mesh_H__
#define __DCEL_Mesh_H__

//includes????  namespace????
#include <vector>
#include <queue>
#include <iostream>
#include <limits>	//necessary for infinity
#include <math.h>	//necessary for atan
#include <set>

#include "lcm.h"
#include "dcel.h"
#include "xi_point.h"
#include "cell_persistence_data.h"
#include "../math/persistence_data.h"
#include "../math/multi_betti.h"

#include <boost/multiprecision/cpp_int.hpp>
typedef boost::multiprecision::cpp_rational exact;

class xiSupportMatrix;
#include "xi_support_matrix.h"


////struct to store xi points, to help send data to the VisualizationWindow
//struct xiPoint
//{
//    unsigned x, y;  //coordinates (discrete)
//    int zero, one;  //multiplicities of xi_0 and xi_1 at this point ---- TODO: maybe should be unsigned?

//    xiPoint(unsigned xc, unsigned yc, int m0, int m1) : x(xc), y(yc), zero(m0), one(m1)
//    { }
//};


//the Mesh class
class Mesh
{
	public:
        Mesh(const std::vector<double>& xg, const std::vector<exact>& xe, const std::vector<double>& yg, const std::vector<exact>& ye, int v);
            //constructor; sets up bounding box (with empty interior) for the affine Grassmannian
            //  requires references to vectors of all multi-grade values (both double and exact values)
		
		~Mesh();	//destructor: IMPLEMENT THIS, MAKE SURE ALL MEMORY IS RELEASED!!!!
		
        void store_xi_points(MultiBetti& mb, std::vector<xiPoint>& xi_pts);
            //stores xi support points from MultiBetti in Mesh (in a sparse array) and in the supplied vector
            //also computes and stores LCMs in Mesh; LCM curves will be created when build_arrangment() is called

        void build_arrangement();
            //function to build the arrangement using a version of the Bentley-Ottmann algorithm
            //precondition: all LCMs have been stored via store_xi_points()
		
        void build_persistence_data(std::vector<std::pair<unsigned, unsigned> > &xi, SimplexTree* bifiltration, int dim);
			//associates persistence data to each face, requires all support points of xi_0 and xi_1, the bifiltration, and the dimension of homology
		
        PersistenceData* get_persistence_data(double angle, double offset, std::vector<std::pair<unsigned, unsigned> > & xi);
            //returns persistence diagram data associated with the specified point (line); angle should be in RADIANS
		
        void print_stats(); //prints a summary of the arrangement information, such as the number of LCMS, vertices, halfedges, and faces
		void print();	//prints all the data from the mesh
        void test_consistency();    //attempts to find inconsistencies in the DCEL arrangement
		

        //references to vectors of multi-grade values
        const std::vector<double>& x_grades;   //floating-point values for x-grades
        const std::vector<exact>& x_exact;     //exact values for all x-grades
        const std::vector<double>& y_grades;   //floating-point values for y-grades
        const std::vector<exact>& y_exact;     //exact values for all y-grades

        //these are necessary for comparisons, but should they really be static members of Mesh???
        static double epsilon;
        static bool almost_equal(const double a, const double b);

    private:
        std::vector<Vertex*> vertices;		//all vertices in the mesh
		std::vector<Halfedge*> halfedges;	//all halfedges in the mesh
		std::vector<Face*> faces;		//all faces in the mesh
		
		const double INFTY;

        std::set<LCM*, LCM_LeftComparator> all_lcms;	//set of LCMs that are represented in the mesh, ordered by position of curve along left side of strip
		
        Halfedge* topleft;			//pointer to Halfedge that points down from top left corner (theta=0, r=infty)
        Halfedge* bottomleft;       //pointer to Halfedge that points up from bottom left corner (theta=0, r=-infty)
        Halfedge* bottomright;      //pointer to Halfedge that points up from bottom right corner (theta=pi/2, r=-infty)
		
		const int verbosity;			//controls display of output, for debugging

        Halfedge* insert_vertex(Halfedge* edge, double x, double y);	//inserts a new vertex on the specified edge, with the specified coordinates, and updates all relevant pointers
        Halfedge* create_edge_left(Halfedge* edge, LCM* lcm);    //creates the first pair of Halfedges in an LCM curve, anchored on the left edge of the strip

		std::pair<bool, double> project(double angle, double offset, double x, double y);	//projects (x,y) onto the line determined by angle and offset
		
        unsigned HID(Halfedge* h);		//halfedge ID, for printing and debugging
        unsigned FID(Face* f);		//face ID, for printing and debugging

        xiSupportMatrix xi_matrix;  //sparse matrix to hold xi support points


      //struct to hold a future intersection event
        struct Crossing {
            LCM* a;     //pointer to one line
            LCM* b;     //pointer to the other line -- must ensure that line for LCM a is below line for LCM b just before the crossing point!!!!!
            double x;   //x-coordinate of intersection point (floating-point)
            Mesh* m;    //pointer to the mesh, so the Crossing has access to the vectors x_grades, x_exact, y_grades, and y_exact

            Crossing(LCM* a, LCM* b, Mesh* m);  //precondition: LCMs a and b must be comparable
            bool x_equal(const Crossing* other) const;  //returns true iff this Crossing has (exactly) the same x-coordinate as other Crossing
        };

      //comparator class for ordering crossings: first by x (left to right); for a given x, then by y (low to high)
        struct CrossingComparator {
            bool operator()(const Crossing* c1, const Crossing* c2) const;	//returns true if c1 comes after c2
        };

};//end class Mesh

#endif // __DCEL_Mesh_H__

