/*
 *  Copyright 2024, Diego Roux, diegoroux04 at proton dot me
 *  Distributed under the terms of the MIT License.
 */
#ifndef _VIRTIO_SND_H
#define _VIRTIO_SND_H

typedef uint32_t le32;

struct virtio_snd_config { 
	le32 jacks; 
	le32 streams; 
	le32 chmaps; 
};

#endif