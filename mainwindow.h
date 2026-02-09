#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QMessageBox>
#include <QMouseEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QPoint>
#include <QRect>

#include <QVector>
#include <QWidget>

#include <QHash>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSize>
#include <QTimer>
#include <QProcess>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <string>
//#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
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


class AllST
{
public:
    QStringList s;
    QStringList s_tmp;
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


protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct PieceCell
    {
        int file = -1;  // 0..8
        int rank = -1;  // 0..9
        bool captured = false;
    };

    void captureBaseLayout();
    void applyScaledLayout();
    void recomputeBoardCoordinates();
    void updateInitialPieceCoords();
    QVector<PieceCell> snapshotPieceCells() const;
    void restorePieceCells(const QVector<PieceCell>& cells);
    void prepareScalableImages();
    bool isPieceButton(const QWidget* w) const;

    bool m_baseCaptured = false;
    bool m_firstShowApplied = false;
    QSize m_designCentralSize;
    QHash<QWidget*, QRect> m_baseGeom;
    QHash<QWidget*, int> m_baseFontPt;
    double m_scaleX = 1.0;
    double m_scaleY = 1.0;
    QPoint m_offset = QPoint(0, 0);
    int m_pieceSize = 68;

private slots:

    bool eventFilter(QObject* obj, QEvent* event);

    void analysisStep(const std::string& stepStr, bool ifnRepeat = true);

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

    void myabout();

    void on_actionCloudbookSettings_triggered();

    void on_pvp_radioButton_clicked();

    void on_Replay_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
