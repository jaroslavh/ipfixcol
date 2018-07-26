/**
 * \file storage_basic.h
 * \author Lukas Hutak <xhutak01@stud.fit.vutbr.cz>
 * \brief Basic storage management (source file)
 *
 * Copyright (C) 2015-2017 CESNET, z.s.p.o.
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

#include <ipfixcol.h>
#include "lnfstore.h"

#include "storage_basic.h"
#include "storage_common.h"
#include "configuration.h"

/** \brief Basic storage structure */
struct stg_basic_s {
	/** Pointer to the plugin configuration */
	const struct conf_params *params;
	files_mgr_t *mgr; /**< Output files     */
};


stg_basic_t *
stg_basic_create(const struct conf_params *params)
{
	// Prepare the internal structure
	stg_basic_t *instance = (stg_basic_t *) calloc(1, sizeof(*instance));
	if (!instance) {
		MSG_ERROR(msg_module, "Unable to allocate memory (%s:%d)",
			__FILE__, __LINE__);
		return NULL;
	}

	// Create an output file manager
	files_mgr_t * mgr;
	mgr = stg_common_files_mgr_create(params, params->files.path);
	if (!mgr) {
		MSG_ERROR(msg_module, "Failed to create output manager.");
		return NULL;
	}

	instance->params = params;
	instance->mgr = mgr;
	return instance;
}

void
stg_basic_destroy(stg_basic_t *storage)
{
	files_mgr_destroy(storage->mgr);
	free(storage);
}

int
stg_basic_store(stg_basic_t *storage, lnf_rec_t *rec)
{
	return files_mgr_add_record(storage->mgr, rec);
}

int
stg_basic_new_window(stg_basic_t *storage, time_t window)
{
	int ret = files_mgr_new_window(storage->mgr, &window);
	if (ret) {
		MSG_WARNING(msg_module, "New time window is not properly created.");
		return 1;
	} else {
		MSG_INFO(msg_module, "New time window successfully created.");
		return 0;
	}
}



