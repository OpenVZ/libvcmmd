/*
 *  Copyright (c) 2015-2017, Parallels International GmbH
 *  Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file is part of OpenVZ libraries. OpenVZ is free software; you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dbus/dbus.h>

#include "vcmmd.h"

static inline bool vcmmd_ve_config_entry_is_string(
		vcmmd_ve_config_key_t key)
{
	if (key == VCMMD_VE_CONFIG_NODE_LIST ||
		key == VCMMD_VE_CONFIG_CPU_LIST)
		return true;
	return false;
}

bool vcmmd_ve_config_extract_string(const struct vcmmd_ve_config *config,
			     vcmmd_ve_config_key_t key, const char **str)
{
	int i;

	if (!vcmmd_ve_config_entry_is_string(key))
		return false;

	for (i = 0; i < config->nr_entries; i++) {
		if (config->entries[i].key == key && config->entries[i].str) {
			*str = config->entries[i].str;
			return true;
		}
	}
	return false;
}

bool vcmmd_ve_config_extract(const struct vcmmd_ve_config *config,
			     vcmmd_ve_config_key_t key, uint64_t *value)
{
	int i;

	if (vcmmd_ve_config_entry_is_string(key))
		return false;

	for (i = 0; i < config->nr_entries; i++) {
		if (config->entries[i].key == key) {
			*value = config->entries[i].value;
			return true;
		}
	}
	return false;
}

static bool vcmmd_ve_config_key_present(const struct vcmmd_ve_config *config,
			     vcmmd_ve_config_key_t key)
{
	int i;
	for (i = 0; i < config->nr_entries; i++) {
		if (config->entries[i].key == key) {
			return true;
		}
	}
	return false;
}

static bool _vcmmd_ve_config_append(struct vcmmd_ve_config *config,
					  vcmmd_ve_config_key_t key,
					  uint64_t value,
					  const char *str)
{
	if (vcmmd_ve_config_key_present(config, key) ||
		config->nr_entries == __NR_VCMMD_VE_CONFIG_KEYS)
		return false;

	char *str_dup = strdup(str ? str : "");
	if (!str_dup)
		return false;

	struct vcmmd_ve_config_entry entry = {
		.key = key,
		.value = value,
		.str = str_dup,
	};

	config->entries[config->nr_entries++] = entry;
	return true;
}

bool vcmmd_ve_config_append_string(struct vcmmd_ve_config *config,
					  vcmmd_ve_config_key_t key,
					  const char *str)
{

	if (!vcmmd_ve_config_entry_is_string(key))
		return false;

	return _vcmmd_ve_config_append(config, key, 0, str);
}

bool vcmmd_ve_config_append(struct vcmmd_ve_config *config,
					  vcmmd_ve_config_key_t key,
					  uint64_t value)
{
	if (vcmmd_ve_config_entry_is_string(key))
		return false;

	return _vcmmd_ve_config_append(config, key, value, NULL);
}

char *vcmmd_strerror(int err, char *buf, size_t buflen)
{
	static const char *success = "Success";
	static const char *unknown = "Unknown error";

	static const char *service_err_list[] = {
		"Invalid VE name",				/* 1 */
		"Invalid VE type",				/* 2 */
		"Invalid VE configuration",			/* 3 */
		"VE name already in use",			/* 4 */
		"VE not registered",				/* 5 */
		"VE already active",				/* 6 */
		"VE operation failed",				/* 7 */
		"Unable to apply VE guarantee",			/* 8 */
		"VE not active",				/* 9 */
		"Too many requests",				/* 10 */
	};

	static const char *lib_err_list[] = {
		"Failed to allocate memory",			/* 1000 */
		"Failed to connect to VCMMD service",		/* 1001 */
	};

	const char *err_str;

	if (!err)
		err_str = success;
	else if (err >= __VCMMD_SERVICE_ERROR_START &&
		 err < __VCMMD_SERVICE_ERROR_END)
		err_str = service_err_list[err - __VCMMD_SERVICE_ERROR_START];
	else if (err >= __VCMMD_LIB_ERROR_START &&
		 err < __VCMMD_LIB_ERROR_END)
		err_str = lib_err_list[err - __VCMMD_LIB_ERROR_START];
	else
		err_str = unknown;

	strncpy(buf, err_str, buflen);
	buf[buflen - 1] = '\0';

	return buf;
}

