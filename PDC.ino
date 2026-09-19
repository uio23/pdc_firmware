#include <mcp2515.h>
#include <cstdio>

#include "PDC.h"


MCP2515 can_controller(SPI_CS_PIN);
HardwareTimer timer(TIM1);

struct can_frame tx, rx;
 /* Extra char for \0 */
char tx_string[sizeof(tx.data) + 1] = {' '};
char rx_string[sizeof(rx.data) + 1] = {' '};

volatile bool broadcast = false;


void set_broadcast()
{
  digitalWrite(PC13, !digitalRead(PC13));
  broadcast = true;
}

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

bool
handle_request()
{
  channel_t *channel;
  battery_t *battery;

  char *method  = strtok(rx_string, " ");
  char *name    = strtok(NULL, " ");
  char *param   = strtok(NULL, " ");
  char *str_val = strtok(NULL, " ");
  int  val      = -1;

  /* If missing any of 3 essential parameters, drop */
  if (method == NULL || name == NULL || param == NULL) return false;

  if (*method == 'S')
  {
    /* Drop set requests that aren't from ground control */
    if (rx.can_id != GROUND_CONTROL_CAN_ID) return false;

    /* If no value or a non-integer value was sent, drop */
    val = atoi(str_val);
    if (*str_val != '0' && val == 0) return false;
  }

  channel = get_channel(name);
  battery = get_battery(name);

  if (channel != NULL && *method == 'G' && *param == 'S')
  {
    val = digitalRead(channel->state_pin);
  }
  else if (channel != NULL && *method == 'S' && *param == 'S')
  {
    /* If new state is not 0 or 1, drop */
    if (val != 0 && val != 1) return false;
    digitalWrite(channel->state_pin, val);
  }
  else if (battery != NULL && *method == 'G' && *param == 'V')
  {
    val = VOLTAGE(analogRead(battery->voltage_pin));
    /* Report voltage in tens of volts */
    val = val * 10;
  }
  else
  {
    return false;
  }

  snprintf(tx_string, sizeof(tx_string), "%s %c %03d", name, *param, val); 
  return true;
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

    /* LED flashes every broadcast */
    pinMode(PC13, OUTPUT);

    can_controller.reset();
    can_controller.setBitrate(CAN_125KBPS);
    can_controller.setNormalOneShotMode(); /* Do not require acks */

    /* Broadcast data every 200ms */
    timer.pause();
    timer.setOverflow(BROADCAST_INTERVAL, MICROSEC_FORMAT);
    timer.attachInterrupt(set_broadcast);
    timer.refresh();
    timer.resume();
}

void loop(void)
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
    for (int i = 0; i < ARRAY_SIZE(CHANNELS); i++)
    {
      /* Each state added as a bit */
      tx_string[i + 2] = '0' + digitalRead(CHANNELS[i].state_pin);
    }
    transmit();

    /* Broadcast battery voltages as one CAN frame */
    strcpy(tx_string, "V:");
    int voltage;
    for (int i = 0; i < ARRAY_SIZE(BATTERIES); i++)
    {
      /* Each voltage added as a string in the range of 000 - 999 */
      voltage = VOLTAGE(analogRead(BATTERIES[i].voltage_pin));
      /* Report voltage in tens of volts */
      voltage = voltage * 10;
      tx_string[i * 3 + 2] = '0' + (voltage / 100) % 10;
      tx_string[i * 3 + 3] = '0' + (voltage / 10 ) % 10;
      tx_string[i * 3 + 4] = '0' + (voltage      ) % 10;
    }
    transmit();

    broadcast = false;
  }
}
