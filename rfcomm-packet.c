/*
 * RFCOMM packetgenerator
 *
 * By Bastian Ballmann <balle@chaostal.de>
 * http://www.datenterrorist.de
 * Last update: 08.05.2008
 *
 * License: GPLv3 
 */

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#define RFCOMM_SABM     0x2f
#define RFCOMM_DISC     0x43
#define RFCOMM_UA       0x63
#define RFCOMM_DM       0x0f
#define RFCOMM_UIH      0xef

#define RFCOMM_TEST     0x08
#define RFCOMM_FCON     0x28
#define RFCOMM_FCOFF    0x18
#define RFCOMM_MSC      0x38
#define RFCOMM_RPN      0x24
#define RFCOMM_RLS      0x14
#define RFCOMM_PN       0x20
#define RFCOMM_NSC      0x04

#define __addr(cr, dlci)       (((dlci & 0x3f) << 2) | (cr << 1) | 0x01)
#define __ctrl(type, pf)       (((type & 0xef) | (pf << 4)))
#define __len8(len)            (((len) << 1) | 1)


/* ---- RFCOMM FCS computation ---- */

/* reversed, 8-bit, poly=0x07 */
static unsigned char rfcomm_crc_table[256] = {
  0x00, 0x91, 0xe3, 0x72, 0x07, 0x96, 0xe4, 0x75,
  0x0e, 0x9f, 0xed, 0x7c, 0x09, 0x98, 0xea, 0x7b,
  0x1c, 0x8d, 0xff, 0x6e, 0x1b, 0x8a, 0xf8, 0x69,
  0x12, 0x83, 0xf1, 0x60, 0x15, 0x84, 0xf6, 0x67,

  0x38, 0xa9, 0xdb, 0x4a, 0x3f, 0xae, 0xdc, 0x4d,
  0x36, 0xa7, 0xd5, 0x44, 0x31, 0xa0, 0xd2, 0x43,
  0x24, 0xb5, 0xc7, 0x56, 0x23, 0xb2, 0xc0, 0x51,
  0x2a, 0xbb, 0xc9, 0x58, 0x2d, 0xbc, 0xce, 0x5f,

  0x70, 0xe1, 0x93, 0x02, 0x77, 0xe6, 0x94, 0x05,
  0x7e, 0xef, 0x9d, 0x0c, 0x79, 0xe8, 0x9a, 0x0b,
  0x6c, 0xfd, 0x8f, 0x1e, 0x6b, 0xfa, 0x88, 0x19,
  0x62, 0xf3, 0x81, 0x10, 0x65, 0xf4, 0x86, 0x17,

  0x48, 0xd9, 0xab, 0x3a, 0x4f, 0xde, 0xac, 0x3d,
  0x46, 0xd7, 0xa5, 0x34, 0x41, 0xd0, 0xa2, 0x33,
  0x54, 0xc5, 0xb7, 0x26, 0x53, 0xc2, 0xb0, 0x21,
  0x5a, 0xcb, 0xb9, 0x28, 0x5d, 0xcc, 0xbe, 0x2f,

  0xe0, 0x71, 0x03, 0x92, 0xe7, 0x76, 0x04, 0x95,
  0xee, 0x7f, 0x0d, 0x9c, 0xe9, 0x78, 0x0a, 0x9b,
  0xfc, 0x6d, 0x1f, 0x8e, 0xfb, 0x6a, 0x18, 0x89,
  0xf2, 0x63, 0x11, 0x80, 0xf5, 0x64, 0x16, 0x87,

  0xd8, 0x49, 0x3b, 0xaa, 0xdf, 0x4e, 0x3c, 0xad,
  0xd6, 0x47, 0x35, 0xa4, 0xd1, 0x40, 0x32, 0xa3,
  0xc4, 0x55, 0x27, 0xb6, 0xc3, 0x52, 0x20, 0xb1,
  0xca, 0x5b, 0x29, 0xb8, 0xcd, 0x5c, 0x2e, 0xbf,

  0x90, 0x01, 0x73, 0xe2, 0x97, 0x06, 0x74, 0xe5,
  0x9e, 0x0f, 0x7d, 0xec, 0x99, 0x08, 0x7a, 0xeb,
  0x8c, 0x1d, 0x6f, 0xfe, 0x8b, 0x1a, 0x68, 0xf9,
  0x82, 0x13, 0x61, 0xf0, 0x85, 0x14, 0x66, 0xf7,

  0xa8, 0x39, 0x4b, 0xda, 0xaf, 0x3e, 0x4c, 0xdd,
  0xa6, 0x37, 0x45, 0xd4, 0xa1, 0x30, 0x42, 0xd3,
  0xb4, 0x25, 0x57, 0xc6, 0xb3, 0x22, 0x50, 0xc1,
  0xba, 0x2b, 0x59, 0xc8, 0xbd, 0x2c, 0x5e, 0xcf
};



