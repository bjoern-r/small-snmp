/*
Copyright (c) 2011, Fraunhofer FOKUS - Fraunhofer Institute for Open Communication Systems FOKUS
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name of Fraunhofer FOKUS - Fraunhofer Institute for Open Communication Systems FOKUS
nor the names of its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

///////////////////////////////////////////////////////////////////////
//  Module:			main.c
//  Author:		Andreas Bartusch <xelean@googlemail.com>
//  Contact@FOKUS:	bjoern.riemer@fokus.fraunhofer.de
//  Purpose:		
//  Description:	Provides the main control logic
//  Date:			22.02.2011
////////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "snmp.h"
#include "db.h"

// globals
unsigned char community_read[]="public";
unsigned char community_write[]="private";
time_t startup_time;

unsigned char description[]="Pikkerton ZBS-110 Agent";
unsigned int timeticks = 123;
unsigned char MasterName[]="Pikkerton ZBS-110 Masteragent";
unsigned char MasterLocation[]="Floor4 foyer";
unsigned int NumberOfAgents=1;
unsigned char PID[]="ZBS-110";
unsigned char HW[]="0201";
unsigned char SW[]="0104";
unsigned char SN[]="ZBS110000283";
unsigned char ID[]="ZBS110000283";
unsigned int UB=0x7f;
unsigned int POW=0x01;
unsigned int WORK=403;
unsigned int FREQ=499375;
unsigned int VRMS=23000;
unsigned int IRMS=17900;
unsigned int LOAD=4100;

struct oid_db *oid_db;

//functions

void debugg(unsigned char *packet, int length)
{
	int position=0;
	int width=16;
	int i;
	unsigned char displayNumb[width];
	unsigned char displayChar[width];
	while (position<length) {
		for (i=0; i<width;i++) {
			displayNumb[i]=0x00;
			displayChar[i]=0x00;
		}
		for (i=0; i<(width); i++) {

			char temp;
			temp=packet[position++];
			displayNumb[i]=temp;
			if (temp<32) {
				temp='-';
			};
			displayChar[i]=temp;
			if (position>=length) {
				i=width;
			}
		}
		for (i=0; i<(width); i++) {
			printf("%02X", displayNumb[i]);
            if (i < (width - 1)) {
                printf(" ");
            }
		}
		printf(" -->   ");
		for (i=0;i< (width); i++) {
			printf("%c", displayChar[i]);
		}
		printf("\n");
	}
}

void process_varbind_list(struct varbind_list_rx *varbind_list, unsigned char pdu_type)
{
    int i;
    int idx;
	time_t t;

	for(i=0;i<varbind_list->varbind_idx;i++){
        if(pdu_type == PDU_GET_NEXT_REQ)
            idx = search_next_oid(oid_db, varbind_list->varbind_list[i]->oid);
        else
            idx = search_oid(oid_db, varbind_list->varbind_list[i]->oid);

        if(idx < 0)
            continue;


        /* Update some dynamic parameters */

        /* Timeticks */
        if(!strcmp((const char *) varbind_list->varbind_list[i]->oid, "1.3.6.1.2.1.1.3.0")) {
            timeticks = (time(&t) - startup_time) * 100;
            update_varbind(varbind_list->varbind_list[i],
                           oid_db->oids[idx]->data_type, &timeticks);

        /* All static parameters are taken from DB as is */
        } else {
            update_varbind(varbind_list->varbind_list[i],
                           oid_db->oids[idx]->data_type, oid_db->oids[idx]->value);
		}
	}

}

