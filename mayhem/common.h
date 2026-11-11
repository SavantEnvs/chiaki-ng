// Shared helpers for the chiaki-ng harnesses.
#ifndef MAYHEM_COMMON_H
#define MAYHEM_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <chiaki/log.h>

static void mh_log_cb(ChiakiLogLevel level, const char *msg, void *user)
{
	(void)level; (void)user;
	// read the formatted message so a bad format string / unterminated buffer is caught
	volatile char c = msg[strlen(msg)];
	(void)c;
}

static void mh_log_init(ChiakiLog *log)
{
	chiaki_log_init(log, CHIAKI_LOG_ALL, mh_log_cb, NULL);
}

// Exact-size heap copy, so ASan sees any read or write past the bytes the library was told about.
static uint8_t *mh_dup(const uint8_t *data, size_t size)
{
	uint8_t *p = malloc(size ? size : 1);
	if(p && size)
		memcpy(p, data, size);
	return p;
}

#endif
