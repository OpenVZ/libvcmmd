/*
 *  Copyright (C) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef _VCMMD_H_
#define _VCMMD_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * Error codes
 */
enum {
	/*
	 * Errors returned by VCMMD service.
	 */

	__VCMMD_SERVICE_ERROR_START = 1,

	VCMMD_ERROR_INVALID_VE_NAME =
		__VCMMD_SERVICE_ERROR_START,			/* 1 */
	VCMMD_ERROR_INVALID_VE_TYPE,				/* 2 */
	VCMMD_ERROR_INVALID_VE_CONFIG,				/* 3 */
	VCMMD_ERROR_VE_NAME_ALREADY_IN_USE,			/* 4 */
	VCMMD_ERROR_VE_NOT_REGISTERED,				/* 5 */
	VCMMD_ERROR_VE_ALREADY_ACTIVE,				/* 6 */
	VCMMD_ERROR_VE_OPERATION_FAILED,			/* 7 */
	VCMMD_ERROR_UNABLE_APPLY_VE_GUARANTEE,			/* 8 */
	VCMMD_ERROR_VE_NOT_ACTIVE,				/* 9 */

	__VCMMD_SERVICE_ERROR_END,

	/*
	 * Library errors
	 */

	__VCMMD_LIB_ERROR_START = 1000,

	VCMMD_ERROR_NO_MEMORY =
		__VCMMD_LIB_ERROR_START,			/* 1000 */
	VCMMD_ERROR_CONNECTION_FAILED,				/* 1001 */

	__VCMMD_LIB_ERROR_END,
};

/*
 * VE type
 */
typedef enum {
	VCMMD_VE_CT,		/* container */
	VCMMD_VE_VM,		/* virtual machine */
	VCMMD_VE_VM_LINUX,	/* virtual machine Linux */
	VCMMD_VE_VM_WINDOWS,	/* virtual machine Windows */
	VCMMD_VE_SERVICE,	/* service container */
} vcmmd_ve_type_t;

/*
 * VE state
 */
typedef enum {
	VCMMD_VE_UNREGISTERED,		/* VE is unknown to VCMMD */
	VCMMD_VE_REGISTERED,		/* VE is registered, but inactive */
	VCMMD_VE_ACTIVE,		/* VE is registered and active */
} vcmmd_ve_state_t;

/*
 * VE config keys
 */
typedef enum {
	/*
	 * VE memory best-effort protection, in bytes.
	 *
	 * A VE should be always given at least as much memory as specified by
	 * this parameter unless things get really bad on the host.
	 */
	VCMMD_VE_CONFIG_GUARANTEE,

	/*
	 * VE memory limit, in bytes.
	 *
	 * Maximal size of host memory that can be used by a VE.
	 *
	 * Must be >= guarantee.
	 */
	VCMMD_VE_CONFIG_LIMIT,

	/*
	 * VE swap hard limit, in bytes.
	 *
	 * Maximal size of host swap that can be used by a VE.
	 */
	VCMMD_VE_CONFIG_SWAP,

	/*
	 * Video RAM size, in bytes.
	 *
	 * Amount of memory that should be reserved for a VE's graphic card.
	 */
	VCMMD_VE_CONFIG_VRAM,

	__NR_VCMMD_VE_CONFIG_KEYS,
} vcmmd_ve_config_key_t;

/*
 * VE config key-value pair
 */
struct vcmmd_ve_config_entry {
	vcmmd_ve_config_key_t key;
	uint64_t value;
};

/*
 * VE config
 *
 * Use vcmmd_ve_config_{init,append} helpers to build a config. If a value for
 * a particular config key is omitted, the current value will be used if any,
 * otherwise the default value.
 */
struct vcmmd_ve_config {
	unsigned int nr_entries;
	struct vcmmd_ve_config_entry entries[__NR_VCMMD_VE_CONFIG_KEYS];
};

static inline void vcmmd_ve_config_init(struct vcmmd_ve_config *config)
{
	memset(config, 0, sizeof(*config));
}

static inline void vcmmd_ve_config_append(struct vcmmd_ve_config *config,
					  vcmmd_ve_config_key_t key,
					  uint64_t value)
{
	struct vcmmd_ve_config_entry entry = {
		.key = key,
		.value = value,
	};

	config->entries[config->nr_entries++] = entry;
}

