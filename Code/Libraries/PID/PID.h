#include <Arduino.h>
#ifndef PID_H
#define PID_H

class PID {
public:
    bool verbose = true;
    int heat = 1;
    double K_p;
    double K_i;
    double K_d;

    PID(double K_p, double K_i, double K_d, double dt, int update, bool verbose);
    void pid(double temp_v, double err);
    void set_start_time();
    void set_integral(double val);

private:
    const double A = 0.001125308852122;
    const double B = 0.000234711863267;
    const double C = 0.000000085663516;
    const double vcc = 5;
    const double ref = 2.5;
    double dt;
    double pout;
    double vout;
    double integral = 0;
    double derivative = 0;
    int update_num = 40;
    int count = 0;
    double start;

    // Filtering
    double y_n;
    double y_n1 = 0;   // Last filter output y_{n-1}
    double y_n2 = 0;
    double x_n1 = 0;   // Last filter input x_{n-1}
    double x_n2 = 0;
    double x_n = 0;
    double num[3] = {0.00000682859420, 0.0000136751884, 0.00000682859420};
    double denom[3] = {1, -1.99259523, 0.99262254};
    
    // Derivative approximation
    static const int DERIV_NUM = 5;
    double a;
    double b;
    double coef;
    double v[DERIV_NUM];
    double y[DERIV_NUM];
    int ind = 0;

    void update_derivative(double err);
    void update_int(double err);
    void print(double data, int dec);
    void println(double data, int dec);
};

#endif