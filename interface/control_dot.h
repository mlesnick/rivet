#ifndef CONTROL_DOT_H
#define CONTROL_DOT_H


#include <QPainter>
#include <QGraphicsItem>

#include "slice_diagram.h"
class SliceDiagram;

#include "slice_line.h"
class SliceLine;


class ControlDot : public QGraphicsItem
{
public:
//    ControlDot(SliceDiagram* sd,...); //constructs a control dot that defaults to left-bottom
    ControlDot(SliceLine* line, bool left_bottom);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void set_position(const QPointF &newpos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    SliceLine* slice_line;
    QGraphicsTextItem* coords;

    bool pressed;
    bool left_bottom;   //TRUE if this is a left-bottom control dot, FALSE if this is a right-top control dot
    bool update_lock;   //TRUE when the dot is being moved as result of external input; to avoid update loops

};

#endif // CONTROL_DOT_H
