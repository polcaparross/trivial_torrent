
// Trivial Torrent

// TODO: some includes here

#include "file_io.h"
#include "logger.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <fiu.h>


// TODO: hey!? what is this?

/**
 * This is the magic number (already stored in network byte order).
 * See https://en.wikipedia.org/wiki/Magic_number_(programming)#In_protocols
 */
static const uint32_t MAGIC_NUMBER = 0xde1c3233; // = htonl(0x33321cde);

static const uint8_t MSG_REQUEST = 0;
static const uint8_t MSG_RESPONSE_OK = 1;
static const uint8_t MSG_RESPONSE_NA = 2;



enum { RAW_MESSAGE_SIZE = 13 };



/**
 * Main function.
 */
int main(int argc, char **argv) {
	assert(argc > 0);
	assert(argv != NULL);
	set_log_level(LOG_DEBUG);

	log_printf(LOG_INFO, "Trivial Torrent (build %s %s) by %s", __DATE__, __TIME__, "J. DOE and J. DOE");

	struct torrent_t tor;
	struct block_t block;
	uint32_t magic_number;
	uint8_t message_code;
	
	if (argc == 2) {//si tenemos dos arguemntos sabemos que estamos en el cliente

		char noext[1691];
		strcpy(noext, argv[1]);
		if(access(noext, F_OK) ==-1){
			perror("Fichero Inexistente");
			return 1;
		}
		char *ext = strrchr(noext, '.');
		if(ext != NULL)
			*ext = '\0';
			
		int c = create_torrent_from_metainfo_file(argv[1], &tor, noext);//creamos el torrent
		if (c < 0) {			
			perror("Create Error");
			return 1;
		}
			
		uint8_t buffer[RAW_MESSAGE_SIZE];//buffer que vamos a enviar con el magic number el message code i el block number
		if (tor.block_map[0] == 1 && tor.block_map[1] == 1 && tor.block_map[2] == 1) {//tenemos todos los bloques destruimos el torrent i hacemos un return
			destroy_torrent(&tor);
			return 1;
		}
		
		for (uint64_t i = 0; i<tor.peer_count; i++) { //hacemos un bucle tantas veces como peers haya
			int s = socket(AF_INET,SOCK_STREAM, 0);//creamos el socket
			if (s < 0){
				perror("Socket Error");
				return 1;
			}
				
			//inicializamos el socket
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = tor.peers[i].peer_port;
			addr.sin_addr.s_addr = htonl(((uint32_t) tor.peers[i].peer_address[0] << 24) |  ((uint32_t) tor.peers[i].peer_address[1] << 16) | ((uint32_t) tor.peers[i].peer_address[2] << 8)  | ((uint32_t) tor.peers[i].peer_address[3] << 0));
				

			int co = connect(s, (struct sockaddr *) &addr, sizeof(addr));//conectamos el socket
			if (co == -1) {
				perror("Connect Error");
				close(s);
				return 1;
			}

				
			for (uint64_t block_number=0; block_number<tor.block_count; block_number++)//hacemos un bucle por cada bloque
			{
				
				magic_number = MAGIC_NUMBER;
				message_code = MSG_REQUEST;
				
				if (tor.block_map[block_number] == 1)//si el block_map esta lleno seguimos
					continue;
					
				//inicializamos el bucle
				buffer[0] = (uint8_t)(magic_number>>24);
				buffer[1] = (uint8_t)(magic_number>>16);
				buffer[2] = (uint8_t)(magic_number>>8);
				buffer[3] = (uint8_t)(magic_number>>0);
				buffer[4] = message_code;
				buffer[5] = (uint8_t)(block_number>>56);
				buffer[6] = (uint8_t)(block_number>>48);
				buffer[7] = (uint8_t)(block_number>>40);
				buffer[8] = (uint8_t)(block_number>>32);
				buffer[9] = (uint8_t)(block_number>>24);
				buffer[10] = (uint8_t)(block_number>>16);
				buffer[11] = (uint8_t)(block_number>>8);
				buffer[12] = (uint8_t)(block_number>>0);
					
				
				ssize_t se = send(s, buffer, RAW_MESSAGE_SIZE,0);//enviamos el buffer
				if (se < 0) {
					perror("Send Error");
					return 1;
				}
				
				ssize_t re = recv(s, &buffer, RAW_MESSAGE_SIZE,0);//recivimos el buffer
				if (re < 0) {
					perror("Recive Error");
					return 1;
				}
				
				//generamos la variable magic_number a partir del mensaje devueto
				magic_number = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | ((uint32_t)buffer[3] << 0);
				
					
				//si el magic_number no ha cambiado i el message_code pasa a ser response ok...
				if (buffer[4] == MSG_RESPONSE_OK && magic_number == MAGIC_NUMBER) {

					block.size = get_block_size(&tor, block_number);//para saber el tamaño del bloque
						
					ssize_t r = recv(s, &block.data, block.size,MSG_WAITALL);
					if (r < 0) {
						perror("Recive Error");
						return 1;
					}
						
					ssize_t st = store_block(&tor, block_number, &block);//guardamos el bloque
					if (st < 0) {
						perror("Store Error");
						return 1;
					}
				}
			}
			if (tor.block_map[0] == 1 && tor.block_map[1] == 1 && tor.block_map[2] == 1)//tenemos todos los bloques
				break;
			//cerramos el socket
			close(s);
				
		}
		destroy_torrent(&tor);	//destruimos el torrent
		return 0;
	}
	else {
		if (argc == 4) {//si tenemos cuatro arguementos sabemos que estamos en el servidor
		
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons((uint16_t)atoi(argv[2]));
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			socklen_t addr_len = sizeof(addr);
			struct block_t block_s;
			struct torrent_t torrent_s;
			
			magic_number = MAGIC_NUMBER;
			message_code = MSG_REQUEST;
			
			char noext[1691];
			strcpy(noext, argv[3]);
			if(access(noext, F_OK) ==-1){
				perror("Fichero Inexistente");
				return 1;
			}
			char *ext = strrchr(noext, '.');
			if(ext != NULL)
				*ext = '\0';
			
			int control = create_torrent_from_metainfo_file(argv[3], (struct torrent_t *) &torrent_s, noext);
			if (control == -1)
			{
				perror("Server Error");
				return 1;
			}
			
			int s1 = socket(AF_INET,SOCK_STREAM, 0);//creamos el socket
			if (s1 < 0) {
				perror("Socket Error");
				return 1;
			}
			
			int b = bind(s1, (struct sockaddr *) &addr, sizeof(addr));
			if (b == -1) {
				perror("Bind Error");
				return 1;
			}
			
			int l = listen(s1,10); //hacemos el listen
			if (l == -1) {
				perror("Listen Error");
				return 1;
			}
			while(1)
			
			{
				int s2 = accept(s1, (struct sockaddr *) &addr,  &addr_len); //aceptamos el socket
				if (s2 == -1) {
					perror("Accept Error");
					return 1;
				}
				int pid= fork();
				
				if (pid==0) // hijo
				{
					
					close(s1);
					while(1)
					{
						//inicializamos buffer
						uint64_t init = 0;
						uint8_t buffer[RAW_MESSAGE_SIZE] = {0};
						buffer[0] = (uint8_t)(magic_number>>24);
						buffer[1] = (uint8_t)(magic_number>>16);
						buffer[2] = (uint8_t)(magic_number>>8);
						buffer[3] = (uint8_t)(magic_number>>0);
						buffer[4] = message_code;
						buffer[5] = (uint8_t)(init>>56);
						buffer[6] = (uint8_t)(init>>48);
						buffer[7] = (uint8_t)(init>>40);
						buffer[8] = (uint8_t)(init>>32);
						buffer[9] = (uint8_t)(init>>24);
						buffer[10] = (uint8_t)(init>>16);
						buffer[11] = (uint8_t)(init>>8);
						buffer[12] = (uint8_t)(init>>0);
						
						ssize_t re = recv(s2, &buffer, RAW_MESSAGE_SIZE,MSG_WAITALL);//recibimos buffer
						if (re < 0) {
							perror("Recive Error");
							return 1;
						}
						else if (re == 0) {
							break;
						}
						//creamos un bloque
						uint64_t block2 = 
						((uint64_t)buffer[5]<<56) |
						((uint64_t)buffer[6]<<48) |
						((uint64_t)buffer[7]<<40) |
						((uint64_t)buffer[8]<<32) |
						((uint64_t)buffer[9]<<24) |
						((uint64_t)buffer[10]<<16) |
						((uint64_t)buffer[11]<<8) |
						((uint64_t)buffer[12]<<0);
						
						magic_number = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | ((uint32_t)buffer[3] << 0);
						if (buffer[4] == MSG_REQUEST && magic_number == MAGIC_NUMBER)
						{
							//cargamos el bloque
							int lb = load_block((const struct torrent_t * )&torrent_s, block2, (struct block_t * const)&block_s); 
							if (lb < 0) {
								perror("Load Error");
								return 1;
								}
							block_s.size = get_block_size((const struct torrent_t * )&torrent_s, block2);
							buffer[4] = MSG_RESPONSE_OK;
							
							assert(buffer != NULL);
							
							ssize_t se = send(s2, buffer, sizeof(buffer),0);//enviamos el buffer
							if (se < 0) {
								perror("Send Error");
								return 1;
								}
							ssize_t se2 = send(s2, block_s.data, block_s.size ,0);//enviamos el buffer
							if (se2 < 0) {
								perror("Send Error");
								return 1;
								}
						}
						else
						{
							buffer[4] = MSG_RESPONSE_NA;
							ssize_t se = send(s2, buffer, RAW_MESSAGE_SIZE,0);//enviamos el buffer
							if (se < 0) {
								perror("Send Error");
								return 1;
								}
						}
						
					}
					close(s2);
					exit(0);
				}
				else //padre
				{
					close(s2);
				}
			}
			close(s1);
		}

	}
	return 0;
}
		
	



