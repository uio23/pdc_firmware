#include <mcp2515.h>
#include <cstdio>

#include "PDC.h"


MCP2515 can_controller(SPI_CS_PIN);
HardwareTimer timer(TIM1);

struct can_frame tx, rx;
 /* Both strings can fit the maximum number of bytes
  * that may be received in a CAN frame.
  * Extra char for null byte */
char tx_string[sizeof(tx.data) + 1] = {' '};
char rx_string[sizeof(rx.data) + 1] = {' '};

volatile bool broadcast = false;


/**
 * Set broadcast flag and toggle user LED to indicate this.
 * Used as an ISR for timer interrupt.
 */
void set_broadcast()
{
  digitalWrite(USER_LED, !digitalRead(USER_LED));
  broadcast = true;
}

/**
 * Transmit chars in tx_string over CAN bus in a frame
 *
 * @return true, if the transmission was successful;
 *         false, otherwise.
 */
bool
transmit() {
  /* Write chars from tx_string into CAN frame */
  size_t str_len = strlen(tx_string);

  tx.can_dlc = str_len;
  memcpy(tx.data, tx_string, str_len);

  /* Return status of transmission */
  if (can_controller.sendMessage(&tx) == MCP2515::ERROR_OK) return true;
  return false;
}

/**
 * Attempt to receive a CAN frame. On success, put all its data bytes into rx_string
 * and null-terminate it.
 *
 * @return true, if a frame was received;
 *         false, otherwise;
 */
bool
receive() {
  /* Drop if receive unsuccessful */
  if (can_controller.readMessage(&rx) != MCP2515::ERROR_OK) return false;

  /* Copy data into null-terminated rx_string */
  memcpy(rx_string, rx.data, rx.can_dlc);
  rx_string[rx.can_dlc] = '\0';

  return true;
}

/**
 * Return pointer to channel with matching name.
 *
 * @param name Channel name to search for
 *
 * @return pointer to channel with matching name, if it exists;
 *         NULL, otherwise.
 */
channel_t *
get_channel(char *name)
{
  for (size_t i = 0; i < ARRAY_SIZE(CHANNELS); i++)
  {
    if (strcmp(CHANNELS[i].name, name) == 0) return &CHANNELS[i];
  }

  return NULL;
}

/**
 * Return pointer to battery with matching name.
 *
 * @param name Battery name to search for
 *
 * @return pointer to battery with matching name, if it exists;
 *         NULL, otherwise.
 */
battery_t *
get_battery(char *name)
{
  for (size_t i = 0; i < ARRAY_SIZE(BATTERIES); i++)
  {
    if (strcmp(BATTERIES[i].name, name) == 0) return &BATTERIES[i];
  }

  return NULL;
}

/**
 * Validate, parse and preform request string. Put corresponding response string
 * into rx_string.
 *
 * @return false if the request was malformed or not valid;
 *        true, otherwise.
 */
bool
handle_request()
{
  channel_t *channel;
  battery_t *battery;

  char *method  = strtok(rx_string, " ");
  char *name    = strtok(NULL, " ");
  char *param   = strtok(NULL, " ");
  char *str_val = strtok(NULL, " ");
  int val       = (str_val == NULL) ? -1 : atoi(str_val);

  /* --- Validate --- */
  /* If missing any of 3 essential parameters, drop */
  if (method == NULL || name == NULL || param == NULL) return false;

  channel = get_channel(name);
  battery = get_battery(name);

  /* If incorrect component name, drop */
  if (*param == 'S' && channel == NULL) return false;
  if (*param == 'V' && battery == NULL) return false;

  /* If set request not from ground control, drop */
  if (*method == 'S' && rx.can_id != GROUND_CONTROL_CAN_ID) return false;

  /* If set request does not contain integer, drop */
  if (*method == 'S' && str_val == NULL) return false;
  if (*method == 'S' && *str_val != '0' && val == 0) return false;

  /* --- Parse --- */
  if
    (channel != NULL && *method == 'G' && *param == 'S')
  {
    val = digitalRead(channel->state_pin);
  }
  else if
    (channel != NULL && *method == 'S' && *param == 'S')
  {
      /* If new state is not 0 or 1, drop */
      if (val != 0 && val != 1) return false;
      digitalWrite(channel->state_pin, val);
  }
  else if
    (battery != NULL && *method == 'G' && *param == 'V')
  {
    /* Report voltage in tens of volts */
    val = VOLTAGE(analogRead(battery->voltage_pin)) * 10;
  }
  else
  {
    return false;
  }

  snprintf(tx_string, sizeof(tx_string), "%s %c %03d", name, *param, val);
  return true;
}

