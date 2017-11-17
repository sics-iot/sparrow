/*
 * Copyright (c) 2017, RISE SICS AB.
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
 *         Neighbours instance
 * \author
 *         Bengt Ahlgren <bengta@sics.se>
 */

/*
 * Modelled after instance-rtable* by
 * Fredrik Ljungberg <flag@yanzinetworks.com>
 * Joakim Eriksson <joakime@sics.se>
 * Niclas Finne <nfi@sics.se>
 */

#include <stdlib.h>
#include "sparrow-oam.h"
#include "instance-nbr.h"
#include "instance-nbr-var.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/link-stats.h"
#include "net/ipv6/uip-ds6-nbr.h"

#define YLOG_LEVEL YLOG_LEVEL_NONE
#define YLOG_NAME  "nbr"
#include "ylog.h"

static nbr_entry_t *local_nbr_table = NULL;
static int nbr_table_length = 0;

/*---------------------------------------------------------------------------*/
static void update_local_nbr_table()
{
    nbr_entry_t *new_nbr_table, *old_nbr_table;
    uip_ds6_nbr_t *nbr;
    rpl_instance_t *def_instance;
    const uip_lladdr_t *lladdr;
    rpl_parent_t *p;
    const struct link_stats *ls;
    int num_nbr, i;

    def_instance = rpl_get_default_instance();

    /* alloc new table */
    num_nbr = uip_ds6_nbr_num();
    new_nbr_table = malloc(num_nbr * sizeof(nbr_entry_t));
    if (new_nbr_table == NULL) {
	YLOG_ERROR("Failed to alloc new table.\n");
	return;
    }
    memset(new_nbr_table, 0, num_nbr * sizeof(nbr_entry_t));

    /* get all info similarly to "nbr" command */
    for (nbr = nbr_table_head(ds6_neighbors), i = 0;
	 (nbr != NULL) && (i < num_nbr);
	 nbr = nbr_table_next(ds6_neighbors, nbr), i++) {

	new_nbr_table[i].ipaddr = nbr->ipaddr;
	new_nbr_table[i].remaining =
	    uip_htonl(stimer_expired(&nbr->reachable) ? 0 : stimer_remaining(&nbr->reachable));
	new_nbr_table[i].state = nbr->state;

        lladdr = uip_ds6_nbr_get_ll(nbr);
        p = nbr_table_get_from_lladdr(rpl_parents, (const linkaddr_t*)lladdr);

	if (p != NULL) {
	    new_nbr_table[i].rpl_flags = p->flags;
	    new_nbr_table[i].rpl_flags |= NBR_FLAG_PARENT;

	    if(def_instance != NULL && def_instance->current_dag != NULL &&
	       def_instance->current_dag->preferred_parent == p)
		new_nbr_table[i].rpl_flags |= NBR_FLAG_PREFERRED;

	    new_nbr_table[i].rpl_rank = uip_htons(p->rank);
	}

        ls = link_stats_from_lladdr((const linkaddr_t *)lladdr);
	new_nbr_table[i].link_etx = uip_htons(ls->etx);
	new_nbr_table[i].link_rssi = uip_htons(ls->rssi);
	new_nbr_table[i].link_fresh = ls->freshness;
    }

    if (num_nbr != i) {
	YLOG_DEBUG("Neighbour count mismatch (%d vs %d) - neighbour list updated while read.\n", num_nbr, i);
	num_nbr = i;
    }

    /* switch to new table */
    old_nbr_table = local_nbr_table;
    local_nbr_table = new_nbr_table;
    nbr_table_length = num_nbr;
    /* free old table */
    if (old_nbr_table != NULL)
	free(old_nbr_table);
}
/*---------------------------------------------------------------------------*/
/**
 * Process a request TLV.
 *
 * Writes a response TLV starting at "reply", no more than "len"
 * bytes.
 *
 * Returns number of bytes written to "reply".
 */
static size_t
nbr_process_request(const sparrow_oam_instance_t *instance,
		    sparrow_tlv_t *request, uint8_t *reply, size_t len,
		    sparrow_oam_processing_t *oam_processing)
{
    uint8_t error = SPARROW_TLV_ERROR_UNKNOWN_VARIABLE;

    if (request->variable == VARIABLE_NBR_COUNT) {
	if (request->opcode == SPARROW_TLV_OPCODE_GET_REQUEST) {
	    update_local_nbr_table();
	    return sparrow_tlv_write_reply32int(request, reply, len, nbr_table_length);
	}

	error = SPARROW_TLV_ERROR_UNKNOWN_OP_CODE;
    }

    else if (request->variable == VARIABLE_NBR_TABLE) {
	if (request->opcode == SPARROW_TLV_OPCODE_VECTOR_GET_REQUEST) {
	    if (request->offset > nbr_table_length)
		request->elements = 0;
	    else
		request->elements = MIN(request->elements, (nbr_table_length - request->offset));
	    return sparrow_tlv_write_reply_vector(request, reply, len, ((uint8_t *)local_nbr_table));
	}

	error = SPARROW_TLV_ERROR_UNKNOWN_OP_CODE;
    }

    return sparrow_tlv_write_reply_error(request, error, reply, len);
}
/*---------------------------------------------------------------------------*/
SPARROW_OAM_INSTANCE(instance_nbr,
                     INSTANCE_NBR_OBJECT_TYPE, INSTANCE_NBR_LABEL,
                     instance_nbr_variables,
                     .process_request = nbr_process_request);
/*---------------------------------------------------------------------------*/
