/*
  BlueZ example code to use the hci inquiry mode
  to scan for bluetooth devices.

  Programmed by Bastian Ballmann
  http://www.geektown.de

  Compile with gcc -lbluetooth <executable> <source>
*/

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main(void)
{
  inquiry_info *info = NULL;
  bdaddr_t bdaddr;
  char name[248];
  int dev_id = 0;
  int num_rsp = 10;
  int flags = 0;
  int length = 8;
  int dd, i;

  printf("Scanning ...\n");
  num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);

  if(num_rsp < 0) 
    {
      perror("Inquiry failed.");
      exit(1);
    }

  if ((dd = hci_open_dev(dev_id)) < 0) 
    {
      perror("HCI device open failed");
      free(info);
      exit(1);
    }
  
  for(i = 0; i < num_rsp; i++) 
    {
      memset(name, 0, sizeof(name));

      if(hci_read_remote_name(dd, &(info+i)->bdaddr, sizeof(name), name, 100000) < 0)
	{
	  strcpy(name, "n/a");
	}

      baswap(&bdaddr, &(info+i)->bdaddr);
      printf("\t%s\t%s\n", batostr(&bdaddr), name);
  }
  
  close(dd);
  free(info);
  return 0;
}