void sendPacket(struct in_addr host, short port, int sock,
                struct snmp_message_tx *snmp_msg)
{
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr =host;
	bzero(&(server_addr.sin_zero),8);
    sendto(sock, snmp_msg->snmp_message, snmp_msg->snmp_message_len, 0,
           (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    printf("\nsend snmp-message to %s , %d \n", inet_ntoa(server_addr.sin_addr),
           ntohs(server_addr.sin_port));
	debugg(snmp_msg->snmp_message,snmp_msg->snmp_message_len);
}

int main()
{
	time(&startup_time);
	int sock;
	socklen_t addr_len;
	int bytes_read;
	char recv_data[1024];
	struct sockaddr_in server_addr , client_addr;

    oid_db = create_oid_db();

    add_oid(oid_db, (unsigned char *) "1.3.6.1.2.1.1.1.0", PRIMV_OCTSTR, description);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.2.1.1.3.0", PRIMV_TIMTICK, &timeticks);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.1.1.0", PRIMV_OCTSTR, MasterName);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.1.2.0", PRIMV_OCTSTR, MasterLocation);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.1.3.0", PRIMV_INT, &NumberOfAgents);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.1.0", PRIMV_OCTSTR, PID);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.2.0", PRIMV_OCTSTR, HW);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.3.0", PRIMV_OCTSTR, SW);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.4.0", PRIMV_OCTSTR, SN);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.5.0", PRIMV_OCTSTR, ID);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.1.6.0", PRIMV_INT, &UB);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.1.0", PRIMV_INT, &POW);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.2.0", PRIMV_INT, &WORK);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.3.0", PRIMV_INT, &FREQ);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.4.0", PRIMV_INT, &VRMS);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.5.0", PRIMV_INT, &IRMS);
    add_oid(oid_db, (unsigned char *) "1.3.6.1.4.1.99.2.1.2.6.0", PRIMV_INT, &LOAD);

    print_oid_db(oid_db);

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("Socket");
		exit(1);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(161); 
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);
	if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
		perror("Bind");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	printf("\nWaiting for SNMP-clients on port 161");
	fflush(stdout);

	while (1){
        bytes_read =
            recvfrom(sock, recv_data, 1024, 0, (struct sockaddr *) &client_addr,
                     &addr_len);
		recv_data[bytes_read] = '\0';

        printf("\n(%s , %d)\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
		printf("\n");

        struct snmp_message_rx *snmp_msg =
            create_snmp_message_rx((unsigned char *) &recv_data[0]);

		if(snmp_msg!=NULL){
			disp_snmp_message_rx(snmp_msg);

			struct snmp_pdu_rx* snmp_pdu=create_snmp_pdu_rx(snmp_msg->snmp_pdu);
			if(snmp_pdu!=NULL){
				printf("\n");
				disp_snmp_pdu_rx(snmp_pdu);

                struct varbind_list_rx *varbind_list =
                    create_varbind_list_rx(snmp_pdu->varbindings);

				printf("\n");
				disp_varbind_list_rx(varbind_list);


				unsigned int a=5; //testing
                struct varbind *varbind =
                    create_varbind(varbind_list->varbind_list[0]->oid,
                                   PRIMV_INT, &a);

				printf("\n");
				disp_varbind(varbind);

                process_varbind_list(varbind_list, snmp_pdu->snmp_pdu_type);

				printf("\n");
				disp_varbind_list_rx(varbind_list);

                /* Response */
                struct varbind_list_tx *varbind_list_to_send =
                    create_varbind_list_tx(varbind_list);

                struct snmp_pdu_tx *snmp_pdu_tx =
                    create_snmp_pdu_tx(PDU_GET_RESP, snmp_pdu->request_id, 0x00,
                                       0x00, varbind_list_to_send);

				printf("\n");

                struct snmp_message_tx *snmp_msg_tx =
                    create_snmp_message_tx(snmp_msg->community,
                                           snmp_pdu_tx);
                sendPacket(client_addr.sin_addr, ntohs(client_addr.sin_port), sock,
                           snmp_msg_tx);
				clr_snmp_message_tx(snmp_msg_tx);
				clr_snmp_pdu_tx(snmp_pdu_tx);
				clr_varbind_list_tx(varbind_list_to_send);
				clr_varbind(varbind);
				clr_varbind_list_rx(varbind_list);
				clr_snmp_pdu_rx(snmp_pdu);
			}
			clr_snmp_message_rx(snmp_msg);
		}
		fflush(stdout);
	}
	return 0;
}
