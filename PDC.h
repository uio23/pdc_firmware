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


/* The voltage percentage adjustments are initialised to these */
#define B1_PERC (1.00)
#define BF_PERC (1.00)

#define ADC_RES    (12)
#define ADC_MAX    (4095)
#define VOLTAGE(x) ( (x * 18.3f) / ADC_MAX)

#define PDC_CAN_ID            (0xDC) /* For (P)DC */
#define GROUND_CONTROL_CAN_ID (0x01)
#define SPI_CS_PIN            (PA0)

#define BROADCAST_INTERVAL (250000) /* 250 milliseconds in microseconds */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define USER_LED PC13


/**
 * A channel controlled by the PDC
 */
typedef struct channel_t {
  char name[3];

  uint32_t state_pin;
} channel_t;

/**
 * An E-Fuse controlling multiple channels
 */
typedef struct efuse_t {
  char name[3];

  uint32_t current_pin;

  uint32_t state_pin;
} efuse_t;

/**
 * A battery supplying the PDC
 */
typedef struct battery_t {
    char name[3];

    uint32_t voltage_pin;
    float    voltage_perc;
} battery_t;


channel_t CHANNELS[] = {
    {"C1", PB3 },
    {"C2", PA15},
    {"C3", PA10},
    {"C4", PA8 },
    {"C5", PB14},
    {"F1", PA9 }
};

efuse_t EFUSES[] = {
  {"E1", PB1, PA7},
  {"E2", PB0, PA6}
};

battery_t BATTERIES[] = {
  {"B1", PA5, B1_PERC},
  {"BF", PA4, BF_PERC}
};
