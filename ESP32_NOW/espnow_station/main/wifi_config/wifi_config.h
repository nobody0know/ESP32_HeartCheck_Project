/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
#define CONNECTED_BIT BIT0
#define ESPTOUCH_DONE_BIT BIT1

extern void wifi_config_init(void);
extern char * get_ip_addr(void);
extern uint8_t ifneed_wifi_provision(void);
extern void wifi_save_config(uint8_t wifi_config_flag, char wifi_ssid[], char wifi_password[]);