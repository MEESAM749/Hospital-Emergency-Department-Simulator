#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "ui_mainwindow.h"
#include <random>
//Global Variables
double critical_emergency = 1;
int simulationDuration = 0;
//End of Global Variables
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::runSimulation()
{
    // Get inputs from the GUI
    simulationDuration = ui->durationSpinBox->value();
    int consultants = ui->consultantsSpinBox->value();
    int registrars = ui->registrarsSpinBox->value();
    int seniorResidents = ui->seniorResidentsSpinBox->value();
    int juniorResidents = ui->juniorResidentsSpinBox->value();
    int interns = ui->internsSpinBox->value();
    int rbeds = ui->resuscitationBedsSpinBox->value();
    int abeds = ui->acuteBedsSpinBox->value();
    int sabeds = ui->subacuteBedsSpinBox->value();
    int mopbeds = ui->minorOpBedsSpinBox->value() + 16;

    // Open admission file
    QFile admissionFile("admission.csv");
    if (!admissionFile.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open file admission.csv");
        return;
    }
    QTextStream out(&admissionFile);

    // Simulation variables
    QVector<Patient> ResuscitationBeds, AcuteBeds, SubacuteBeds, MinorOpBeds;
    QVector<Patient> Queue1, Queue2, Queue3, Queue4, Queue5;
    double nextArrivalTime = 0.0;
    int ID = 0;

    double interarrivalScaler = findMagnitude(simulationDuration);

    // Main simulation loop
    for (int time = 1; time <= simulationDuration; ++time) {
        // Handle patient arrival
        if (time >= nextArrivalTime) {
            double arrivalTime = time;
            double treatTime = rejectionSampling(355, 1.64, 5.72, 5.72) * 100;
            double postTreatmentTime = pddtDensity(simulationDuration, 156) * interarrivalScaler
                                       * 10000.0;
            int triage = generateRandomNumber();
            double waitTime = (triage == 1) ? 0 : (std::pow(2, triage - 1) * 600) / 60.0; // minutes
            double dischargeTime = treatTime + postTreatmentTime + waitTime;

            Patient newPatient;
            newPatient.setID(ID++);
            newPatient.setArrival(arrivalTime);
            newPatient.setTreatTime(treatTime);
            newPatient.setWaitTime(waitTime);
            newPatient.setDischargeTime(dischargeTime);
            newPatient.setCategory(triage);

            // Assign patient to a bed or queue
            assignBed(newPatient,
                      triage,
                      ResuscitationBeds,
                      AcuteBeds,
                      SubacuteBeds,
                      MinorOpBeds,
                      rbeds,
                      abeds,
                      sabeds,
                      mopbeds,
                      Queue1,
                      Queue2,
                      Queue3,
                      Queue4,
                      Queue5);

            double interarrivalTime = (generateWeibullRejectionSampling(1, 1.5) * 10)
                                      / critical_emergency;
            nextArrivalTime = time + interarrivalTime;
        }
    }

    // Treat patients in beds
    auto treatPatients = [&](QVector<Patient> &beds,
                             QVector<Patient> &queue,
                             int &staff,
                             const QString &role,
                             int availableBeds,
                             int simulationDuration,
                             int maxSimultaneousPatients,
                             int &supervisors,
                             const QString &supervisorRole,
                             bool canTreatCat1And2) {
        int i = 0;
        while (i < simulationDuration) {
            // Process simultaneous patients based on role capacity
            int patientsTreated = 0;
            QVector<int> supervisedPatients; // To track patients needing supervision

            while (!beds.isEmpty() && staff > 0 && patientsTreated < maxSimultaneousPatients) {
                Patient &patient = beds.front();

                // Determine if supervision is required
                bool needsSupervision = (patient.getCategory() <= 2 && !canTreatCat1And2);

                if (needsSupervision && supervisors <= 0) {
                    // Skip patient if no supervisors are available
                    break;
                }

                // Process patient
                out << "Patient ID: " << patient.getID() << ","
                    << "Category: " << patient.getCategory() << ","
                    << "Arrival Time: " << patient.getArrival() << ","
                    << "Wait Time: " << patient.getWaitTime() << ","
                    << "Treatment Time: " << patient.getTreatTime() << ","
                    << "Discharge Time: " << patient.getDischargeTime() << ",";
                if (needsSupervision) {
                    out << "Handled By: " << role << " (Supervised by " << supervisorRole << ")\n";
                    --supervisors; // Use one supervisor
                } else {
                    out << "Handled By: " << role << "\n";
                }

                // Remove treated patient from beds
                beds.pop_front();
                ++patientsTreated;

                // Add new patient from the queue if there's space
                if (!queue.isEmpty() && beds.size() < availableBeds) {
                    beds.push_back(queue.front());
                    queue.pop_front();
                }
            }

            // Decrement available staff for this iteration
            staff -= patientsTreated;
            i++; // Advance simulation time
        }
    };

    // Consultants and Registrars handle Category 1-2 patients directly
    treatPatients(ResuscitationBeds,
                  Queue1,
                  consultants,
                  "Consultant",
                  rbeds,
                  simulationDuration,
                  1,
                  registrars,
                  "Registrar",
                  true);
    treatPatients(AcuteBeds,
                  Queue2,
                  registrars,
                  "Registrar",
                  abeds,
                  simulationDuration,
                  1,
                  consultants,
                  "Consultant",
                  true);

    // Senior Residents can handle Category 3-5 patients independently and assist with Category 1-2 under supervision
    treatPatients(SubacuteBeds,
                  Queue3,
                  seniorResidents,
                  "Senior Resident",
                  sabeds,
                  simulationDuration,
                  4,
                  consultants,
                  "Consultant",
                  false);

    // Junior Residents can handle Category 3-5 patients independently and assist with Category 1-2 under supervision
    treatPatients(SubacuteBeds,
                  Queue4,
                  juniorResidents,
                  "Junior Resident",
                  sabeds,
                  simulationDuration,
                  3,
                  registrars,
                  "Registrar",
                  false);

    // Interns can handle Category 3-5 patients but always require supervision
    treatPatients(MinorOpBeds,
                  Queue5,
                  interns,
                  "Intern",
                  mopbeds,
                  simulationDuration,
                  2,
                  seniorResidents,
                  "Senior Resident",
                  false);

    // Log remaining patients in queues
    out << "Patients remaining in queues:\n";
    out << "Category 1: " << Queue1.size() << "\n";
    out << "Category 2: " << Queue2.size() << "\n";
    out << "Category 3: " << Queue3.size() << "\n";
    out << "Category 4: " << Queue4.size() << "\n";
    out << "Category 5: " << Queue5.size() << "\n";

    admissionFile.close();

    qDebug() << "ResuscitationBeds: " << ResuscitationBeds.size();
    qDebug() << "AcuteBeds: " << AcuteBeds.size();
    qDebug() << "SubacuteBeds: " << SubacuteBeds.size();
    qDebug() << "MinorOpBeds: " << MinorOpBeds.size();

    // Update GUI with results
    ui->textEdit->setMarkdown("Simulation Completed...");
    ui->listWidget_2->clear();
    ui->listWidget_2->addItem(QString::number(
        ID - (Queue1.size() + Queue2.size() + Queue3.size() + Queue4.size() + Queue5.size())));
    ui->listWidget_2->addItem(QString::number(Queue1.size() + Queue2.size() + Queue3.size()
                                              + Queue4.size() + Queue5.size()));
    ui->listWidget_2->addItem(QString::number(Queue1.size()));
    ui->listWidget_2->addItem(QString::number(Queue2.size()));
    ui->listWidget_2->addItem(QString::number(Queue3.size()));
    ui->listWidget_2->addItem(QString::number(Queue4.size()));
    ui->listWidget_2->addItem(QString::number(Queue5.size()));
}

