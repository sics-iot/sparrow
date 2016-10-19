/*
 * Copyright (c) 2012-2016, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         Flash firmware instance implementation (for writing new images and
           checking status)
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Niclas Finne <nfi@sics.se>
 */

/**
 * Flash instance. Implements object 0x0090DA0303010010.
 * Currently hard-coded for 2 instances.
 */

#include "sparrow-oam.h"
#include "instance-flash.h"
#include "image-trailer.h"
#include "dev/sparrow-device.h"
#include "dev/sparrow-flash.h"
#include "instance-flash-var.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t primaryWrite_enable = FALSE;
static uint8_t backupWrite_enable = FALSE;

#define FLASH_WRITE_CONTROL_ERASE 0x911
#define FLASH_WRITE_CONTROL_WRITE_ENABLE 0x23

SPARROW_OAM_INSTANCE_NAME(instance_flash_primary);
SPARROW_OAM_INSTANCE_NAME(instance_flash_backup);

/*---------------------------------------------------------------------------*/

/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", ni more than "len"
 * bytes.
 *
 * if the "request" is of type vectorAbortIfEqualRequest or
 * abortIfEqualRequest, and processing should be aborted,
 * "oam_processing" is set to SPARROW_OAM_PROCESSING_ABORT_TLV_STACK.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
