#include "patient.h"

Patient::Patient()
{
    ID = -1;
    arrival_time = -1;
    wait_time = -1;
    treatment_time = -1;
    discharge_time = -1;
    category = -1;
    bedAssigned = "Nil";
}

// Getters
int Patient::getID() const
{
    return ID;
}
double Patient::getArrival() const
{
    return arrival_time;
}
double Patient::getWaitTime() const
{
    return wait_time;
}
double Patient::getTreatTime() const
{
    return treatment_time;
}
double Patient::getDischargeTime() const
{
    return discharge_time;
}
int Patient::getCategory() const
{
    return category;
}
std::string Patient::getBed() const
{
    return bedAssigned;
}

// Setters
void Patient::setID(int x)
{
    ID = x;
}
void Patient::setArrival(double x)
{
    arrival_time = x;
}
void Patient::setWaitTime(double x)
{
    wait_time = x;
}
void Patient::setTreatTime(double x)
{
    treatment_time = x;
}
void Patient::setDischargeTime(double x)
{
    discharge_time = x;
}
void Patient::setCategory(int x)
{
    category = x;
}
void Patient::assignBed(const std::string &bed)
{
    bedAssigned = bed;
}