static bool append_str(DBusMessageIter *iter, const char *val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &val);
}

static bool append_int32(DBusMessageIter *iter, dbus_int32_t val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &val);
}

static bool append_uint16(DBusMessageIter *iter, dbus_uint16_t val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16, &val);
}

static bool append_uint32(DBusMessageIter *iter, dbus_uint32_t val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &val);
}

static bool append_uint64(DBusMessageIter *iter, dbus_uint64_t val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &val);
}

static bool append_config_entry(DBusMessageIter *iter,
				const struct vcmmd_ve_config_entry *entry)
{
	DBusMessageIter sub;

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, NULL,
					      &sub) ||
	    !append_uint16(&sub, entry->key) ||
	    !append_uint64(&sub, entry->value) ||
	    !append_str(&sub, entry->str) ||
	    !dbus_message_iter_close_container(iter, &sub))
		return false;

	return true;
}

static bool append_config(DBusMessageIter *iter,
			  const struct vcmmd_ve_config *config)
{
	DBusMessageIter sub;
	int i;

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					      DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					      DBUS_TYPE_UINT16_AS_STRING
					      DBUS_TYPE_UINT64_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING
					      DBUS_STRUCT_END_CHAR_AS_STRING,
					      &sub))
		return false;

	for (i = 0; i < config->nr_entries; i++)
		if (!append_config_entry(&sub, &config->entries[i]))
			return false;

	if (!dbus_message_iter_close_container(iter, &sub))
		return false;

	return true;
}

static DBusMessage *make_msg(const char *method, DBusMessageIter *args)
{
	DBusMessage *msg;

	msg = dbus_message_new_method_call("com.virtuozzo.vcmmd",
					   "/LoadManager",
					   "com.virtuozzo.vcmmd.LoadManager",
					   method);
	if (msg && args)
		dbus_message_iter_init_append(msg, args);

	return msg;
}

static DBusMessage *__send_msg(DBusMessage *msg)
{
	DBusConnection *conn;
	DBusMessage *reply;

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) {
		dbus_message_unref(msg);
		return NULL;
	}

	reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_INFINITE, NULL);
	dbus_message_unref(msg);
	dbus_connection_flush(conn);

	return reply;
}

static int send_msg(DBusMessage *msg)
{
	DBusMessage *reply;
	dbus_int32_t err;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_INT32, &err,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	dbus_message_unref(reply);

	return err;
}

