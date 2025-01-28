#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
	#include <stdint.h>
	typedef __int64 ssize_t;
	#include <winsock2.h>
  	#include <Ws2tcpip.h>
#else
	#include <netinet/in.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif

#include "harald.h"

HARALD Harald;

unsigned long long retrieve_number(char *cur, uint32_t size)
{
	unsigned long long num = 0;
	for (uint32_t i = 0; i < size; i++)
	{
		num = (num << 8) | cur[i];
	}

	return num;
}

uint32_t retrieve_uint32(char *cur)
{
	uint32_t num = (uint32_t)((unsigned char)cur[0] << 24 |
							  (unsigned char)cur[1] << 16 |
							  (unsigned char)cur[2] << 8 |
							  (unsigned char)cur[3]);

	return num;
}

uint32_t retrieve_uint16(char *cur)
{
	uint32_t num = (uint32_t)((unsigned char)cur[0] << 8 |
							  (unsigned char)cur[1]);

	return num;
}

void __clean_reg(char REG)
{
	if (NULL != Harald.registries[REG].result && Harald.registries[REG].requires_cleanup)
	{
		free(Harald.registries[REG].result);
		Harald.registries[REG].result = NULL;
		Harald.registries[REG].length = 0;
	}
}

void __append(PTR_PAYLOAD dst, PTR_PAYLOAD src, size_t size)
{
	dst->cursor = dst->payload + dst->size;
	dst->size += size;
	memcpy(dst->cursor, src->cursor, size);

	dst->cursor += size;
	src->cursor += size;
}

void __set_err(HARALD_ERRORCODE code)
{
	Harald.last_error.err_code = code;
#ifdef DEBUG
	Harald.last_error.reason = HARALD_ERRORS_STRINGS[code];
#endif
}

void h_append()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--APPEND\n", H_F_APPEND);
#endif

	uint32_t size = retrieve_uint32(Harald.protocol->cursor);
	Harald.protocol->cursor += sizeof(uint32_t);

	Harald.end_payload->payload = (char *)realloc(Harald.end_payload->payload, Harald.end_payload->size + size);
	if (NULL == Harald.end_payload->payload)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}
	__append(Harald.end_payload, Harald.protocol, size);
}

void h_prepend()
{
}

void h_split()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--SPLIT\n", H_F_SPLIT);
#endif

	uint32_t fragment_size = retrieve_uint32(Harald.protocol->cursor);
	Harald.protocol->cursor += sizeof(uint32_t);

	Harald.end_payload->fragment_size = fragment_size;
}

void h_compute_fragment_length()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--COMPUTE_FRAGMENT_LENGTH\n", H_F_COMPUTE_FRAG_LENGTH);
#endif

	int remaining = Harald.payload->payload + Harald.payload->size - Harald.payload->cursor;
	if (Harald.end_payload->fragment_size > remaining)
	{
		Harald.end_payload->fragment_size = remaining;
	}

	__clean_reg(H_REG_RET);

	Harald.registries[H_REG_RET].length = sizeof(Harald.end_payload->fragment_size);
	Harald.registries[H_REG_RET].result = (void*)Harald.end_payload->fragment_size;
	Harald.registries[H_REG_RET].requires_cleanup = 0;
}

void h_payload_inject()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--PAYLOAD_INJECT\n", H_F_PAYLOAD_INJECT);
#endif

	int remaining = Harald.payload->payload + Harald.payload->size - Harald.payload->cursor;
	if (Harald.end_payload->fragment_size > remaining)
	{
		Harald.end_payload->fragment_size = remaining;
	}

	Harald.end_payload->payload = (char *)realloc(Harald.end_payload->payload, Harald.end_payload->size + Harald.end_payload->fragment_size);
	if (NULL == Harald.end_payload->payload)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}
	__append(Harald.end_payload, Harald.payload, Harald.end_payload->fragment_size);
}

void h_encode_str()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--ENCODE_STR\n", H_F_ENCODE_STR);
#endif

	char is_registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	char _f;
	size_t to_encode;
	if (is_registry) {
		char reg = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		_f = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);

		to_encode = (size_t)Harald.registries[reg].result;

	} else {
		char h_payload = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		char info = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		_f = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);

			if (h_payload >= sizeof(Harald.payloads) / sizeof(Harald.payloads[0]) || info >= sizeof(Harald.payloads[0]))
		{
			__set_err(HARALD_ERR_UNRECOGNIZED_PAYLOAD);
			return;
		}
		to_encode = *(size_t *)Harald.payloads[h_payload][info];
	}

	char formatter[] = {'%', 'z', _f, 0x00};

	char *encoded = (char *)malloc(sizeof(char) * 1024);
	if (NULL == encoded)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}
	int size = snprintf(encoded, 1024, formatter, to_encode);

	__clean_reg(H_REG_RET);

	Harald.registries[H_REG_RET].length = size;
	Harald.registries[H_REG_RET].result = encoded;
	Harald.registries[H_REG_RET].requires_cleanup = 1;
}

