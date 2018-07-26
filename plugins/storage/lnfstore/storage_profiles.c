/**
 * \file storage_profiles.c
 * \author Lukas Hutak <xhutak01@stud.fit.vutbr.cz>
 * \brief Profile storage management (source file)
 *
 * Copyright (C) 2015, 2016 CESNET, z.s.p.o.
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
#include <string.h>
#include <ipfixcol/profiles.h>
#include <limits.h>

#include "storage_profiles.h"
#include "lnfstore.h"
#include "files_manager.h"
#include "profiler_events.h"
#include "configuration.h"
#include "storage_common.h"

/**
 * \brief Global data shared among all channels (read-only)
 */
struct stg_profiles_global {
	/** Pointer to the plugin parameters */
	const struct conf_params *params;
	/** Start of current time window (required for runtime reconfiguration of
	 *  channels i.e. creating/deleting) */
	time_t window_start;

	/** Operation status (returns status of selected callbacks) */
	int op_status;
};

/**
 * \brief Local data for each channel
 */
struct stg_profiles_chnl_local {
	/** Output file(s)  */
	files_mgr_t *manager;
};

/**
 * \brief Internal structure of the manager
 */
struct stg_profiles_s {
	/** Profile event manager */
	pevents_t *event_mgr;

	/** Pointer to the global parameters shared among all channels */
	struct stg_profiles_global global;
};

/**
 * \brief Generate an output directory filename of a channel
 *
 * Based on an output directory specified by the channel's parent profile and
 * channel name generates directory filename.
 * Format: 'profile_dir'/'chanel_name'̈́/
 *
 * \warning Return value MUST be freed by user using free().
 * \param[in] channel Pointer to the channel
 * \return On success returns a pointer to the filename (string). Otherwise
 *   returns a non-zero value.
 */
static char *
channel_get_dirname(void *channel)
{
	const char *channel_subdir = "channels";
	const char *channel_name;
	const char *profile_dir;
	void *profile_ptr;

	channel_name = channel_get_name(channel);
	profile_ptr  = channel_get_profile(channel);
	profile_dir  = profile_get_directory(profile_ptr);

	const size_t len_extra = strlen(channel_subdir) + 4U; // 4 = 3x'/' + 1x'\0'
	size_t dir_len = strlen(profile_dir) + strlen(channel_name) + len_extra;
	if (dir_len >= PATH_MAX) {
		MSG_ERROR(msg_module, "Failed to create directory path (Directory name "
			"is too long)");
		return NULL;
	}

	char *out_dir = (char *) malloc(dir_len * sizeof(char));
	if (!out_dir) {
		MSG_ERROR(msg_module, "Unable to allocate memory (%s:%d)",
			__FILE__, __LINE__);
		return NULL;
	}

	int ret = snprintf(out_dir, dir_len, "%s/%s/%s/", profile_dir,
		channel_subdir, channel_name);
	if (ret < 0 || ((size_t) ret) >= dir_len) {
		MSG_ERROR(msg_module, "snprintf() error");
		free(out_dir);
		return NULL;
	}

	return out_dir;
}

/**
 * \brief Close a channel's storage
 *
 * Destroy a files manager.
 * \param[in,out] local Local data of the channel
 */
static void
channel_storage_close(struct stg_profiles_chnl_local *local)
{
	// Close & delete a files manager
	if (local->manager) {
		files_mgr_destroy(local->manager);
		local->manager = NULL;
	}
}

/**
 * \brief Open a channel's storage
 *
 * First, if the previous instance of the storage exists, it'll be closed and
 * deleted. Second, the function generates a name of an output directory
 * and creates a new files manager for output files.
 * \param[in,out] local       Local data of the channel
 * \param[in]     global      Global structure shared among all channels
 * \param[in]     channel_ptr Pointer to a channel (from a profiler)
 * \return On success returns 0. Otherwise returns a non-zero value and the
 *   files manager is not initialized (i.e. no records can be created).
 */
static int
channel_storage_open(struct stg_profiles_chnl_local *local,
	const struct stg_profiles_global *global, void *channel_ptr)
{
	// Make sure that the previous instance is deleted
	channel_storage_close(local);

	// Get a new directory name
	char *dir = channel_get_dirname(channel_ptr);
	if (!dir) {
		// Failed to create the directory name
		return 1;
	}

	// Create a new files manager
	files_mgr_t *new_mgr;
	new_mgr = stg_common_files_mgr_create(global->params, dir);
	free(dir);
	if (!new_mgr) {
		// Failed to create a files manager
		return 1;
	}

	local->manager = new_mgr;
	return 0;
}

