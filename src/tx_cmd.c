#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "tx_cmd.h"


int send_command(int port, cmd_req_t req, size_t req_size)
{
    struct sockaddr_in addr;
    cmd_resp_t resp;
    int rc;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(addr.sin_family, "127.0.0.1", &addr.sin_addr);

    rc = sendto(fd, &req, req_size, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0)
    {
        perror("sendto");
        return 1;
    }

    rc = recv(fd, &resp, sizeof(resp), 0);
    if (rc < 0)
    {
        perror("recvfrom");
        return 1;
    }

    if(rc != sizeof(resp) || resp.req_id != req.req_id)
    {
        fprintf(stderr, "Invalid response\n");
        return 1;
    }

    rc = ntohl(resp.rc);

    if(rc != 0)
    {
        fprintf(stderr, "Command failed: %s\n", strerror(rc));
        return 1;
    }

    return 0;
}


int set_fec(char *progname, int port, int argc, char **argv)
{
    int opt;
    uint8_t k=8, n=12;
    cmd_req_t req = { .req_id = htonl(rand()), .cmd_id = CMD_SET_FEC };

    while ((opt = getopt(argc, argv, "k:n:h")) != -1)
    {
        switch (opt)
        {
        case 'k':
            k = atoi(optarg);
            break;

        case 'n':
            n = atoi(optarg);
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s <port> %s [-k RS_K] [-n RS_N]\n", progname, argv[0]);
            fprintf(stderr, "Default: k=%d, n=%d\n", k, n);
            fprintf(stderr, "WFB-ng version " WFB_VERSION "\n");
            fprintf(stderr, "WFB-ng home page: <http://wfb-ng.org>\n");
            return 1;
        }
    }

    req.u.cmd_set_fec.k = k;
    req.u.cmd_set_fec.n = n;

    return send_command(port, req, offsetof(cmd_req_t, u) + sizeof(req.u.cmd_set_fec));
}

int set_radio(char *progname, int port, int argc, char **argv)
{
    int opt;
    int bandwidth = 20;
    int short_gi = 0;
    int stbc = 0;
    int ldpc = 0;
    int mcs_index = 1;
    int vht_nss = 1;
    bool vht_mode = false;
    cmd_req_t req = { .req_id = htonl(rand()), .cmd_id = CMD_SET_RADIO };

    while ((opt = getopt(argc, argv, "B:G:S:L:M:N:h")) != -1)
    {
        switch (opt)
        {
        case 'B':
            bandwidth = atoi(optarg);
            // Force VHT mode for bandwidth >= 80
            if (bandwidth >= 80) {
                vht_mode = true;
            }
            break;

        case 'G':
            short_gi = (optarg[0] == 's' || optarg[0] == 'S') ? 1 : 0;
            break;

        case 'S':
            stbc = atoi(optarg);
            break;

        case 'L':
            ldpc = atoi(optarg);
            break;

        case 'M':
            mcs_index = atoi(optarg);
            break;

        case 'N':
            vht_nss = atoi(optarg);
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s <port> %s [-B bandwidth] [-G guard_interval] [-S stbc] [-L ldpc] [-M mcs_index] [-N VHT_NSS]\n",
                    progname, argv[0]);
            fprintf(stderr, "Default: bandwidth=%d guard_interval=%s stbc=%d ldpc=%d mcs_index=%d vht_nss=%d\n",
                    bandwidth, short_gi ? "short" : "long", stbc, ldpc, mcs_index, vht_nss);
            fprintf(stderr, "WFB-ng version " WFB_VERSION "\n");
            fprintf(stderr, "WFB-ng home page: <http://wfb-ng.org>\n");
            return 1;
        }
    }

    req.u.cmd_set_radio.stbc = stbc;
    req.u.cmd_set_radio.ldpc = ldpc;
    req.u.cmd_set_radio.short_gi = short_gi;
    req.u.cmd_set_radio.bandwidth = bandwidth;
    req.u.cmd_set_radio.mcs_index = mcs_index;
    req.u.cmd_set_radio.vht_mode = vht_mode;
    req.u.cmd_set_radio.vht_nss = vht_nss;

    return send_command(port, req, offsetof(cmd_req_t, u) + sizeof(req.u.cmd_set_radio));
}


int main(int argc, char **argv)
{
    int port;
    char *command;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <port> {set_fec | set_radio} ...\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    port = atoi(argv[1]);
    command = argv[2];

    if (strcmp(command, "set_fec") == 0)
    {
        return set_fec(argv[0], port, argc - 2, argv + 2);
    }
    else if (strcmp(command, "set_radio") == 0)
    {
        return set_radio(argv[0], port, argc - 2, argv + 2);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
}


