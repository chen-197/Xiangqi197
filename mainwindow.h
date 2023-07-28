#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QProcess>
//#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
//#include <unistd.h>
namespace fs = std::filesystem;

class preStep
{
  public:
    QPushButton* preButton;
    QPoint prePoint;
    preStep(QPushButton* button, QPoint point);
    preStep();
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    bool eventFilter(QObject* obj, QEvent* event);

    void analysisStep(std::string stepStr);

    void on_readoutput();

    void xiangqitimeEvent();

    void on_Start_clicked();

    void on_Pause_clicked();

    void on_Continue_clicked();

    void on_Repent_clicked();

    void on_pve_radioButton_clicked();

    void on_eve_radioButton_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_Save_clicked();

    void on_Load_clicked();

    void chooseBin();

    void choosePy();

    void myabout();

    void on_pvp_radioButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
