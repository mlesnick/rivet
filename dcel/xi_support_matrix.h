#ifndef XI_SUPPORT_MATRIX_H
#define XI_SUPPORT_MATRIX_H


#include <vector>
#include <list>

#include "../math/multi_betti.h"

#include "xi_point.h"
#include "multigrade.h"

//// these are the nodes in the sparse matrix
struct xiMatrixEntry
{
    unsigned x;     //discrete x-grade of this support point
    unsigned y;     //discrete y-grade of this support point
    unsigned index; //index of this support point in the vector of support points stored in VisualizationWindow

    xiMatrixEntry* down;     //pointer to the next support point below this one
    xiMatrixEntry* left;     //pointer to the next support point left of this one

    std::list<Multigrade*> low_simplices;     //associated multigrades for simplices of lower dimension --- MAYBE SHOULD BE forward_list
    std::list<Multigrade*> high_simplices;    //associated multigrades for simplices of higher dimension --- MAYBE SHOULD BE forward_list

    xiMatrixEntry();    //empty constructor, e.g. for the entry representing infinity
    xiMatrixEntry(unsigned x, unsigned y, unsigned i, xiMatrixEntry* d, xiMatrixEntry* l);  //regular constructor

    void add_multigrade(unsigned x, unsigned y, unsigned first_col, unsigned last_col, bool low);  //associates a multigrades to this xi entry
        //the "low" argument is true if this multigrade is for low_simplices, and false if it is for high_simplices
};

//// sparse matrix to store the set U of support points of the multi-graded Betti numbers
class xiSupportMatrix
{
    public:
        xiSupportMatrix(unsigned width, unsigned height);
        ~xiSupportMatrix();

        void fill(MultiBetti& mb, std::vector<xiPoint>& xi_pts); //stores xi support points from MultiBetti in the xiSupportMatrix and in the supplied vector

        xiMatrixEntry* get_row(unsigned r); //gets a pointer to the leftmost entry in row r; returns NULL if row r is empty
        xiMatrixEntry* get_col(unsigned c); //gets a pointer to the top entry in column c; returns NULL if column c is empty
        xiMatrixEntry* get_infinity();      //gets a pointer to the infinity entry


    private:
        std::vector<xiMatrixEntry*> columns;
        std::vector<xiMatrixEntry*> rows;
        xiMatrixEntry infinity;

        ///TODO: also need lists of multigrades at each row and column, to support lazy updates

};

#endif // XI_SUPPORT_MATRIX_H
