#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AD7190.h>
#include <PID.h>
#include <math.h>
#include <EEPROM.h>

#define CS 10

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display data
byte curr_ind = 0;
byte low[128];
byte high[128];
double max_candidate = -4;
double min_candidate = 4;

AD7190 ad7190(2.495);
//      130, 0.685, 5400
PID pid(130, 0.685, 5400, 0.00825, 120, true);
String c;
String in;
char buf1[16];
char buf2[16];
const double A = 0.001125308852122;
const double B = 0.000234711863267;
const double C = 0.000000085663516;
double vcc = 2.5;
int count = 0;
double sp_off = -0.05;
double last_params[4] = {0, 0, 0, 0};
double params_buf[4];

void double_to_eeprom(double d, int start_addr) {
  double *d_ptr = &d;
  long long d_int = *((long long*) d_ptr);
  byte temp;
  for (int i = 7; i >= 0; i--) {
    temp = (d_int >> (i*8)) & 0xFF;
    EEPROM.write(start_addr + 7-i, temp);
  }
}

double double_from_eeprom(int start_addr) {
  long long buf = 0;
  for (int i = 0; i < 8; i++) {
    long long temp = EEPROM.read(start_addr + i);
    buf |= temp << (7-i)*8;
  }
  long long *buf_ptr = &buf;
  double d = *((double*) buf_ptr);
  return d;
}

void load_params(double *buf) {
  // [sp, p, i, d]
  // !!!! Can cause buffer overflow if len(buf) < 4, don't do that !!!!
  for (int i = 0; i < 4; i++) {
    double temp = double_from_eeprom(8*i);
    if (isnan(temp) && i == 0) {
      temp = 27.5;
    }
    else if (isnan(temp)) {
      temp = 1;
    }
    buf[i] = temp;
  }
}

void save_params(double *params) {
  // !!!! len(params) must be 4 for correct operation !!!!
  for (int i = 0; i < 4; i++) {
    if (last_params[i] != params[i]) {
      last_params[i] = params[i];
      double_to_eeprom(params[i], 8*i);
    }
  }
}

double t_to_r(double T) {
  double x = 1/C*(A-1/(T+273.15));
  double y = sqrt(pow(B/(3*C), 3) + pow(x/2, 2));
  return exp(cbrt(y-x/2) - cbrt(y+x/2));
}

double r_to_v(double R) {
  return vcc * R / (10000 + R);
}

void updateDisplay(byte *low, byte *high, int curr_ind) {
  display.clearDisplay();
  int start = (curr_ind + 1) % 128;
  for (int i = 0; i < 128; i++) {
    for (int j = low[(i + start) % 128]; j <= high[(i + start) % 128]; j++) {
      display.drawPixel(i, j, SSD1306_WHITE);
    }
  }
  display.display();
}

void updateParams() {
  c = Serial.readStringUntil(' ');
  in = Serial.readStringUntil('\n');
  if (c != NULL && in != NULL) {
    c.toCharArray(buf1, 16);
    in.toCharArray(buf2, 16);
    int setting = atof(buf2);
    if (strcmp(buf1, "p") == 0) {
      pid.K_p = setting;
      params_buf[1] = setting;
    }
    else if (strcmp(buf1, "i") == 0) {
      pid.K_i = setting;
      params_buf[2] = setting;
    }
    else if (strcmp(buf1, "d") == 0) {
      pid.K_d = setting;
      params_buf[3] = setting;
    }
    else if (strcmp(buf1, "heat") == 0) {
      pid.heat = setting;
    }
    else if (strcmp(buf1, "setp") == 0) {
      sp_off = r_to_v(t_to_r(setting)) - 1.181;
      params_buf[0] = setting;
    }
    else if (strcmp(buf1, "verbose") == 0) {
      pid.verbose = setting;
    }
    else if (strcmp(buf1, "sint") == 0) {
      pid.set_integral(setting);
    }
  }
  else if (in != NULL) {
    in.toCharArray(buf1, 16);
    if (strcmp(buf1, "rstime") == 0) {
      pid.set_start_time();
    }
    else if (strcmp(buf1, "save") == 0) {
      save_params(params_buf);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  analogWriteResolution(12);
  Serial.setTimeout(2);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // Select ADC
  pinMode(CS, OUTPUT);
  digitalWrite(CS, LOW);

  // Initialize ADC and PID
  ad7190.init(AD7190::AIN2_2);
  pid.set_start_time();

  // Retrieve parameters from EEPROM
  load_params(params_buf);
  sp_off = r_to_v(t_to_r(params_buf[0])) - 1.181;
  pid.K_p = params_buf[1];
  pid.K_i = params_buf[2];
  pid.K_d = params_buf[3];

  // Set up display buffers
  for (int i = 0; i < 128; i++) {
    low[i] = 16;
    high[i] = 16;
  }
  updateDisplay(low, high, 0);
}

void loop() {
  double temp_v = 1.185 - ad7190.read_data(AD7190::AIN1_2);
  updateParams();
  double err = ad7190.read_data(AD7190::AIN3_4) + sp_off;

  double temp_err = 37.1*err*1000;
  if (temp_err > max_candidate) {
    max_candidate = temp_err;
  }
  if (temp_err < min_candidate) {
    min_candidate = temp_err;
  }
  count++;
  if (count == 150*120) {
    low[curr_ind] = (1-(max_candidate+3.0)/6.0)*32.0;
    high[curr_ind] = (1-(min_candidate+3.0)/6.0)*32.0;
    updateDisplay(low, high, curr_ind);
    curr_ind = (curr_ind + 1) % 128;
    count = 0;
    min_candidate = 4;
    max_candidate = -4;
  }
  pid.pid(temp_v, err);
  updateParams();
}
