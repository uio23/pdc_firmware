/*
 * PDC.h - Power Distribution Controller Firmware for
 *         monitoring and controlling onboard power channels.
 * Created by Alexander Kashpir,
 * for the Univeristy of Waikato Aeronautics Club,
 * on a microcontroller by Elven Aerospace Industrices Ltd.
 *
 */
#include <cstdint>


#pragma once


#define PDC_CAN_ID            (0xDC) /* For (P)DC */
#define GROUND_CONTROL_CAN_ID (0x01)
#define SPI_CS_PIN            (PA0)
#define NUM_CHANNELS          (6)


/*
 * A channel controlled by the PDC
 */
typedef struct channel_t {
  char name[3];
  float current;
  float c_max;
  uint32_t current_pin;
  int state;
  uint32_t state_pin;
} channel_t;


channel_t CHANNELS[NUM_CHANNELS] = {
    /* High current channels */
    {"H1", 0, 40, PA7, 0, PB14},
    {"H2", 0, 20, PA6, 0, PA15},
    {"H3", 0, 20, PA5, 0, PB3},
    /* Low current channels */
    {"L1", 0, 10, PA4, 0, PA8},
    {"L2", 0, 10, PB1, 0, PA9},
    {"L3", 0, 10, PB0, 0, PA10}
};
