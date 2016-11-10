/*
 * Copyright (c) 2013-2016, Yanzi Networks AB.
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
 */

/**
 * \file
 *         Sparrow Encap over serial
 * \author
 *         Fredrik Ljungberg <flag@yanzinetworks.com>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>

#include "lib/list.h"
#include "lib/memb.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "udp-cmd.h"
#include "brm-stats.h"
#include "lib/crc32.h"
#include "sparrow-encap.h"
#include "enc-dev.h"
#include "br-config.h"

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "enc"
#include "ylog.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern speed_t br_config_b_rate;

PROCESS_NAME(serial_input_process);

static void stty_telos(int fd);

#define DEBUG_LINE_MARKER '\r'

#ifdef ENC_DEV_CONF_BUFFER_SIZE
#define ENC_DEV_BUFFER_SIZE ENC_DEV_CONF_BUFFER_SIZE
#else
#define ENC_DEV_BUFFER_SIZE 4096
#endif

#define LOG_LIMIT_ERROR(...)                                            \
  do {                                                                  \
    static clock_time_t _log_error_last;                                \
    static uint8_t _log_error_count = 0;                                \
    if(_log_error_count >= 5) {                                         \
      if(clock_time() - _log_error_last > 5 * 60UL * CLOCK_SECOND) {    \
        _log_error_count = 0;                                           \
      }                                                                 \
    } else {                                                            \
      _log_error_count++;                                               \
    }                                                                   \
    if(_log_error_count < 5) {                                          \
      YLOG_ERROR(__VA_ARGS__);                                          \
      _log_error_last = clock_time();                                   \
    }                                                                   \
  } while(0)

int devopen(const char *dev, int flags);

static const struct enc_dev_tunnel *tunnel = NULL;

static uint32_t next_seqno;
static uint32_t last_sent_seqno;
static uint32_t last_received_seqno;

/* delay between serial packets */
static struct ctimer send_delay_timer;
static uint32_t send_delay = 0;
static uint8_t is_using_ack = 0;
static uint8_t is_using_nack = 0;
static uint8_t has_received_seqno = 0;

#define WITH_FLOW_CONTROL 0
#if WITH_FLOW_CONTROL
static uint16_t max_packets_to_send = 0;
static int use_packet_flow_control = 0;
#endif /* WITH_FLOW_CONTROL */

/* for statistics */
unsigned long slip_sent = 0;
unsigned long slip_sent_to_fd = 0;
unsigned int  slip_max_buffer_usage = 0;

static int serialfd = -1;

#define PACKET_MAX_COUNT 64
#define PACKET_MAX_TRANSMISSIONS 2
#define PACKET_MAX_SIZE 1280

enum {
  PACKET_DATA,
  PACKET_SEQNO,
  PACKET_REPORT
};

typedef struct {
  void *next;
  uint32_t seqno;
  uint8_t type;
  uint8_t tx;
  uint16_t len;
  uint8_t data[PACKET_MAX_SIZE];
} packet_t;

MEMB(packet_memb, packet_t, PACKET_MAX_COUNT);
LIST(pending_packets);
static packet_t *active_packet = NULL;
static packet_t *last_sent_packet = NULL;

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while(0)

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335


/*---------------------------------------------------------------------------*/
/* Read thread */

static pthread_t thread;

/* 16 KB buffer size */
#define INPUT_BUFFER_SIZE 16384
static uint8_t input_buffer[INPUT_BUFFER_SIZE];
static volatile uint16_t write_pos = 0;
static volatile uint16_t read_pos = 0;