#ifdef __cplusplus
extern "C" {
#endif

/*
 * vcmmd_ve_config_extract: extract value from config given a key
 * @config: config
 * @key: config key
 * @value: pointer to buffer to write value to
 *
 * Returns %true if the key was found in the config, %false otherwise.
 */
bool vcmmd_ve_config_extract(const struct vcmmd_ve_config *config,
			     vcmmd_ve_config_key_t key, uint64_t *value);

/*
 * vcmmd_strerror: return string describing error code
 * @err: the error code
 * @buf: buffer to store the error string
 * @buflen: the buffer length
 *
 * Returns @buf.
 */
char *vcmmd_strerror(int err, char *buf, size_t buflen);

/*
 * vcmmd_register_ve: register VE
 * @ve_name: VE name
 * @ve_type: VE type
 * @ve_config: VE config
 *
 * This function tries to register a VE with the VCMMD service. It should be
 * called before VE start. VCMMD will check if it can meet the requirements
 * claimed by the VE and report back. The caller must refrain from starting the
 * VE if VCMMD returns failure. If VCMMD finds that the VE requirements can be
 * met, it will remember the VE and return success, but it will not start
 * tuning the VE's parameters until the VE is activated (see vcmmd_activate_ve).
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_INVALID_VE_NAME
 *   %VCMMD_ERROR_INVALID_VE_TYPE
 *   %VCMMD_ERROR_INVALID_VE_CONFIG
 *   %VCMMD_ERROR_VE_NAME_ALREADY_IN_USE
 *   %VCMMD_ERROR_UNABLE_APPLY_VE_GUARANTEE
 */
int vcmmd_register_ve(const char *ve_name, vcmmd_ve_type_t ve_type,
		      const struct vcmmd_ve_config *ve_config,
		      unsigned int flags);

/*
 * vcmmd_activate_ve: activate VE
 * @ve_name: VE name
 *
 * This function notifies VCMMD that a VE which has been previously registered
 * with vcmmd_register_ve can now be managed. VCMMD may not tune VE's parameters
 * until this function is called. If this function fails, which normally can
 * only happen if VCMMD fails to connect to the VE, the caller should stop and
 * unregister VE with vcmmd_unregister_ve.
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_VE_NOT_REGISTERED
 *   %VCMMD_ERROR_VE_ALREADY_ACTIVE
 *   %VCMMD_ERROR_VE_OPERATION_FAILED
 */
int vcmmd_activate_ve(const char *ve_name, unsigned int flags);

/*
 * vcmmd_update_ve: update VE config
 * @ve_name: VE name
 * @ve_config: VE config
 *
 * This function requests the VCMMD service to update a VE's configuration. It
 * may only be called on active VEs (see vcmmd_activate_ve). This function
 * may fail if VCMMD finds that it will not be able to meet the new VE's
 * requirements.
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_INVALID_VE_CONFIG
 *   %VCMMD_ERROR_VE_NOT_REGISTERED
 *   %VCMMD_ERROR_VE_NOT_ACTIVE
 *   %VCMMD_ERROR_VE_OPERATION_FAILED
 *   %VCMMD_ERROR_UNABLE_APPLY_VE_GUARANTEE
 */
int vcmmd_update_ve(const char *ve_name,
		    const struct vcmmd_ve_config *ve_config,
		    unsigned int flags);

/*
 * vcmmd_deactivate_ve: deactivate VE
 * @ve_name: VE name
 *
 * This function notifies VCMMD that an active VE must not be managed any
 * longer. After this operation completes, VE still stays in the VCMMD list of
 * registered VEs and hence contributes to the host load, but VCMMD is not
 * allowed to tune its parameters at runtime. This function is supposed to be
 * called before pausing a VE. To undo it, call vcmmd_activate_ve.
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_VE_NOT_REGISTERED
 *   %VCMMD_ERROR_VE_NOT_ACTIVE
 */
int vcmmd_deactivate_ve(const char *ve_name);

/*
 * vcmmd_unregister_ve: unregister VE
 * @ve_name: VE name
 *
 * This function makes VCMMD forget about a VE. The caller is supposed to stop
 * the VE after this function returns (if VE is running, of course).
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_VE_NOT_REGISTERED
 */
int vcmmd_unregister_ve(const char *ve_name);

/*
 * vcmmd_get_ve_config: get VE config
 * @ve_name: VE name
 * @ve_config: pointer to buffer to write config to
 *
 * Returns 0 on success, an error code on failure.
 *
 * Error codes:
 *
 *   %VCMMD_ERROR_VE_NOT_REGISTERED
 */
int vcmmd_get_ve_config(const char *ve_name, struct vcmmd_ve_config *ve_config);

/*
 * vcmmd_get_ve_state: get VE state
 * @ve_name: VE name
 *
 * Returns 0 on success, an error code on failure.
 */
int vcmmd_get_ve_state(const char *ve_name, vcmmd_ve_state_t *ve_state);

#ifdef __cplusplus
}
#endif

#endif /* _VCMMD_H_ */
