/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <string>
#include <cstring>

using namespace std;    //Or use std::;
#include <sstream>    //Lets you create strings using << for instance


#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "../oled/oled.h"

#define FORWARD (+1)
#define BACKWARD (-1)

#define INSTRUCTION_PULSE 1
#define INSTRUCTION_SET_DIRECTION_FORWARD 2
#define INSTRUCTION_SET_DIRECTION_BACKWARD 3

void printEncoderPosition();


int currentLine = 0;

volatile int encoderPosition = 0;

volatile int GCodePosition = 0;
volatile int GCodeDirection = BACKWARD;
volatile int stepsPerMillimeter = 80;

volatile int totalIRQCalls = 0;
volatile int IRQCallsA = 0;
volatile int IRQCallsB = 0;
volatile int AriseCalls = 0;
volatile int BriseCalls = 0;
volatile int AfallCalls = 0;
volatile int BfallCalls = 0;

volatile int GPulseCalls = 0;
volatile int GDirRiseCalls = 0;
volatile int GDIRFallCalls = 0;

int a0 = 0;
int b0 = 0;
int c0 = 0;

bool ledLit = false;

//PIN DISPLAY
//SCK 4
//SDA 5

const uint PIN_LED = PICO_DEFAULT_LED_PIN;
const uint PIN_OUT_DIRECTION = 0;
const uint PIN_OUT_PULSE = 1;

//const uint PIN_INPUT_B = 17;
//const uint PIN_INPUT_A = 16;
const uint PIN_INPUT_B = 17;
const uint PIN_INPUT_A = 16;

const uint PIN_IN_PULSE = 13;
const uint PIN_IN_DIRECTION = 14;
const uint PIN_IN_RESET = 26;


const uint PIN_PROBE = 26;

void encoderCallbackV1(uint gpio, uint32_t events) {
    bool direction = gpio_get(PIN_INPUT_B);
    bool pulse = (events & GPIO_IRQ_EDGE_RISE) > 0;
    if (direction != pulse) {
        encoderPosition++;
    } else {
        encoderPosition--;
    }
}

void encoderCallbackV2(uint gpio, uint32_t events) {
    bool a = (events & GPIO_IRQ_EDGE_RISE) > 0;
    bool b = gpio_get(PIN_INPUT_B);
    if (a != a0) {              // A changed
        a0 = a;
        if (b != c0) {
            c0 = b;
            if (a == b) { encoderPosition++; } else { encoderPosition--; }
        }
    }
}

void encoderISR(uint gpio, uint32_t events) {
    bool direction = (events & GPIO_IRQ_EDGE_FALL) > 0;

    if (gpio == PIN_INPUT_A) {
        a0 = direction;
        if (direction) { AriseCalls++; } else { AfallCalls++; }
        if (b0 == a0) { encoderPosition--; } else { encoderPosition++; }
        IRQCallsA++;
    }
    if (gpio == PIN_INPUT_B) {
        b0 = direction;
        if (direction) { BriseCalls++; } else { BfallCalls++; }
        if (a0 == b0) { encoderPosition++; } else { encoderPosition--; }
        IRQCallsB++;
    }
    totalIRQCalls++;
}

void putInstruction(uint32_t instruction) {
    multicore_fifo_push_blocking(instruction);
}

void GCodeDirectionISR(uint gpio, uint32_t events) {
    GCodeDirection = -1 + ((events & GPIO_IRQ_EDGE_RISE) > 0) * 2;
    if ((events & GPIO_IRQ_EDGE_RISE) > 0) {
        putInstruction(INSTRUCTION_SET_DIRECTION_FORWARD);
        GDirRiseCalls++;
    } else {
        putInstruction(INSTRUCTION_SET_DIRECTION_BACKWARD);
        GDIRFallCalls++;
    }
}

void GCodePulseISR(uint gpio, uint32_t events) {
    GCodePosition += GCodeDirection;
    GPulseCalls++;
    putInstruction(INSTRUCTION_PULSE);

}

void blinkLedBlocking(int time) {
    gpio_put(PIN_LED, true);
    sleep_ms(time);
    gpio_put(PIN_LED, false);
    sleep_ms(time);
}

void read_fifo_pulses() {
    int pulseLength = 10;
    uint32_t instruction = multicore_fifo_pop_blocking();
    switch (instruction) {
        case INSTRUCTION_PULSE:
            gpio_put(PIN_OUT_PULSE, true);
            sleep_us(pulseLength);
            gpio_put(PIN_OUT_PULSE, false);
            sleep_us(pulseLength);
            break;
        case INSTRUCTION_SET_DIRECTION_BACKWARD:
            gpio_put(PIN_OUT_DIRECTION, false);
            break;
        case INSTRUCTION_SET_DIRECTION_FORWARD:
            gpio_put(PIN_OUT_DIRECTION, true);
            break;
    }
}


