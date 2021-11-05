#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include <QTime>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_ccwButton_clicked() {
}

void MainWindow::on_cwButton_clicked() {
}

void MainWindow::on_pressButton_clicked() {
}

void MainWindow::process_events() {
}

void MainWindow::on_renderButton_clicked() {
    ui->oledScreen->repaint();
}
