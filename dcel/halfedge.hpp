/**
 * \class	Halfedge
 * \brief	Stores one halfedge in the DCEL decomposition of the affine Grassmannian; two halfedges make one edge.
 * \author	Matthew L. Wright
 * \date	March 2014
 */

#ifndef __DCEL_Halfedge_H__
#define __DCEL_Halfedge_H__

//includes????  namespace????
//#include <utility>

class LCM;
class Vertex;
class Face;

class Halfedge
{
	public:
		Halfedge(Vertex* v, LCM* p);	//constructor, requires origin vertex as well as LCM corresponding to this halfedge (LCM never changes)
		Halfedge();			//constructor for a null Halfedge
		
		void set_twin(Halfedge* e);	//set the twin halfedge
		Halfedge* get_twin() const;		//get the twin halfedge
		
		void set_next(Halfedge* e);	//set the next halfedge in the boundary of the face that this halfedge borders
		Halfedge* get_next() const;		//get the next halfedge
		
		void set_prev(Halfedge* e);	//set the previous halfedge in the boundary of the face that this halfedge borders
		Halfedge* get_prev() const;		//get the previous halfedge
		
		//void set_origin(Vertex* v); -- I don't think we will ever change the origin vertex
		Vertex* get_origin() const;		//get the origin vertex
		
		void set_face(Face* f);		//set the face that this halfedge borders
		Face* get_face() const;		//get the face that this halfedge borders
		
		LCM* get_LCM() const;			//get the LCM coordinates
		
		friend std::ostream& operator<<(std::ostream& os, const Halfedge& e);	//for printing the halfedge
		
	private:
		Vertex* origin;		//pointer to the vertex from which this halfedge originates
		Halfedge* twin;		//pointer to the halfedge that, together with this halfedge, make one edge
		Halfedge* next;		//pointer to the next halfedge around the boundary of the face to the right of this halfedge
		Halfedge* prev;		//pointer to the previous halfedge around the boundary of the face to the right of this halfedge
		Face* face;		//pointer to the face to the right of this halfedge
		LCM* lcm;		//stores the (time, dist)-coordinates (in persistence space) of the LCM corresponding to this halfedge
	
};//end class Halfedge


////////// implementation //////////

Halfedge::Halfedge(Vertex* v, LCM* p) : 
	origin(v),
	twin(NULL),
	next(NULL),
	prev(NULL),
	face(NULL),
	lcm(p)
{ }

Halfedge::Halfedge() : 
	origin(NULL),
	twin(NULL),
	next(NULL),
	prev(NULL),
	face(NULL),
	lcm(NULL)
{ }

void Halfedge::set_twin(Halfedge* e)
{
	twin = e;
}

Halfedge* Halfedge::get_twin() const
{
	return twin;
}

void Halfedge::set_next(Halfedge* e)
{
	next = e;
}

Halfedge* Halfedge::get_next() const
{
	return next;
}

void Halfedge::set_prev(Halfedge* e)
{
	prev = e;
}

Halfedge* Halfedge::get_prev() const
{
	return prev;
}

Vertex* Halfedge::get_origin() const
{
	return origin;
}

void Halfedge::set_face(Face* f)
{
	face = f;
}

Face* Halfedge::get_face() const
{
	return face;
}

LCM* Halfedge::get_LCM() const
{
	return lcm;
}

std::ostream& operator<<(std::ostream& os, const Halfedge& e)
{
	Halfedge* t = e.twin;
	os << *(e.origin) << "--" << *(t->origin) << "; ";
	if(e.lcm == NULL)
		os << "LCM null; ";
	else
		os << "LCM coords (" << e.lcm->get_time() << ", " << e.lcm->get_dist() << "); ";
	os << "twin: " << (e.twin) << "; next: " << (e.next) << "; prev: " << (e.prev) << "; face: " << (e.face);
	return os;
}


#endif // __DCEL_Halfedge_H__

