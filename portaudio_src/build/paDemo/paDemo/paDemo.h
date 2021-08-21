#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_paDemo.h"
#include "portaudio.h"

class paDemo : public QMainWindow
{
    Q_OBJECT

public:
    paDemo(QWidget *parent = Q_NULLPTR);
    ~paDemo();
    void updateTimer();
    Ui::paDemoClass ui;

private:
    PaStreamParameters  inputParameters,
        outputParameters;
    PaStream* stream;
    QTimer* timer;

    int ShowWSAPI();
    void startRecord();

private slots:
    void on_btnStart_clicked();
    void on_btnStartRecord_clicked();
    void on_btnStopRecord_clicked();
};
