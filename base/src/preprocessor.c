/**
 * \file preprocessor.c
 * \author Radek Krejci <rkrejci@cesnet.cz>
 * \brief Simple parsing of IPFIX packets for Storage plugins.
 *
 * Copyright (C) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#include "configurator.h"
#include "preprocessor.h"
#include "data_manager.h"
#include "queues.h"
#include <ipfixcol.h>
#include <ipfixcol/ipfix_message.h>
#include "crc.h"

/** Identifier to MSG_* macros */
static char *msg_module = "preprocessor";

static struct ring_buffer *preprocessor_out_queue = NULL;
static configurator *global_config = NULL;

/* Sequence number counter for each flow data source */
struct data_source_info {
	uint32_t exporter_ip_addr, odid, sequence_number;
	uint32_t free_tid;
	struct data_source_info *next;
};

struct data_source_info *data_source_info = NULL;

/**
 * \brief Get sequence number counter for given flow data source
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return Pointer to sequence number counter
 */
struct data_source_info *data_source_info_get(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info;
	while (aux_info) {
		if (aux_info->exporter_ip_addr == exporter_ip_addr && aux_info->odid == odid) {
			return aux_info;
		}

		aux_info = aux_info->next;
	}

	return NULL;
}

/**
 * \brief Add new flow data source info
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return Pointer to sequence number counter
 */
struct data_source_info *data_source_info_add(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info;

	aux_info = calloc(1, sizeof(struct data_source_info));
	if (!aux_info) {
		MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
		return NULL;
	}

	aux_info->exporter_ip_addr = exporter_ip_addr;
	aux_info->odid = odid;
	aux_info->free_tid = 256;

	if (!data_source_info) {
		data_source_info = aux_info;
	} else {
		aux_info->next = data_source_info->next;
		data_source_info->next = aux_info;
	}

	return aux_info;
}

/**
 * \brief Add new data source info
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return Pointer to sequence number counter
 */
struct data_source_info *data_source_info_add_source(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info_get(exporter_ip_addr, odid);
	if (!aux_info) {
		return data_source_info_add(exporter_ip_addr, odid);
	}

	MSG_WARNING(msg_module, "Something strange has happened; trying to add the same data source again");
	return NULL;
}

/**
 * \brief Remove data source info
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 */
void data_source_info_remove_source(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info_get(exporter_ip_addr, odid);
	if (!aux_info) {
		return;
	}
}

/**
 * \brief Get data source info struct or add a new one
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return data_source_info
 */
struct data_source_info *data_source_info_get_or_add(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info_get(exporter_ip_addr, odid);
	if (!aux_info) {
		aux_info = data_source_info_add(exporter_ip_addr, odid);
	}

	return aux_info;
}

/**
 * \brief Get sequence number for given data source
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return Pointer to sequence number value
 */
uint32_t *data_source_info_get_sequence_number(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info_get_or_add(exporter_ip_addr, odid);
	if (!aux_info) {
		return NULL;
	}

	return &(aux_info->sequence_number);
}

/**
 * \brief Get free template ID for data source
 *
 * \param[in] exporter_ip_addr CRC32 of exporter IP address
 * \param[in] odid Observation Domain ID
 * \return Next free template ID
 */
uint32_t data_source_info_get_free_tid(uint32_t exporter_ip_addr, uint32_t odid)
{
	struct data_source_info *aux_info = data_source_info_get_or_add(exporter_ip_addr, odid);
	if (!aux_info) {
		return 256;
	}

	return aux_info->free_tid++;
}

/**
 * \brief Remove all data source info
 */
void data_source_info_destroy()
{
	struct data_source_info *aux_info = data_source_info;
	while (aux_info) {
		data_source_info = data_source_info->next;
		free(aux_info);
		aux_info = data_source_info;
	}
}

/**
 * \brief Set new output queue
 */
void preprocessor_set_output_queue(struct ring_buffer *out_queue)
{
	preprocessor_out_queue = out_queue;
}

/**
 * \brief Set new configurator
 *
 * \param[in] conf configurator
 */
void preprocessor_set_configurator(configurator *conf)
{
	global_config = conf;
}

/**
 * \brief Returns pointer to preprocessors output queue.
 */
struct ring_buffer *get_preprocessor_output_queue()
{
	return preprocessor_out_queue;
}

