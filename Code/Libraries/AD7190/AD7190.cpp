#include "AD7190.h"

AD7190::AD7190(double Vref) {
    vref = Vref;
    SPI.begin();
}

// Reset circuitry and serial interface by writing 40 consecutive 1s. Done automatically on startup
// All registers are restored to original values
void AD7190::reset() {
    for (int i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }
    delayMicroseconds(600);
}

// Write data to register reg
void AD7190::write_reg(byte reg, long data, int nbytes) {
    int comm = reg << 3;
    spi_transfer(comm);
    for (int i = nbytes-1; i >= 0; i--) {
        spi_transfer((data >> (8*i)) & 0xFF);
    }
}

// Read contents of register reg
long AD7190::read_reg(byte reg, int nbytes) {
    byte bytes[nbytes];
    int comm = (reg << 3) | 0x40;
    spi_transfer(comm);
    long regval = 0;
    for (int i = nbytes-1; i >= 0; i--) {
        bytes[i] = spi_transfer(0x0);
    }
    for (int i = 0; i < nbytes; i++) {
        regval |= bytes[i] << 8*i;
    }
    return regval;
}

// Read data from data register
double AD7190::read_data(int ch) {
    set_config(refsel, ch, unipolar, vgain, buf);
    while (read_reg(COMMSTAT, 1) & 0x80) {
        Serial.println(read_reg(COMMSTAT, 1), BIN);
    }
  
    int ret = read_reg(DATA, 3);
    double err = ((double) ret) / 0xFFFFFF * 2 * fullsc - fullsc;
    if (single_conv) {
        set_mode(PULSE, clock);
    }

    return err;
}

// Set mode register values (see datasheet p.21)
void AD7190::set_mode(byte mode, byte clock) {
    unsigned long regval = (mode << 21) | (clock << 18);

    write_reg(MODE, regval, 3);
}

// Run a zero-scale and full-scale calibration
void AD7190::calibrate(byte clock) {
    set_mode(INTZEROCAL, clock);
    delay(1000);

    set_mode(INTFULLCAL, clock);
    delay(1000);
}

// Set config register values (see datasheet p.23). Note: multiple channels can be selected by setting each relevant
// bit instead of just one. Ex: channels = AIN1_2 | AIN3_4. Doing this means each channel will be loaded into the
// data register sequentially.
void AD7190::set_config(byte refsel, byte channels, byte polarity, byte gain, byte buffer) {
    unsigned long regval = (refsel << 20) | (channels << 8) | (buffer << 4) | (polarity << 3) | gain;

    write_reg(CFG, regval, 3);
}

void AD7190::init(byte mode, byte clockval, byte refselval, byte channels, byte polarity, byte vgainval, byte buffer) {
    unipolar = polarity;
    refsel = refselval;
    vgain = vgainval;
    clock = clockval;
    buf = buffer;
    fullsc = vref/pow(2, vgain);
    if (mode == PULSE) {
        single_conv = true;
    }
    reset();
    set_mode(mode, clock);
    calibrate(clock);
    set_mode(mode, clock);
    set_config(refsel, channels, polarity, vgain, buf);
}

void AD7190::init(byte channels) {
    init(PULSE, INTCLK2, REFIN1, channels, BIPOLAR, GAIN_32, BUF);
}