void h_decode_str()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--DECODE_STR\n", H_F_DECODE_STR);
#endif
	char from_reg = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	long number = strtol(Harald.registries[from_reg].result, NULL, 10);

	__clean_reg(H_REG_RET);
	Harald.registries[H_REG_RET].length = sizeof(long);
	Harald.registries[H_REG_RET].result = (void *)number;
	Harald.registries[H_REG_RET].requires_cleanup = 0;
}

void h_encode_b64()
{
}

void h_encode_hex()
{
}

void h_inject()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--INJECT\n", H_F_INJECT);
#endif

	char registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	if (registry >= sizeof(Harald.registries))
	{
		__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
		return;
	}

	RESULT _w = Harald.registries[registry];
	PAYLOAD what;
	what.cursor = (char *)_w.result;
	what.size = _w.length;
	Harald.end_payload->payload = (char *)realloc(Harald.end_payload->payload, Harald.end_payload->size + what.size);
	if (NULL == Harald.end_payload->payload)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}
	__append(Harald.end_payload, &what, what.size);
}

void h_sock_init()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--SOCK_INIT\n", H_F_SOCK_INIT);
#endif

	char is_ipaddress = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	struct sockaddr_in address;
	Harald.SSL_enabled = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	char socket_type = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	uint32_t host;
	uint16_t port;
	char* hostname;
	struct hostent *_h;
	
#ifdef _WIN32
    WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

	if (is_ipaddress) {
		host = htonl(retrieve_uint32(Harald.protocol->cursor));
		Harald.protocol->cursor += sizeof(uint32_t);
		port = htons(retrieve_uint16(Harald.protocol->cursor));
		Harald.protocol->cursor += sizeof(uint16_t);
	} else {
		uint32_t hostname_size = retrieve_uint32(Harald.protocol->cursor);
		Harald.protocol->cursor += sizeof(uint32_t);
		hostname = (char*) malloc(hostname_size * sizeof(char));
		memcpy(hostname, Harald.protocol->cursor, hostname_size);
		Harald.protocol->cursor += hostname_size;
		port = htons(retrieve_uint16(Harald.protocol->cursor));
		Harald.protocol->cursor += sizeof(uint16_t);
		if ( ( _h = gethostbyname(hostname)) == NULL ) {
			__set_err(HARALD_ERR_SOCKET_CREATION_ERR);
#ifdef DEBUG
			DEBUG_PRINT("\n Unable to resolve hostname\n", NULL);
#endif
			return;
		} else {
			host = *(long*)_h->h_addr_list[0];
		}

	}

	if (0 == Harald.sockfd)
	{
		if ((Harald.sockfd = socket(AF_INET, socket_type, 0)) < 0)
		{
			__set_err(HARALD_ERR_SOCKET_CREATION_ERR);
#ifdef DEBUG
			DEBUG_PRINT("\n Socket creation error \n", NULL);
#endif
			return;
		}
#ifdef DEBUG
		DEBUG_PRINT("SOCKET CREATED\n", NULL);
#endif
		address.sin_family = AF_INET;
		address.sin_port = port;
		address.sin_addr.s_addr = host;

		if (Harald.SSL_enabled)
		{
#ifdef DEBUG
			DEBUG_PRINT("SSL_INITIALIZATION...\n", NULL);
#endif
			SSL_library_init();
			SSL_load_error_strings();
			OpenSSL_add_ssl_algorithms();

			const SSL_METHOD *method = SSLv23_client_method();
			Harald.ctx = SSL_CTX_new(method);
			if (!Harald.ctx)
			{
				__set_err(HARALD_ERR_SOCKET_SSLCTX_ERR);
#ifdef DEBUG
				DEBUG_PRINT("Error creating SSL_CTX\n", NULL);
#endif
				return;
			}
			Harald.ssl = SSL_new(Harald.ctx);
			if(!is_ipaddress)
				SSL_set_tlsext_host_name(Harald.ssl, hostname);
			SSL_set_fd(Harald.ssl, Harald.sockfd);
		}

		if (connect(Harald.sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
		{
			__set_err(HARALD_ERR_SOCKET_CONNECTION_FAILED);
#ifdef DEBUG
			DEBUG_PRINT("\nConnection Failed \n", NULL);
#endif
			return;
		}
#ifdef DEBUG
		DEBUG_PRINT("CONNECTION CREATED\n", NULL);
#endif

		if (Harald.SSL_enabled)
		{
#ifdef DEBUG
			DEBUG_PRINT("SSL_CONTEXT CREATED\n", NULL);
#endif
			SSL_set_connect_state(Harald.ssl);
			if (SSL_connect(Harald.ssl) <= 0)
			{
				__set_err(HARALD_ERR_SOCKET_SSL_CONNECTION_FAILED);
#ifdef DEBUG
				DEBUG_PRINT("\nSSL Connection Failed \n", NULL);
#endif
				return;
			}
#ifdef DEBUG
			DEBUG_PRINT("SSL ENABLED\n", NULL);
#endif
		}
	}
}

void h_send()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--SEND\n", H_F_SEND);
#endif

	int result = 0;
	if (Harald.SSL_enabled)
	{
		result = SSL_write(Harald.ssl, Harald.end_payload->payload, Harald.end_payload->size);
	}
	else
	{
		result = send(Harald.sockfd, Harald.end_payload->payload, Harald.end_payload->size, 0);
	}

	if (result < 0)
	{
		__set_err(HARALD_ERR_SOCKET_SEND);
	}
}

