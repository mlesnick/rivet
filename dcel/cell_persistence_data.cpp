#include "cell_persistence_data.h"

#include <iostream> //for testing only
#include <math.h>   //for trig functions

CellPersistenceData::CellPersistenceData(int v) : verbosity(v)
{ }

void CellPersistenceData::set_x(double t)
{
    x = t;

    if(verbosity >= 6) { std::cout << "    --x set to " << x << "\n"; }
}

double CellPersistenceData::get_x()
{
    return x;
}

void CellPersistenceData::set_y(double t)
{
    y = t;

    if(verbosity >= 6) { std::cout << "    --y set to " << y << "\n"; }
}

double CellPersistenceData::get_y()
{
    return y;
}

//computes the persistence data, requires all support points of xi_0 and xi_1, and the bifiltration
void CellPersistenceData::compute_data(std::vector<std::pair<unsigned, unsigned> > & xi, SimplexTree* bifiltration, int dim, const std::vector<double>& x_grades, const std::vector<double>& y_grades)
{
    if(verbosity >= 6) { std::cout << "    ---in this cell, x = " << x << " and y = " << y << "\n"; }

    //map to hold ordered list of projections (key = projection_coordinate, value = global_xi_support_point_index; i.e. map: projection_coord -> global_xi_support_point_index)
    std::map<double,int> xi_proj;

    //loop through all support points, compute projections and store the unique ones
    for(unsigned i=0; i<xi.size(); i++)
    {
        std::pair<bool,double> projection = project( x_grades[xi[i].first], y_grades[xi[i].second] );

        if(projection.first == true)	//then the projection exists
        {
            xi_proj.insert( std::pair<double,int>(projection.second, i) );

            if(verbosity >= 8) { std::cout << "    ----support point " << i << " (" << xi[i].first << ", " << xi[i].second << ") projected to coordinate " << projection.second << "\n"; }
        }
    }

    //build two lists/maps of xi support points
    //  vector xi_global is stored permanently in this data structure, and is used as a map: order_xi_support_point_index -> global_xi_support_point_index
    //  map xi_order is used temporarily in this function, as a map: global_xi_support_point_index -> order_xi_support_point_index
    std::map<int,int> xi_order;
    int n=0;
    for (std::map<double,int>::iterator it=xi_proj.begin(); it!=xi_proj.end(); ++it)
    {
        xi_global.push_back( it->second );
        xi_order.insert( std::pair<int,int>( it->second, n ) );
        n++;
    }

    //order xi support point indexes are 0, ..., (num_xi_proj-1)
    //however, simplices that project after all xi support points will be said to project to order xi support index num_xi_proj
    int num_xi_proj = xi_proj.size();

    //NOTE: it is also true that num_xi_proj == xi_global.size()

    if(verbosity >= 6)
    {
        std::cout << "    --This cell has " << num_xi_proj << " xi support points, ordered as follows: ";
        for (std::vector<int>::iterator it=xi_global.begin(); it!=xi_global.end(); ++it)
            std::cout << *it << ", ";
        std::cout << "\n";
    }

    //get simplex projection data
        //TODO: optimize -- we don't actually care about the projected ordering of the (d-1) simplices, right?
    std::multimap<int,int> face_p_order;	//projected ordering of (d-1)-simplices; key: order xi support point index; value: global simplex index
    std::multimap<int,int> simplex_p_order;	//projected ordering of d-simplices; key: order xi support point index; value: global simplex index
    std::multimap<int,int> coface_p_order;	//projected ordering of (d+1)-simplices; key: order xi support point index; value: global simplex index
    for(int i=0; i<bifiltration->get_num_simplices(); i++)
    {
        //get multi-index and dimension of simplex
        SimplexData sdata = bifiltration->get_simplex_data(i);	//TODO: should we cache this data somewhere to increase speed?

        if(sdata.dim == (dim-1) || sdata.dim == dim || sdata.dim == (dim+1))
        {
            if(verbosity >= 8) { std::cout << "    --Simplex " << i << " has multi-index (" << sdata.x << ", " << sdata.y << "), dimension " << sdata.dim; }

            //project multi-index onto the line
            std::pair<bool, double> p_pair = project(x_grades[sdata.x], y_grades[sdata.y]);

            if(p_pair.first == true)	//then projection exists
            {
                //find the global xi support point index of the closest support point whose projection is greater than or equal to the multi-index projection
                std::map<double,int>::iterator xi_it = xi_proj.lower_bound(p_pair.second);

                if(xi_it != xi_proj.end())	//then the simplex projects to an xi support point
                {
                    if(verbosity >= 8) { std::cout << "; projected to " << p_pair.second << ", support point " << xi_it->second << "\n"; }

                    //find the corresponding order xi support point index
                    int xi_pt = xi_order.find(xi_it->second)->second;

                    if(sdata.dim == dim-1)
                        face_p_order.insert( std::pair<int,int>(xi_pt, i) );
                    else if(sdata.dim == dim)
                        simplex_p_order.insert( std::pair<int,int>(xi_pt, i) );
                    else if(sdata.dim == dim+1)
                        coface_p_order.insert( std::pair<int,int>(xi_pt, i) );
                }
                else	//then the simplex projects after all xi support points
                {
                    if(verbosity >= 8) { std::cout << "; projected to " << p_pair.second << ", which comes after all xi support points\n"; }

                    if(sdata.dim == dim-1)
                        face_p_order.insert( std::pair<int,int>(num_xi_proj, i) );
                    else if(sdata.dim == dim)
                        simplex_p_order.insert( std::pair<int,int>(num_xi_proj, i) );
                    else if(sdata.dim == dim+1)
                        coface_p_order.insert( std::pair<int,int>(num_xi_proj, i) );

                }
            }
        }//end if(dim...)
    }//end for(simplex i in bifiltration)

    //convert (dim+1)-simplex (coface) multimap to a total order
    std::vector<int> coface_global;		// index: order_simplex_index; value: global_simplex_index
    std::vector<int> coface_order_xi;	// index: order_simplex_index; value: order_xi_support_point_index
    for(std::multimap<int,int>::iterator it=coface_p_order.begin(); it!=coface_p_order.end(); ++it)
    {
        if(verbosity >= 7) { std::cout << "CHECK: coface, order xi support pt ind " << it->first << ", global simp ind " << it->second << "\n"; }
        coface_global.push_back(it->second);
        coface_order_xi.push_back(it->first);
    }

    //convert dim-simplex (simplex) multimap to a total order, with reverse lookup for simplex indexes
    std::vector<int> simplex_global;	// index: order_simplex_index; value: global_simplex_index
    std::map<int,int> simplex_order;	// key: global_simplex_index; value: order_simplex_index
    std::vector<int> simplex_order_xi;	// index: order_simplex_index; value: order_xi_support_point_index
    int ord_ind = 0;
    for(std::multimap<int,int>::iterator it=simplex_p_order.begin(); it!=simplex_p_order.end(); ++it)
    {
        if(verbosity >= 7) { std::cout << "CHECK: simplex ord ind " << ord_ind << ", order xi support pt ind " << it->first << ", global simp ind " << it->second << "\n"; }
        simplex_global.push_back(it->second);
        simplex_order[it->second] = ord_ind;
        simplex_order_xi.push_back(it->first);
        ord_ind++;
    }

    //convert (dim-1)-simplex (face) multimap to a total order, with reverse lookup for simplex indexes
    std::map<int,int> face_order;	// key: global_simplex_index; value: order_simplex_index
    ord_ind = 0;
    for(std::multimap<int,int>::iterator it=face_p_order.begin(); it!=face_p_order.end(); ++it)
    {
        if(verbosity >= 7) { std::cout << "CHECK: face ord ind " << ord_ind << ", order xi support pt ind " << it->first << ", global simp ind " << it->second << "\n"; }
        face_order[it->second] = ord_ind;
        ord_ind++;
    }

    //create boundary matrix for (dim+1)-simplices
        // for (d+1)-simplices, requires a map: order_simplex_index -> global_simplex_index
        // for d-simplices, requires a map: global_simplex_index -> order_simplex_index
    MapMatrix* bdry_coface = bifiltration->get_boundary_mx(coface_global, simplex_order);

    if(verbosity >= 6) { std::cout << "    --Created boundary matrix for " << (dim+1) << "-simplices\n"; }
    if(verbosity >= 7) { bdry_coface->print(); }

    //reduce boundary matrix via Edelsbrunner column algorithm
    bdry_coface->col_reduce();

    if(verbosity >= 6) { std::cout << "    --Reduced boundary matrix for " << (dim+1) << "-simplices\n"; }
    if(verbosity >= 7) { bdry_coface->print(); }

    //identify and store persistence pairs
        //each persistence pair consists of the two order xi support point indexes
        // for (d+1)-simplices, requires a map: order_simplex_index -> order_xi_support_point_index
        // for d-simplices, requires map: order_simplex_index -> order_xi_support_point_index
    std::set<int> paired_order_indexes;	//stores order_simplex_index for each dim-simplex that is paired
    for(int j=0; j<bdry_coface->width(); j++)
    {
        //consider low(j)
        int i = bdry_coface->low(j);
        if(i >= 0)
        {
            if(verbosity >= 7) { std::cout << "    ----pair (" << i << "," << j << ") ==>> (" << simplex_order_xi[i] << "," << coface_order_xi[j] << ")\n"; }

            //mark that this simplex is paired
            paired_order_indexes.insert(i);

            //store this pair if it is nontrivial (i.e. not on the diagonal of the persistence diagram)
            if(simplex_order_xi[i] != coface_order_xi[j])
                persistence_pairs.push_back( std::pair<int,int>( simplex_order_xi[i], coface_order_xi[j] ) );
        }
    }

    if(verbosity >= 6) { std::cout << "    --Found persistence pairs\n"; }
    if(verbosity >= 7)
    {
        std::cout << "        ";
        for(unsigned i=0; i<persistence_pairs.size(); i++)
            std::cout << "(" << persistence_pairs[i].first << "," << persistence_pairs[i].second << ") ";
        std::cout << "\n";

        std::cout << "        paired order indexes: ";
        for(std::set<int>::iterator it=paired_order_indexes.begin(); it!=paired_order_indexes.end(); ++it)
            std::cout << *it << ", ";
        std::cout << "\n";
    }

    //make a list of only those dim-simplices that didn't get paired; this ordering shall be called np_order_simplex_index (np = not paired)
    std::vector<int> simplex_np_global;	// index: np_order_simplex_index; value: global_simplex_index
    std::set<int>::iterator poi_it = paired_order_indexes.begin();	//keeps track of current position in vector paired_order_indexes
    for(unsigned i=0; i<simplex_global.size(); i++)
    {
        if(poi_it != paired_order_indexes.end() && i == *poi_it ) //then simplex with order index i is paired
        {
            if(verbosity >= 7) { std::cout << "CHECK: simplex with global index " << simplex_global[i] << " is already paired\n"; }
            ++poi_it;
        }
        else	//simplex is not paired
        {
            if(verbosity >= 7) { std::cout << "CHECK: simplex with global index " << simplex_global[i] << " is NOT paired\n"; }
            simplex_np_global.push_back(simplex_global[i]);
        }
    }

    //create boundary matrix for dim-simplices
        // for d-simplices, requires a map order_simplex_index -> global_simplex_index
        // for (d-1)-simplices, requires a map: global_simplex_index -> order_simplex_index
    MapMatrix* bdry_simplex = bifiltration->get_boundary_mx(simplex_np_global, face_order);

    if(verbosity >= 6) { std::cout << "    --Created boundary matrix for " << dim << "-simplices\n"; }
    if(verbosity >= 7) { bdry_simplex->print(); }

    //reduce boundary matrix via Edelsbrunner column algorithm
    bdry_simplex->col_reduce();

    if(verbosity >= 6) { std::cout << "    --Reduced boundary matrix for " << dim << "-simplices\n"; }
    if(verbosity >= 7) { bdry_simplex->print(); }

    //identify and store the essential cycles
        //each essential cycle is represented by an un-paired xi support-point index (corresponding to a d-simplex)
        // for d-simplices, requires map: order_simplex_index -> order_xi_support_point_index
    for(int j=0; j<bdry_simplex->width(); j++)
    {
        //consider low(j)
        if(bdry_simplex->low(j) == -1)
            essential_cycles.push_back( simplex_order_xi[j] );	//TODO: check this!
    }

    if(verbosity >= 6) { std::cout << "    --Found essential cycles\n"; }
    if(verbosity >= 7)
    {
        std::cout << "        ";
        for(unsigned i=0; i<essential_cycles.size(); i++)
            std::cout << essential_cycles[i] << ", ";
        std::cout << "\n";
    }

    //clean up
    delete bdry_coface;
    delete bdry_simplex;
}//end compute_data()