void *
read_code(void *argument)
{
   int i;
   uint16_t next_pos;

   YLOG_DEBUG("Serial Reader started!\n");
   if(serialfd > 0) {
     size_t free_size;
     ssize_t size;
     while(1) {
       if(read_pos > write_pos) {
         free_size = read_pos - write_pos;
       } else {
         free_size = INPUT_BUFFER_SIZE - write_pos;
       }
       size = read(serialfd, &input_buffer[write_pos], free_size);

       if(size <= 0) {

         if(size == 0) {
           /* Serial connection has closed */
           YLOG_ERROR("*** serial connection closed.\n");
           for(i = 5; i > 0; i--) {
             YLOG_ERROR("*** exit in %d seconds\n", i);
             sleep(1);
           }
           exit(EXIT_FAILURE);
         }

         if(errno != EINTR && errno != EAGAIN) {
           err(1, "enc-dev: read serial");
         }
         continue;
       }

       next_pos = (write_pos + size) % INPUT_BUFFER_SIZE;

       if(next_pos == read_pos) {
         BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_DROPPED);
         LOG_LIMIT_ERROR("*** reader has not read... overwriting read buffer! (%u)\n",
                         BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_DROPPED));
         slip_max_buffer_usage = INPUT_BUFFER_SIZE;
       } else if(read_pos > next_pos) {
         if(next_pos + INPUT_BUFFER_SIZE - read_pos > slip_max_buffer_usage) {
           slip_max_buffer_usage = next_pos + INPUT_BUFFER_SIZE - read_pos;
         }
       } else if(next_pos - read_pos > slip_max_buffer_usage) {
         slip_max_buffer_usage = next_pos - read_pos;
       }

       write_pos = next_pos;
       PRINTF("Read %d bytes WP:%d RP:%d\n", (int) size, write_pos, read_pos);

       process_poll(&serial_input_process);
     }
   } else {
     YLOG_ERROR("**** reader thread exiting - serialfd not initialized... \n");
   }
   return NULL;
}

