#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include <QTextStream>
#include <QVector>
#include "patient.h" // Include the Patient class

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_consultantsSpinBox_valueChanged(int value);
    void on_registrarsSpinBox_valueChanged(int value);
    void on_seniorResidentsSpinBox_valueChanged(int value);
    void on_juniorResidentsSpinBox_valueChanged(int value);
    void on_internsSpinBox_valueChanged(int value);
    void on_resuscitationBedsSpinBox_valueChanged(int value);
    void on_acuteBedsSpinBox_valueChanged(int value);
    void on_subacuteBedsSpinBox_valueChanged(int value);
    void on_minorOpBedsSpinBox_valueChanged(int value);
    void on_durationSpinBox_valueChanged(int value);
    void assignBed(Patient &p,
                   int Triage,
                   QVector<Patient> &RBEDS,
                   QVector<Patient> &ABEDS,
                   QVector<Patient> &SABEDS,
                   QVector<Patient> &MOBS,
                   int rbeds,
                   int abeds,
                   int sabeds,
                   int mopbeds,
                   QVector<Patient> &QUEUE1,
                   QVector<Patient> &QUEUE2,
                   QVector<Patient> &QUEUE3,
                   QVector<Patient> &QUEUE4,
                   QVector<Patient> &QUEUE5);
    void on_StartButton_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_CriticalEmergency_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    void runSimulation();
    int generateRandomNumber();
    double weibullPDF(double x, double lambda, double k);
    double pearsonVIDensity(double x, double beta, double alpha, double p, double q);
    double pddtDensity(double x, double mean);
    int findMagnitude(int number);
    double betaFunction(double p, double q);
    double rejectionSampling(double beta, double alpha, double p, double q);
    double generateWeibullRejectionSampling(double lambda, double k);
};

#endif // MAINWINDOW_H