uint32_t preprocessor_compute_crc(struct input_info *input_info)
{
	if (input_info->type == SOURCE_TYPE_IPFIX_FILE) {
		struct input_info_file *input_file = (struct input_info_file *) input_info;
		return crc32(input_file->name, strlen(input_file->name));
	}

	struct input_info_network *input = (struct input_info_network *) input_info;

	char buff[INET6_ADDRSTRLEN + 5 + 1]; // 5: port; 1: null
	if (input->l3_proto == 6) { /* IPv6 */
		inet_ntop(AF_INET6, &(input->src_addr.ipv6.s6_addr), buff, INET6_ADDRSTRLEN);
	} else { /* IPv4 */
		inet_ntop(AF_INET, &(input->src_addr.ipv4.s_addr), buff, INET_ADDRSTRLEN);
	}

	uint8_t ip_addr_len = strlen(buff);
	snprintf(buff + ip_addr_len, 5 + 1, "%u", input->src_port);

	return crc32(buff, strlen(buff));
}

/**
 * \brief Fill in udp_info structure when managing UDP input
 *
 * @param[in] input_info 	Input information from input plugin
 * @param[in,out] udp_conf 	UDP template configuration structure to be filled in
 * @return void
 */
static void preprocessor_udp_init(struct input_info_network *input_info, struct udp_conf *udp_conf)
{
	if (input_info->type == SOURCE_TYPE_UDP) {
		if (input_info->template_life_time != NULL) {
			udp_conf->template_life_time = atoi(((struct input_info_network *) input_info)->template_life_time);
		} else {
			udp_conf->template_life_time = TM_UDP_TIMEOUT;
		}

		if (input_info->template_life_packet != NULL) {
			udp_conf->template_life_packet = atoi(input_info->template_life_packet);
		} else {
			udp_conf->template_life_packet = 0;
		}

		if (input_info->options_template_life_time != NULL) {
			udp_conf->options_template_life_time = atoi(((struct input_info_network *) input_info)->options_template_life_time);
		} else {
			udp_conf->options_template_life_time = TM_UDP_TIMEOUT;
		}

		if (input_info->options_template_life_packet != NULL) {
			udp_conf->options_template_life_packet = atoi(input_info->options_template_life_packet);
		} else {
			udp_conf->options_template_life_packet = 0;
		}
	}
}

/**
 * \brief Process one template from template set
 *
 * \param[in] tmpl template
 * \param[in] max_len maximal length of this template
 * \param[in] type type of the template
 * \param[in] msg_counter message counter
 * \param[in] input_info input info structure
 * \param[in] key template key with filled crc and odid
 * \return length of the template
 */
