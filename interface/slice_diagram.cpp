#include "slice_diagram.h"

#include "barcode.h"
#include "config_parameters.h"
#include "control_dot.h"
#include "persistence_bar.h"
#include "slice_line.h"

#include <QDebug>
#include <QGraphicsView>

#include <limits>
#include <math.h>
#include <set>
#include <sstream>


SliceDiagram::SliceDiagram(ConfigParameters* params, QObject* parent) :
    QGraphicsScene(parent),
    config_params(params),
    dot_left(), dot_right(), slice_line(),
    selected(-1), NOT_SELECTED(-1),
    padding(20)
{ }

SliceDiagram::~SliceDiagram()
{
    ///TODO: IMPLEMENT THIS!!!
}

//receives an xi support point, which will be drawn when create_diagram() is called
void SliceDiagram::add_point(double x_coord, double y_coord, int xi0m, int xi1m)
{
    points.push_back(xiFloatingPoint(x_coord, y_coord, xi0m, xi1m));
}

//NOTE: create_diagram() simply creates all objects; resize_diagram() handles positioning of objects
void SliceDiagram::create_diagram(QString x_text, QString y_text, double xmin, double xmax, double ymin, double ymax, bool norm_coords)
{
    //set data-dependent parameters
    data_xmin = xmin;
    data_xmax = xmax;
    data_ymin = ymin;
    data_ymax = ymax;
    normalized_coords = norm_coords;
    data_infty = 4*(xmax - xmin + ymax - ymin);

    //pens and brushes
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    QBrush xi0brush(config_params->xi0color);
    QBrush xi1brush(config_params->xi1color);
    QPen grayPen(Qt::gray);
    QPen highlighter(QBrush(config_params->persistenceHighlightColor), 6);

    //draw bounds
    gray_line_vertical = addLine(QLineF(), grayPen); //(diagram_width, 0, diagram_width, diagram_height, grayPen);
    gray_line_horizontal = addLine(QLineF(), grayPen); //0, diagram_height, diagram_width, diagram_height, grayPen);
    control_rect = addRect(QRectF(), blackPen);  //0,0,diagram_width + padding,diagram_height + padding, blackPen);

    //draw labels
    std::ostringstream s_xmin;
    s_xmin.precision(4);
    s_xmin << data_xmin;
    data_xmin_text = addSimpleText(QString(s_xmin.str().data()));
    data_xmin_text->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    std::ostringstream s_xmax;
    s_xmax.precision(4);
    s_xmax << data_xmax;
    data_xmax_text = addSimpleText(QString(s_xmax.str().data()));
    data_xmax_text->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    std::ostringstream s_ymin;
    s_ymin.precision(4);
    s_ymin << data_ymin;
    data_ymin_text = addSimpleText(QString(s_ymin.str().data()));
    data_ymin_text->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    std::ostringstream s_ymax;
    s_ymax.precision(4);
    s_ymax << data_ymax;
    data_ymax_text = addSimpleText(QString(s_ymax.str().data()));
    data_ymax_text->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    x_label = addSimpleText(x_text);
    x_label->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    y_label = addSimpleText(y_text);
    y_label->setTransform(QTransform(0, 1, 1, 0, 0, 0));

    //draw points
    for(unsigned i = 0; i < points.size(); i++)
    {
        if(points[i].zero > 0)  //then draw a xi0 disk
        {
            QGraphicsEllipseItem* item = addEllipse(QRectF(), Qt::NoPen, xi0brush);
            xi0_dots.push_back(item);
        }
        if(points[i].one > 0)  //then draw a xi1 disk
        {
            QGraphicsEllipseItem* item = addEllipse(QRectF(), Qt::NoPen, xi1brush);
            xi1_dots.push_back(item);
        }
    }

    //add control objects
    line_vert = false;  //IS IT POSSIBLE THAT THE INITIAL LINE COULD BE VERTICAL???????????????????????????????????
    line_slope = (data_ymax - data_ymin)/(data_xmax - data_xmin);    //slope in data units
    line_pos = 0;   //start the line at the lower left corner of the box

    slice_line = new SliceLine(this, config_params);
    addItem(slice_line);

    dot_left = new ControlDot(slice_line, true, config_params);
    addItem(dot_left);

    dot_right = new ControlDot(slice_line, false, config_params);
    addItem(dot_right);

    slice_line->setDots(dot_left, dot_right);

    //add highlight line object, which is hidden until it is requested
    highlight_line = addLine(0, 0, 1, 1, highlighter);
    highlight_line->hide();

    //fit scene to view -- THIS SETS POSITIONS OF ALL OBJECTS CREATED ABOVE!!!
    resize_diagram();

    //update angle and offset boxes in VisualizationWindow
    update_window_controls();
}//end create_diagram()

