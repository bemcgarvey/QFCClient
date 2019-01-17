#include "qledwidget.h"
#include <QPainter>

QLEDWidget::QLEDWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    currentValue = 0;
}


void QLEDWidget::setValue(int newValue) {
    currentValue = newValue;
    update();
}

QSize QLEDWidget::sizeHint() const {
    QSize size(60, 35);
    return size;
}

void QLEDWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    if (this->isEnabled()) {
        painter.setOpacity(1.0);
    } else {
        painter.setOpacity(0.1);
    }
    QBrush brush(Qt::SolidPattern);
    painter.setBrush(painter.background());
    switch (currentValue) {
    case 0:
        painter.drawEllipse(0, 0, 20, 20);
        painter.drawEllipse(25, 0, 20, 20);
        painter.drawEllipse(50, 0, 20, 20);
        break;
    case 1:
        painter.drawEllipse(0, 0, 20, 20);
        brush.setColor(Qt::blue);
        painter.setBrush(brush);
        painter.drawEllipse(25, 0, 20, 20);
        painter.setBrush(painter.background());
        painter.drawEllipse(50, 0, 20, 20);
        break;
    case 2:
        painter.drawEllipse(0, 0, 20, 20);
        brush.setColor(Qt::blue);
        painter.setBrush(brush);
        painter.drawEllipse(25, 0, 20, 20);
        brush.setColor(Qt::green);
        painter.setBrush(brush);
        painter.drawEllipse(50, 0, 20, 20);
        break;
    case 3:
        painter.drawEllipse(0, 0, 20, 20);
        painter.drawEllipse(25, 0, 20, 20);
        brush.setColor(Qt::green);
        painter.setBrush(brush);
        painter.drawEllipse(50, 0, 20, 20);
        break;
    case 4:
        brush.setColor(Qt::yellow);
        painter.setBrush(brush);
        painter.drawEllipse(0, 0, 20, 20);
        painter.setBrush(painter.background());
        painter.drawEllipse(25, 0, 20, 20);
        brush.setColor(Qt::green);
        painter.setBrush(brush);
        painter.drawEllipse(50, 0, 20, 20);
        break;
    }


}