static int preprocessor_process_one_template(void *tmpl, int max_len, int type,
		uint32_t msg_counter, struct input_info *input_info, struct ipfix_template_key *key)
{
	struct ipfix_template_record *template_record;
	struct ipfix_template *template;
	uint16_t template_id;
	int ret;

	template_record = (struct ipfix_template_record *) tmpl;
	template_id = ntohs(template_record->template_id);
	key->tid = template_id;

	/* check for withdraw all templates message */
	/* these templates are no longer used (checked in data_manager_withdraw_templates()) */
	if (input_info->type == SOURCE_TYPE_UDP && ntohs(template_record->count) == 0) {
		/* got withdrawal message with UDP -> this is wrong */
		MSG_WARNING(msg_module, "[%u] Received template withdrawal message over UDP; ignoring...", input_info->odid);
		return TM_TEMPLATE_WITHDRAW_LEN;
	} else if ((template_id == IPFIX_TEMPLATE_FLOWSET_ID ||
				template_id == IPFIX_OPTION_FLOWSET_ID) &&
			ntohs(template_record->count) == 0) {
		/* withdraw template or option template */
		// TODO: add support
		/* don't try to parse the withdraw template */
		MSG_WARNING(msg_module, "[%u] Received All %s withdrawal message. Not supported by this version of the collector!", input_info->odid, (type == TM_TEMPLATE) ? "Template" : "Options template");
		return TM_TEMPLATE_WITHDRAW_LEN;
		/* check for withdraw template message */
	} else if (ntohs(template_record->count) == 0) {
		ret = tm_remove_template(template_mgr, key);
		MSG_INFO(msg_module, "[%u] Received %s withdrawal message", input_info->odid, (type == TM_TEMPLATE) ? "Template" : "Options template");
		/* Log error when removing unknown template */
		if (ret == 1) {
			MSG_WARNING(msg_module, "[%u] %s withdrawal message received for unknown template ID %u", input_info->odid,
					(type == TM_TEMPLATE) ? "Template" : "Options template", template_id);
		}
		return TM_TEMPLATE_WITHDRAW_LEN;
		/* check whether template exists */
	} else if ((template = tm_get_template(template_mgr, key)) == NULL) {
		/* add template */
		/* check that the template has valid ID (>= 256) */
		if (template_id < 256) {
			MSG_WARNING(msg_module, "[%u] %s ID %i is reserved and not valid for data set", key->odid, (type == TM_TEMPLATE) ? "Template" : "Options template", template_id);
		} else {
			MSG_INFO(msg_module, "[%u] New %s ID %i", key->odid, (type == TM_TEMPLATE) ? "template" : "options template", template_id);
			template = tm_add_template(template_mgr, tmpl, max_len, type, key);
			/* Set new template ID according to ODID */
			if (template) {
				template->template_id = data_source_info_get_free_tid(preprocessor_compute_crc(input_info), key->odid);
			}
		}
	} else {
		/* template already exists */
		MSG_DEBUG(msg_module, "[%u] %s ID %i already exists; rewriting...", key->odid,
				(type == TM_TEMPLATE) ? "Template" : "Options template", template->template_id);
		template = tm_update_template(template_mgr, tmpl, max_len, type, key);
	}

	if (template == NULL) {
		MSG_WARNING(msg_module, "[%u] Cannot parse %s set; skipping to next set...", key->odid,
				(type == TM_TEMPLATE) ? "template" : "options template");
		return 0;
		/* update UDP timeouts */
	} else if (input_info->type == SOURCE_TYPE_UDP) {
		template->last_message = msg_counter;
		template->last_transmission = time(NULL);
	}

	/* Set new template id to original template record */
	template_record->template_id = htons(template->template_id);

	/* return the length of the original template:
	 * = template length - template header length + template record header length */
	if (type == TM_TEMPLATE) {
		return template->template_length - sizeof(struct ipfix_template) + sizeof(struct ipfix_template_record);
	}

	/* length of the options template */
	return template->template_length - sizeof(struct ipfix_template) + sizeof(struct ipfix_options_template_record);
}

static int mdata_max = 0;

void fill_metadata(uint8_t *rec, int rec_len, struct ipfix_template *templ, void *data)
{
	struct ipfix_message *msg = (struct ipfix_message *) data;

	/* Allocate space for metadata */
	if (mdata_max == 0) {
		mdata_max = 75;
		msg->metadata = calloc(mdata_max, sizeof(struct metadata));
		if (!msg->metadata) {
			MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
			mdata_max = 0;
			return;
		}
	}

	/* Need more space */
	if (msg->data_records_count == mdata_max) {
		void *new_mdata = realloc(msg->metadata, mdata_max * 2 * sizeof(struct metadata));

		if (!new_mdata) {
			MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
			return;
		}

		msg->metadata = new_mdata;
		memset(&(msg->metadata[mdata_max]), 0, mdata_max * sizeof(struct metadata));

		mdata_max *= 2;
	}

	/* Fill metadata */
	msg->live_profile = (global_config) ? config_get_current_profiles(global_config) : NULL;
	msg->metadata[msg->data_records_count].record.record = rec;
	msg->metadata[msg->data_records_count].record.length = rec_len;
	msg->metadata[msg->data_records_count].record.templ = templ;

	msg->data_records_count++;
}

/**
 * \brief Process templates
 *
 * Currently template management does not conform to RFC 5101 in following:
 * - If template is reused without previous withdrawal or timeout (UDP),
 *   only warning is logged and template is updated (template MUST be of
 *   the same length).
 * - If template is not found, data is not coupled with template,
 *   i.e. data_set[x]->template == NULL
 * - When template is malformed and cannot be added to template manager,
 *   rest of the template set is discarded (template length cannot be
 *   determined)
 *
 * @param[in] msg IPFIX			message
 * @return uint32_t Number of received data records
 */
