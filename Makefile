LDLIBS := -lbluetooth

all:rfcomm-server 
#rfcomm-client rfcomm-packet rfcomm-server sdpbrowse
#	$(CC) hciscan.c -o hciscan
#	$(CC) hcishow.c -o hciscan
#	$(CC) hcisniff.c -o hciscan
#	$(CC) l2cap-packet.c -o $@
#	$(CC) obex-test.c -o $@

rfcomm-client:
	$(CC) rfcomm-client.c -o $@

rfcomm-packet:
	$(CC) rfcomm-packet.c -o $@

rfcomm-server:
	$(CC) rfcomm-server.c -o $@ ${LDLIBS}

sdpbrowse:
	$(CC) sdpbrowse.c -o $@

clean:
	rm -f *.o
	rm -f rfcomm-client rfcomm-packet rfcomm-server sdpbrowse
