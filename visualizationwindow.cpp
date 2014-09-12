#include "visualizationwindow.h"
#include "ui_visualizationwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include "interface/input_manager.h"
#include "math/simplex_tree.h"
#include "math/multi_betti.h"
#include "dcel/lcm.h"
#include "dcel/dcel.h"
#include "dcel/mesh.h"
#include "dcel/cell_persistence_data.h"
#include "math/persistence_data.h"

#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;


VisualizationWindow::VisualizationWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VisualizationWindow),
    verbosity(5),
    slice_diagram(NULL), slice_update_lock(false),
    p_diagram(NULL), persistence_diagram_drawn(false)
{
    ui->setupUi(this);

    //set up the slice diagram scene
    sliceScene = new QGraphicsScene(this);
    ui->sliceView->setScene(sliceScene);
//    ui->sliceView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->sliceView->scale(1,-1);
    ui->sliceView->setRenderHint(QPainter::Antialiasing);

    //set up the persistence diagram scene
    pdScene = new QGraphicsScene(this);
    ui->pdView->setScene(pdScene);
    ui->pdView->scale(1,-1);
    ui->pdView->setRenderHint(QPainter::Antialiasing);
}

VisualizationWindow::~VisualizationWindow()
{
    delete ui;
}

//sets the name of the data file
void VisualizationWindow::setFile(QString name)
{
    fileName = name;
    ui->statusBar->showMessage("file: "+fileName);
}

//sets parameters for the computation
void VisualizationWindow::setComputationParameters(int hom_dim, unsigned num_x_bins, unsigned num_y_bins, QString x_text, QString y_text)
{
    dim = hom_dim;
    x_bins = num_x_bins;
    y_bins = num_y_bins;
    x_label = x_text;
    y_label = y_text;
}

void VisualizationWindow::on_angleDoubleSpinBox_valueChanged(double angle)
{
//    qDebug() << "angleDoubleSpinBox_valueChanged(); angle: " << angle << "; slice_update_lock: " << slice_update_lock;

    if(!slice_update_lock)
        slice_diagram->update_line(angle, ui->offsetSpinBox->value());

    if(persistence_diagram_drawn)
        update_persistence_diagram();
}

void VisualizationWindow::on_offsetSpinBox_valueChanged(double offset)
{
//    qDebug() << "offsetSpinBox_valueChanged(); offset: " << offset << "; slice_update_lock: " << slice_update_lock;

    if(!slice_update_lock)
        slice_diagram->update_line(ui->angleDoubleSpinBox->value(), offset);

    if(persistence_diagram_drawn)
        update_persistence_diagram();
}

void VisualizationWindow::on_normCoordCheckBox_clicked(bool checked)
{
    if(slice_diagram != NULL)
    {
        slice_diagram->set_normalized_coords(checked);
        slice_diagram->resize_diagram();
        if(p_diagram != NULL)
            p_diagram->resize_diagram(slice_diagram->get_slice_length(), slice_diagram->get_pd_scale());
    }
}

void VisualizationWindow::on_barcodeCheckBox_clicked(bool checked)
{
    if(slice_diagram != NULL)
    {
        slice_diagram->toggle_barcode(checked);
    }
}