//resizes diagram to fill the QGraphicsView
void SliceDiagram::resize_diagram()
{
    //parameters
    int scene_padding = 30; //pixels
    int text_padding = 5;   //pixels

    //get dimensions of the QGraphicsView
    QList<QGraphicsView*> view_list = views();
    int view_width = view_list[0]->width();
    int view_height = view_list[0]->height();

    //determine scale
    double left_text_width = std::max(data_ymin_text->boundingRect().width(), data_ymax_text->boundingRect().width());
    double diagram_max_width = view_width - padding - 2*scene_padding - text_padding - left_text_width;
    double lower_text_height = std::max(data_xmin_text->boundingRect().height(), data_xmax_text->boundingRect().height());
    double diagram_max_height = view_height - padding - 2*scene_padding - text_padding - lower_text_height;

    if(data_xmax > data_xmin)
        scale_x = diagram_max_width/(data_xmax - data_xmin);
    else    //then there is only one x-grade
        scale_x = 1;                 //IS THIS WHAT WE WANT???

    if(data_ymax > data_ymin)
        scale_y = diagram_max_height/(data_ymax - data_ymin);
    else    //then there is only one x-grade
        scale_y = 1;                 //IS THIS WHAT WE WANT???

    if(!normalized_coords)  //then we want scale_x and scale_y to be the same (choose the smaller of the two)
    {
        if(scale_y < scale_x)
            scale_x = scale_y;
        else
            scale_y = scale_x;
    }

    //determine diagram size
    diagram_width = scale_x*(data_xmax - data_xmin);  //units: pixels
    diagram_height = scale_y*(data_ymax - data_ymin); //units: pixels

    //reposition reference objects
    control_rect->setRect(0, 0, diagram_width + padding, diagram_height + padding);
    gray_line_vertical->setLine(diagram_width, 0, diagram_width, diagram_height);
    gray_line_horizontal->setLine(0, diagram_height, diagram_width, diagram_height);

    data_xmin_text->setPos(data_xmin_text->boundingRect().width()/(-2), -1*text_padding);
    data_xmax_text->setPos(diagram_width - data_xmax_text->boundingRect().width()/2, -1*text_padding);
    data_ymin_text->setPos(-1*text_padding - data_ymin_text->boundingRect().width(), data_ymin_text->boundingRect().height()/2);
    data_ymax_text->setPos(-1*text_padding - data_ymax_text->boundingRect().width(), diagram_height + data_ymax_text->boundingRect().height()/2);

    x_label->setPos((diagram_width - x_label->boundingRect().width())/2, -1*text_padding);
    y_label->setPos(-1*text_padding - y_label->boundingRect().height(), (diagram_height - y_label->boundingRect().width())/2);

    //reposition points
    redraw_dots();

    //reposition slice line
    slice_line->update_bounds(diagram_width, diagram_height, padding);

    double x = 0, y = 0;
    if(line_pos < 0)    //then left-bottom endpoint is along bottom edge of box
        x = -1*line_pos*diagram_width;
    else                //then left-bottom endpoint is along left edge of box
        y = line_pos*diagram_height;
    slice_line->update_position(x, y, line_vert, line_slope*scale_y/scale_x);

    //reposition bars
    double infty = get_zero() + data_infty;
    unsigned count = 1;
    for(unsigned i = 0; i < bars.size(); i++)
    {
        for(std::list<PersistenceBar*>::iterator it = bars[i].begin(); it != bars[i].end(); ++it)
        {
            double start = (*it)->get_start();
            double end = (*it)->get_end();
            if(end == std::numeric_limits<double>::infinity())
                end = infty;

            std::pair<double,double> p1 = compute_endpoint(start, count);
            std::pair<double,double> p2 = compute_endpoint(end, count);
            (*it)->set_line(p1.first, p1.second, p2.first, p2.second);
            count++;
        }
    }

    //reposition highlighting
    if(selected != NOT_SELECTED)
        update_highlight();

    //set scene rectangle (necessary to prevent auto-scrolling)
    double scene_rect_x = -left_text_width - text_padding;
    double scene_rect_y = -lower_text_height - text_padding;
    double scene_rect_w = diagram_width + padding + text_padding + left_text_width;
    double scene_rect_h = diagram_height + padding + text_padding + lower_text_height;
    setSceneRect(scene_rect_x, scene_rect_y, scene_rect_w, scene_rect_h);
}//end resize_diagram()

