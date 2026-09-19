#include <mcp2515.h>
#include <cstdio>

#include "PDC.h"


MCP2515 can_controller(SPI_CS_PIN);

struct can_frame tx, rx;
 /* Extra char for \0 */
char tx_string[sizeof(tx.data) + 1] = {' '};
char rx_string[sizeof(rx.data) + 1] = {' '};

/* Vars used in loop, to avoid re-allocating memory */
int l_i;
char *l_method, *l_name, *l_param;
char *l_str_val;
int l_val;
channel_t *l_channel;
battery_t *l_battery;

bool
transmit() {
  tx.can_id = PDC_CAN_ID;
  tx.can_dlc = sizeof(tx.data);

  /* Write and blank tx_string up to last char */
  for (int i = 0; i < tx.can_dlc; i++)
  {
    tx.data[i] = (tx_string[i] == '\0') ? ' ' : tx_string[i];
    tx_string[i] = ' ';
  }

  if (can_controller.sendMessage(&tx) == MCP2515::ERROR_OK) return true;
  return false;
}

bool
receive() {
  /* Drop if read not ok or not 8 chars long */
  if (can_controller.readMessage(&rx) != MCP2515::ERROR_OK) return false;
  if (rx.can_dlc != sizeof(rx.data)) return false;

  for (int i = 0; i < rx.can_dlc; i++)
  {
    rx_string[i] = rx.data[i];
  }
  rx_string[sizeof(rx.data)] = '\0'; /* Ensure \0 char at end */

  return true;
}

channel_t *
get_channel(char *channel_name)
{
  for (int i = 0; i < ARRAY_SIZE(CHANNELS); i++)
  {
    if (strcmp(CHANNELS[i].name, channel_name) == 0) return &CHANNELS[i];
  }

  return NULL;
}

battery_t *
get_battery(char *battery_name)
{
  for (int i = 0; i < ARRAY_SIZE(BATTERIES); i++)
  {
    if (strcmp(BATTERIES[i].name, battery_name) == 0) return &BATTERIES[i];
  }

  return NULL;
}

/**
 * Perform action on a channel, either getting or setting a value.
 * Will return false for an undefined \c method and \c param pair.
 */
bool
channel_request(char method, char param, channel_t *channel, int *val)
{

  switch (method)
  {
    case 'G':
      switch (param)
      {
        case 'S':
          *val = digitalRead(channel->state_pin);
          return true;
      }
      break;

    case 'S':
      switch (param)
      {
        case 'S': 
          digitalWrite(channel->state_pin, *val); 
          return true;
      }
      break;
  }

  return false;
}

void setup(void)
{
    /* Set each channel state pin as output and each channel state to 0 (off) */ 
    for (int i = 0; i < ARRAY_SIZE(CHANNELS); i++)
    {
      pinMode(CHANNELS[i].state_pin, OUTPUT);

      /* Turn off channel */
      digitalWrite(CHANNELS[i].state_pin, 0); 
      CHANNELS[i].state = 0;
    }

    /* TODO: When E-Fuses are supported by hardware, include them */

    /* Set each battery voltage pin to input and initialise voltage to 0 */
    for (int i = 0; i < ARRAY_SIZE(BATTERIES); i++)
    {
      pinMode(BATTERIES[i].voltage_pin, INPUT);
      BATTERIES[i].voltage = 0;
    }

    can_controller.reset();
    can_controller.setBitrate(CAN_125KBPS);
    can_controller.setNormalOneShotMode(); /* Do not require acks */
}

void loop(void)
{
    if (receive())
    {
        l_method  = strtok(rx_string, " ");
        l_name    = strtok(NULL, " ");
        l_param   = strtok(NULL, " ");
        l_str_val = strtok(NULL, " ");
        l_val     = -1;

        /* If missing any of 3 essential parameters, drop */
        if (l_method == NULL || l_name == NULL || l_param == NULL) return;

        if (*l_method == 'S')
        {
          /* Drop set requests that aren't from ground control */
          if (rx.can_id != GROUND_CONTROL_CAN_ID) return;

          /* If no value or a non-integer value was sent, drop */
          l_val = atoi(l_str_val);
          if (*l_str_val != '0' && l_val == 0) return;
        }

        l_channel = get_channel(l_name);
        l_battery = get_battery(l_name);

        if (l_channel != NULL && *l_method == 'G' && *l_param == 'S')
        {
          l_val = digitalRead(l_channel->state_pin);
        }
        else if (l_channel != NULL && *l_method == 'S' && *l_param == 'S')
        {
          /* If new state is not 0 or 1, drop */
          if (l_val != 0 && l_val != 1) return;
          digitalWrite(l_channel->state_pin, l_val);
        }
        else if (l_battery != NULL && *l_method == 'G' && *l_param == 'V')
        {
          l_val = VOLTAGE(analogRead(l_battery->voltage_pin));
        }
        else
        {
          /* Drop any other not supported requests */
          return;
        }

        snprintf(tx_string, sizeof(tx_string), "%s %c %03d", l_name, *l_param, l_val); 
        transmit();
    }
}