/**
 * \brief Create a new time window
 *
 * Close & reopen output files
 * \param[in,out] local       Local data of the channel
 * \param[in]     global      Global structure shared among all channels
 * \param[in]     channel_ptr Pointer to a channel (from a profiler)
 * \return On success returns 0. Otherwise returns a non-zero value.
 */
static int
channel_storage_new_window(struct stg_profiles_chnl_local *local,
	const struct stg_profiles_global *global)
{
	if (!local->manager) {
		// Uninitialized manager
		return 1;
	}

	// Start a time window
	if (files_mgr_new_window(local->manager, &global->window_start)) {
		// Totally or partially failed...
		return 1;
	};

	return 0;
}

/**
 * \brief Create a new channel
 *
 * The main purpose of this function is to create files managers (output files)
 * for this channel.
 *
 * \param[in,out] ctx Information context about the channel
 * \return On success returns a pointer to local data. Otherwise returns NULL.
 */
static void *
channel_create_cb(struct pevents_ctx *ctx)
{
	const char *channel_path = channel_get_path(ctx->ptr.channel);
	const char *channel_name = channel_get_name(ctx->ptr.channel);
	MSG_DEBUG(msg_module, "Processing new channel '%s%s'...", channel_path,
		channel_name);

	// Create an internal structure
	struct stg_profiles_chnl_local *local_data;
	local_data = calloc(1, sizeof(*local_data));
	if (!local_data) {
		MSG_ERROR(msg_module, "Unable to allocate memory (%s:%d)",
			__FILE__, __LINE__);
		MSG_ERROR(msg_module, "Failed to create storage of channel '%s%s' "
			"(memory allocation error). Unrecoverable error occurred, please, "
			"delete and create the channel or restart this plugin.",
			channel_path, channel_name);
		return NULL;
	}

	if (channel_storage_open(local_data, ctx->user.global, ctx->ptr.channel)) {
		// Failed
		MSG_WARNING(msg_module, "Failed to create storage of channel '%s%s'. "
			"Further records of this channel will NOT be stored.",
			channel_path, channel_name);
		return local_data;
	}

	if (channel_storage_new_window(local_data, ctx->user.global)) {
		// Failed
		MSG_WARNING(msg_module, "Failed to create a new time window of channel "
			"'%s%s'. Output file(s) of this channel are not prepared and "
			"further records will NOT be stored.", channel_path, channel_name);
		return local_data;
	}

	MSG_INFO(msg_module, "Channel '%s%s' has been successfully created.",
		channel_path, channel_name);
	return local_data;
}

/**
 * \brief Delete a channel
 *
 * The function deletes early create a file manager (aka local data)
 * \param[in,out] ctx Information context about the channel
 */
static void
channel_delete_cb(struct pevents_ctx *ctx)
{
	const char *channel_path = channel_get_path(ctx->ptr.channel);
	const char *channel_name = channel_get_name(ctx->ptr.channel);
	MSG_DEBUG(msg_module, "Deleting channel '%s%s'...", channel_path,
		channel_name);

	struct stg_profiles_chnl_local *local_data;
	local_data = (struct stg_profiles_chnl_local *) ctx->user.local;
	if (local_data != NULL) {
		channel_storage_close(local_data);
		free(local_data);
	}

	MSG_INFO(msg_module, "Channel '%s%s' has been successfully closed.",
		channel_path, channel_name);
}

/**
 * \brief Update a channel
 *
 * Based on a configuration of the channel and parent's profile, open/change/
 * close file storage.
 *
 * \param[in,out] ctx   Information context about the channel
 * \param[in]     flags Changes (see #PEVENTS_CHANGE)
 */
