#include "PIDImpl.h"


PIDImpl::PIDImpl(double max, double min, double Kp, double Kd, double Ki) : t(millis()),_max(max), _min(min), _Kp(Kp), _Kd(Kd), _Ki(Ki), _pre_error(0), _integral(0) 
{}


double PIDImpl::calculate(double setpoint, double pv)
{
    prevTime = t;
    t = millis();
    unsigned long _dt = t-prevTime/1000; // In ms

    // Calculate error
    double error = setpoint - pv;

    // Proportional term
    double Pout = _Kp * error;

    // Integral term
    _integral += error * _dt;
    double Iout = _Ki * _integral;

    // Derivative term
    double derivative = (error - _pre_error) / _dt;
    double Dout = _Kd * derivative;

    // Calculate total output
    double output = Pout + Iout + Dout;

    // Restrict to max/min
    if (output > _max) output = _max;
    else if (output < _min) output = _min;

    // Save error to previous error
    _pre_error = error;

    return output;
}