// Random number generation
int MainWindow::generateRandomNumber()
{
    // Generate a random number between 1 and 5 with 3 being most frequent
    int randomValue = rand() % 10 + 1; // Random number between 1 and 10

    if (randomValue <= 5) {
        return 3; // 50% chance for 3
    } else {
        return (randomValue <= 8) ? 1 : (randomValue <= 9) ? 2 : (randomValue == 10) ? 4 : 5;
        // 30% chance for 1 or 2
        // 20% chance for 4 or 5
    }
}

// Weibull Distribution Density Calculation
double MainWindow::weibullPDF(double x, double lambda, double k)
{
    if (x < 0) {
        return 0.0; // The PDF is 0 for negative x
    }
    return (k / lambda) * std::pow(x / lambda, k - 1) * std::exp(-std::pow(x / lambda, k));
}

// Function to calculate the beta function
double MainWindow::betaFunction(double p, double q)
{
    return std::tgamma(p) * std::tgamma(q) / std::tgamma(p + q);
}

// Pearson VI Density Calculation
double MainWindow::pearsonVIDensity(double x, double beta, double alpha, double p, double q)
{
    if (x < 0) {
        return 0.0; // The density is 0 for negative x
    }
    double betaValue = betaFunction(p, q); // Calculate B(p, q)
    return (std::pow(x, alpha - 1) / std::pow(beta, alpha) / betaValue)
           * std::pow(1 + (x / beta), -(p + q));
}

