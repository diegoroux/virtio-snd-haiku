/*
 *  Copyright 2024, Diego Roux, diegoroux04 at proton dot me
 *  Distributed under the terms of the MIT License.
 */

#include <fs/devfs.h>
#include <virtio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "virtio_sound.h"

#define VIRTIO_SOUND_DRIVER_MODULE_NAME 	"drivers/audio/virtio/driver_v1"
#define VIRTIO_SOUND_DEVICE_MODULE_NAME 	"drivers/audio/virtio/device_v1"
#define VIRTIO_SOUND_DEVICE_ID_GEN 			"virtio_sound/device_id"

#define ERROR(x...)		dprintf("\33[33mvirtio_sound:\33[0m " x)


struct VirtIOSoundDriverInfo {
	device_node* 				node;
	::virtio_device 			virtioDev;
	virtio_device_interface*	virtio;

	::virtio_queue				controlQueue;
	::virtio_queue				eventQueue;
	::virtio_queue				txQueue;
	::virtio_queue				rxQueue;

	uint32_t					nJacks;
	uint32_t					nStreams;
	uint32_t					nChmaps;
};

struct VirtIOSoundHandle {
    VirtIOSoundDriverInfo*		info;
};

static device_manager_info*		sDeviceManager;


static float
SupportsDevice(device_node* parent)
{
	uint16 deviceType;
	const char* bus;

	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK) {
		return 0.0f;
	}

	if (strcmp(bus, "virtio") != 0)
		return 0.0f;

	if (deviceType != VIRTIO_DEVICE_ID_SOUND)
		return 0.0f;

	dprintf("virtio_sound: Virtio sound device found!\n");

	return 1.0f;
}


static status_t
InitDriver(device_node* node, void** cookie)
{
	VirtIOSoundDriverInfo* info;
	
	info = (VirtIOSoundDriverInfo*)malloc(sizeof(VirtIOSoundDriverInfo));

	if (info == NULL)
		return B_NO_MEMORY;

	info->node = node;
	*cookie = info;

	return B_OK;
}


static void
UninitDriver(void* cookie)
{
	free(cookie);
}


static status_t
InitDevice(void* _info, void** cookie)
{
	VirtIOSoundDriverInfo* info = (VirtIOSoundDriverInfo*)_info;

	device_node *parent = sDeviceManager->get_parent_node(info->node);
	
	sDeviceManager->get_driver(parent, (driver_module_info**)&info->virtio,
		(void**)&info->virtioDev);
	
	sDeviceManager->put_node(parent);

	::virtio_queue queues[4];
	status_t status = info->virtio->alloc_queues(info->virtioDev, 4, queues);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}

	info->controlQueue = queues[0];
	info->eventQueue = queues[1];
	info->txQueue = queues[2];
	info->rxQueue = queues[3];

	status = info->virtio->read_device_config(info->virtioDev,
		offsetof(struct virtio_snd_config, jacks), &info->nJacks,
		sizeof(uint32_t));
	
	if (status != B_OK) {
		ERROR("device config read failed (%s)\n", strerror(status));
		return status;
	}

	// TODO: Query about jack info.

	status = info->virtio->read_device_config(info->virtioDev,
		offsetof(struct virtio_snd_config, streams), &info->nStreams,
		sizeof(uint32_t));

	if (status != B_OK) {
		ERROR("device config read failed (%s)\n", strerror(status));
		return status;
	}

	// TODO: Query about streams info.

	status = info->virtio->read_device_config(info->viritoDev,
		offsetof(struct virtio_snd_config, chmaps), &info->nChmaps,
		sizeof(uint32_t));

	if (status != B_OK) {
		ERROR("device config read failed (%s)\n", strerror(status));
		return status;
	}

	// TODO: Query about chmaps info.

	*cookie = info;
	return B_OK;
}


static void
UninitDevice(void *_info)
{
	VirtIOSoundDriverInfo* info;
	
	info = (VirtIOSoundDriverInfo*)_info;
	info->virtio->free_queues(info->virtioDev);

	return;
}


struct driver_module_info sVirtioSoundDriver = {
	{
		VIRTIO_SOUND_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	.supports_device = SupportsDevice,

	.init_driver = InitDriver,
	.uninit_driver = UninitDriver,
};


struct device_module_info sVirtioSoundDevice = {
	{
		VIRTIO_SOUND_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	.init_device = InitDevice,
	.uninit_device = UninitDevice,
};


module_info* modules[] = {
	(module_info*)&sVirtioSoundDriver,
	(module_info*)&sVirtioSoundDevice,
	NULL
};


module_dependency module_dependencies[] = {
	{
		B_DEVICE_MANAGER_MODULE_NAME,
		(module_info**)&sDeviceManager
	},
	{}
};