/**
 * Initialise channel and voltage pins, turn off all channels,
 * set-up CAN peripheral, and set-up broadcast interrupt
 */
void
setup(void)
{
  /* Set each channel state pin to output and turn each channel off */
  for (int i = 0; i < ARRAY_SIZE(CHANNELS); i++)
  {
    pinMode(CHANNELS[i].state_pin, OUTPUT);
    digitalWrite(CHANNELS[i].state_pin, 0);
  }

  /* TODO: When E-Fuses are supported by hardware, include them */

  /* Set each battery voltage pin to input */
  for (int i = 0; i < ARRAY_SIZE(BATTERIES); i++)
  {
    pinMode(BATTERIES[i].voltage_pin, INPUT);
  }

  /* Set-up CAN peripheral */
  can_controller.reset();
  can_controller.setBitrate(CAN_125KBPS);
  can_controller.setNormalOneShotMode();

  /* Set tx frame id */
  tx.can_id = PDC_CAN_ID;

  /* Update ADC resolution */
  analogReadResolution(ADC_RES);

  /* Enable user LED  as broadcast indicator */
  pinMode(PC13, OUTPUT);

  /* Broadcast data at a set requency */
  timer.pause();
  timer.setOverflow(BROADCAST_INTERVAL, MICROSEC_FORMAT);
  timer.attachInterrupt(set_broadcast);
  timer.refresh();
  timer.resume();
}

/**
 * Attempt to receive a CAN frame, then attempt to handle the request in it,
 * and finally transmit a response to a valid request.
 *
 * If the broadcast flag is set, send a CAN frame with all the channel states,
 * and another frame with all the voltage readings.
 */
void
loop(void)
{
  if (receive())
  {
    if (handle_request())
    {
      transmit();
    }
  }

  if (broadcast)
  {
    /* Broadcast channel states as one CAN frame */
    strcpy(tx_string, "S:");
    for (size_t i = 0; i < ARRAY_SIZE(CHANNELS); i++)
    {
      /* Stop adding states if they don't fit into the string */
      if ( (i + 3) >= sizeof(tx_string) ) break;

      /* Each state added as a bit char */
      tx_string[i + 2] = '0' + digitalRead(CHANNELS[i].state_pin);
      tx_string[i + 3] = '\0';
    }
    transmit();

    /* Broadcast battery voltages as one CAN frame */
    strcpy(tx_string, "V:");
    int voltage;
    for (size_t i = 0; i < ARRAY_SIZE(BATTERIES); i++)
    {
      /* Stop adding values if they don't fit into the string */
      if ( (i * 3 + 5) >= sizeof(tx_string) ) break;

      /* Report voltage in tens of volts */
      /* Each voltage added as a string in the range of 000 - 999 representing 00.0V - 99.9V*/
      voltage = VOLTAGE(analogRead(BATTERIES[i].voltage_pin)) * 10;
      tx_string[i * 3 + 2] = '0' + (voltage / 100) % 10;
      tx_string[i * 3 + 3] = '0' + (voltage / 10 ) % 10;
      tx_string[i * 3 + 4] = '0' + (voltage      ) % 10;
      tx_string[i * 3 + 5] = '\0';
    }
    transmit();

    broadcast = false;
  }
}
