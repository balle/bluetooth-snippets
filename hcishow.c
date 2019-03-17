/*
  BlueZ example code to use hci to get information
  about a local bluetooth device.

  Programmed by Bastian Ballmann
  http://www.geektown.de

  Compile with gcc -lbluetooth <executable> <source>
*/

#include <sys/socket.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main(void)
{
  struct hci_dev_info di;
  bdaddr_t bdaddr;
  struct hci_dev_stats *st;

  if(hci_devinfo(0, &di) < 0) 
    {
      perror("HCI device info failed");
      exit(1);
    }

  baswap(&bdaddr, &di.bdaddr);
  st = &di.stat;

  printf("%s:\tType: %s\n", di.name, hci_dtypetostr(di.type) );
  printf("\tBD Address: %s ACL MTU: %d:%d  SCO MTU: %d:%d\n",
	 batostr(&bdaddr), di.acl_mtu, di.acl_pkts,
	 di.sco_mtu, di.sco_pkts);

  printf("\tLink policy: %s\n", hci_lptostr(di.link_policy));
  printf("\tLink mode: %s\n", hci_lmtostr(di.link_mode));

  printf("\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n%s\n", 
	 di.features[0], di.features[1],
	 di.features[2], di.features[3],
	 lmp_featurestostr(di.features, "\t\t", 3));

  printf("\tRX bytes:%d acl:%d sco:%d events:%d errors:%d\n",
	 st->byte_rx, st->acl_rx, st->sco_rx, st->evt_rx, st->err_rx);

  printf("\tTX bytes:%d acl:%d sco:%d commands:%d errors:%d\n",
	 st->byte_tx, st->acl_tx, st->sco_tx, st->cmd_tx, st->err_tx);

  return 0;
}

