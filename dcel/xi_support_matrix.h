#ifndef XI_SUPPORT_MATRIX_H
#define XI_SUPPORT_MATRIX_H


#include <vector>
#include <list>

#include "../math/multi_betti.h"

#include "xi_point.h"
struct Multigrade;



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

    ///TODO: THINK ABOUT THE FOLLOWING VARIABLES -- IS THERE A BETTER WAY TO STORE THIS INFO?
    unsigned low_count;     //number of columns in matrix of simplices of lower dimension that are mapped to this xiMatrixEntry (does not depend on whether this entry is the head of an equivalence class)
    unsigned high_count;    //number of columns in matrix of simplices of higher dimension that are mapped to this xiMatrixEntry (does not depend on whether this entry is the head of an equivalence class)

    //DEFINITION: this entry is the "head" of its equivalence class if it is the rightmost entry in a horizontal equivalence class or the topmost entry in a vertical equivalence class
    //  NOTE: this entry is the head of its class iff low_class_size is nonnegative
    int low_class_size;     //if(head of class), then low_class_size stores the TOTAL number of simplices of lower dimension that are mapped to this equivalence class; otherwise low_class_size is -1
    int high_class_size;    //if(head of class), then high_class_size stores the TOTAL number of simplices of higher dimension that are mapped to this equivalence class; otherwise high_class_size is arbitrary and UNRELIABLE
    int low_index;     //if(head_of_class) then low_index is the index of rightmost column in matrix of simplices of lower dimension that is mapped to this equivalence class; otherwise, low_index is arbitrary and UNRELIABLE
    int high_index;    //if(head_of_class) then high_index is the index of rightmost column in matrix of simplices of higher dimension that is mapped to this equivalence class; otherwise, low_index is arbitrary and UNRELIABLE
        //NOTE: if there are no low (resp. high) columns mapped to this xiMatrixEntry, then low_index (resp. high_index) is the index of the column just left of where such columns would appear (could be -1)

    xiMatrixEntry();    //empty constructor, e.g. for the entry representing infinity
    xiMatrixEntry(unsigned x, unsigned y, unsigned i, xiMatrixEntry* d, xiMatrixEntry* l);  //regular constructor

    void add_multigrade(unsigned x, unsigned y, unsigned num_cols, int index, bool low);  //associates a (new) multigrades to this xi entry
        //the "low" argument is true if this multigrade is for low_simplices, and false if it is for high_simplices

    void insert_multigrade(Multigrade* mg, bool low);  //inserts a Multigrade at the beginning of the list for the given dimension; does not update column counts!
};


//// each node in the sparse matrix maintains two lists of multigrades
struct Multigrade
{
    unsigned x;     //x-coordinate of this multigrade
    unsigned y;     //y-coordinate of this multigrade

    unsigned num_cols; //number of columns (i.e. simplices) at this multigrade
    int simplex_index; //last dim_index of the simplices at this multigrade; necessary so that we can build the boundary matrix

    xiMatrixEntry* xi_entry; //pointer to the entry in the xiSupportMatrix to which this multigrade is assocated

    Multigrade(unsigned x, unsigned y, unsigned num_cols, int simplex_index, xiMatrixEntry* xi);   //constructor
};


//// sparse matrix to store the set U of support points of the multi-graded Betti numbers
class xiSupportMatrix
{
    public:
        xiSupportMatrix(unsigned width, unsigned height);   //constructor
        ~xiSupportMatrix();     //destructor

        void fill(MultiBetti& mb, std::vector<xiPoint>& xi_pts); //stores xi support points from MultiBetti in the xiSupportMatrix and in the supplied vector        

        xiMatrixEntry* get_row(unsigned r); //gets a pointer to the rightmost entry in row r; returns NULL if row r is empty
        xiMatrixEntry* get_col(unsigned c); //gets a pointer to the top entry in column c; returns NULL if column c is empty
        xiMatrixEntry* get_infinity();      //gets a pointer to the infinity entry

        unsigned height();  //retuns the number of rows;

    private:
        std::vector<xiMatrixEntry*> columns;
        std::vector<xiMatrixEntry*> rows;
        xiMatrixEntry infinity;

        ///TODO: also need lists of multigrades at each row and column, to support lazy updates

};

#endif // XI_SUPPORT_MATRIX_H
