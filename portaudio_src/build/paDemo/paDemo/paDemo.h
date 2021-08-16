#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_paDemo.h"

class paDemo : public QMainWindow
{
    Q_OBJECT

public:
    paDemo(QWidget *parent = Q_NULLPTR);
    Ui::paDemoClass ui;


private slots:
    void on_btnStart_clicked();
};
