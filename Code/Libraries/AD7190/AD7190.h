#include <Arduino.h>
#include <SPI.h>
#ifndef AD7190_H
#define AD7190_H

class AD7190 {
    public:
        // On-chip registers
        static const byte COMMSTAT = 0x0;
        static const byte MODE = 0x1;
        static const byte CFG = 0x2;
        static const byte DATA = 0x3;
        static const byte ID = 0x4;
        static const byte GPOCON = 0x5;
        static const byte OFFSET = 0x6;
        static const byte FULLSC = 0x7;

        // Operating modes
        static const byte CONT = 0x0;
        static const byte PULSE = 0x1;
        static const byte IDLE = 0x2;
        static const byte PWDN = 0x3;
        static const byte INTZEROCAL = 0x4;
        static const byte INTFULLCAL = 0x5;
        static const byte SYSZEROCAL = 0x6;
        static const byte SYSFULLCAL = 0x7;

        // Clock modes
        static const byte EXTXTAL = 0x0;
        static const byte EXTCLK = 0x1;
        static const byte INTCLK = 0x2;   // MCLK2 tristated
        static const byte INTCLK2 = 0x3;   // Internal clock available on MCLK2

        // Gain
        static const byte GAIN_1 = 0x0;
        static const byte GAIN_8 = 0x3;
        static const byte GAIN_16 = 0x4;
        static const byte GAIN_32 = 0x5;
        static const byte GAIN_64 = 0x6;
        static const byte GAIN_128 = 0x7;

        // Input channels. First number is + input, second is - input
        static const byte AIN1_2 = 0x1;
        static const byte AIN3_4 = 0x2;
        static const byte TEMP = 0x4;
        static const byte AIN2_2 = 0x8;
        static const byte AIN1_COM = 0x10;
        static const byte AIN2_COM = 0x20;
        static const byte AIN3_COM = 0x40;
        static const byte AIN4_COM = 0x80;

        static const byte BIPOLAR = 0x0;
        static const byte UNIPOLAR = 0x1;

        static const byte REFIN1 = 0x0;
        static const byte REFIN2 = 0x1;

        static const byte UNBUF = 0x0;
        static const byte BUF = 0x1;

        bool unipolar = false;

        byte spi_transfer(byte data) {
            SPI.beginTransaction(SPISettings(2460000, MSBFIRST, SPI_MODE3));
            byte ret = SPI.transfer(data);
            SPI.endTransaction();
            return ret;
        }

        AD7190(double Vref);
        void reset();
        void set_next_op(byte read, byte reg);
        void write_reg(byte reg, long data, int nbytes);
        long read_reg(byte reg, int nbytes);
        double read_data(int ch);
        void set_mode(byte mode, byte clock);
        void calibrate(byte clock);
        void set_config(byte refsel, byte channels, byte polarity, byte gain, byte buffer);
        void init(byte mode, byte clockval, byte refselval, byte channels, byte polarity, byte vgainval, byte buffer);
        void init(byte channels);

    private:
        static const int COPI = 11;
        static const int CIPO = 12;
        static const int SCLK = 13;
        double fullsc;
        double vref;
        byte refsel;
        byte vgain;
        byte clock;
        byte buf;
        bool single_conv;
};

#endif