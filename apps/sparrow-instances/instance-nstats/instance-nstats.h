/*
 * Copyright (c) 2014-2016, Yanzi Networks AB.
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
 *         Instance for network statistics
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef INSTANCE_NSTATS_H_
#define INSTANCE_NSTATS_H_

#define INSTANCE_NSTATS_VERSION                  0

#define INSTANCE_NSTATS_CAPABILITY_PUSH      0x400
#define INSTANCE_NSTATS_CAPABILITY_ROUTING   0x200
#define INSTANCE_NSTATS_CAPABILITY_ROUTER    0x100

#define INSTANCE_NSTATS_DATA_VERSION             0

#define INSTANCE_NSTATS_DATA_NONE                0
#define INSTANCE_NSTATS_DATA_DEFAULT             1
#define INSTANCE_NSTATS_DATA_PARENT_INFO         2
#define INSTANCE_NSTATS_DATA_BEACONS             3
#define INSTANCE_NSTATS_DATA_NETSELECT           4
#define INSTANCE_NSTATS_DATA_RADIO               5
#define INSTANCE_NSTATS_DATA_CONFIG              6

/* INSTANCE_NSTATS_DATA_DEFAULT */
typedef struct {
  uint8_t seqno;                     /* Lollipop */
  uint8_t free_routes;
  uint8_t free_neighbors;
  uint8_t parent_switches;

  uint8_t parent[4];

  uint8_t parent_rank[2];
  uint8_t parent_metric[2];

  uint8_t dag_rank[2];
  uint8_t dio_interval;
  uint8_t dio_sent;

  uint8_t dao_received;
  uint8_t dao_accepted;
  uint8_t dao_sent;
  uint8_t dao_nopath_sent;

  uint8_t dao_nack_sent;
  uint8_t dao_loops_detected;
  uint8_t ext_hdr_errors;
  uint8_t oam_last_activity;
} network_stats_default_t;

/* INSTANCE_NSTATS_DATA_PARENT_INFO */
typedef struct {
  uint8_t last_parent_switch[4];     /* Time since last switch in seconds */
} network_stats_parent_info_t;

/* INSTANCE_NSTATS_DATA_NETSELECT */
typedef struct {
  uint8_t netselect_elapsed[4];      /* Seconds spent in netselect */
  uint8_t netselect_last[4];         /* Seconds since last in netselect */
  uint8_t netselect_count;           /* Number of times in netselect since boot */
  uint8_t reserved[3];
} network_stats_netselect_t;

/* INSTANCE_NSTATS_DATA_BEACONS */
typedef struct {
  uint8_t beacons_received[2];
  uint8_t beacons_sent[2];
  uint8_t beacons_reqs_sent[2];
  uint8_t beacons_reqs_received[2];
} network_stats_beacons_t;

/* INSTANCE_NSTATS_DATA_RADIO */
typedef struct {
  uint8_t ll_acks[2];
  uint8_t ll_noacks[2];

  uint8_t ll_broadcasts[2];
  uint8_t ll_rexmits[2];

  uint8_t ll_rx[2];
  uint8_t ll_contentiondrop[2];
} network_stats_radio_t;

/* INSTANCE_NSTATS_DATA_CONFIG */
typedef struct {
  uint8_t routes[2];
  uint8_t neighbors[2];

  uint8_t defroutes[2];
  uint8_t prefixes[2];

  uint8_t unicast_addresses[2];
  uint8_t multicast_addresses[2];

  uint8_t anycast_addresses[2];
  uint8_t reserved[2];
} network_stats_config_t;

#endif /* INSTANCE_NSTATS_H_ */