/*---------------------------------------------------------------------------*/
unsigned
serial_get_baudrate(void)
{
  return br_config_b_rate;
}
/*---------------------------------------------------------------------------*/
void
serial_set_baudrate(unsigned speed)
{
  br_config_b_rate = speed;
  if(br_config_host == NULL && serialfd > 0) {
    stty_telos(serialfd);
  }
}
/*---------------------------------------------------------------------------*/
int
serial_get_ctsrts(void)
{
  return br_config_flowcontrol;
}
/*---------------------------------------------------------------------------*/
void
serial_set_ctsrts(int ctsrts)
{
  br_config_flowcontrol = ctsrts;
  if(br_config_host == NULL && serialfd > 0) {
    stty_telos(serialfd);
  } else if(ctsrts) {
    YLOG_ERROR("can not set cts/rts - no serial connection\n");
  }
}
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_mode(void)
{
  uint32_t v = 0;
  if(is_using_ack) {
    v |= SERIAL_MODE_ACK;
  }
  if(is_using_nack) {
    v |= SERIAL_MODE_NACK;
  }
  return v;
}
/*---------------------------------------------------------------------------*/
void
serial_set_mode(uint32_t mode)
{
  is_using_ack = mode & SERIAL_MODE_ACK;
  is_using_nack = mode & SERIAL_MODE_NACK;
}
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_send_delay(void)
{
  return send_delay;
}
/*---------------------------------------------------------------------------*/
void
serial_set_send_delay(uint32_t delayms)
{
  if(send_delay != delayms) {
    send_delay = delayms;

#if CLOCK_SECOND != 1000
#warning "send delay is assuming CLOCK_SECOND is milliseconds on native platform"
#endif

    if(send_delay > 0) {
      /* No callback function is needed here - the callback timer is just
         to make sure that the application is not sleeping when it is time
         to continue sending data. set_fd()/handle_fd() will be called
         anyway when the application is awake.
      */
      ctimer_set(&send_delay_timer, send_delay, NULL, NULL);
    }
    /* Make sure the send delay timer is expired from start */
    ctimer_stop(&send_delay_timer);
  }
}
/*---------------------------------------------------------------------------*/
static packet_t *
alloc_packet(void)
{
  packet_t *p;
  p = memb_alloc(&packet_memb);

  if(p == NULL) {
    BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_OVERFLOWS);
    LOG_LIMIT_ERROR("*** dropping pending packet (%u)\n",
                    BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_OVERFLOWS));

    /* Drop latest pending packet */
    p = list_chop(pending_packets);
  }

  if(p) {
    memset(p, 0, sizeof(packet_t));
  }
  return p;
}
/*---------------------------------------------------------------------------*/
static void
free_packet(packet_t *p)
{
  memb_free(&packet_memb, p);
}
/*---------------------------------------------------------------------------*/
#if 0
static void *
get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
#endif /* 0 */
/*---------------------------------------------------------------------------*/
int
enc_dev_is_remote(void)
{
  return br_config_host != NULL;
}
/*---------------------------------------------------------------------------*/
void
enc_dev_set_tunnel(const struct enc_dev_tunnel *t)
{
  tunnel = t;
}
/*---------------------------------------------------------------------------*/
static int
connect_to_server(const char *host, const char *port)
{
  /* Setup TCP connection */
  struct addrinfo hints, *servinfo, *p;
  /* char s[INET6_ADDRSTRLEN]; */
  int rv, fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
    err(1, "getaddrinfo: %s", gai_strerror(rv));
    return -1;
  }

  /* loop through all the results and connect to the first we can */
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if(connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(fd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if(p == NULL) {
    err(1, "can't connect to ``%s:%s''", host, port);

    /* all done with this structure */
    freeaddrinfo(servinfo);
    return -1;
  }

  /* Not need for nonblocking when p-thread reader */
  /*  fcntl(fd, F_SETFL, O_NONBLOCK); */

  /* inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), */
  /*           s, sizeof(s)); */

  /* all done with this structure */
  freeaddrinfo(servinfo);
  return fd;
}
/*---------------------------------------------------------------------------*/
int
is_sensible_string(const unsigned char *s, int len)
{
  int i;
  for(i = 1; i < len; i++) {
    if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
      continue;
    } else if(s[i] < ' ' || '~' < s[i]) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/

static void
serial_packet_input(const uint8_t *data, int len)
{
  packetbuf_copyfrom(data, len);
  if(br_config_verbose_output > 1) {
    YLOG_DEBUG("Packet input over serial: %d\n", len);
  }

  NETSTACK_RDC.input();
}
/*---------------------------------------------------------------------------*/
static void
send_report(void)
{
  uint8_t buf[4];
  buf[0] = (last_received_seqno >> 24) & 0xff;
  buf[1] = (last_received_seqno >> 16) & 0xff;
  buf[2] = (last_received_seqno >> 8) & 0xff;
  buf[3] = last_received_seqno & 0xff;
  if(br_config_verbose_output > 2) {
    PRINTF("O ACK %u\n", last_received_seqno);
  }
  write_to_slip_payload_type(buf, 4, SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT);
}

/*---------------------------------------------------------------------------*/
/*
 * Read from serial, when we have a packet call serial_packet_input. No output
 * buffering, input buffered by the reader thread...
 */
static void
serial_input(void)
{
  /* the unslipped input buffer */
  static unsigned char inbuf[ENC_DEV_BUFFER_SIZE];
  static int inbufptr = 0;
  static int state = 0;
  int i, enclen, j;
  uint32_t seqno;
  sparrow_encap_pdu_info_t pinfo;
  int insize;

  if(write_pos == read_pos) {
    /* nothing to read */
    PRINTF("Nothing to read...\n");
    return;
  }
  insize = write_pos - read_pos;
  if(insize < 0) {
    /* Wrapped - add a BUF_SIZE */
    insize += INPUT_BUFFER_SIZE;
  }
  /* Only read half a buffer size here... */
  if(insize > ENC_DEV_BUFFER_SIZE / 2) {
    insize = ENC_DEV_BUFFER_SIZE / 2;
  }

  PRINTF("Reading: %d\n", insize);

  BRM_STATS_DEBUG_ADD(BRM_STATS_DEBUG_SLIP_RECV, insize);

  if(tunnel != NULL) {
    if(tunnel->input) {
      /* tunnel->input(inputbuf[read_buf], bufsize[read_buf]); */
      if(read_pos + insize > INPUT_BUFFER_SIZE) {
        /* first read the end-part of the buffer */
        tunnel->input(&input_buffer[read_pos], INPUT_BUFFER_SIZE - read_pos);
        /* then read the rest at the beginning */
        tunnel->input(&input_buffer[0], insize - (INPUT_BUFFER_SIZE - read_pos));
      } else {
        tunnel->input(&input_buffer[read_pos], insize);
      }
      /* update the read_pos */
      read_pos = (read_pos + insize) % INPUT_BUFFER_SIZE;
    }
    return;
  }

  /* handle the data */
  for(j = 0; j < insize; j++) {
    /* step one step forward */
    unsigned char c = input_buffer[read_pos];
    read_pos = (read_pos + 1) % INPUT_BUFFER_SIZE;
    switch(c) {
    case SLIP_END:
      state = 0;
      if(inbufptr > 0) {
        BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_FRAMES);
        /* debug line marker is the only one that goes without encap... */
        if(inbuf[0] == DEBUG_LINE_MARKER) {
          YLOG_INFO("SR: ");
          fwrite(inbuf + 1, inbufptr - 1, 1, stdout);
          if(inbuf[inbufptr - 1] != '\n') {
            printf("\n");
          }
        } else {
          if(br_config_verbose_output > 4) {
            printf("IN(%03u): ", inbufptr);
            for(i = 0; i < inbufptr; i++) printf("%02x", inbuf[i]);
            printf("\n");
          }

          BRM_STATS_ADD(BRM_STATS_ENCAP_RECV, inbufptr);

          enclen = sparrow_encap_parse_and_verify(inbuf, inbufptr, &pinfo);
          if(enclen <  0) {
            BRM_STATS_INC(BRM_STATS_ENCAP_ERRORS);
            if(br_config_verbose_output) {
              if(enclen == SPARROW_ENCAP_ERROR_BAD_CHECKSUM) {
                YLOG_ERROR("packet input failed (bad CRC), len: %d, error: %d\n",
                           inbufptr, enclen);
              } else {
                YLOG_ERROR("packet input failed, len: %d, error: %d\n",
                           inbufptr, enclen);
              }

              if(br_config_verbose_output > 1 && br_config_verbose_output < 5) {
                for(i = 0; i < inbufptr; i++) printf("%02x", inbuf[i]);
                printf("\n");
              }
            }
            if(has_received_seqno && is_using_nack) {
              send_report();
            }

            /* empty the input buffer and continue */
            inbufptr = 0;
            continue;
          }

          if(pinfo.fpmode == SPARROW_ENCAP_FP_MODE_LENOPT
             && pinfo.fplen == 4 && pinfo.fp
             && pinfo.fp[1] == SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC) {
            /* Packet includes sequence number */
            seqno = inbuf[enclen + 0] << 24;
            seqno |= inbuf[enclen + 1] << 16;
            seqno |= inbuf[enclen + 2] << 8;
            seqno |= inbuf[enclen + 3];
            enclen += 4;

            if(has_received_seqno) {

              if(seqno == last_received_seqno) {
                /* This packet has already been received */
                if(br_config_verbose_output > 2) {
                  YLOG_DEBUG("I S *** dup %u\n", seqno);
                }
                send_report();
                inbufptr = 0;
                continue;
              }

              if(last_received_seqno - seqno < 50) {
                /* Received an older packet - drop it quietly */
                if(br_config_verbose_output > 2) {
                  YLOG_DEBUG("I S *** drop %u (%u)\n", seqno, last_received_seqno);
                }
                inbufptr = 0;
                continue;
              }
            } else {
              has_received_seqno = 1;
            }

            if(br_config_verbose_output > 2) {
              PRINTF("I S %u\n", seqno);
            }

            last_received_seqno = seqno;
            send_report();
            has_received_seqno = 1;
          }

          if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_SERIAL) {
            BRM_STATS_INC(BRM_STATS_ENCAP_SERIAL);
            if(inbuf[enclen] == '!') {
              command_context = CMD_CONTEXT_RADIO;
              cmd_input(&inbuf[enclen], pinfo.payload_len);
            } else if(inbuf[enclen] == '?') {
              /* no queries expected over slip? */
            } else {
              if(br_config_verbose_output > 1) {
                YLOG_DEBUG("Raw packet from serial of length %d\n", inbufptr);
              }
              serial_packet_input(&inbuf[enclen], pinfo.payload_len);
            }
          } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_TLV) {
            BRM_STATS_INC(BRM_STATS_ENCAP_TLV);
            udp_cmd_process_tlv_from_radio(&inbuf[enclen], pinfo.payload_len);
          } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT) {
            seqno = inbuf[enclen + 0] << 24;
            seqno |= inbuf[enclen + 1] << 16;
            seqno |= inbuf[enclen + 2] << 8;
            seqno |= inbuf[enclen + 3];

            if(br_config_verbose_output > 2) {
              PRINTF("I report: %u (%u)\n", seqno, last_sent_seqno);
            }

            if(last_sent_packet != NULL) {
              if(seqno == last_sent_packet->seqno) {
                /* Release the last packet */
                free_packet(last_sent_packet);
                last_sent_packet = NULL;
              } else if(seqno + 1 == last_sent_packet->seqno) {
                if(last_sent_packet->tx < PACKET_MAX_TRANSMISSIONS) {
                  /* Retransmit last packet */
                  if(br_config_verbose_output > 2) {
                    YLOG_DEBUG("I *** R: %u < %u\n", seqno, last_sent_packet->seqno);
                  }
                  list_push(pending_packets, last_sent_packet);
                  last_sent_packet = NULL;
                }
              }
            }

#if WITH_FLOW_CONTROL
          } else if(pinfo.payload_type == SPARROW_ENCAP_PAYLOAD_START_TRANSMITTER) {
            max_packets_to_send = (inbuf[enclen + 2] << 8) + inbuf[enclen + 3];
            use_packet_flow_control = 1;
#endif /* WITH_FLOW_CONTROL */
          } else {
            BRM_STATS_INC(BRM_STATS_ENCAP_UNPROCESSED);
          }
        }
        /* empty the input buffer and continue */
        inbufptr = 0;
      }
      break;

    case SLIP_ESC:
      state = SLIP_ESC;
      break;
    case SLIP_ESC_END:
      if(state == SLIP_ESC) {
        inbuf[inbufptr++] =  SLIP_END;
        state = 0;
      } else {
        inbuf[inbufptr++] =  SLIP_ESC_END;
      }
      break;
    case SLIP_ESC_ESC:
      if(state == SLIP_ESC) {
        inbuf[inbufptr++] = SLIP_ESC;
        state = 0;
      } else {
        inbuf[inbufptr++] = SLIP_ESC_ESC;
      }
      break;

    default:
      inbuf[inbufptr++] = c;
      state = 0;
      break;
    }

    if(inbufptr >= ENC_DEV_BUFFER_SIZE) {
      /* slip buffer is full, drop everything */
      BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_OVERFLOWS);
      LOG_LIMIT_ERROR("*** serial input buffer overflow\n");
      inbufptr = 0;
      state = 0;
    }
  }
  if(write_pos != read_pos) {
    /* Still more to read */
    process_poll(&serial_input_process);
  }
}
/*---------------------------------------------------------------------------*/
long
slip_buffered(void)
{
  return list_length(pending_packets);
}
/*---------------------------------------------------------------------------*/
int
slip_empty()
{
  return active_packet == NULL && list_head(pending_packets) == NULL;
}
/*---------------------------------------------------------------------------*/
static struct ctimer ack_timer;
static void
ack_retransmit(void *ptr)
{
  if(last_sent_packet != NULL) {
    if(last_sent_packet->tx < PACKET_MAX_TRANSMISSIONS) {
      if(br_config_verbose_output > 2) {
        YLOG_DEBUG("O *** auto R: %u\n", last_sent_packet->seqno);
      }
      list_push(pending_packets, last_sent_packet);
    } else {
      if(br_config_verbose_output > 2) {
        YLOG_DEBUG("O *** no ack: %u\n", last_sent_packet->seqno);
      }
      free_packet(last_sent_packet);
    }
    last_sent_packet = NULL;
  }
}
/*---------------------------------------------------------------------------*/
void
slip_flushbuf(int fd)
{
  /* Ensure the slip buffer will be large enough for worst case encoding */
  static uint8_t slip_buf[PACKET_MAX_SIZE * 2];
  static uint16_t slip_begin, slip_end = 0;

  int i, n;

  if(active_packet == NULL) {

    if(slip_empty()) {
      /* Nothing to send */
      return;
    }

    if(last_sent_packet != NULL) {
      /* Still awaiting ack but always allow receive reports to be sent */
      packet_t *h;
      h = list_head(pending_packets);
      if(h == NULL || h->type != PACKET_REPORT) {
        return;
      }
    }

#if WITH_FLOW_CONTROL
    if(use_packet_flow_control) {
      static int overflow = 0;
      if(max_packets_to_send == 0) {
        overflow++;
        if(overflow < 1000) {
          LOG_LIMIT_ERROR("*** flow control overflow\n");
        }
        return;
      }
      overflow = 0;
      max_packets_to_send--;
    }
#endif /* WITH_FLOW_CONTROL */

    active_packet = list_pop(pending_packets);
    if(active_packet == NULL) {
      /* Nothing to send */
      return;
    }

    slip_begin = slip_end = 0;
    if(active_packet->type == PACKET_SEQNO) {
      last_sent_seqno = active_packet->seqno;
      if(br_config_verbose_output > 2) {
        PRINTF("O send %u/%u\n", last_sent_seqno, next_seqno - 1);
      }
    }

    if(br_config_verbose_output > 4) {
      printf("OUT(%03u): ", active_packet->len);
      for(i = 0; i < active_packet->len; i++) {
        printf("%02x", active_packet->data[i]);
      }
      printf("\n");
    }

    slip_buf[slip_end++] = SLIP_END;
    for(i = 0; i < active_packet->len; i++) {
      switch(active_packet->data[i]) {
      case SLIP_END:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_END;
        break;
      case SLIP_ESC:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_ESC;
        break;
      default:
        slip_buf[slip_end++] = active_packet->data[i];
        break;
      }
    }
    slip_buf[slip_end++] = SLIP_END;

    if(br_config_verbose_output > 2) {
      PRINTF("send %u/%u\n", slip_end, active_packet->len);
    }
  }

  n = write(fd, slip_buf + slip_begin, slip_end - slip_begin);

  if(n == -1) {
    if(errno == EAGAIN) {
      PROGRESS("Q");           /* Outqueue is full! */
    } else {
      err(1, "slip_flushbuf write failed");
    }
  } else {

    slip_sent_to_fd += n;
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
      slip_sent++;

      if(active_packet != NULL) {
        active_packet->tx++;
        if(active_packet->type == PACKET_SEQNO) {
          /* This packet might require retransmissions */
          if(last_sent_packet != NULL) {
            free_packet(last_sent_packet);
          }
          last_sent_packet = active_packet;
          ctimer_restart(&ack_timer);
        } else {
          /* This packet is no longer needed */
          free_packet(active_packet);

          /* a delay between non acked slip packets to avoid losing data */
          if(send_delay != 0) {
            ctimer_restart(&send_delay_timer);
          }
        }
        active_packet = NULL;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
write_to_serial(int outfd, const uint8_t *inbuf, int len, uint8_t payload_type)
{
  const uint8_t *p = inbuf;
  uint8_t *buffer;
  uint8_t finger[4];
  int enc_res;
  int use_seqno = 0;
  uint32_t crc_value;
  sparrow_encap_pdu_info_t pinfo;
  packet_t *packet;

  if(tunnel) {
    /* In tunnel mode - only the tunnel can write to serial */
    return;
  }

  packet = alloc_packet();
  if(packet == NULL) {
    /* alloc_packet will log the overflow */
    return;
  }

  buffer = packet->data;

  /* do encap header + CRC32 here!!! */

  finger[0] = 0;
  finger[1] = SPARROW_ENCAP_FP_LENOPT_OPTION_CRC;
  finger[2] = (len >> 8);
  finger[3] = len & 0xff;

  if(is_using_ack) {
    if(payload_type == SPARROW_ENCAP_PAYLOAD_SERIAL || payload_type == SPARROW_ENCAP_PAYLOAD_TLV) {
      /* Only use sequence number for serial or TLV data */
      use_seqno = 1;
      finger[1] = SPARROW_ENCAP_FP_LENOPT_OPTION_SEQNO_CRC;
    }
  }

  pinfo.version = SPARROW_ENCAP_VERSION1;
  pinfo.fp = finger;
  pinfo.iv = NULL;
  pinfo.payload_len = len;
  pinfo.payload_type = payload_type;
  pinfo.fpmode = SPARROW_ENCAP_FP_MODE_LENOPT;
  pinfo.fplen = 4;
  pinfo.ivmode = SPARROW_ENCAP_IVMODE_NONE;
  pinfo.ivlen = 0;

  enc_res = sparrow_encap_write_header(buffer, PACKET_MAX_SIZE, &pinfo);

  if(enc_res) {

    if(use_seqno) {
      packet->type = PACKET_SEQNO;
      packet->seqno = next_seqno++;
      buffer[enc_res + 0] = (packet->seqno >> 24) & 0xff;
      buffer[enc_res + 1] = (packet->seqno >> 16) & 0xff;
      buffer[enc_res + 2] = (packet->seqno >>  8) & 0xff;
      buffer[enc_res + 3] = packet->seqno & 0xff;

      enc_res += 4;
    }

    /* copy the data into the buffer */
    memcpy(buffer + enc_res, p, len);

    /* do a crc32 calculation of the whole message */
    crc_value = crc32(buffer, len + enc_res);

    buffer[len + enc_res + 0] = (crc_value >> 0L) & 0xff;
    buffer[len + enc_res + 1] = (crc_value >> 8L) & 0xff;
    buffer[len + enc_res + 2] = (crc_value >> 16L) & 0xff;
    buffer[len + enc_res + 3] = (crc_value >> 24L) & 0xff;

    packet->len = len + enc_res + 4;

    if(payload_type == SPARROW_ENCAP_PAYLOAD_RECEIVE_REPORT) {
      /* Prioritize control messages */
      list_push(pending_packets, packet);
    } else {
      list_add(pending_packets, packet);
    }
  } else {
    free_packet(packet);
  }
  PROGRESS("t");
}
/*---------------------------------------------------------------------------*/
/* writes an 802.15.4 packet to serial-radio */
void
write_to_slip(const uint8_t *buf, int len)
{
  if(serialfd > 0) {
    write_to_serial(serialfd, buf, len, SPARROW_ENCAP_PAYLOAD_SERIAL);
  }
}
/*---------------------------------------------------------------------------*/
void
write_to_slip_payload_type(const uint8_t *buf, int len, uint8_t payload_type)
{
  if(serialfd > 0) {
    write_to_serial(serialfd, buf, len, payload_type);
  }
}
/*---------------------------------------------------------------------------*/
static void
stty_telos(int fd)
{
  struct termios tty;
  speed_t speed = br_config_b_rate;
  int i;

  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");

  if(fcntl(fd, F_SETFL, 0) == -1) err(1, "fcntl");

  if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

  cfmakeraw(&tty);

  /* Nonblocking read. */
  /* tty.c_cc[VTIME] = 0; */
  /* tty.c_cc[VMIN] = 0; */
  if(br_config_flowcontrol) {
    tty.c_cflag |= CRTSCTS;
  } else {
    tty.c_cflag &= ~CRTSCTS;
  }
  tty.c_cflag &= ~HUPCL;
  tty.c_cflag &= ~CLOCAL;

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

#if 1
  /* Nonblocking read and write. */
  /* if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) err(1, "fcntl"); */

  tty.c_cflag |= CLOCAL;
  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

  i = TIOCM_DTR;
  if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

  usleep(10*1000);             /* Wait for hardware 10ms. */

  /* Flush input and output buffers. */
  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(tunnel != NULL) {
    if(tunnel->has_output && tunnel->has_output()) {
      FD_SET(serialfd, wset);
    }
    return 1;
  }

  /* Anything to flush? */
  if(!slip_empty()
     && (send_delay == 0 || ctimer_expired(&send_delay_timer))) {
    FD_SET(serialfd, wset);
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  int ret, i;

  /* Reading is handled by a pthread */
  /* if(serialfd > 0 && FD_ISSET(serialfd, rset)) { */
  /* } */

  if(serialfd > 0 && FD_ISSET(serialfd, wset)) {
    if(tunnel != NULL) {
      if(tunnel->output) {
        ret = tunnel->output(serialfd);
        if(ret == 0) {
          /* Serial connection has closed */
          YLOG_ERROR("*** serial connection closed.\n");
          for(i = 5; i > 0; i--) {
            YLOG_ERROR("*** exit in %d seconds\n", i);
            sleep(1);
          }
          exit(EXIT_FAILURE);
        } else if(ret < 0) {
          if(errno != EINTR && errno!= EAGAIN) {
            err(1, "enc-dev: write tunnel");
          }
        }
      }
    } else {
      slip_flushbuf(serialfd);
    }
  }
}

/*---------------------------------------------------------------------------*/
static const struct select_callback slip_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
void
enc_dev_init(void)
{
  static uint8_t is_initialized = 0;
  int rc;

  if(is_initialized) {
    return;
  }
  is_initialized = 1;

  memb_init(&packet_memb);
  list_init(pending_packets);

  ctimer_set(&ack_timer, CLOCK_SECOND / 16, ack_retransmit, NULL);
  ctimer_stop(&ack_timer);

  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  if(br_config_host != NULL) {
    if(br_config_port == NULL) {
      br_config_port = "60001";
    }
    serialfd = connect_to_server(br_config_host, br_config_port);
    if(serialfd == -1) {
      err(1, "can't connect to ``%s:%s''", br_config_host, br_config_port);
    }

  } else if(br_config_siodev != NULL) {
    if(strcmp(br_config_siodev, "null") == 0) {
      /* Disable slip */
      return;
    }
    serialfd = devopen(br_config_siodev, O_RDWR | O_NDELAY);
    if(serialfd == -1) {
      err(1, "can't open siodev ``/dev/%s''", br_config_siodev);
    }

  } else {
    static const char *siodevs[] = {
      "ttyUSB0", "ttyACM0", "ttyACM1", "cuaU0", "ucom0" /* linux, fbsd6, fbsd5 */
    };
    int i;
    for(i = 0; i < sizeof(siodevs) / sizeof(const char *); i++) {
      br_config_siodev = siodevs[i];
      serialfd = devopen(br_config_siodev, O_RDWR | O_NDELAY);
      if(serialfd != -1) {
        break;
      }
    }
    if(serialfd == -1) {
      err(1, "can't open siodev");
    }
  }

  /* will only be for the write */
  select_set_callback(serialfd, &slip_callback);

  if(br_config_host != NULL) {
    YLOG_INFO("******** opened to ``%s:%s''\n", br_config_host,
              br_config_port);
  } else {
    YLOG_INFO("******** started on ``/dev/%s''\n", br_config_siodev);
    stty_telos(serialfd);
  }

  if(br_config_siodev_delay > 0) {
    YLOG_DEBUG("6LoWPAN fragment send delay: %u msec\n",
               (unsigned)br_config_siodev_delay);
  }
  serial_set_send_delay(br_config_siodev_delay);

  process_start(&serial_input_process, NULL);

  rc = pthread_create(&thread, NULL, read_code, NULL);
  if(rc) {
    YLOG_ERROR("failed to start the serial reader thread: %d\n", rc);
    exit(EXIT_FAILURE);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS(serial_input_process, "Input Process");

PROCESS_THREAD(serial_input_process, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(write_pos != read_pos);
    serial_input();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