int vcmmd_register_ve(const char *ve_name, vcmmd_ve_type_t ve_type,
		      const struct vcmmd_ve_config *ve_config,
		      unsigned int flags)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("RegisterVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name) ||
	    !append_int32(&args, ve_type) ||
	    !append_config(&args, ve_config) ||
	    !append_uint32(&args, flags))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_activate_ve(const char *ve_name, unsigned int flags)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("ActivateVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name) ||
	    !append_uint32(&args, flags))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_update_ve(const char *ve_name,
		    const struct vcmmd_ve_config *ve_config,
		    unsigned int flags)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("UpdateVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name) ||
	    !append_config(&args, ve_config) ||
	    !append_uint32(&args, flags))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_deactivate_ve(const char *ve_name)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("DeactivateVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_unregister_ve(const char *ve_name)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("UnregisterVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_get_ve_config(const char *ve_name, struct vcmmd_ve_config *ve_config)
{
	DBusMessage *msg, *reply;
	DBusMessageIter args, array, structure;
	dbus_int32_t err;
	dbus_uint16_t t;
	vcmmd_ve_config_key_t tag;
	dbus_uint64_t value;
	char *string;

	msg = make_msg("GetVEConfig", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	vcmmd_ve_config_init(ve_config);

	dbus_message_iter_init(reply, &args);
	if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
		goto error;

	dbus_message_iter_get_basic(&args, &err);
	if (err)
		goto error;

	if (FALSE == dbus_message_iter_next(&args) ||
			DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args))
		goto error;
	dbus_message_iter_recurse(&args, &array);

	do {
		if (DBUS_TYPE_STRUCT != dbus_message_iter_get_arg_type(&array))
			goto error;
		dbus_message_iter_recurse(&array, &structure);
		if (DBUS_TYPE_UINT16 != dbus_message_iter_get_arg_type(&structure))
			goto error;
		dbus_message_iter_get_basic(&structure, &t);
		tag = (vcmmd_ve_config_key_t)t;
		if (FALSE == dbus_message_iter_next(&structure) ||
				DBUS_TYPE_UINT64 != dbus_message_iter_get_arg_type(&structure))
			goto error;
		dbus_message_iter_get_basic(&structure, &value);
		if (FALSE == dbus_message_iter_next(&structure) ||
				DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&structure))
			goto error;
		dbus_message_iter_get_basic(&structure, &string);

		if (vcmmd_ve_config_entry_is_string(tag)) {
			if (!vcmmd_ve_config_append_string(ve_config, tag, string))
				goto error;
		}
		else {
			if (!vcmmd_ve_config_append(ve_config, tag, value))
				goto error;
		}
	} while (TRUE == dbus_message_iter_next(&array));


	/* Frees data */
	dbus_message_unref(reply);

	return 0;

error:
	dbus_message_unref(reply);
	vcmmd_ve_config_deinit(ve_config);
	return VCMMD_ERROR_CONNECTION_FAILED;
}

int vcmmd_get_ve_state(const char *ve_name, vcmmd_ve_state_t *ve_state)
{
	DBusMessage *msg, *reply;
	DBusMessageIter args;
	dbus_int32_t err;
	dbus_bool_t active;

	msg = make_msg("IsVEActive", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL,
				   DBUS_TYPE_INT32, &err,
				   DBUS_TYPE_BOOLEAN, &active,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	dbus_message_unref(reply);

	if (err) {
		if (err == VCMMD_ERROR_VE_NOT_REGISTERED) {
			*ve_state = VCMMD_VE_UNREGISTERED;
			err = 0;
		}
		return err;
	}

	if (active)
		*ve_state = VCMMD_VE_ACTIVE;
	else
		*ve_state = VCMMD_VE_REGISTERED;
	return 0;
}

int vcmmd_get_current_policy(char *policy_name, int len)
{
	DBusMessage *msg, *reply;
	char *ret;

	msg = make_msg("GetCurrentPolicy", NULL);
	if (!msg)
		return VCMMD_ERROR_NO_MEMORY;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL,
				   DBUS_TYPE_STRING, &ret,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	if (strlen(ret) > len - 1) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_NO_MEMORY;
	}

	strcpy(policy_name, ret);
	dbus_message_unref(reply);
	return 0;
}

int vcmmd_get_policy_from_file(char *policy_name, int len)
{
	DBusMessage *msg, *reply;
	char *ret;

	msg = make_msg("GetPolicyFromFile", NULL);
	if (!msg)
		return VCMMD_ERROR_NO_MEMORY;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL,
				   DBUS_TYPE_STRING, &ret,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	if (strlen(ret) > len - 1) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_NO_MEMORY;
	}

	strcpy(policy_name, ret);
	dbus_message_unref(reply);
	return 0;
}

int vcmmd_set_policy(const char *policy_name)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("SwitchPolicy", &args);
	if (!msg ||
	    !append_str(&args, policy_name))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

void __attribute__ ((constructor)) vcmmd_init(void)
{
	if (!dbus_threads_init_default())
		abort();
}
