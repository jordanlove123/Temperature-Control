#include "PID.h"

// Initialize PID object with pid parameters Kp, Ki, and Kd. dtval is the time difference between
// data points. The heater output will change and data will be printed every update points
PID::PID(double K_pval, double K_ival, double K_dval, double dtval, int update, bool verboseval) {
    start = ((double) millis()) / 1000;
    K_p = K_pval;
    K_i = K_ival;
    K_d = K_dval;
    dt = dtval;
    update_num = update;
    verbose = verboseval;

    a = DERIV_NUM*(DERIV_NUM-1)*(2*DERIV_NUM-1)/6*pow(dt, 2);
    b = DERIV_NUM*(DERIV_NUM-1)/2*dt;
    coef = 1/(a*DERIV_NUM-pow(b, 2));

    // Initialize derivative fit vectors
    for (int i = 0; i < DERIV_NUM; i++) {
        v[i] = coef*(DERIV_NUM*i*dt-b);
        y[i] = 0;
    }
}

void PID::update_derivative(double err) {
    y[ind] = err;
    derivative = 0;
    for (int i = 0; i < DERIV_NUM; i++) {
        derivative += v[i] * y[(i + ind + 1) % DERIV_NUM];
    }
    ind = (ind + 1) % DERIV_NUM;
}

void PID::update_int(double err) {
    integral += err*dt;

    // Stop windup
    if (K_i*integral >= 10) {
        integral = 10 / K_i;
    }
    else if (K_i*integral <= -10) {
        integral = -10 / K_i;
    }
}

void PID::print(double data, int dec) {
    if (data >= 0) {
        Serial.print(" ");
    }
    Serial.print(data, dec);
    Serial.print(" ");
}

void PID::println(double data, int dec) {
    if (data >= 0) {
        Serial.print(" ");
    }
    Serial.println(data, dec);
}

// Update the PID fields using the new data temp_v
void PID::pid(double temp_v, double err) {
    double R = 10000*temp_v / (ref - temp_v);
    double temp = 1 / (A + B*log(R) + C*pow(log(R), 3)) - 273.15;
    double time = ((double) millis()) / 1000 - start;

    update_int(err);
    // Filter input for derivative
    x_n = err;
    y_n = num[0]*x_n + num[1]*x_n1 + num[2]*x_n2 - denom[1]*y_n1 - denom[2]*y_n2;
    update_derivative(y_n);
    x_n2 = x_n1;
    x_n1 = x_n;
    y_n2 = y_n1;
    y_n1 = y_n;

    if (count == 0) {
        pout = -(K_p*y_n + K_i*integral + K_d*derivative);
        if (pout < 0) {
        pout = 0;
        vout = 0;
        }
        else {
            vout = (sqrt(pout) + 0.7) * 5/vcc;
            if (vout > 5) {
                vout = 5;
            }
        }
    }
    if (count == 0 && verbose) {
        print(time, 2);
        print(temp_v, 6);
        print(temp, 6);
        print(err, 9);
        print(-K_p*y_n, 6);
        print(-K_i*integral, 6);
        print(-K_d*derivative, 6);
        print(pout, 6);
        println(vout, 6);
    }
    analogWrite(A0, (int) (vout/5 * 4095 * heat));
    count = (count + 1) % update_num;
}

void PID::set_start_time() {
    start = ((double) millis()) / 1000;
}

void PID::set_integral(double val) {
    integral = val;
}