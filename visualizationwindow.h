#ifndef VISUALIZATIONWINDOW_H
#define VISUALIZATIONWINDOW_H

#include <QMainWindow>
#include <QtGui>

class DataSelectDialog;
#include "dataselectdialog.h"
#include "dcel/mesh.h"
#include "interface/barcode.h"

#include <boost/multiprecision/cpp_int.hpp>
typedef boost::multiprecision::cpp_rational exact;

#include "computationthread.h"
#include "interface/slice_diagram.h"
class SliceDiagram;
#include "interface/persistence_diagram.h"
class PersistenceDiagram;
class Mesh;





namespace Ui {
class VisualizationWindow;
}


class VisualizationWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit VisualizationWindow(QWidget *parent = 0);
    ~VisualizationWindow();

//DEPRECATED    void setFile(QString name); //sets the name of the data file
//DEPRECATED    void setComputationParameters(int hom_dim, unsigned num_x_bins, unsigned num_y_bins, QString x_text, QString y_text);    //sets parameters for the computation
    void start_computation(); //begins the computation pipeline

    void set_line_parameters(double angle, double offset);

    void select_bar(unsigned index);
    void deselect_bar();
    void select_dot(unsigned index);
    void deselect_dot();

protected:
    void showEvent(QShowEvent* event);      //shows the DataSelectDialog and blocks until it is closed
    void resizeEvent(QResizeEvent*);
    
private slots:
    void on_angleDoubleSpinBox_valueChanged(double angle);

    void on_offsetSpinBox_valueChanged(double arg1);

    void on_xi0CheckBox_toggled(bool checked);

    void on_xi1CheckBox_toggled(bool checked);

    void on_normCoordCheckBox_clicked(bool checked);

    void on_barcodeCheckBox_clicked(bool checked);

private:
    Ui::VisualizationWindow *ui;

    //data items
    const int verbosity;
    const double INFTY;

    InputParameters input_params;     //parameters set by the user via the DataSelectDialog
    DataSelectDialog ds_dialog;       //dialog box that gets the input parameters

    std::vector<double> x_grades;     //floating-point x-coordinates of the grades
    std::vector<double> y_grades;     //floating-point y-coordinates of the grades
    std::vector<xiPoint> xi_support;  //stores discrete coordinates of xi support points, with multiplicities

    Mesh* arrangement; //pointer to the DCEL arrangement

    //computation items
    ComputationThread cthread;


    //items for slice diagram
    QGraphicsScene* sliceScene;
    SliceDiagram* slice_diagram;
    bool slice_update_lock;

    //items for persistence diagram
    QGraphicsScene* pdScene;
    PersistenceDiagram* p_diagram;
    bool persistence_diagram_drawn;

    void update_persistence_diagram();

    Barcode* rescale_barcode_template(BarcodeTemplate &dbc, double angle, double offset);
    double project(xiPoint& pt, double angle, double offset);
};

#endif // VISUALIZATIONWINDOW_H
