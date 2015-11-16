#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <getopt.h> // getopt_long

#include <fcntl.h> // openfile


#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_eth_ctrl.h>

#include <rte_log.h>
#include <rte_cycles.h>

#include "getport_capabilities.h"
//#include "getflags_name.h"

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NB_QUEUE 1
#define MAX_PKT_BURST 32


#define NUM_MBUFS 1023 
#define MBUF_SIZE (1600 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define MBUF_CACHE_SIZE 250


#define RTE_LOGTYPE_GETPORTINFO RTE_LOGTYPE_USER1

/*********************************************/

typedef struct port_information_s {
  uint8_t port_id;
  uint16_t rings_tx;
  uint16_t rings_rx;

  struct queue {
    struct rte_mempool * mempool_rx; 
    struct rte_mempool * mempool_tx; 
  } queue[NB_QUEUE] ;
} port_information_t;


typedef struct main_information_s {
  uint8_t port_nb;
  uint32_t port_mask;
} main_information_t;
main_information_t main_information;