//returns vector of global indexes of xi support points
std::vector<int>* CellPersistenceData::get_xi_global()
{
    return &xi_global;
}
//returns a vector of persistence pairs (with respect to order xi indexes)
std::vector< std::pair<int,int> >* CellPersistenceData::get_pairs()
{
    return &persistence_pairs;
}

//returns a vector of essential cycles (with respect to order xi indexes)
std::vector<int>* CellPersistenceData::get_cycles()
{
    return &essential_cycles;
}


//computes the 1-D coordinate of the projection of a point (x,y) onto the line
//returns a pair: first value is true if there is a projection, false otherwise; second value contains projection coordinate
//TODO: possible optimization: when computing projections initially, the horizontal and vertical cases should not occur (since an interior point in each cell cannot be on the boundary)
std::pair<bool, double> CellPersistenceData::project(double x, double y)
{
    double p = 0;	//if there is a projection, then this will be set to its coordinate
    bool b = true;	//if there is no projection, then this will be set to false

 /*   if(theta == 0)	//horizontal line
    {
        if(y <= r)	//then point is below line
            p = x;
        else	//no projection
            b = false;
    }
    else if(theta < HALF_PI)	//line is neither horizontal nor vertical
    //TODO: this part is correct up to a linear transfomation; is it good enough?
    {
        if(y > x*tan(theta) + r/cos(theta))	//then point is above line
            p = y; //project right
        else
        {
            double hp = x*tan(theta) + r/cos(theta); //project up
//            p = hp.convert_to<double>();
            p = hp;
        }
    }
    else	//vertical line
    {
        if(x <= -1*r)
            p = y;
        else	//no projection
            b = false;
    }
*/
    return std::pair<bool, double>(b,p);
}//end project()
