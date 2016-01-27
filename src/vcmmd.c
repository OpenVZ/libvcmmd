#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dbus/dbus.h>

#include "vcmmd.h"

bool vcmmd_ve_config_extract(const struct vcmmd_ve_config *config,
			     vcmmd_ve_config_key_t key, uint64_t *value)
{
	int i;

	for (i = 0; i < config->nr_entries; i++) {
		if (config->entries[i].key == key) {
			*value = config->entries[i].value;
			return true;
		}
	}
	return false;
}

char *vcmmd_strerror(int err, char *buf, size_t buflen)
{
	static const char *success = "Success";
	static const char *unknown = "Unknown error";

	static const char *service_err_list[] = {
		"Invalid VE name",				/* 1 */
		"Invalid VE type",				/* 2 */
		"Conflicting VE config parameters",		/* 3 */
		"VE name already in use",			/* 4 */
		"VE not registered",				/* 5 */
		"VE already active",				/* 6 */
		"VE operation failed",				/* 7 */
		"Unable to meet VE requirements",		/* 8 */
		"VE not active",				/* 9 */
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

static bool append_uint16(DBusMessageIter *iter, dbus_uint16_t val)
{
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16, &val);
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

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL);
	dbus_message_unref(msg);
	dbus_connection_flush(conn);

	return reply;
}

static int send_msg(DBusMessage *msg)
{
	DBusMessage *reply;
	int32_t ret;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_INT32, &ret,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	dbus_message_unref(reply);

	return ret;
}

int vcmmd_register_ve(const char *ve_name, vcmmd_ve_type_t ve_type,
		      const struct vcmmd_ve_config *ve_config)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("RegisterVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name) ||
	    !append_uint16(&args, ve_type) ||
	    !append_config(&args, ve_config))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_activate_ve(const char *ve_name)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("ActivateVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	return send_msg(msg);
}

int vcmmd_update_ve(const char *ve_name,
		    const struct vcmmd_ve_config *ve_config)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = make_msg("UpdateVE", &args);
	if (!msg ||
	    !append_str(&args, ve_name) ||
	    !append_config(&args, ve_config))
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
	DBusMessageIter args;
	uint64_t *data;
	int i, size;
	int32_t ret;

	msg = make_msg("GetVEConfig", &args);
	if (!msg ||
	    !append_str(&args, ve_name))
		return VCMMD_ERROR_NO_MEMORY;

	reply = __send_msg(msg);
	if (!reply)
		return VCMMD_ERROR_CONNECTION_FAILED;

	if (!dbus_message_get_args(reply, NULL,
				   DBUS_TYPE_INT32, &ret,
				   DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64,
				   &data, &size,
				   DBUS_TYPE_INVALID)) {
		dbus_message_unref(reply);
		return VCMMD_ERROR_CONNECTION_FAILED;
	}

	if (ret) {
		dbus_message_unref(reply);
		return ret;
	}

	/* Ignore uknown parameters */
	if (size > __NR_VCMMD_VE_CONFIG_KEYS)
		size = __NR_VCMMD_VE_CONFIG_KEYS;

	vcmmd_ve_config_init(ve_config);
	for (i = 0; i < size; i++)
		vcmmd_ve_config_append(ve_config, i, data[i]);

	/* Frees data */
	dbus_message_unref(reply);

	return 0;
}

void __attribute__ ((constructor)) vcmmd_init(void)
{
	if (!dbus_threads_init_default())
		abort();
}