//redraws the support points of the multigraded Betti numbers
void SliceDiagram::redraw_dots()
{
    //NOTE: this should be fine, but if it is too slow, we could store the radius of each dot so that we don't have to compute it on each resize
    std::vector<QGraphicsEllipseItem*>::iterator it0 = xi0_dots.begin();
    std::vector<QGraphicsEllipseItem*>::iterator it1 = xi1_dots.begin();
    for(unsigned i = 0; i < points.size(); i++)
    {
        if(points[i].zero > 0)
        {
            double radius = round( config_params->bettiDotRadius*sqrt(points[i].zero) );
            (*it0)->setRect((points[i].x - data_xmin)*scale_x - radius, (points[i].y - data_ymin)*scale_y - radius, 2*radius, 2*radius);
            ++it0;
        }
        if(points[i].one > 0)  //then draw a red disk
        {
            double radius = round( config_params->bettiDotRadius*sqrt(points[i].one) );
            (*it1)->setRect((points[i].x - data_xmin)*scale_x - radius, (points[i].y - data_ymin)*scale_y - radius, 2*radius, 2*radius);
            ++it1;
        }
    }
}//end redraw_dots()

//updates the diagram after a change in configuration parameters
void SliceDiagram::receive_parameter_change()
{
    //update colors of the xi dots (necessary because I didn't override their paint() function)
    QBrush xi0brush(config_params->xi0color);
    QBrush xi1brush(config_params->xi1color);
    for(std::vector<QGraphicsEllipseItem*>::iterator it = xi0_dots.begin(); it != xi0_dots.end(); ++it)
        (*it)->setBrush(xi0brush);
    for(std::vector<QGraphicsEllipseItem*>::iterator it = xi1_dots.begin(); it != xi1_dots.end(); ++it)
        (*it)->setBrush(xi1brush);

    //update sizes of the xi dots
    redraw_dots();

    //update the slice line highlight
    QPen highlighter(QBrush(config_params->persistenceHighlightColor), 6);
    highlight_line->setPen(highlighter);
}//end update_diagram()

//updates the line, in response to a change in the controls in the VisualizationWindow
//NOTE: angle is in DEGREES
void SliceDiagram::update_line(double angle, double offset)
{
    if(angle == 90)     //handle vertical line
    {
        //update SliceDiagram data values
        line_vert = true;
        line_pos = offset/(data_xmax - data_xmin);  //relative units

        //update the SliceLine
        int xpos = (-1*offset - data_xmin)*scale_x; //pixel units
        slice_line->update_position(xpos, 0, true, 0);
    }
    else if(angle == 0) //handle horizontal line
    {
        //update SliceDiagram data values
        line_vert = false;
        line_slope = 0;
        line_pos = offset/(data_ymax - data_ymin);  //relative units

        //update the SliceLine
        int ypos = (offset - data_ymin)*scale_y;    //pixel units
        slice_line->update_position(0, ypos, false, 0);
    }
    else    //handle non-vertical and non-horizontal line
    {
        //update SliceDiagram data values
        line_vert = false;
        double radians = angle*3.14159265/180;
        line_slope = tan(radians);

        //update line_pos and the SliceLine
        double y_coord = line_slope*data_xmin + offset/cos(radians); //y-coordinate of slice line at x=data_xmin; data units
        if(y_coord >= data_ymin)    //then slice line intersects left edge of box
        {
            line_pos = (y_coord - data_ymin)/(data_ymax - data_ymin);   //relative units
            slice_line->update_position(0, (y_coord - data_ymin)*scale_y, false, line_slope*scale_y/scale_x);
        }
        else    //then slice line intersects bottom of box
        {
            double x_coord = (data_ymin - offset/cos(radians))/line_slope;   //x-coordinate of slice line at y=data_ymin; data units
            line_pos = -1*(x_coord - data_xmin)/(data_xmax - data_xmin);   //relative units
            slice_line->update_position( (x_coord - data_xmin)*scale_x, 0, false, line_slope*scale_y/scale_x);
        }
    }

    highlight_line->hide(); //since the line has changed, the highlighting is no longer valid
}//end update_line()