void h_push()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--PUSH\n", H_F_PUSH);
#endif

	char is_registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	if (is_registry)
	{
		char src_registry = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		char dst_registry = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		if (src_registry >= sizeof(Harald.registries) || dst_registry >= sizeof(Harald.registries))
		{
			__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
			return;
		}

		Harald.registries[dst_registry].result = Harald.registries[src_registry].result;
		Harald.registries[dst_registry].length = Harald.registries[src_registry].length;
		Harald.registries[dst_registry].requires_cleanup = Harald.registries[src_registry].requires_cleanup;
	} // TODO: else.
}

void h_pop()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--POP\n", H_F_POP);
#endif

	char src_registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	if (src_registry >= sizeof(Harald.registries))
	{
		__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
		return;
	}

	Harald.registries[H_REG_RET] = Harald.registries[src_registry];
}

void __retrieve_values(char is_registry, long long *num1, long long *num2)
{
	if (is_registry)
	{

		char reg1 = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		char reg2 = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		if (reg1 >= sizeof(Harald.registries) || reg2 >= sizeof(Harald.registries))
		{
			__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
			return;
		}

		*num1 = (long long)Harald.registries[reg1].result;
		*num2 = (long long)Harald.registries[reg2].result;
	}
	else
	{
		uint32_t size = retrieve_uint32(Harald.protocol->cursor);
		Harald.protocol->cursor += sizeof(uint32_t);

		void *buffer = (void *)malloc(sizeof(char) * size);
		if (NULL == buffer)
		{
			__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
			return;
		}
		memcpy(buffer, Harald.protocol->cursor, size);
		Harald.protocol->cursor += size;
		char reg1 = *Harald.protocol->cursor;
		Harald.protocol->cursor += sizeof(char);
		if (reg1 >= sizeof(Harald.registries))
		{
			__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
			return;
		}

		*num1 = (long long)Harald.registries[reg1].result;
		*num2 = retrieve_number(buffer, size);
		free(buffer);
	}
}

void h_read()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--READ\n", H_F_READ);
#endif

	char from_reg = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	char size_reg = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	size_t size = (size_t)Harald.registries[size_reg].result;

	void *info = malloc(size);

	if (NULL == info)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}

	memcpy(info, Harald.registries[from_reg].result, size);

	__clean_reg(H_REG_RET);
	Harald.registries[H_REG_RET].length = size;
	Harald.registries[H_REG_RET].result = info;
	Harald.registries[H_REG_RET].requires_cleanup = 1;
}

void h_substract()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--SUBSTRACT\n", H_F_SUBSTRACT);
#endif

	char is_registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	long long num1, num2;
	__retrieve_values(is_registry, &num1, &num2);
	unsigned long long num_result = num1 - num2;

	__clean_reg(H_REG_RET);
	Harald.registries[H_REG_RET].length = sizeof(num_result);
	Harald.registries[H_REG_RET].result = (void *)num_result;
	Harald.registries[H_REG_RET].requires_cleanup = 0;
}

