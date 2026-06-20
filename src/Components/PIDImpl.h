#ifndef PIDIMPL_H
#define PIDIMPL_H


#include <cmath>
#include "Particle.h"


class PIDImpl
{
public:
    // max - maximum value of manipulated variable
    // min - minimum value of manipulated variable
    // Kp -  proportional gain
    // Kd -  derivative gain
    // Ki -  Integral gain
    PIDImpl(double max, double min, double Kp, double Kd, double Ki);
    double calculate(double setpoint, double pv);

private:
    unsigned long elapsTime;
    unsigned long prevTime;
    unsigned long t;

    double _max;
    double _min;
    double _Kp;
    double _Kd;
    double _Ki;
    double _pre_error;
    double _integral;
};
#endif