#ifndef PATIENT_H
#define PATIENT_H

#include <string>

class Patient
{
    int ID;
    double arrival_time;
    double wait_time;
    double treatment_time;
    double discharge_time;
    int category;
    std::string bedAssigned;
    double remainingTreatTime;
    std::string bedType;

public:
    Patient();
    // Getters
    int getID() const;
    double getArrival() const;
    double getWaitTime() const;
    double getTreatTime() const;
    double getDischargeTime() const;
    int getCategory() const;
    std::string getBed() const;

    // Setters
    void setID(int x);
    void setArrival(double x);
    void setWaitTime(double x);
    void setTreatTime(double x);
    void setDischargeTime(double x);
    void setCategory(int x);
    void assignBed(const std::string &bed);
};

#endif // PATIENT_H