// Function to calculate the PDDT density (Exponential Distribution)
double MainWindow::pddtDensity(double x, double mean)
{
    if (x < 0) {
        return 0.0; // The density is 0 for negative x
    }
    double lambda = 1.0 / mean; // Calculate rate parameter
    return lambda * std::exp(-lambda * x);
}

// Function to find the magnitude of a number
int MainWindow::findMagnitude(int number)
{
    if (number == 0)
        return 0; // Special case for 0
    return std::pow(10, static_cast<int>(std::log10(std::abs(number))));
}
double MainWindow::rejectionSampling(double beta, double alpha, double p, double q)
{
    // Proposal distribution (Gamma distribution for simplicity)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::gamma_distribution<> proposal(1.0, 1.0); // Simple Gamma distribution as proposal

    // Find the maximum of the Pearson VI distribution for normalization
    double maxDensity = pearsonVIDensity(1.0, beta, alpha, p, q); // Start with small value of x

    double sample, acceptRejectRatio, proposalDensity;

    while (true) {
        // Sample from the proposal distribution (Gamma)
        sample = proposal(gen);

        // Calculate the proposal density (Gamma PDF)
        proposalDensity = std::pow(sample, alpha - 1) * std::exp(-sample / beta)
                          / (std::tgamma(alpha) * std::pow(beta, alpha));

        // Calculate the target density (Pearson VI PDF)
        double targetDensity = pearsonVIDensity(sample, beta, alpha, p, q);

        // Accept or reject the sample based on the ratio
        acceptRejectRatio = targetDensity / (maxDensity * proposalDensity);
        if (std::uniform_real_distribution<>(0.0, 1.0)(gen) < acceptRejectRatio) {
            return sample; // Accept the sample
        }
    }
}
double MainWindow::generateWeibullRejectionSampling(double lambda, double k)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> uniform_dist(0.0, 1.0); // Uniform distribution [0, 1]

    // Find the maximum value of the Weibull PDF for scaling
    double maxPDF = weibullPDF(lambda,
                               lambda,
                               k); // The Weibull PDF at x = lambda will give a rough maximum
    double scale = maxPDF;         // Scale factor

    double x, u;
    do {
        // Step 1: Sample from uniform distribution
        x = lambda
            * std::pow(-std::log(1 - uniform_dist(gen)),
                       1 / k); // Using inverse transform sampling for Weibull
        u = uniform_dist(gen); // Uniform random number between 0 and 1

        // Step 2: Check if the sample is accepted based on the Weibull PDF
    } while (u > weibullPDF(x, lambda, k) / scale); // If u is greater, reject the sample

    return x; // Accepted inter-arrival time sample
}
void MainWindow::assignBed(Patient &p,
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
                           QVector<Patient> &QUEUE5)
{
    switch (Triage) {
    case 1:
        if (RBEDS.size() < rbeds) {
            p.assignBed("ResuscitationBed");
            RBEDS.push_back(p);
        } else if (ABEDS.size() < abeds) {
            p.assignBed("AcuteBed");
            ABEDS.push_back(p);
        } else if (SABEDS.size() < sabeds) {
            p.assignBed("SubacuteBed");
            SABEDS.push_back(p);
        } else if (MOBS.size() < mopbeds) {
            p.assignBed("MinorOpBed");
            MOBS.push_back(p);
        } else {
            QUEUE1.push_back(p);
        }
        break;
    case 2:
        if (ABEDS.size() < abeds) {
            p.assignBed("AcuteBed");
            ABEDS.push_back(p);
        } else if (SABEDS.size() < sabeds) {
            p.assignBed("SubacuteBed");
            SABEDS.push_back(p);
        } else if (MOBS.size() < mopbeds) {
            p.assignBed("MinorOpBed");
            MOBS.push_back(p);
        } else {
            QUEUE2.push_back(p);
        }
        break;
    case 3:
        if (SABEDS.size() < sabeds) {
            p.assignBed("SubacuteBed");
            SABEDS.push_back(p);
        } else if (MOBS.size() < mopbeds) {
            p.assignBed("MinorOpBed");
            MOBS.push_back(p);
        } else {
            QUEUE3.push_back(p);
        }
        break;
    case 4:
        if (MOBS.size() < mopbeds) {
            p.assignBed("MinorOpBed");
            MOBS.push_back(p);
        } else {
            QUEUE4.push_back(p);
        }
        break;
    case 5:
        if (MOBS.size() < mopbeds) {
            p.assignBed("MinorOpBed");
            MOBS.push_back(p);
        } else {
            QUEUE5.push_back(p);
        }
        break;
    }
}

