#include <openobex/obex.h>
#include <obexftp/obexftp.h>
#include <obexftp/client.h>

#define BTADDR "00:01:E3:00:D3:63"
#define CHANNEL 4

int main(void)
{
  obexftp_client_t *cli;

  cli = obexftp_cli_open (OBEX_TRANS_BLUETOOTH, ctrans, NULL);
  obexftp_cli_connect(obexftp_client_t *cli, char *device, int port);

  obexftp_setpath(obexftp_client_t *cli, char *name);
  obexftp_put_file(obexftp_client_t *cli, char *localname, char *remotename);

  obexftp_cli_disconnect(obexftp_client_t *cli);

  return 0;
}
