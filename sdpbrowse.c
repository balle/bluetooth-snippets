/*
  BlueZ example code to read the services of
  a remote sdp server.

  Programmed by Bastian Ballmann
  http://www.geektown.de

  Compile with gcc -lbluetooth -lsdp <executable> <source>
*/

#include <stdlib.h>
#include <unistd.h>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/hci_lib.h>

int main(int argc, char *argv[])
{
  bdaddr_t bdaddr;
  sdp_list_t *attrid, *search, *seq;
  uint32_t range = 0x0000ffff;
  sdp_session_t *sess;
  struct hci_dev_info di;
  uuid_t root_uuid;

  if(argc < 2)
    {
      printf("%s <btaddr>\n", argv[0]);
      exit(0);
    }

  // Get local bluetooth address
  if(hci_devinfo(0, &di) < 0) 
    {
      perror("HCI device info failed");
      exit(1);
    }

  str2ba(argv[1],&bdaddr);

  // Connect to remote SDP server
  sess = sdp_connect(&di.bdaddr, &bdaddr, SDP_RETRY_IF_BUSY);

  if(!sess) 
    {
      perror("Failed to connect to SDP server");
      exit(1);
    }

  printf("Browsing %s ...\n", argv[1]);

  // Build linked lists
  sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
  attrid = sdp_list_append(0, &range);
  search = sdp_list_append(0, &root_uuid);

  // Get a linked list of services
  if(sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq) < 0)
    {
      perror("SDP service search");
      sdp_close(sess);
      exit(1);
    }

  sdp_list_free(attrid, 0);
  sdp_list_free(search, 0);

  // Loop through the list of services
  for(; seq; seq = seq->next)
    {
      sdp_record_t *rec = (sdp_record_t *) seq->data;
      sdp_list_t *access = NULL;
      int channel;

      // Print the service name
      sdp_record_print(rec);

      // Print the RFCOMM channel
      sdp_get_access_protos(rec, &access);

      if(access)
	{
	  channel = sdp_get_proto_port(access, RFCOMM_UUID);
	  printf("Channel: %d\n", channel);
	}
    }

    free(seq);
    sdp_close(sess);
    return 0;
}