//void VisualizationWindow::on_computeButton_clicked() //read the file and do the persistent homology computation
void VisualizationWindow::compute()
{
    //start the input manager
    im = new InputManager(dim, verbosity);
    const char* filestr = fileName.toStdString().data();
    im->start(filestr, x_bins, y_bins);

    //get the bifiltration
    bifiltration = im->get_bifiltration();

    //get data extents
    double data_xmin = bifiltration->grade_x_value(0);
    double data_xmax = bifiltration->grade_x_value(bifiltration->num_x_grades() - 1);
    double data_ymin = bifiltration->grade_y_value(0);
    double data_ymax = bifiltration->grade_y_value(bifiltration->num_y_grades() - 1);

    //print bifiltration statistics
    std::cout << "\nBIFILTRATION:\n";
    std::cout << "   Number of simplices of dimension " << dim << ": " << bifiltration->get_size(dim) << "\n";
    std::cout << "   Number of simplices of dimension " << (dim+1) << ": " << bifiltration->get_size(dim+1) << "\n";
    std::cout << "   Number of x-grades: " << bifiltration->num_x_grades() << "; values " << data_xmin << " to " << data_xmax << "\n";
    std::cout << "   Number of y-grades: " << bifiltration->num_y_grades() << "; values " << data_ymin << " to " << data_ymax << "\n";
    std::cout << "\n";

    //compute xi_0 and xi_1 at all multi-grades
    if(verbosity >= 2) { std::cout << "COMPUTING xi_0 AND xi_1 FOR HOMOLOGY DIMENSION " << dim << ":\n"; }
    MultiBetti mb(bifiltration, dim, verbosity);

    //start timer
    ptime time_xi_start(microsec_clock::local_time());

    //compute
    mb.compute_fast();

    //stop timer
    ptime time_xi_end(microsec_clock::local_time());
    time_duration duration_xi(time_xi_end - time_xi_start);

    //print computation time
    std::cout << "   xi_i computation took " << duration_xi << "\n";

    //initialize the slice diagram
    slice_diagram = new SliceDiagram(sliceScene, this, data_xmin, data_xmax, data_ymin, data_ymax, ui->normCoordCheckBox->isChecked());


    //find all support points of xi_0 and xi_1
    for(int i=0; i < bifiltration->num_x_grades(); i++)
    {
        for(int j=0; j < bifiltration->num_y_grades(); j++)
        {
            if(mb.xi0(i,j) != 0 || mb.xi1(i,j) != 0)
            {
                xi_support.push_back(std::pair<int,int>(i,j));
                slice_diagram->add_point(bifiltration->grade_x_value(i), bifiltration->grade_y_value(j), mb.xi0(i,j), mb.xi1(i,j));
            }
        }
    }

    //print support points of xi_0 and xi_1
    if(verbosity >= 2)
    {
        std::cout << "  SUPPORT POINTS OF xi_0 AND xi_1: ";
        for(unsigned i=0; i<xi_support.size(); i++)
            std::cout << "(" << xi_support[i].first << "," << xi_support[i].second << "), ";
        std::cout << "\n";
    }

    ui->statusBar->showMessage("computed xi support points");

    //draw slice diagram
    slice_diagram->create_diagram(x_label, y_label);
        //slice diagram automatically updates angle box and offset box to default values

    //update offset extents   //TODO: FIX THIS!!!
    ui->offsetSpinBox->setMinimum(-1*data_xmax);
    ui->offsetSpinBox->setMaximum(data_ymax);

    //find LCMs, build decomposition of the affine Grassmannian                             TODO: THIS CAN BE IMPROVED
    if(verbosity >= 2) { std::cout << "CALCULATING LCMs AND DECOMPOSING THE STRIP:\n"; }
    dcel = new Mesh(verbosity);

    for(unsigned i=0; i<xi_support.size(); i++)
    {
        for(unsigned j=i+1; j<xi_support.size(); j++)
        {
            int ax = xi_support[i].first, ay = xi_support[i].second;
            int bx = xi_support[j].first, by = xi_support[j].second;

            if((ax - bx)*(ay - by) <= 0)	//then the support points are (at least weakly) incomparable, so we have found an LCM
            {
                double t = bifiltration->grade_x_value(std::max(ax,bx));
                double d = bifiltration->grade_y_value(std::max(ay,by));

                if(verbosity >= 6) { std::cout << "  LCM at (" << std::max(ax,bx) << "," << std::max(ay,by) << ") => (" << t << "," << d << ") determined by (" << ax << "," << ay << ") and (" << bx << "," << by << ")\n"; }

                dcel->add_lcm(t, d);
            }
        }
    }

    //start timer
    ptime time_dcel_start(microsec_clock::local_time());

    //build the DCEL arrangement
    dcel->build_arrangement();

    //stop timer
    ptime time_dcel_end(microsec_clock::local_time());
    time_duration duration_dcel(time_dcel_end - time_dcel_start);

    //print DCEL info
    std::cout << "   building the arrangement took " << duration_dcel << "\n";
    dcel->print_stats();

//    //print the DCEL arrangement
//    if(verbosity >= 4)
//    {
//        std::cout << "DCEL ARRANGEMENT:\n";
//        dcel->print();
//    }

   //do the persistence computations in each cell
    if(verbosity >= 2) { std::cout << "COMPUTING PERSISTENCE DATA FOR EACH CELL:\n"; }

    //start timer
    ptime time_pdata_start(microsec_clock::local_time());

    //do computations
    dcel->build_persistence_data(xi_support, bifiltration, dim);

    //stop timer
    ptime time_pdata_end(microsec_clock::local_time());
    time_duration duration_pdata(time_pdata_end - time_pdata_start);

//*    ui->statusBar->showMessage("computed persistence data");
    std::cout << "   computing persistence data took " << duration_pdata << "\n";
    if(verbosity >= 2) { std::cout << "DATA COMPUTED; READY FOR INTERACTIVITY.\n"; }

//    qDebug() << "zero: " << slice_diagram->get_zero();

    //draw persistence diagram
    p_diagram = new PersistenceDiagram(pdScene, this, &fileName, dim);
    p_diagram->resize_diagram(slice_diagram->get_slice_length(), slice_diagram->get_pd_scale());

    double radians = (ui->angleDoubleSpinBox->value())*3.14159265359/180;   //convert to radians
    PersistenceData* pdata = dcel->get_persistence_data(radians, ui->offsetSpinBox->value(), xi_support, bifiltration);

    //TESTING
    std::cout<< "PERSISTENCE CYCLES: ";
    for(std::set< double >::iterator it = pdata->get_cycles()->begin(); it != pdata->get_cycles()->end(); ++it)
        std::cout << *it << ", ";
    std::cout << "\n";
    std::cout<< "PERSISTENCE PAIRS: ";
    for(std::set< std::pair<double,double> >::iterator it = pdata->get_pairs()->begin(); it != pdata->get_pairs()->end(); ++it)
        std::cout << "(" << it->first << ", " << it->second << ") ";
    std::cout << "\n";

    p_diagram->draw_points(slice_diagram->get_zero(), pdata);
    slice_diagram->draw_barcode(pdata, ui->barcodeCheckBox->isChecked());

//    p_diagram->draw_points(slice_diagram->get_zero(), pdata->get_pairs(), pdata->get_cycles());        //I should be able to send pdata to draw_points, but I can't resolve the "multiple definition errors" that occur
//    slice_diagram->draw_bars(pdata->get_pairs(), pdata->get_cycles());  //again, I would rather send pdata here...

    delete pdata;

    persistence_diagram_drawn = true;
    ui->statusBar->showMessage("persistence diagram drawn");
//*/


}//end on_computeButton_clicked()