//updates controls in the VisualizationWindow in response to a change in the line (also update SliceDiagram data values)
void SliceDiagram::update_window_controls()
{
    //update SliceDiagram data values
    line_vert = slice_line->is_vertical();
    line_slope = slice_line->get_slope()*scale_x/scale_y;   //convert pixel units to data units
    if(slice_line->pos().x() > 0)
        line_pos = -1*slice_line->pos().x()/diagram_width;
    else
        line_pos = slice_line->pos().y()/diagram_height;

    //update VisualizatoinWindow control objects
    //defaults for vertical line
    double angle = 90;
    double offset = -1*(slice_line->pos().x()/scale_x + data_xmin); //data units

    //handle non-vertical line
    if(!line_vert)
    {
        angle = atan(line_slope);   //radians

        double y_intercept = (slice_line->pos().y()/scale_y + data_ymin - line_slope * (slice_line->pos().x()/scale_x + data_xmin) );   //data units
        offset = cos(angle) * y_intercept;

        angle = angle*180/3.14159265;   //convert to degrees
    }

    //send updates
    emit set_line_control_elements(angle, offset);

    //since the line has changed, the highlighting is no longer valid
    highlight_line->hide();
}//end update_window_controls()

//draws the barcode parallel to the slice line
void SliceDiagram::draw_barcode(Barcode *bc, bool show)
{
    double zero_coord = get_zero();

    //draw bars
    unsigned num_bars = 1;
    unsigned index = 0;
    for(std::multiset<MultiBar>::iterator it = bc->begin(); it != bc->end(); ++it)
    {
        double start = it->birth - zero_coord;
        double end = it->death - zero_coord;
        bars.resize(bc->size());

        for(unsigned i=0; i < it->multiplicity; i++)
        {
            std::pair<double,double> p1 = compute_endpoint(start, num_bars);
            std::pair<double,double> p2 = compute_endpoint(end, num_bars);

            PersistenceBar* bar = new PersistenceBar(this, config_params, start, end, index);
            bar->set_line(p1.first, p1.second, p2.first, p2.second);
            bar->setVisible(show);
            addItem(bar);
            bars[index].push_back(bar);
            num_bars++;
        }
        index++;
    }
}//end draw_barcode()

//updates the barcode (e.g. after a change in the slice line)
//TODO: would it be better to move bars, instead of deleting and re-creating them?
void SliceDiagram::update_barcode(Barcode *bc, bool show)
{
    //remove old bars
    selected = -1;    //remove any current selection
    for(std::vector< std::list<PersistenceBar*> >::iterator it = bars.begin(); it != bars.end(); ++it)
    {
        while(!it->empty())
        {
            removeItem(it->back());
            delete it->back();
            it->pop_back();
        }
    }

    //draw new bars
    draw_barcode(bc, show);
}

//computes an endpoint of a bar in the barcode
std::pair<double,double> SliceDiagram::compute_endpoint(double coordinate, unsigned offset)
{
    //difference in offset between consecutive bars (pixel units)
    int step_size = 10;

    //handle infinity
    if(coordinate == std::numeric_limits<double>::infinity())
        coordinate = get_zero() + data_infty;

    //compute x and y relative to slice line (pixel units)
    double x = 0;
    double y = 0;
    if(line_vert)
    {
        y = coordinate*scale_y; //position along the line
        x = -1*(int)(step_size*offset);
    }
    else
    {
        //position along the line
        double angle = atan(line_slope);    //angle (data)      NOTE: it would be slightly more efficient to only compute this once per barcode update
        x = coordinate*cos(angle)*scale_x;
        y = coordinate*sin(angle)*scale_y;

        //offset from slice line
        double pixel_angle = atan(line_slope*scale_y/scale_x);  //angle (pixels)    NOTE: it would be slightly more efficient to only compute this once per barcode update
        x -= step_size*offset*sin(pixel_angle);
        y += step_size*offset*cos(pixel_angle);
    }

    //adjust for position of slice line
    x += dot_left->pos().x();
    y += dot_left->pos().y();

    return std::pair<double,double>(x,y);
}//end compute_endpoint()

//highlight the specified bar, selected in the slice diagram, and propagate to the persistence diagram
void SliceDiagram::select_bar(PersistenceBar* clicked)
{
    //remove old selection
    if(selected != NOT_SELECTED && clicked->get_index() != selected)
    {
        for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
            (*it)->deselect();
    }

    //remember current selection
    selected = clicked->get_index();

    //select all bars with the current index
    for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
        (*it)->select();

    //highlight part of slice line
    update_highlight();

    //highlight part of the persistence diagram
    emit persistence_bar_selected(clicked->get_index());
}//end select_bar()

