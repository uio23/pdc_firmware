#include <mcp2515.h>
#include <cstdio>

#include "PDC.h"


/* Variables for Logic Solutions */
/* Current from pa7, pa6, pa5 */
float pb14, pa15, pb3;
/* State pins*/
float state_pb14, state_pa15, state_pb3;

/* Current from pa4, pb1, pb0 */
float pa8, pa9, pa10;
/* State pins */
float state_pa8, state_pa9, state_pa10;
/* ***************************** */


MCP2515 can_controller(SPI_CS_PIN);

struct can_frame tx, rx;
 /* Extra char for \0 */
char tx_string[sizeof(tx.data) + 1] = {' '};
char rx_string[sizeof(rx.data) + 1] = {' '};

/* Vars used in loop, to avoid re-allocating memory */
int l_i;
char *l_method, *l_channel_name, *l_param;
char *l_str_rel_val;
int l_rel_val;
channel_t *l_channel;

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
  /* Drop if read not ok, not from ground control, or not 8 chars long */
  if (can_controller.readMessage(&rx) != MCP2515::ERROR_OK) return false;
  if (rx.can_id != GROUND_CONTROL_CAN_ID) return false;
  if (rx.can_dlc != sizeof(rx.data)) return false;

  for (int i = 0; i < rx.can_dlc; i++)
  {
    rx_string[i] = rx.data[i];
  }
  rx_string[sizeof(rx.data)] = '\0'; /* Ensure \0 char at end */

  return true;
}

channel_t *
getChannel(char *channel_name)
{
  for (int i = 0; i < NUM_CHANNELS; i++)
  {
    if (strcmp(CHANNELS[i].name, channel_name) == 0) return &CHANNELS[i];
  }

  return NULL;
}

bool
performRequest(char method, char param, channel_t *channel, int *rel_val)
{
  switch (method)
  {
    case 'G':
      switch (param)
      {
        case 'C': *rel_val = channel->current; break;
        case 'S': *rel_val = channel->state;   break;
        default:  return false;
      }
      break;

    case 'S':
      switch (param)
      {
        case 'S': 
          digitalWrite(channel->state_pin, *rel_val); 
          channel->state = *rel_val;
          break;
        default: return false;
      }
      break;

    default:
      return false;
  }

  return true;
}

void setup(void)
{
    /* Set each curent pin as input, each state pin as output */ 
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
      pinMode(CHANNELS[i].current_pin, INPUT);
      pinMode(CHANNELS[i].state_pin, OUTPUT);
    }

    can_controller.reset();
    can_controller.setBitrate(CAN_125KBPS);
    can_controller.setNormalOneShotMode(); /* Do not require acks */
}

void loop(void)
{
    /* TODO enable once hardward supports this */
    /* Update current reading from each channel,
     * and switch channel off if it's drawing too much current */
#if 0
    for (l_i = 0; l_i < NUM_CHANNELS; l_i++)
    {
        l_channel = &CHANNELS[l_i];
        l_channel->current = analogRead(l_channel->current_pin);

        if (l_channel->current > l_channel->c_max)
        { 
            digitalWrite(l_channel->state_pin, 0);
            l_channel->state = 0;

            snprintf(tx_string, sizeof(tx_string), "A %s S 0", l_channel->name); 
            transmit();
        }
    }
#endif

    if (receive())
    {
        l_method = strtok(rx_string, " ");
        l_channel_name = strtok(NULL, " ");
        l_param = strtok(NULL, " ");
        l_str_rel_val = strtok(NULL, " ");
        l_rel_val = 0;
        l_channel = NULL;

        /* If missing any of 3 essential parameters, drop */
        if (l_method == NULL || l_channel_name == NULL || l_param == NULL) return;
        /* For a set command, try get new rel_val*/
        if (*l_method == 'S')
        {
            l_rel_val = atoi(l_str_rel_val);
            /* If none or a non-integer value was sent, drop */
            if (*l_str_rel_val != '0' && l_rel_val == 0) return;
            /* If new state is not 0 or 1, drop */
            if (l_rel_val != 0 && l_rel_val != 1) return;
        }
        /* If a non-existing channel was named, drop */
        l_channel = getChannel(l_channel_name);
        if (l_channel == NULL) return;

        /* Will fail for undefined methods/params,
         * and also for setting current */
        if (performRequest(*l_method, *l_param, l_channel, &l_rel_val))
        {
            snprintf(tx_string, sizeof(tx_string), "%s %c %d", l_channel_name, *l_param, l_rel_val); 
            transmit();
        }
    }

    /* Set variables for Logic Solutions */
    pb14 = CHANNELS[0].current;
    pa15 = CHANNELS[1].current;
    pb3 = CHANNELS[2].current;

    state_pb14 = CHANNELS[0].state;
    state_pa15 = CHANNELS[1].state;
    state_pb3  = CHANNELS[2].state;

    pa8 = CHANNELS[3].current;
    pa9 = CHANNELS[4].current;
    pa10 = CHANNELS[5].current;

    state_pa8 = CHANNELS[3].state;
    state_pa9 = CHANNELS[4].state;
    state_pa10  = CHANNELS[5].state;
    /* ********************************* */
}
