#include "getport_capabilities_main.h"

/***************************** Static definitions ********************************/

/* Default port */
/* ETHER_MAX_LEN = 1518 */
/* http://dpdk.org/doc/api/rte__ether_8h.html */

static const struct rte_eth_conf port_conf_default = {
  .rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN ,
  }
};

/* static const struct rte_eth_txconf tx_conf = { */
/* 	.tx_thresh = { */
/* 		.pthresh = TX_PTHRESH, */
/* 		.hthresh = TX_HTHRESH, */
/* 		.wthresh = TX_WTHRESH, */
/* 	}, */
/* 	.tx_free_thresh = 0,  */
/* 	.tx_rs_thresh = 0,  */
/* 	.txq_flags = 0,     */
/* }; */

/* mask of enabled ports */
static uint32_t main_enabled_port_mask = 0;


/***************************** Check port link status ********************************/
/* Check the link status of all ports in up to 9s, and print them finally */
/* wait before link up */
static void check_all_ports_link_status(uint8_t port_num, uint32_t port_mask){
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
  uint8_t portid, count, all_ports_up, print_flag = 0;
  struct rte_eth_link link;

  printf("---------------------------------\n");
  printf("Checking link status\n");
  fflush(stdout);
  for (count = 0; count <= MAX_CHECK_TIME; count++) {
    all_ports_up = 1;

    for (portid = 0; portid < port_num; portid++) {
      if ((port_mask & (1 << portid)) == 0)
	continue;
      memset(&link, 0, sizeof(link));
      rte_eth_link_get_nowait(portid, &link);
      
      /* print link status if flag set */
      if (print_flag == 1) {
	if (link.link_status){
	  printf("Port %d Link Up - speed %u "
		 "Mbps - %s\n", (uint8_t)portid,
		 (unsigned)link.link_speed,
		 (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
		 ("full-duplex") : ("half-duplex\n"));
      } else {
	  printf("Port %d Link Down\n",
		 (uint8_t)portid);
	}
	continue;
      }
      
      /* clear all_ports_up flag if any link down */
      if (link.link_status == 0) {
	all_ports_up = 0;
	break;
      }
    }
    /* after finally printing all link status, get out */
    if (print_flag == 1){
      break;
    }
    
    if (all_ports_up == 0) {
      printf(".");
      fflush(stdout);
      rte_delay_ms(CHECK_INTERVAL);
    }

    /* set the print_flag if all ports up or timeout */
    if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
      print_flag = 1;
      printf("done\n");
    }
  }
}