void h_add()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--ADD\n", H_F_ADD);
#endif

	char is_registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);

	long long num1, num2;
	__retrieve_values(is_registry, &num1, &num2);
	unsigned long long num_result = num1 + num2;

	__clean_reg(H_REG_RET);
	Harald.registries[H_REG_RET].length = sizeof(num_result);
	Harald.registries[H_REG_RET].result = (void *)num_result;
	Harald.registries[H_REG_RET].requires_cleanup = 0;
}

void h_recv()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--RECV\n", H_F_RECV);
#endif

	uint32_t response_size = retrieve_uint32(Harald.protocol->cursor);
	Harald.protocol->cursor += sizeof(uint32_t);

	void *response = malloc(response_size);

	if (response == NULL)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}

	ssize_t total_bytes_read = 0;
	while (1)
	{
		ssize_t read_size;
		if (Harald.SSL_enabled)
		{
			read_size = SSL_read(Harald.ssl, (char*)response + total_bytes_read, response_size);
		}
		else
		{
			read_size = recv(Harald.sockfd, (char*)response + total_bytes_read, response_size, 0);
		}
		if (read_size <= 0)
		{
			break;
		}

		total_bytes_read += read_size;

		if (total_bytes_read >= response_size)
		{
			char *new_buffer = (char *)realloc(response, response_size * BUFFER_GROWTH_FACTOR);
			if (new_buffer == NULL)
			{
				__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
				free(response);
				return;
			}
			response = new_buffer;
		}
	}

	__clean_reg(H_REG_RET);
	Harald.registries[H_REG_RET].length = total_bytes_read;
	Harald.registries[H_REG_RET].result = response;
	Harald.registries[H_REG_RET].requires_cleanup = 1;
}

void h_close()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--CLOSE\n", H_F_CLOSE);
#endif

	if (Harald.SSL_enabled)
	{
#ifdef DEBUG
		DEBUG_PRINT("SSL CLEANUP...\n", NULL);
#endif
	SSL_shutdown(Harald.ssl);
	SSL_free(Harald.ssl);
	SSL_CTX_free(Harald.ctx);
	CRYPTO_cleanup_all_ex_data();
	CONF_modules_unload(1);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	}
#ifdef _WIN32
	closesocket(Harald.sockfd);
	WSACleanup();
#else
	close(Harald.sockfd);
#endif
	Harald.sockfd = 0;
}

void *memmem(const void *haystack, size_t haystack_len, const void *const needle, const size_t needle_len)
{
	// BSD Implementation
	if (haystack == NULL)
		return NULL;
	if (haystack_len == 0)
		return NULL;
	if (needle == NULL)
		return NULL;
	if (needle_len == 0)
		return NULL;

	for (const char *h = haystack;
		 haystack_len >= needle_len;
		 ++h, --haystack_len)
	{
		if (!memcmp(h, needle, needle_len))
		{
			return (void *)h;
		}
	}
	return NULL;
}

void h_search()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--SEARCH\n", H_F_SEARCH);
#endif

	char registry = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	if (registry >= sizeof(Harald.registries))
	{
		__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
		return;
	}
	uint32_t needle_length = retrieve_uint32(Harald.protocol->cursor);
	Harald.protocol->cursor += sizeof(uint32_t);

	RESULT haystack_reg = Harald.registries[registry];
	void *found = memmem(haystack_reg.result, haystack_reg.length, Harald.protocol->cursor, needle_length);
	Harald.protocol->cursor += needle_length;

	if (NULL != found)
	{
		__clean_reg(H_REG_RET);
		Harald.registries[H_REG_RET].length = sizeof(void *);
		Harald.registries[H_REG_RET].result = found;
		Harald.registries[H_REG_RET].requires_cleanup = 0;
	}
}

void h_store()
{
#ifdef DEBUG
	DEBUG_PRINT("OPCODE::0x%x--STORE\n", H_F_STORE);
#endif

	char src_reg_idx = *Harald.protocol->cursor;
	Harald.protocol->cursor += sizeof(char);
	if (src_reg_idx >= sizeof(Harald.registries))
	{
		__set_err(HARALD_ERR_REGISTRY_OVERFLOW);
		return;
	}

	RESULT *new_registry = (RESULT *)malloc(sizeof(RESULT));
	if (NULL == new_registry)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}

	if (Harald.results.size == Harald.results.length)
	{
		Harald.results.results = (PTR_RESULT *)realloc(Harald.results.results, sizeof(PTR_RESULT) * Harald.results.length * 2);
		if (NULL == Harald.results.results)
		{
			__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
			return;
		}
		Harald.results.length *= 2;
	}
	new_registry->length = Harald.registries[src_reg_idx].length;
	void *new_result = malloc(new_registry->length);
	if (NULL == new_result)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return;
	}
	memcpy(new_result, Harald.registries[src_reg_idx].result, new_registry->length);
	new_registry->result = new_result;
	Harald.results.results[Harald.results.size] = new_registry;
	Harald.results.size++;
}

