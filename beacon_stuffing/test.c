#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

void test_callback(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
    int i=0;

    printf("Recieved Packet Size: %d\n", pkthdr->len);
    printf("Payload:\n");
    for(i=0;i<pkthdr->len;i++) {
        if(isprint(packet[i]))
            printf("%c ",packet[i]);
        else
            printf(" . ",packet[i]);
        if((i%16==0 && i!=0) || i==pkthdr->len-1)
            printf("\n");
    }
}

int main(int argc, char **argv) {
    // Buffer for storing an error message in case of an error.
    char errbuf[PCAP_ERRBUF_SIZE];

    // The network interface we want to open for packet capture.
    char *device = argv[1];

    // Open device for reading in promiscuous mode.
    pcap_t *handler = pcap_open_live(device, BUFSIZ, 1, -1, errbuf);
    if(handler == NULL) {
        fprintf(stderr, "pcap_open_live(): %s\n", errbuf);
        exit(-1);
    }

    // Stores the compiled version of the filter.
    struct bpf_program fp;

    // Compile the filter expression to get only beacon frames.
    char *filter = "wlan[0] == 0x08";
    if(pcap_compile(handler, &fp, filter, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Error compiling the filter expression\n");
        exit(-1);
    }

    // Set the filter.
    if(pcap_setfilter(handler, &fp) == -1) {
        fprintf(stderr, "Error setting filter.\n");
        exit(-1);
    }

    // Start collecting packets.
    pcap_loop(handler, -1, test_callback, NULL);
    return 0;
}