static uint32_t preprocessor_process_templates(struct ipfix_message *msg)
{
	uint8_t *ptr;
	uint32_t records_count = 0;
	int i, ret;
	uint16_t max_len; /* length to the end of the set = max length of the template */
	uint16_t set_len;
	uint32_t msg_counter = 0;

	struct udp_conf udp_conf = {0};
	struct ipfix_template_key key;

	msg->data_records_count = msg->templ_records_count = msg->opt_templ_records_count = 0;

	key.odid = ntohl(msg->pkt_header->observation_domain_id);
	key.crc = preprocessor_compute_crc(msg->input_info);

	preprocessor_udp_init((struct input_info_network *) msg->input_info, &udp_conf);

	/* check for new templates */
	for (i = 0; i < MSG_MAX_TEMPL_SETS && msg->templ_set[i]; i++) {
		ptr = (uint8_t *) &msg->templ_set[i]->first_record;
		set_len = ntohs(msg->templ_set[i]->header.length);
		while (ptr < (uint8_t *) msg->templ_set[i] + set_len) {
			if ((uint8_t *) msg->templ_set[i] + set_len - ptr < 4) {
				/* Stop processing; padding bytes detected */
				break;
			}

			max_len = ((uint8_t *) msg->templ_set[i] + set_len) - ptr;
			ret = preprocessor_process_one_template(ptr, max_len, TM_TEMPLATE, msg_counter, msg->input_info, &key);
			if (ret == 0) {
				break;
			} else {
				msg->templ_records_count++;
				ptr += ret;
			}
		}
	}

	/* check for new option templates */
	for (i = 0; i < MSG_MAX_OTEMPL_SETS && msg->opt_templ_set[i]; i++) {
		ptr = (uint8_t *) &msg->opt_templ_set[i]->first_record;
		set_len = ntohs(msg->opt_templ_set[i]->header.length);
		max_len = ((uint8_t *) msg->opt_templ_set[i] + set_len) - ptr;
		while (ptr < (uint8_t *) msg->opt_templ_set[i] + set_len) {
			if ((uint8_t *) msg->opt_templ_set[i] + set_len - ptr < 4) {
				/* Stop processing; padding bytes detected */
				break;
			}

			ret = preprocessor_process_one_template(ptr, max_len, TM_OPTIONS_TEMPLATE, msg_counter, msg->input_info, &key);
			if (ret == 0) {
				break;
			} else {
				msg->opt_templ_records_count++;
				ptr += ret;
			}
		}
	}

	mdata_max = 0;

	/* add template to message data_couples */
	for (i = 0; i < MSG_MAX_DATA_COUPLES && msg->data_couple[i].data_set; i++) {
		key.tid = ntohs(msg->data_couple[i].data_set->header.flowset_id);
		msg->data_couple[i].data_template = tm_get_template(template_mgr, &key);
		if (msg->data_couple[i].data_template == NULL) {
			MSG_WARNING(msg_module, "[%u] Data template with ID %i not found", key.odid, key.tid);
		} else {
			/* Increasing number of references to template */
			tm_template_reference_inc(msg->data_couple[i].data_template);

			/* Set right flowset ID */
			msg->data_couple[i].data_set->header.flowset_id = htons(msg->data_couple[i].data_template->template_id);

			if ((msg->input_info->type == SOURCE_TYPE_UDP) && /* source UDP */
					((time(NULL) - msg->data_couple[i].data_template->last_transmission > udp_conf.template_life_time) || /* lifetime expired */
					(udp_conf.template_life_packet > 0 && /* life packet should be checked */
					(uint32_t) (msg_counter - msg->data_couple[i].data_template->last_message) > udp_conf.template_life_packet))) {
				MSG_WARNING(msg_module, "[%u] Data template with ID %i has expired; using old template...", key.odid,
						msg->data_couple[i].data_template->template_id);
			}

			/* compute sequence number and fill metadata */
			records_count += data_set_process_records(msg->data_couple[i].data_set, msg->data_couple[i].data_template, fill_metadata, msg);
		}
	}

	/*
	 * FILL METADATA, two options:
	 * a) fill metadata AFTER counting data records
	 *		+ allocate whole array at once
	 *		- data sets and data records are accesed twice (data_set_records_count and data_set_process_records)
	 *
	 * b) fill metadata WHILE counting data records (using now) (replace data_set_records_count with data_set_process_records and add callback)
	 *		+ one acces to data sets
	 *		- needs reallocation
	 */

	/* return number of data records */
	return msg->data_records_count;
}

