#include "harald.h"

char p[] = {'H','e','l','l','o',' ','G','i','t','H','u','b','!'};


char http_protocol[] = {
	// -------------------------------- SEND ------------------------------ //
	H_F_SPLIT, 0x00, 0x00, 0x00, 0xFF,                                      // Split the payload in fragments of size
	H_F_APPEND, 0x00, 0x00, 0x01, 0x85, 'P','O','S','T',' ',
	'/','r','e','p','o','s','/','YOUR_GH_USER','/','YOUR_GH_REPO',
	'/','i','s','s','u','e','s','/','ISSUE_NUMBER',
	'/','c','o','m','m','e','n','t','s',' ',
	'H','T','T','P','/','1','.','1','\r','\n',
	'H','o','s','t',':',' ','a','p','i','.','g','i','t','h','u','b','.','c','o','m','\r','\n',
	'A','u','t','h','o','r','i','z','a','t','i','o','n',':',' '
	,'B','e','a','r','e','r',' ','g','i','t','h','u','b','_','p','a','t','_',
	'YOUR_GH_PAT','\r','\n',
	'X','-','G','i','t','H','u','b','-','A','p','i','-','V','e','r','s','i','o','n',':',' ',
	'2','0','2','2','-','1','1','-','2','8','\r','\n',
	'U','s','e','r','-','A','g','e','n','t',':',' ','H','a','r','a','l','d','/','0','.','1','b','\r','\n',
	'A','c','c','e','p','t',':',' ','a','p','p','l','i','c','a','t','i','o','n','/',
	'v','n','d','.','g','i','t','h','u','b','+','j','s','o','n','\r','\n',
	'C','o','n','t','e','n','t','-','T','y','p','e',':',' ',
	'a','p','p','l','i','c','a','t','i','o','n','/',
	'x','-','w','w','w','-','f','o','r','m','-',
	'u','r','l','e','n','c','o','d','e','d','\r','\n',
	'C','o','n','n','e','c','t','i','o','n',':',' ','c','l','o','s','e','\r','\n',
	'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ',
	// DON'T FORGET TO CHECK THE SIZE OF THE H_F_APPEND ABOVE ^^^
	H_F_COMPUTE_FRAG_LENGTH,                                        // Calculate the next End Payload fragment size
	H_F_ADD, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0B, H_REG_RET, // JSON {"body":""}
	H_F_ENCODE_STR, 0x01, H_REG_RET, 'u',    // Encode a field in a payload with a given encoding
	H_F_INJECT, H_REG_RET,                                          // Inject the given registry in the End Payload
	H_F_APPEND, 0x00, 0x00, 0x00, 0x0D, '\r', '\n', '\r', '\n', '{', '"', 'b', 'o', 'd', 'y', '"', ':', '"',     // Append the following Bytes to the End Payload (Body begins)
	H_F_PAYLOAD_INJECT,                                             // Inject the Fragment to send in the End Payload
	H_F_APPEND, 0x00, 0x00, 0x00, 0x02, '"', '}',
	H_F_SOCK_INIT, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x0F, 'a', 'p', 'i', '.', 'g', 'i', 't', 'h', 'u', 'b', '.', 'c', 'o', 'm', '\0', 0x01, 0xBB,  // Initialize an SSL TCP socket connection to 140.82.121.5 on port 443
	H_F_SEND,                                                       // Send the End Payload
	// --------------------------- RECIEVE AND PARSE -----------------------//
	H_F_RECV, 0x00, 0x00, 0x10, 0x00,                                                 // Recieve and read up to 0x1000b (4096b)
	H_F_PUSH, 0x01, 0x00, 0x01,	                                                      // push REG? 0x00 to REG! 0x01
	H_F_SEARCH, 0x01, 0x00, 0x00, 0x00, 0x10,                                         // Search string 'Content-Length: ' in REG 0x01
	  'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'L', 'e', 'n', 'g', 't', 'h', ':', ' ', 
	H_F_ADD, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00,
	H_F_PUSH, 0x01, 0x00, 0x02,							  // Store pointer from REG 0x00 in REG 0x02
	H_F_SEARCH, 0x02, 0x00, 0x00, 0x00, 0x02, '\r', '\n', // Search string '\r\n' in REG 0x02
	H_F_PUSH, 0x01, 0x00, 0x03,							  // Store pointer from REG 0x00 in REG 0x03
	H_F_SUBSTRACT, 0x01, 0x03, 0x02,					  // reg 0x03 - reg 0x02 so we get the length of the numeric value of Content-Length header
	H_F_PUSH, 0x01, 0x00, 0x04,							  // Store pointer from REG 0x00 in REG 0x05
	H_F_READ, 0x02, 0x04,								  // read from reg 0x02, up to length in reg 0x04 (Content-Length header value!)
	H_F_DECODE_STR, 0x00,
	H_F_PUSH, 0x01, 0x00, 0x05,										  // Store pointer from REG 0x00 in REG 0x05
	H_F_SEARCH, 0x01, 0x00, 0x00, 0x00, 0x04, '\r', '\n', '\r', '\n', // Search string '\r\n\r\n' in REG 0x02 to find body
	H_F_PUSH, 0x01, 0x00, 0x06,										  // Store pointer from REG 0x00 in REG 0x06
	H_F_ADD, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x06,
	H_F_PUSH, 0x01, 0x00, 0x08, // Store pointer from REG 0x00 in REG 0x08
	H_F_READ, 0x08, 0x05,		// read body stored in 0x08 up to length in reg 0x05
	H_F_STORE, 0x00,			// store result from reg 0x00 for agent processing*/
	H_F_CLOSE					// Keep-alive?
};


void get_results(PTR_HARALD harald) {
	#ifdef DEBUG
	if (harald->last_error.err_code != HARALD_STATUS_OK)
	{
		DEBUG_PRINT("[!] HARALD EXECUTION TERMINATED!\n\tERR_CODE: %#010x\n\tREASON: %s\n", harald->last_error.err_code, harald->last_error.reason);
	}
#endif

	printf("OUTPUT:: %i\n", (int)harald->results.size);
	for (size_t i = 0; i < harald->results.size; ++i)
	{
		printf("%.*s\n", (int)harald->results.results[i]->length, (char *)harald->results.results[i]->result);

		free(harald->results.results[i]->result);
		free(harald->results.results[i]);

	}
	 harald->results.size = 0;

	free(harald->results.results);
	harald->results.length = 0;
}


int main()
{

	PTR_HARALD harald;
	
	harald = Run(http_protocol, sizeof(http_protocol), p, sizeof(p));
	get_results(harald);

	printf("\n\t\t-----------\n\n");

	if (harald->sockfd)
	{
		h_close();
	}

	return 0;
}