// Functions
void usage(void);
//u8 __fcs2(u8 *data);
//asm static inline u8 __fcs2(u8 *data);


// MAIN PART
int main(int argc, char *argv[]) {
  struct rfcomm_cmd *cmd;    // struct detailed in kernel_source/include/net/bluetooth/rfcomm.h
  struct sockaddr_rc laddr, raddr;
  struct hci_dev_info di;
  char *buf, *remote_address = NULL;
  struct rfcomm_session *s;
  char dummy_payload[] = "greets from ccc gpn7 2008";
  char *payload = dummy_payload;
  int sock, c, i;
  int rf_channel = 0;
  int rf_ctrl = RFCOMM_TEST;
  int rf_len = 42;
  int rf_fcs = __fcs2(&cmd);  // rfcomm_cmd stuff can be found in kernel_source/net/bluetooth/rfcomm/core.c

  // Get params
  while ((c = getopt (argc, argv, "a:c:i:p:s:")) != -1) {
    switch (c) {
      case 'a':
           remote_address = optarg;
           break;

      case 'c':
           rf_channel = atoi(optarg);
           break;

      case 'i':
           rf_ctrl = atoi(optarg);
           break;

      case 'p':
           payload = (char *)optarg;
           break;

      case 's':
           rf_len = atoi(optarg);
           break;

      default:
        usage();
        break;
    }
  }

  if (remote_address == NULL) {
    printf(">>> I need at least a remote btaddr...\n\n");
    usage();
    exit(EXIT_FAILURE);
  }

  if (rf_len == 42) {
    rf_len = strlen(payload);
  }

  // Get local device info
  if (hci_devinfo(0, &di) < 0) {
    perror("HCI device info failed");
    exit(EXIT_FAILURE);
  }

  printf("Local device %s\n", batostr(&di.bdaddr));
  printf("Remote device %s\n", remote_address);
  printf("Remote channel %d\n", rf_channel);
  printf("RFCOMM ctrl %d\n", rf_ctrl);
  printf("RFCOMM len %d\n", rf_len);
  printf("RFCOMM fcs %d\n", rf_fcs);

  /* Construct local addr */
  memset(&laddr, 0, sizeof(laddr));
        laddr.rc_family = AF_BLUETOOTH;
        laddr.rc_bdaddr = di.bdaddr;
        laddr.rc_channel = 0;

  /* Construct remote addr */
  memset(&raddr, 0, sizeof(raddr));
        raddr.rc_family = AF_BLUETOOTH;
        str2ba(remote_address, &raddr.rc_bdaddr);
        raddr.rc_channel = rf_channel;

  /* Create a Bluetooth raw socket */
  if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_RFCOMM)) < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* ...and bind it to the local device */
  if (bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /* Let's try to connect */
  if (connect(sock, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  /* Init packet buffer */
  if (!(buf = (char *) malloc (sizeof(cmd) + strlen(payload))) ) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  // RFCOMM session
  s = rfcomm_session_get(laddr.rc_bdaddr, raddr.rc_bdaddr);

  /* Set RFCOMM header properties */
  cmd = (struct rfcomm_cmd *) buf;
  cmd->addr = raddr.rc_bdaddr; //__addr(s->initiator, remote_address);
  cmd->ctrl = __ctrl(rf_ctrl);
  cmd->len = __len8(rf_len);
  cmd->fcs = __fcs2(rf_fcs);

  /* Copy payload after rfcomm header */
  strncpy((buf + sizeof(cmd)), payload, strlen(payload));

  /* Throw the packet into the air */
  if (send(sock, buf, sizeof(cmd) + strlen(payload), 0) <= 0) {
    perror("send");
  }

  printf("RFCOMM packet was send\n");

  /* Disconnect */
  close(sock);

  return EXIT_SUCCESS;
}

// Print usage
void usage(void) {
  printf("rfcomm-packet -a <bdaddr> -c <channel> -i <rfcomm_instruction> -p <payload> -s <packetsize>\n");
  exit(EXIT_SUCCESS);
}

/* FCS on 3 bytes */
static inline u8 __fcs2(u8 *data) {
  return (0xff - rfcomm_crc_table[__crc(data) ^ data[2]]);
}

// EOF dude.
