/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
#define CONNECTED_BIT BIT0
#define ESPTOUCH_DONE_BIT BIT1

extern void wifi_config_init(void);
extern char * get_ip_addr(void);
extern uint8_t ifneed_smart_config(void);