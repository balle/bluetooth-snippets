/*
  BlueZ example code to use a bluetooth raw socket
  to sniff hci packets.
  But by now I have no idea how to decode them...
  BlueZ source code reading in progress! :)

  Programmed by Bastian Ballmann
  http://www.geektown.de

  Compile with gcc -lbluetooth <executable> <source>
*/

#include <sys/types.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

struct dump_hdr 
{
  __u16	len;
  __u8	in;
  __u8	pad;
  __u32	ts_sec;
  __u32   ts_usec;
} __attribute__ ((packed));
#define DUMP_HDR_SIZE (sizeof(struct dump_hdr))

struct frame 
{
  void	*data;
  int	data_len;
  void	*ptr;
  int	len;
  int 	in;
  int	handle;
  long	flags;
  struct timeval ts;
};

void hci_dump(struct frame *frm)
{
  __u8 type = *(__u8 *)frm->ptr; 
  
  frm->ptr++; frm->len--;
  
  if(type == HCI_ACLDATA_PKT)
    {
      hci_acl_hdr *hdr = (void *) frm->ptr;
      __u16 handle = btohs(hdr->handle);
      __u16 dlen = btohs(hdr->dlen);
      __u8 flags = acl_flags(handle);
      printf("ACL data: handle 0x%4.4x flags 0x%2.2x dlen %d\n",
       acl_handle(handle), flags, dlen);
    }
}

int main(void)
{
  struct sockaddr_hci addr;
  struct hci_filter filter;
  int sock, one = 1;
  char packet[HCI_MAX_FRAME_SIZE];
  struct cmsghdr *cmsg;
  struct msghdr msg;
  struct iovec  iv;
  struct dump_hdr *dh;
  struct frame frm;
  char *buf, *ctrl;

  if((sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0)
    {
      perror("socket");
      exit(1);
    }

  if(setsockopt(sock, SOL_HCI, HCI_DATA_DIR, &one, sizeof(one)) < 0) 
    {
      perror("Can't enable data direction info");
      exit(1);
    }
  
  if(setsockopt(sock, SOL_HCI, HCI_TIME_STAMP, &one, sizeof(one)) < 0) 
    {
      perror("Can't enable time stamp");
      exit(1);
    }

  hci_filter_clear(&filter);
  hci_filter_all_ptypes(&filter);
  hci_filter_all_events(&filter);
  
  if(setsockopt(sock, SOL_HCI, HCI_FILTER, &filter, sizeof(filter)) < 0) 
    {
      perror("Can't set HCI filter");
      exit(1);
    }

  addr.hci_family = AF_BLUETOOTH;
  addr.hci_dev = 0;
  
  if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
      perror("bind");
      exit(1);
    }

  if (!(buf = malloc(DUMP_HDR_SIZE))) 
    {
      perror("Can't allocate data buffer");
      exit(1);
    }
  
  dh = (void *) buf;
  frm.data = buf + DUMP_HDR_SIZE;
  
  if (!(ctrl = malloc(100))) 
    {
      perror("Can't allocate control buffer");
      exit(1);
    }
  
  memset(&msg, 0, sizeof(msg));

  while (1) 
    {
      iv.iov_base = frm.data;
      iv.iov_len  = snap_len;
      
      msg.msg_iov = &iv;
      msg.msg_iovlen = 1;
      msg.msg_control = ctrl;
      msg.msg_controllen = 100;
      
      if ((frm.data_len = recvmsg(sock, &msg, 0)) < 0) 
	{
	  perror("Receive failed");
	  exit(1);
	}

      /* Process control message */
      frm.in = 0;
      cmsg = CMSG_FIRSTHDR(&msg);

      while (cmsg) 
	{
	  switch (cmsg->cmsg_type) 
	    {
	    case HCI_CMSG_DIR:
	      frm.in = *((int *)CMSG_DATA(cmsg));
	      break;
	    case HCI_CMSG_TSTAMP:
	      frm.ts = *((struct timeval *)CMSG_DATA(cmsg));
	      break;
	    }

	  cmsg = CMSG_NXTHDR(&msg, cmsg);
	}

      frm.ptr = frm.data;
      frm.len = frm.data_len;
      
      /* Parse and print */
      hci_dump(&frm);
    }

  close(sock);
  return 0;
}
