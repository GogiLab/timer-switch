#pragma once
#include <Arduino.h>

bool keyHandle();
void updateTimer(uint8_t key);
void array2Time(uint8_t* tmArray);
void time2Array(uint8_t* tmArray);
void display();