/***************************** Args parser ********************************/
/* display usage */
static void 
main_args_usage(const char *prgname)
{
  char buffer [300];
  int buffer_size = sizeof(buffer);
  int count;

  printf("--------------------------\n");
  count = snprintf(buffer,buffer_size, "%s",
		   prgname);

  printf("%s",buffer);
  
  if ((count >= 0) && (count < buffer_size) == 0 ){
    printf(" (program name too long)");
  } 
   

  printf(" [EAL options] -- -p PORTMASK \n"
	 "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	 ); 
}



/* parse portmask from args */
static int main_parse_portmask(const char *portmask){
  char *end = NULL;
  unsigned long pm;
  
  /* parse hexadecimal string */
  pm = strtoul(portmask, &end, 16);
  if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
    return -1;
  
  if (pm == 0)
    return -1;
  
  return pm;
}



/* Parse the argument given in the command line of the application */
/* taken and modified from l2fwd */
static int main_parse_args(int argc, char **argv){
  int options, return_value;
  char **argv_options;
  int option_index;
  char *program_name = argv[0];

  static struct option options_long[] = {
    {NULL, 0, 0, 0}
  };
  
  argv_options = argv;
  
  while ((options = getopt_long(argc, argv_options, "p:",  options_long, &option_index)) != EOF) {
    
    switch (options) {
      /* portmask */
    case 'p':
      main_enabled_port_mask = main_parse_portmask(optarg);
      main_information.port_mask = main_enabled_port_mask;
      if (main_enabled_port_mask == 0) {
	RTE_LOG(CRIT,PORT,"Parse args : Invalid portmask\n");
	main_args_usage(program_name);
	return -1;
      }
      break;

      /* long options */
    case 0:
      main_args_usage(program_name);
      return -1;
      
    default:
      main_args_usage(program_name);
      return -1;
    }
  }
  
  if (optind >= 0)
    argv[optind-1] = program_name;
  
  return_value = optind-1;
  optind = 0; /* reset getopt lib */
  return return_value;
}


/***************************** Port initilisation ********************************/
/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static
int port_init(uint8_t port_id, struct rte_mempool *mbuf_pool, port_information_t * port_info){
        struct rte_eth_conf port_conf = port_conf_default;
//	struct rte_eth_txconf *tx_conf; 
        struct rte_eth_dev_info dev_info;
	
	const uint16_t tx_rings = 1, rx_rings = 1;
        int retval;
        uint16_t q;

	printf("###");
	printf("port init\n");
        if (port_id >= rte_eth_dev_count()){
	  RTE_LOG(CRIT,PORT," PORT init error\n");
	  return -1;
	}

	/* check port capabilities*/
	rte_eth_dev_info_get(port_id, &dev_info);
	check_port_capabilities(dev_info);
	check_fdir_capabilities(port_id);


        /* Configure the Ethernet device. */
        retval = rte_eth_dev_configure(port_id, rx_rings, tx_rings, &port_conf);
        if (retval != 0){
	  RTE_LOG(CRIT,PORT,"rte_eth_dev_configure\n");
	  return retval;
	}

	/* rte_eth_dev_socket_id check socket validity,
	 * if it fails => 0 (=default)
         * Allocate and set up 1 RX queue per Ethernet port.
	 */
        for (q = 0; q < rx_rings; q++) {
                retval = rte_eth_rx_queue_setup(port_id, q, RX_RING_SIZE,
						rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
                if (retval < 0){
		  RTE_LOG(CRIT,PORT,"Rx queue setup\n");
		  return retval;
		}
        }

        /* Allocate and set up 1 TX queue per Ethernet port. */
        for (q = 0; q < tx_rings; q++) {
                retval = rte_eth_tx_queue_setup(port_id, q, TX_RING_SIZE,
						rte_eth_dev_socket_id(port_id), 
						NULL);
                if (retval < 0){
		  RTE_LOG(CRIT,PORT,"Tx queue setup\n");
		  return retval;
		}
        }

        /* Start the Ethernet port. */
        retval = rte_eth_dev_start(port_id);
        if (retval < 0){
	  RTE_LOG(CRIT,PORT,"start error\n"); 	 
	  return retval;
	}

        /* Display the port MAC address. */
        struct ether_addr addr;
        rte_eth_macaddr_get(port_id, &addr);
        printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
                           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
                        (unsigned)port_id,
                        addr.addr_bytes[0], addr.addr_bytes[1],
                        addr.addr_bytes[2], addr.addr_bytes[3],
                        addr.addr_bytes[4], addr.addr_bytes[5]);

        /* /\* Enable RX in promiscuous mode for the Ethernet device. *\/ */
        /* rte_eth_promiscuous_enable(port); */
	port_info->port_id = port_id;
	port_info->rings_rx = rx_rings;
	port_info->rings_tx = tx_rings;
        return 0;
}

/***************************** Args parser ********************************/

int main(int argc, char **argv){
        int return_value;
        uint8_t nb_ports;
	uint8_t nb_ports_available;
	struct rte_mempool *mbuf_pool;
	uint8_t portid;
	unsigned port;
	static const char* border = "*****************";

	/* not all log information goes to log_fd */
	/* log file */
	FILE *log_fd;
	if ((log_fd = fopen("log_stream","w+")) == NULL){
	  RTE_LOG(ERR,GETPORTINFO,"ERROR open file (log)\n");
	} else {
	  if (rte_openlog_stream(log_fd) != 0){
	    RTE_LOG(ERR,GETPORTINFO,"ERROR openlog_stream (log)\n");
	  }
	}


	/*initialisation EAL*/
	printf("---------------------------------\n");
	printf("- EAL init\n");
        return_value = rte_eal_init(argc, argv);
        if (return_value < 0){
	  rte_exit(EXIT_FAILURE,"Cannot init EAL\n");
	}
        argc -= return_value;
        argv += return_value;

	/* parse application arguments (after the EAL ones) */
        return_value =  main_parse_args(argc, argv);
        if (return_value < 0){
	  rte_exit(EXIT_FAILURE, "Invalid arguments\n");
	}


	 /* Check number of ports to send/receive */
	 /* Rte_eth_dev_count = get total number of eth devices  */
        nb_ports = rte_eth_dev_count();
        if (nb_ports < 1){
	  rte_exit(EXIT_FAILURE, "Error: need at least 1 port\n");
	}
	port_information_t port_list[nb_ports];
	printf("Number of ports = %d\n",nb_ports);



	for (port = 0; port < nb_ports; port++){
	  port_list[port].port_id = (uint8_t) (port);
	}

	/* Creates a new mempool in memory to hold the mbufs. */
	printf("---------------------------------\n");
	printf("Create mempool\n");
        mbuf_pool = rte_mempool_create("MBUF_POOL",
                                       NUM_MBUFS * nb_ports,
                                       MBUF_SIZE,
                                       MBUF_CACHE_SIZE,
                                       sizeof(struct rte_pktmbuf_pool_private),
                                       rte_pktmbuf_pool_init, NULL,
                                       rte_pktmbuf_init,      NULL,
                                       rte_socket_id(),
                                       0);

	if (mbuf_pool == NULL){
	  rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}
	port_list[0].queue[0].mempool_tx = mbuf_pool;
	nb_ports_available = nb_ports;


	/* Initialize all ports. */
	printf("---------------------------------\n");
	fflush(stdout);
        for (portid = 0; portid < nb_ports; portid++){
	  printf("%s Init port n° %"PRIu8 " %s \n",border,portid,border);

	  if ((main_enabled_port_mask & (1 << portid)) == 0) {
	    printf("Skipping disabled port %u\n", (unsigned) portid);
	    nb_ports_available--;
	    continue;
	  }

	  if (port_init(portid, mbuf_pool,&port_list[portid]) != 0){
	    rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n", portid);
	  }
	}


        if (!nb_ports_available) {
	  rte_exit(EXIT_FAILURE,"All available ports are disabled. Please set portmask.\n");
        }
	
	check_all_ports_link_status(nb_ports, main_enabled_port_mask);

	

	printf("Do nothing else\n");
	printf("Exit\n");
        return 0;
}