void MainWindow::on_consultantsSpinBox_valueChanged(int value)
{
    qDebug() << "Consultants changed to:" << value;
}

void MainWindow::on_registrarsSpinBox_valueChanged(int value)
{
    qDebug() << "Registrars changed to:" << value;
}

void MainWindow::on_seniorResidentsSpinBox_valueChanged(int value)
{
    qDebug() << "Senior Residents changed to:" << value;
}

void MainWindow::on_juniorResidentsSpinBox_valueChanged(int value)
{
    qDebug() << "Junior Residents changed to:" << value;
}

void MainWindow::on_internsSpinBox_valueChanged(int value)
{
    qDebug() << "Interns changed to:" << value;
}

void MainWindow::on_resuscitationBedsSpinBox_valueChanged(int value)
{
    qDebug() << "Resuscitation Beds changed to:" << value;
}

void MainWindow::on_acuteBedsSpinBox_valueChanged(int value)
{
    qDebug() << "Acute Beds changed to:" << value;
}

void MainWindow::on_subacuteBedsSpinBox_valueChanged(int value)
{
    qDebug() << "Subacute Beds changed to:" << value;
}

void MainWindow::on_minorOpBedsSpinBox_valueChanged(int value)
{
    qDebug() << "Minor Op Beds changed to:" << value;
}

void MainWindow::on_durationSpinBox_valueChanged(int value)
{
    qDebug() << "Simulation Time changed to:" << value;
}

void MainWindow::on_StartButton_clicked()
{
    ui->textEdit->clear();
    MainWindow::runSimulation();
}

void MainWindow::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_CriticalEmergency_toggled(bool checked)
{
    if (checked) {
        critical_emergency *= 10; //Increases patient influx by a large amount.
    } else {
        critical_emergency = 1;
    }
}
