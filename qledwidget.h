#ifndef QLEDWIDGET_H
#define QLEDWIDGET_H

#include <QWidget>

class QLEDWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
public:
    explicit QLEDWidget(QWidget *parent = nullptr);
    int value() const {return currentValue;}
    //void setValue(int newValue);
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);

signals:

public slots:
    void setValue(int newValue);

private:
    int currentValue;
};

#endif // QLEDWIDGET_H