[[noreturn]] void secondCoreCode() {
    while (true) {

        //printEncoderPosition();


        read_fifo_pulses();
    }

}

void setupInputPin(int pin, bool pullUp = false) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, pullUp, false);
}

void setupOutputPin(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void ISR(uint gpio, uint32_t events) {
    switch (gpio) {
        case PIN_INPUT_A:
        case PIN_INPUT_B:
            encoderISR(gpio, events);
            break;
        case PIN_IN_DIRECTION:
            GCodeDirectionISR(gpio, events);
            break;
        case PIN_IN_PULSE:
            GCodePulseISR(gpio, events);
            break;
        default:
            break;
    }
}


void setup() {

    stdio_init_all(); // enable USB serial

    setupOutputPin(PIN_LED);
    setupOutputPin(PIN_OUT_PULSE);
    setupOutputPin(PIN_OUT_DIRECTION);

    setupInputPin(PIN_INPUT_A, true);
    setupInputPin(PIN_INPUT_B, true);
    setupInputPin(PIN_PROBE);
    setupInputPin(PIN_IN_PULSE);
    setupInputPin(PIN_IN_DIRECTION);
    setupInputPin(PIN_IN_RESET, true);


    gpio_set_irq_enabled_with_callback(PIN_INPUT_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &ISR);
    gpio_set_irq_enabled_with_callback(PIN_INPUT_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &ISR);

    gpio_set_irq_enabled_with_callback(PIN_IN_DIRECTION, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &ISR);
    gpio_set_irq_enabled_with_callback(PIN_IN_PULSE, GPIO_IRQ_EDGE_RISE, true, &ISR);




    //  gpio_set_irq_enabled_with_callback(PIN_INPUT_A,GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE ,true,&rise);

    a0 = gpio_get(PIN_INPUT_A);
    b0 = gpio_get(PIN_INPUT_B);

    init_display(64);
    multicore_launch_core1(secondCoreCode);
}

void printStats(string label, int value) {
    string oledText = label + " " + to_string(value);
    char oledCharText[20] = "";
    oledText.copy(oledCharText, oledText.size() + 1);

    setCursorx(0);
    setCursory(currentLine);
    ssd1306_print(oledCharText);

    currentLine++;
}

void printStatsCol2(string label, int value) {
    string oledText = label + " " + to_string(value);
    char oledCharText[20] = "";
    oledText.copy(oledCharText, oledText.size() + 1);

    setCursorx(8);
    setCursory(currentLine);
    ssd1306_print(oledCharText);

    currentLine++;
}

void printEncoderPosition() {
    printf("encoder position: %i\n", encoderPosition);

    fill_scr(0);
    currentLine = 0;

    printStats("Pos", encoderPosition);
    //  printStats("calls   ", totalIRQCalls);
    printStats("A  ", IRQCallsA);
    printStats("B  ", IRQCallsB);
    /*   printStats("A+", AriseCalls);
       printStats("A-", AfallCalls);         */
    printStats("A-B", IRQCallsA - IRQCallsB);
    printStats("A-A", AriseCalls - AfallCalls);
    printStats("B-B", BriseCalls - BfallCalls);

    currentLine = 1;
    printStatsCol2("g ", GCodePosition);
    printStatsCol2("x ", GCodePosition * 10000 / stepsPerMillimeter);
    printStatsCol2("p ", GPulseCalls);
    printStatsCol2("d+", GDirRiseCalls);
    printStatsCol2("d-", GDIRFallCalls);


    show_scr();
}


void resetAll() {
    encoderPosition = 0;
    totalIRQCalls = 0;
    IRQCallsA = 0;
    IRQCallsB = 0;
    AriseCalls = 0;
    BriseCalls = 0;
    AfallCalls = 0;
    BfallCalls = 0;

    a0 = 0;
    b0 = 0;
    c0 = 0;

    GCodePosition = 0;
    GPulseCalls = 0;
    GDirRiseCalls = 0;
    GDIRFallCalls = 0;
}

int getTimeMs() { return to_ms_since_boot(get_absolute_time()); }

[[noreturn]] int main() {
#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    int lastLed = getTimeMs();

    setup();
    sleep_ms(100);
    resetAll();
//    gpio_put(PIN_LED, true);
    while (true) {
        // blinkLedBlocking(250);
        printEncoderPosition();
        if (!gpio_get(PIN_IN_RESET)) {
            resetAll();
        }
        int time = getTimeMs();

        if (time - lastLed > 300) {
            lastLed = time;
            ledLit = !ledLit;
            gpio_put(PIN_LED, ledLit);
        }
    }
#endif
}