/**
 * \brief Parse IPFIX message and send it to intermediate plugin or output managers queue
 *
 * @param packet Received data from input plugins
 * @param len Packet length
 * @param input_info Input informations about source etc.
 * @param source_status Status of source (new, opened, closed)
 */
void preprocessor_parse_msg(void* packet, int len, struct input_info* input_info, int source_status)
{
	struct ipfix_message* msg;
	uint32_t exporter_ip_addr;
	uint32_t *seqn;

	/* Check input info */
	if (input_info == NULL) {
		MSG_WARNING(msg_module, "Invalid parameters in preprocessor_parse_msg");

		if (packet) {
			free(packet);
		}

		packet = NULL;
		return;
	}

	/* CRC of exporter identification is used to differentiate sources */
	exporter_ip_addr = preprocessor_compute_crc(input_info);

	if (source_status == SOURCE_STATUS_CLOSED) {
		/* Inform intermediate plugins and output manager about closed input */
		msg = calloc(1, sizeof(struct ipfix_message));
		if (!msg) {
			MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
			return;
		}

		msg->input_info = input_info;
		msg->source_status = source_status;
		data_source_info_remove_source(exporter_ip_addr, input_info->odid);
	} else {
		if (packet == NULL) {
			MSG_WARNING(msg_module, "[%u] Received empty IPFIX message", input_info->odid);
			return;
		}

		/* Process IPFIX packet and fill up the ipfix_message structure */
		msg = message_create_from_mem(packet, len, input_info, source_status);
		if (!msg) {
			free(packet);
			packet = NULL;
			return;
		}

		if (source_status == SOURCE_STATUS_NEW) {
			data_source_info_add_source(exporter_ip_addr, ntohl(msg->pkt_header->observation_domain_id));

			if (tm_source_register(template_mgr, input_info->odid, exporter_ip_addr)) {
				MSG_WARNING(msg_module, "[%u] Unable to register a source in the main template manager!", input_info->odid);
			}
		}

		/* Process templates and correct sequence number */
		preprocessor_process_templates(msg);

		/* Get sequence number for current ODID. More inputs can have the same ODID, so we
		 * need to keep that separately.
		 */
		seqn = data_source_info_get_sequence_number(exporter_ip_addr, ntohl(msg->pkt_header->observation_domain_id));

		/* If we have a message with data records (the only one that updates sequence number), check
		 * the sequence numbers.
		 */
		uint32_t pkt_header_seq_number = ntohl(msg->pkt_header->sequence_number);
		if (msg->input_info->sequence_number != pkt_header_seq_number) {
			/* Only print a warning message if this is not the first message, since the first
			 * message always triggers an alert (sequence number is then compared to zero).
			 */
			if (!skip_seq_err && msg->input_info->packets > 0) {
				MSG_WARNING(msg_module, "[%u] Sequence number error; expected %u, got %u",
						input_info->odid, msg->input_info->sequence_number, pkt_header_seq_number);
			}

			/* Add number of seen data records to ODID sequence number to maintain consistency */
			*seqn += pkt_header_seq_number - msg->input_info->sequence_number;

			/* Update sequence number of the source to get in sync again */
			msg->input_info->sequence_number = pkt_header_seq_number;
		}

		/* The message should have sequence number of the ODID from now on */
		msg->pkt_header->sequence_number = htonl(*seqn);

		/* Add the number of records to both ODID and source sequence numbers (for future check) */
		msg->input_info->sequence_number += msg->data_records_count;
		*seqn += msg->data_records_count;

		/* Update other input_info variables */
		++msg->input_info->packets;
		msg->input_info->data_records += msg->data_records_count;
	}

	/* Send data to the first intermediate plugin */
	if (rbuffer_write(preprocessor_out_queue, msg, 1) != 0) {
		MSG_WARNING(msg_module, "[%u] Unable to write into Data Manager input queue; skipping data...",
				input_info->odid);
		message_free(msg);
		packet = NULL;
	}
}

void preprocessor_close()
{
	/* output queue will be closed by intermediate process or output manager */
	data_source_info_destroy();
	return;
}
