#pragma once
#ifndef HARALD_H
#define HARALD_H
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef DEBUG
	#define DEBUG_PRINT(fmt, ...) do {\
	fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
	} while (0);
#endif

#define BUFFER_GROWTH_FACTOR 2

typedef enum
{
	HARALD_STATUS_OK = 0x00,
	HARALD_ERR_MEM_ALLOC_ERROR = 0x01,
	HARALD_ERR_UNRECOGNIZED_PAYLOAD = 0x02,
	HARALD_ERR_REGISTRY_OVERFLOW = 0x03,
	HARALD_ERR_SOCKET_CREATION_ERR = 0x04,
	HARALD_ERR_SOCKET_SSLCTX_ERR = 0x05,
	HARALD_ERR_SOCKET_CONNECTION_CREATION_ERR = 0x06,
	HARALD_ERR_SOCKET_CONNECTION_FAILED = 0x07,
	HARALD_ERR_SOCKET_SSL_CONNECTION_FAILED = 0x08,
	HARALD_ERR_SOCKET_SEND = 0x09,
	HARALD_ERR_RESOLVER = 0x0A
} HARALD_ERRORCODE;

#ifdef DEBUG
	static const char *const HARALD_ERRORS_STRINGS[] =
		{
			"OK",
			"MEMORY ALLOCATION ERROR",
			"THE PAYLOAD WAS UNRECOGNIZED",
			"THE REFERENCED REGISTRY IS ABOVE THE MAX REGISTRY COUNT",
			"SOCKET CREATTION FAILED",
			"SSL CONTEXT CREATION FAILED",
			"SOCKET CONNECTION CREATION FAILED",
			"SOCKET CONNECTION FAILED",
			"SSL CONNECTION OPERATION FAILED",
			"SEND OPERATION FAILED",
			"CANNOT RESOLVE HOST"};
#endif

typedef void harald_function();

typedef struct payload
{
	char *payload;
	char *cursor;
	size_t size;
	size_t fragment_size;
} PAYLOAD, *PTR_PAYLOAD;

typedef struct Result
{
	size_t length;
	void *result;
	char requires_cleanup;
} RESULT, *PTR_RESULT;

typedef struct Results
{
	size_t size;
	size_t length;
	PTR_RESULT *results;
} RESULTS, *PTR_RESULTS;

typedef struct haraldError
{
	HARALD_ERRORCODE err_code;
	const char *reason;
} HARALD_ERROR;

#define H_REG_RET 0
#define H_PAY_PROTO 0
#define H_PAY_END_PAYLOAD 1
#define H_PAY_PAYLOAD 2

#define H_PAY_SIZE 0
#define H_PAY_FRAGMENT_SIZE 1

typedef struct Harald
{
	int sockfd;
	SSL_CTX *ctx;
	SSL *ssl;
	unsigned char SSL_enabled;
	harald_function **HaraldOpcodes;
	PTR_PAYLOAD protocol;
	PTR_PAYLOAD end_payload;
	PTR_PAYLOAD payload;
	RESULTS results;
	RESULT registries[0xFF];
	void *payloads[3][2];
	HARALD_ERROR last_error;
} HARALD, *PTR_HARALD;

#define H_F_APPEND 0x01
#define H_F_PREPEND 0x02
#define H_F_SPLIT 0x03
#define H_F_PAYLOAD_INJECT 0x04
#define H_F_SOCK_INIT 0x05
#define H_F_SEND 0x06
#define H_F_RECV 0x07
#define H_F_CLOSE 0x08
#define H_F_INJECT 0x09
#define H_F_ENCODE_STR 0x0A
#define H_F_DECODE_STR 0xA0
#define H_F_ENCODE_STRB64 0x0B
#define H_F_COMPUTE_FRAG_LENGTH 0x0D
#define H_F_SEARCH 0x0E
#define H_F_SEEK 0x0F
#define H_F_PUSH 0x10
#define H_F_POP 0x11
#define H_F_ADD 0x12
#define H_F_SUBSTRACT 0x13
#define H_F_READ 0x14
#define H_F_STORE 0x15

PTR_HARALD Run(char *protocol_def, size_t protocol_def_size, char *payload_def, size_t payload_def_size);

harald_function *HaraldOpcodes[0xFF];

harald_function h_append;
harald_function h_prepend;
harald_function h_split;
harald_function h_payload_inject;
harald_function h_sock_init;
harald_function h_send;
harald_function h_recv;
harald_function h_close;
harald_function h_inject;
harald_function h_encode_str;
harald_function h_decode_str;
harald_function h_compute_fragment_length;
harald_function h_search;
// harald_function h_seek; WIP
harald_function h_push;
harald_function h_pop;
harald_function h_add;
harald_function h_substract;
harald_function h_read;
harald_function h_store;
#endif