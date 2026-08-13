#ifndef VOLSLIDER_H
#define VOLSLIDER_H

#include <QSlider>
#include <QWidget>

class VolSlider : public QSlider
{
    Q_OBJECT
public:
    explicit VolSlider(QWidget *parent = 0);

signals:

public slots:

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event);
};

#endif // VOLSLIDER_H
