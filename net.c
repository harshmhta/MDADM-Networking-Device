#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n (len) bytes from fd; returns true on success and false on failure. 
It may need to call the system call "read" multiple times to reach the given size len. 
*/
static bool nread(int fd, int len, uint8_t *buf) {
  
  int read_b = 0;
  int x = read(fd, buf, len - read_b);
  for (read_b = 0; read_b < len; read_b = read_b + x) {
	
      if(x < 0) {    //checks if call succeeded
        return false;
      }
    break;
	}
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on failure 
It may need to call the system call "write" multiple times to reach the size len.
*/
static bool nwrite(int fd, int len, uint8_t *buf) {
  
	int write_b = 0;
	int y = write(fd, buf, len - write_b);
	for (write_b = 0; write_b < len; write_b = write_b + y) {
		
	    if(y < 0) {     //checks if call succeeded
	      return false;
	    }
      break;
	}
  return true;
}

/* Through this function call the client attempts to receive a packet from sd 
(i.e., receiving a response from the server.). It happens after the client previously 
forwarded a jbod operation call via a request message to the server.  
It returns true on success and false on failure. 
The values of the parameters (including op, ret, block) will be returned to the caller of this function: 
op - the address to store the jbod "opcode"  
ret - the address to store the return value of the server side calling the corresponding jbod_operation function.
block - holds the received block content if existing (e.g., when the op command is JBOD_READ_BLOCK)
In your implementation, you can read the packet header first (i.e., read HEADER_LEN bytes first), 
and then use the length field in the header to determine whether it is needed to read 
a block of data from the server. You may use the above nread function here.  
*/

static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  uint16_t plen; //length of reccieved message
  uint8_t head[HEADER_LEN]; //holds the message
     
  *ret = ntohs(*ret);
  plen = ntohs(plen);
  *op = ntohl(*op);
    
    if(plen > 8) {  //checks for a block
  
    if(!nread(fd, JBOD_BLOCK_SIZE, block)) {    //reads the block and copies content to block
      
      return false;
    }
    }
    
    if(!nread(fd, HEADER_LEN, head)) { //reads the packet

    return false;
  }
  
  memcpy(op, head + 2, sizeof(*op));
  memcpy(&plen, head, sizeof(plen));
  memcpy(ret, head + 6, sizeof(*ret));

    return true;
  }

/* The client attempts to send a jbod request packet to sd (i.e., the server socket here); 
returns true on success and false on failure. 
op - the opcode. 
block- when the command is JBOD_WRITE_BLOCK, the block will contain data to write to the server jbod system;
otherwise it is NULL.
The above information (when applicable) has to be wrapped into a jbod request packet (format specified in readme).
You may call the above nwrite function to do the actual sending.  
*/

static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  uint8_t pmes[HEADER_LEN + JBOD_BLOCK_SIZE]; //holds the message in an array
  uint16_t plen = HEADER_LEN;
  while (op >> 14 == JBOD_WRITE_BLOCK) {
  
    plen += JBOD_BLOCK_SIZE; //increments message length
    memcpy(pmes + 8, block, JBOD_BLOCK_SIZE); //adds the block
  }
 
  uint32_t pop = htonl(op);
  memcpy(pmes + 2, &pop, sizeof(op));
  plen = htons(plen);
  memcpy(pmes, &plen, sizeof(plen));
  plen = ntohs(plen); //converts back to host format
  return nwrite(sd, plen, pmes);
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. 
 * this function will be invoked by tester to connect to the server at given ip and port.
 * you will not call it in mdadm.c
*/

bool jbod_connect(const char *ip, uint16_t port) {

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  cli_sd = socket(PF_INET, SOCK_STREAM, 0);
  
  if(inet_aton(ip, &address.sin_addr) == 0) {
    return false;
  }
  
  if(cli_sd == -1) {
    return false;
  }

  if(connect(cli_sd, (const struct sockaddr *) &address, sizeof(address)) == -1) {
    return false;
  }

  return true;
}

/* disconnects from the server and resets cli_sd */

void jbod_disconnect(void) {
  cli_sd = -1;
  //close socket connection
  close(cli_sd);
}

/* sends the JBOD operation to the server (use the send_packet function) and receives 
(use the recv_packet function) and processes the response. 
The meaning of each parameter is the same as in the original jbod_operation function. 
return: 0 means success, -1 means failure.
*/

int jbod_client_operation(uint32_t op, uint8_t *block) {
  uint32_t opc; //op code of package
  uint16_t retc; //ret code of package
  send_packet(cli_sd, op, block);
  recv_packet(cli_sd, &opc, &retc, block);
  return retc;
}