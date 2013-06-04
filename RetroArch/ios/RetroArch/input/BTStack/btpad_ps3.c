/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2013 - Jason Fetters
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "boolean.h"
#include "../../rarch_wrapper.h"

#include "btdynamic.h"
#include "btpad.h"

struct btpad_ps3_data
{
   uint8_t data[512];
   
   bd_addr_t address;
   uint32_t handle;
   uint32_t channels[2];
   
   bool have_led;
};

static void btpad_ps3_setleds(struct btpad_ps3_data* device, unsigned leds);
static void* btpad_ps3_connect(const btpad_connection_t* connection)
{
   struct btpad_ps3_data* device = malloc(sizeof(struct btpad_ps3_data));
   memset(device, 0, sizeof(*device));

   memcpy(device->address, connection->address, BD_ADDR_LEN);
   device->handle = connection->handle;
   device->channels[0] = connection->channels[0];
   device->channels[1] = connection->channels[1];
   
   // Magic packet to start reports
   static uint8_t data[] = {0x53, 0xF4, 0x42, 0x03, 0x00, 0x00};
   bt_send_l2cap_ptr(device->channels[0], data, 6);

   // Without this the digital buttons won't be reported
   btpad_ps3_setleds(device, 1);

   return device;
}

static void btpad_ps3_disconnect(struct btpad_ps3_data* device)
{
}

static void btpad_ps3_setleds(struct btpad_ps3_data* device, unsigned leds)
{
   // TODO: Can this be modified to turn of motion tracking?
   static uint8_t report_buffer[] = {
      0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,
      0xff, 0x27, 0x10, 0x00, 0x32,
      0xff, 0x27, 0x10, 0x00, 0x32,
      0xff, 0x27, 0x10, 0x00, 0x32,
      0xff, 0x27, 0x10, 0x00, 0x32,
      0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   };
   
   report_buffer[11] = (leds & 0xF) << 1;
   bt_send_l2cap_ptr(device->channels[0], report_buffer, sizeof(report_buffer));
}

static uint32_t btpad_ps3_get_buttons(struct btpad_ps3_data* device)
{
   return device->data[3] | (device->data[4] << 8) | ((device->data[5] & 1) << 16);
}

static int16_t btpad_ps3_get_axis(struct btpad_ps3_data* device, unsigned axis)
{
   if (axis < 4)
   {
      int val = device->data[7 + axis];
      val = (val << 8) - 0x8000;
      return (abs(val) > 0x1000) ? val : 0;
   }

   return 0;
}

static void btpad_ps3_packet_handler(struct btpad_ps3_data* device, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   if (packet_type == L2CAP_DATA_PACKET && packet[0] == 0xA1)
   {
      if (!device->have_led)
      {
         btpad_ps3_setleds(device, 1);
         device->have_led = true;
      }
   
      memcpy(device->data, packet, size);
   }
}

struct btpad_interface btpad_ps3 =
{
   (void*)&btpad_ps3_connect,
   (void*)&btpad_ps3_disconnect,
   (void*)&btpad_ps3_setleds,
   (void*)&btpad_ps3_get_buttons,
   (void*)&btpad_ps3_get_axis,
   (void*)&btpad_ps3_packet_handler
};