void init_h_functions()
{
	HaraldOpcodes[0x00] = NULL;
	HaraldOpcodes[H_F_APPEND] = h_append;
	HaraldOpcodes[H_F_PREPEND] = h_prepend;
	HaraldOpcodes[H_F_SPLIT] = h_split;
	HaraldOpcodes[H_F_PAYLOAD_INJECT] = h_payload_inject;
	HaraldOpcodes[H_F_SOCK_INIT] = h_sock_init;
	HaraldOpcodes[H_F_SEND] = h_send;
	HaraldOpcodes[H_F_RECV] = h_recv;
	HaraldOpcodes[H_F_CLOSE] = h_close;
	HaraldOpcodes[H_F_INJECT] = h_inject;
	HaraldOpcodes[H_F_ENCODE_STR] = h_encode_str;
	HaraldOpcodes[H_F_DECODE_STR] = h_decode_str;
	HaraldOpcodes[H_F_ENCODE_STRB64] = h_encode_b64;
	HaraldOpcodes[H_F_COMPUTE_FRAG_LENGTH] = h_compute_fragment_length;
	HaraldOpcodes[H_F_SEARCH] = h_search;
	HaraldOpcodes[H_F_PUSH] = h_push;
	HaraldOpcodes[H_F_POP] = h_pop;
	HaraldOpcodes[H_F_ADD] = h_add;
	HaraldOpcodes[H_F_SUBSTRACT] = h_substract;
	HaraldOpcodes[H_F_READ] = h_read;
	HaraldOpcodes[H_F_STORE] = h_store;
}

PTR_HARALD Run(char *protocol_def, size_t protocol_def_size, char *payload_def, size_t payload_def_size)
{
#ifdef DEBUG
	DEBUG_PRINT("Initializing Harald...\n", NULL);
#endif
	__set_err(HARALD_STATUS_OK);
	init_h_functions();
	PAYLOAD _payload = {payload_def, payload_def, payload_def_size};
	PAYLOAD _protocol = {protocol_def, protocol_def, protocol_def_size};
	PAYLOAD _end_payload = {NULL, NULL, 0};

	PTR_RESULT *_results = (PTR_RESULT *)malloc(sizeof(PTR_RESULT) * 1024);
	if (NULL == _results)
	{
		__set_err(HARALD_ERR_MEM_ALLOC_ERROR);
		return &Harald;
	}

	Harald.payload = &_payload;
	Harald.protocol = &_protocol;
	Harald.end_payload = &_end_payload;
	Harald.HaraldOpcodes = (harald_function **)&HaraldOpcodes;
	Harald.results.results = _results;
	Harald.results.length = 1024;
	Harald.results.size = 0;

	Harald.payloads[H_PAY_PROTO][H_PAY_SIZE] = &Harald.protocol->size;
	Harald.payloads[H_PAY_PROTO][H_PAY_FRAGMENT_SIZE] = &Harald.protocol->fragment_size;

	Harald.payloads[H_PAY_END_PAYLOAD][H_PAY_SIZE] = &Harald.end_payload->size;
	Harald.payloads[H_PAY_END_PAYLOAD][H_PAY_FRAGMENT_SIZE] = &Harald.end_payload->fragment_size;

	Harald.payloads[H_PAY_PAYLOAD][H_PAY_SIZE] = &Harald.payload->size;
	Harald.payloads[H_PAY_PAYLOAD][H_PAY_FRAGMENT_SIZE] = &Harald.payload->fragment_size;

	while (Harald.payload->cursor < Harald.payload->payload + Harald.payload->size && Harald.last_error.err_code == HARALD_STATUS_OK)
	{
		while (Harald.protocol->cursor < Harald.protocol->payload + Harald.protocol->size && Harald.last_error.err_code == HARALD_STATUS_OK)
		{
			unsigned char OPCODE = *Harald.protocol->cursor;
			if (OPCODE > sizeof(HaraldOpcodes) / sizeof(*HaraldOpcodes))
				break;
			Harald.protocol->cursor++;
			Harald.HaraldOpcodes[OPCODE]();
		}
		Harald.protocol->cursor = Harald.protocol->payload;
		free(Harald.end_payload->payload);
		Harald.end_payload->payload = NULL;
		Harald.end_payload->cursor = NULL;
		Harald.end_payload->size = 0;
		Harald.end_payload->fragment_size = 0;
	}
	return &Harald;
}
