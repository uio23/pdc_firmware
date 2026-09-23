/*
 * PDC.h - Power Distribution Controller Firmware for
 *         controlling onboard power channels and monitoring voltages.
 * Created by Alexander Kashpir,
 * for the Univeristy of Waikato Aeronautics Club,
 * on a microcontroller by Elven Aerospace Industrices Ltd and Gareth Reid.
 *
 */
#include <cstdint>


#pragma once


#define USER_LED PC13

#define PDC_CAN_ID            (0xDC) /* For (P)DC */
#define GROUND_CONTROL_CAN_ID (0x01)
#define SPI_CS_PIN            (PA0)

#define BROADCAST_INTERVAL    (250000) /* 250 milliseconds in microseconds */

#define ADC_RES               (12)
#define ADC_MAX               (4095)
#define VOLTAGE(x)            ( (x * 18.3f) / ADC_MAX)

#define ARRAY_SIZE(x)         (sizeof(x) / sizeof((x)[0]))


/**
 * A channel controlled by the PDC
 */
typedef struct channel_t {
  char name[3];

  int state;
  uint32_t state_pin;
} channel_t;

/**
 * An E-Fuse controlling multiple channels
 */
typedef struct efuse_t {
  char name[3];

  float current;
  uint32_t current_pin;

  int state;
  uint32_t state_pin;
} efuse_t;

/**
 * A battery supplying the PDC
 */
typedef struct battery_t {
    char name[3];

    float voltage;
    uint32_t voltage_pin;
} battery_t;


channel_t CHANNELS[] = {
    {"C1", 0, PB3 },
    {"C2", 0, PA15},
    {"C3", 0, PA10},
    {"C4", 0, PA8 },
    {"C5", 0, PB14},
    {"F1", 0, PA9 }
};

efuse_t EFUSES[] = {
  {"E1", 0, PB1, 0, PA7},
  {"E2", 0, PB0, 0, PA6}
};

battery_t BATTERIES[] = {
  {"B1", 0, PA5},
  {"BF", 0, PA4}
};