void VisualizationWindow::on_xi0CheckBox_toggled(bool checked)
{
    slice_diagram->toggle_xi0_points(checked);
}

void VisualizationWindow::on_xi1CheckBox_toggled(bool checked)
{
    slice_diagram->toggle_xi1_points(checked);
}

void VisualizationWindow::update_persistence_diagram()
{
    double radians = (ui->angleDoubleSpinBox->value())*3.14159265359/180;   //convert to radians
    PersistenceData* pdata = dcel->get_persistence_data(radians, ui->offsetSpinBox->value(), xi_support, bifiltration);

    //TESTING
    std::cout<< "PERSISTENCE CYCLES: ";
    for(std::set< double >::iterator it = pdata->get_cycles()->begin(); it != pdata->get_cycles()->end(); ++it)
        std::cout << *it << ", ";
    std::cout << "\n";
    std::cout<< "PERSISTENCE PAIRS: ";
    for(std::set< std::pair<double,double> >::iterator it = pdata->get_pairs()->begin(); it != pdata->get_pairs()->end(); ++it)
        std::cout << "(" << it->first << ", " << it->second << ") ";
    std::cout << "\n";

    p_diagram->update_diagram(slice_diagram->get_slice_length(), slice_diagram->get_pd_scale(), slice_diagram->get_zero(), pdata);
    slice_diagram->update_barcode(pdata, ui->barcodeCheckBox->isChecked());

    delete pdata;
}


void VisualizationWindow::set_line_parameters(double angle, double offset)
{
    slice_update_lock = true;

//    qDebug() << "  set_line_parameters: angle = " << angle << "; offset = " << offset;

    ui->angleDoubleSpinBox->setValue(angle);
    ui->offsetSpinBox->setValue(offset);

    slice_update_lock = false;
}

void VisualizationWindow::select_bar(unsigned index)
{
    slice_diagram->select_bar(index);
}

void VisualizationWindow::deselect_bar()
{
    slice_diagram->deselect_bar(false);
}

void VisualizationWindow::select_dot(unsigned index)
{
    p_diagram->select_dot(index);
}

void VisualizationWindow::deselect_dot()
{
    p_diagram->deselect_dot(false);
}

void VisualizationWindow::resizeEvent(QResizeEvent* event)
{
//    qDebug() << "resize event! slice_diagrma = " << slice_diagram;
    if(slice_diagram != NULL)
    {
        slice_diagram->resize_diagram();

        if(p_diagram != NULL)
            p_diagram->resize_diagram(slice_diagram->get_slice_length(), slice_diagram->get_pd_scale());
    }
}