//remove selection; if propagate, then deselect dot in the persistence diagram
void SliceDiagram::deselect_bar()
{
    //remove selection
    if(selected != NOT_SELECTED)
    {
        for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
            (*it)->deselect();
        selected = -1;
    }

    //remove highlighted portion of slice line
    highlight_line->hide();

    //remove highlighting from slice diagram
    emit persistence_bar_deselected();
}//end deselect_bar()

//highlight the specified class of bars, which has been selected externally
void SliceDiagram::receive_bar_selection(unsigned index)
{
    //remove old selection
    if(selected != NOT_SELECTED && index != selected)
    {
        for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
            (*it)->deselect();
    }

    //remember current selection
    selected = index;

    //select all bars with the current index
    for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
        (*it)->select();

    //highlight part of slice line
    update_highlight();
}//end receive_bar_selection()

//remove bar highlighting in response to external command
void SliceDiagram::receive_bar_deselection()
{
    //remove selection
    if(selected != NOT_SELECTED)
    {
        for(std::list<PersistenceBar*>::iterator it = bars[selected].begin(); it != bars[selected].end(); ++it)
            (*it)->deselect();
        selected = -1;
    }

    //remove highlighted portion of slice line
    highlight_line->hide();
}//end receive_bar_deselection()

//highlights part of the slice line
void SliceDiagram::update_highlight()
{
    if(selected != NOT_SELECTED)
    {
        PersistenceBar* selected_bar = bars[selected].front();
        double start = selected_bar->get_start();
        double end = selected_bar->get_end();
        if(end == std::numeric_limits<double>::infinity())
            end = get_zero() + data_infty;

        std::pair<double,double> p1 = compute_endpoint(start, 0);
        std::pair<double,double> p2 = compute_endpoint(end, 0);

        highlight_line->setLine(p1.first, p1.second, p2.first, p2.second);
        highlight_line->show();
    }
}//end update_highlight()

//if "show" is true, then xi_0 support points are drawn; otherwise, they are hidden
void SliceDiagram::toggle_xi0_points(bool show)
{
    for(std::vector<QGraphicsEllipseItem*>::iterator it = xi0_dots.begin(); it != xi0_dots.end(); ++it)
        (*it)->setVisible(show);
}

//if "show" is true, then xi_1 support points are drawn; otherwise, they are hidden
void SliceDiagram::toggle_xi1_points(bool show)
{
    for(std::vector<QGraphicsEllipseItem*>::iterator it = xi1_dots.begin(); it != xi1_dots.end(); ++it)
        (*it)->setVisible(show);
}

//if "show" is true, then barcode is drawn; otherwise, it is hidden
void SliceDiagram::toggle_barcode(bool show)
{
    for(std::vector< std::list<PersistenceBar*> >::iterator it1 = bars.begin(); it1 != bars.end(); ++it1)
        for(std::list<PersistenceBar*>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2)
            (*it2)->setVisible(show);
}

//sets normalized coordinates or default coordinates
void SliceDiagram::set_normalized_coords(bool toggle)
{
    normalized_coords = toggle;
}

//gets the length of the slice, for scaling the persistence diagram
double SliceDiagram::get_slice_length()
{
    double dx = slice_line->get_right_pt_x() - slice_line->pos().x();
    double dy = slice_line->get_right_pt_y() - slice_line->pos().y();

    return sqrt(dx*dx + dy*dy);
}

//gets the number of pixels per unit, for the persistence diagram
double SliceDiagram::get_pd_scale()
{
    double angle = atan(line_slope*scale_y/scale_x);    //line_slope is in data units, so first convert to pixel units
    double sine = sin(angle);
    double cosine = cos(angle);
    double denominator = sqrt(scale_x*scale_x*sine*sine + scale_y*scale_y*cosine*cosine);
    return scale_x*scale_y/denominator;
}

//gets the coordinate on the slice line which we consider "zero" for the persistence diagram        //TODO: IMPROVE!!!
double SliceDiagram::get_zero()
{
    if(slice_line->is_vertical())
        return data_ymin;
    else if(slice_line->get_slope() == 0)
        return data_xmin;
    else
    {
        double x0 = slice_line->pos().x()/scale_x + data_xmin;  //data units
        double y0 = slice_line->pos().y()/scale_y + data_ymin;  //data units

        double radians = atan(line_slope);
        double offset = cos(radians) * (y0 - tan(radians)*x0);
        double x1 = -1*offset * sin(radians);
        double y1 = offset * cos(radians);

//        qDebug() << "radians: " << radians << "; offset: " << offset << "; x1: " << x1 << "; y1: " << y1;

        return sqrt( (x0 - x1)*(x0 - x1) + (y0 - y1)*(y0 - y1) );
    }
}