flash_process_request(const sparrow_oam_instance_t *instance,
                      sparrow_tlv_t *request, uint8_t *reply, size_t len,
                      sparrow_oam_processing_t *oam_processing)
{
  uint32_t local32;
  uint8_t error = SPARROW_TLV_ERROR_NO_ERROR;
  const image_info_t *images;
  const image_info_t *image = NULL;
  uint8_t *write_enable;
  uint8_t image_index = 0;

  images = image_trailer_get_images();

  if(images == NULL) {
    /* No images found */
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
  }

  if(instance == &instance_flash_primary) {
    /* PRIMARY_FLASH_INSTANCE */
    image = &images[0];
    write_enable = &primaryWrite_enable;
    image_index = 1;
  } else if(instance == &instance_flash_backup) {
    /* BACKUP_FLASH_INSTANCE */
    image = &images[1];
    write_enable = &backupWrite_enable;
    image_index = 2;
  } else {
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_INSTANCE, reply, len);
  }

  if((request->opcode == SPARROW_TLV_OPCODE_SET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /*
     * Payload variables
     */
    if(request->variable == VARIABLE_FLASH) {
      if(*write_enable) {
        error = instance_flash_write_flash(image, request->offset, request->elements, (uint32_t *)request->data);
      } else {
        error = SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED;
      }

      if(error == SPARROW_TLV_ERROR_NO_ERROR) {
        return sparrow_tlv_write_reply_vector(request, reply, len, request->data);
      } else {
        return sparrow_tlv_write_reply_error(request, error, reply, len);
      }
    } else if(request->variable == VARIABLE_WRITE_CONTROL) {
      if(!image_index || image_index == SPARROW_DEVICE.get_running_image()) {
        return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_WRITE_ACCESS_DENIED, reply, len);
      }
      local32 = request->data[0] << 24;
      local32 |= request->data[1] << 16;
      local32 |= request->data[2] << 8;
      local32 |= request->data[3];

      if(local32 == FLASH_WRITE_CONTROL_ERASE) {
        if(instance_flash_erase_image(image) != TRUE) {
          return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_HARDWARE_ERROR, reply, len);
        }
        return sparrow_tlv_write_reply32(request, reply, len, sparrow_tlv_zeroes);
      } else if(local32 == FLASH_WRITE_CONTROL_WRITE_ENABLE) {
        *write_enable = TRUE;
        return sparrow_tlv_write_reply32(request, reply, len, sparrow_tlv_zeroes);
      }
    }
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  } else if((request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) || (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST)) {
    /*
     * Payload variables
     */
    if(request->variable == VARIABLE_WRITE_CONTROL) {
      if(*write_enable) {
        local32 = htonl(FLASH_WRITE_CONTROL_WRITE_ENABLE);
      } else {
        local32 = 0;
      }
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    } else if(request->variable == VARIABLE_IMAGE_START_ADDRESS) {
      return sparrow_tlv_write_reply32int(request, reply, len, image->start_address);
    } else if(request->variable == VARIABLE_IMAGE_MAX_LENGTH) {
      return sparrow_tlv_write_reply32int(request, reply, len, image->length);
    } else if(request->variable == VARIABLE_IMAGE_ACCEPTED_TYPE) {
      return sparrow_tlv_write_reply64int(request, reply, len, image->image_type);
    } else if(request->variable == VARIABLE_IMAGE_STATUS) {
      local32 = image_trailer_get_image_status(image);
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    } else if(request->variable == VARIABLE_IMAGE_VERSION) {
      uint64_t ver;
      ver = image_trailer_get_image_version(image);
      return sparrow_tlv_write_reply64int(request, reply, len, ver);
    } else if(request->variable == VARIABLE_IMAGE_LENGTH) {
      local32 = image_trailer_get_image_length(image);
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    } else if(request->variable == VARIABLE_IMAGE_CRC32) {
      local32 = image_trailer_get_image_crc32(image);
      return sparrow_tlv_write_reply32int(request, reply, len, local32);
    }
    return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return sparrow_tlv_write_reply_error(request, SPARROW_TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}
/*---------------------------------------------------------------------------*/

/*
 * Notification from OAM, typically new PDU or unitcontroller watchdog expire.
 */
static uint8_t
flash_oam_notification(sparrow_oam_nt_t type, uint8_t parameter)
{
  if(type == SPARROW_OAM_NT_NEW_PDU) {
    /* On new PDU; clear all write and erase enable states. */
    primaryWrite_enable = FALSE;
    backupWrite_enable = FALSE;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

/*
 * Erase image.
 * Return TRUE on success.
 */
uint8_t
instance_flash_erase_image(const image_info_t *image)
{
  uint8_t success = TRUE;
  uint32_t p;

  if(sparrow_flash_unlock() != SPARROW_FLASH_COMPLETE) {
    PRINTF("instance_flash_erase_image: Failed to unlock flash\n");
    return FALSE;
  }

  if(image->number_of_sectors == 0) {
    for(p = image->start_address; p < image->start_address + image->length; p += 4) {
      if(*((uint32_t *)p) != 0xffffffff) {
        if(sparrow_flash_erase_sector(p) != SPARROW_FLASH_COMPLETE) {
          success = FALSE;
          break;
        }
      }
    }
  }

  if(sparrow_flash_lock() != SPARROW_FLASH_COMPLETE) {
    PRINTF("instance_flash_erase_image: Failed to lock flash\n");
    success = FALSE;
  }

  return success;
}
/*---------------------------------------------------------------------------*/

/*
 * Write flash.
 *
 * @param image pointer to the image definition to write to.
 * @param offset start writing this number of 32-bit words into the image.
 * @param length number of 32-bit words to write.
 * @param data pointer to the data to write.
 *
 * Return TLV error
 */
uint8_t
instance_flash_write_flash(const image_info_t *image,
                           uint32_t offset, uint32_t length,
                           const uint32_t *data)
{
  int i;
  uint8_t retval = SPARROW_TLV_ERROR_NO_ERROR;
  sparrow_flash_status_t status;

  if(offset * 4 >= image->length) {
    return SPARROW_TLV_ERROR_INVALID_VECTOR_OFFSET;
  }

  if(((offset * 4) + (length * 4)) >= image->length) {
    return SPARROW_TLV_ERROR_BAD_NUMBER_OF_ELEMENTS;
  }

  if(sparrow_flash_unlock() != SPARROW_FLASH_COMPLETE) {
    PRINTF("instance_flash_write_flash: Failed to unlock flash\n");
    return SPARROW_TLV_ERROR_HARDWARE_ERROR;
  }

  for(i = 0; i < length; i++) {
    uint32_t present = *((uint32_t *)(image->start_address + (offset + i) * 4));
    if(present == 0xffffffff) {
      status = sparrow_flash_program_word(image->start_address + (offset + i) * 4, data[i]);
      if(status != SPARROW_FLASH_COMPLETE) {
        retval = SPARROW_TLV_ERROR_HARDWARE_ERROR;
        break;
      }
    } else if(present == data[i]) {
      /* same data, do nothing */
    } else {
      PRINTF("flash content not erase and not same as write request at address 0x%08lx\n",
                    (long unsigned)image->start_address + (offset + i) * 4);
    }
  }

  if(sparrow_flash_lock() != SPARROW_FLASH_COMPLETE) {
    PRINTF("instance_flash_write_flash: Failed to lock flash\n");
    retval = SPARROW_TLV_ERROR_HARDWARE_ERROR;
  }

  return retval;
}
/*---------------------------------------------------------------------------*/
static void
init(const sparrow_oam_instance_t *instance)
{
  instance->data->event_array[1] = 1;
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_flash_primary,
                     0x0090DA0303010010ULL, "Primary firmware",
                     instance_flash_variables,
                     .init = init, .process_request = flash_process_request,
                     .notification = flash_oam_notification);
SPARROW_OAM_INSTANCE(instance_flash_backup,
                     0x0090DA0303010010ULL, "Backup firmware",
                     instance_flash_variables,
                     .init = init, .process_request = flash_process_request,
                     .notification = flash_oam_notification);
/*---------------------------------------------------------------------------*/