static void
channel_update_cb(struct pevents_ctx *ctx, uint16_t flags)
{
	const char *channel_path = channel_get_path(ctx->ptr.channel);
	const char *channel_name = channel_get_name(ctx->ptr.channel);
	MSG_DEBUG(msg_module, "Updating channel '%s%s'...", channel_path,
		channel_name);

	struct stg_profiles_chnl_local *local_data;
	local_data = (struct stg_profiles_chnl_local *) ctx->user.local;
	if (!local_data) {
		MSG_ERROR(msg_module, "Channel '%s%s' cannot be updated, because it's "
			"not properly initialized. Try to delete it from a profiling "
			"configuration and create it again.", channel_path, channel_name);
		return;
	}

	void *profile = channel_get_profile(ctx->ptr.channel);
	const enum PROFILE_TYPE type = profile_get_type(profile);

	// Is the profile's type still the same?
	if (type == PT_SHADOW) {
		// The type is shadow -> Delete the files manager, if it exists.
		if (local_data->manager == NULL) {
			// Already deleted
			return;
		}

		channel_storage_close(ctx->user.local);
		MSG_INFO(msg_module, "Channel '%s%s' has been successfully updated "
				"(storage has been closed).", channel_path, channel_name);

		return;
	}

	// Only "Normal" type can be here!
	if ((flags & PEVENTS_CHANGE_DIR) || (flags & PEVENTS_CHANGE_TYPE)) {
		// The profile's directory and/or type has been changed.
		// Delete & Create a new storage
		if (channel_storage_open(ctx->user.local, ctx->user.global,
				ctx->ptr.channel)) {
			// Failed to open
			MSG_WARNING(msg_module, "Failed to create/change storage of "
				"channel '%s%s'. The current storage has been closed to"
				"prevent potential collision with other profiles/channels "
				"and further records of this channel will NOT be stored.",
				channel_path, channel_name);
			return;
		}

		if (channel_storage_new_window(ctx->user.local, ctx->user.global)) {
			// Failed
			MSG_WARNING(msg_module, "Failed to create a new time window of "
				"channel '%s%s'. Output file(s) of this channel are not "
				"prepared and further records will NOT be stored.",
				channel_path, channel_name);
			return;
		}

		MSG_INFO(msg_module, "Channel '%s%s' has been successfully updated "
			"(storage has been created/changed).", channel_path, channel_name);
		return;
	}
}

/**
 * \brief Process data for a channel
 *
 * Data record is stored to output file(s)
 * \param[in,out] ctx Information context about the channel
 * \param[in] data LNF record
 */
static void
channel_data_cb(struct pevents_ctx *ctx, void *data)
{
	struct stg_profiles_chnl_local *local_data;
	local_data = (struct stg_profiles_chnl_local *) ctx->user.local;
	if (!local_data || !local_data->manager) {
		// A file manager is not available
		return;
	}

	lnf_rec_t *rec_ptr = data;
	int ret = files_mgr_add_record(local_data->manager, rec_ptr);
	if (ret != 0) {
		// Failed
		void *channel = ctx->ptr.channel;
		MSG_DEBUG(msg_module, "Failed to store a record into channel '%s%s'.",
			channel_get_path(channel), channel_get_name(channel));
	}
}

/**
 * \brief Auxiliary function for changing time windows
 * \param[in,out] ctx Information context about the channel
 */
static void
channel_new_window(struct pevents_ctx *ctx)
{
	if (!ctx->user.local) {
		// Local data is not initialized -> unrecoverable error
		return;
	}

	struct stg_profiles_global *global = ctx->user.global;
	int ret_val = channel_storage_new_window(ctx->user.local, global);
	if (ret_val != 0) {
		// Failed
		void *channel = ctx->ptr.channel;
		MSG_WARNING(msg_module, "Failed to create a new time window of "
			"channel '%s%s'. Output file(s) of this channel are not prepared "
			"and further records will NOT be stored.",
			channel_get_path(channel), channel_get_name(channel));
		global->op_status = 1;
	}
}

stg_profiles_t *
stg_profiles_create(const struct conf_params *params)
{
	// Prepare an internal structure
	stg_profiles_t *mgr = (stg_profiles_t *) calloc(1, sizeof(*mgr));
	if (!mgr) {
		MSG_ERROR(msg_module, "Unable to allocate memory (%s:%d)",
			__FILE__, __LINE__);
		return NULL;
	}

	mgr->global.params = params;

	// Initialize an array of callbacks
	struct pevent_cb_set channel_cb;
	memset(&channel_cb, 0, sizeof(channel_cb));

	struct pevent_cb_set profile_cb;
	memset(&profile_cb, 0, sizeof(profile_cb));

	channel_cb.on_create = &channel_create_cb;
	channel_cb.on_delete = &channel_delete_cb;
	channel_cb.on_update = &channel_update_cb;
	channel_cb.on_data   = &channel_data_cb;

	// Create an event manager
	mgr->event_mgr = pevents_create(profile_cb, channel_cb);
	if (!mgr->event_mgr) {
		// Failed
		free(mgr);
		return NULL;
	}

	pevents_global_set(mgr->event_mgr, &mgr->global);
	return mgr;
}

void
stg_profiles_destroy(stg_profiles_t *storage)
{
	// Destroy a profile manager and close all files (delete callback)
	pevents_destroy(storage->event_mgr);
	free(storage);
}

int
stg_profiles_store(stg_profiles_t *storage, const struct metadata *mdata,
	lnf_rec_t *rec)
{
	// Store the record to the channels
	return pevents_process(storage->event_mgr, (const void **)mdata->channels,
		rec);
}

int
stg_profiles_new_window(stg_profiles_t *storage, time_t window)
{
	storage->global.window_start = window;
	storage->global.op_status = 0;

	pevents_for_each(storage->event_mgr, NULL, &channel_new_window);
	return storage->global.op_status;
}
