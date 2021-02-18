#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <gl/debug.h>
#include <gl/router.h>
#include <json-c/json.h>
#include <gl/gjson.h>
#include <gl/guci2.h>
#include <gl/shell.h>
#include <gl/debug.h>
//#include <gl/modem_api.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>
#include <uci.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/sockios.h>

#include "uart.h"

#define MSG_MAX 2048
#define BP_SIZE 1024
#define RESPAN_MAX 64
/**/
#define RECV_TIMEOUT 300

static int busy=0;
static char respon[RESPAN_MAX]={0};
static char message[MSG_MAX]={0};
static char last_msg[MSG_MAX]={0};
static pthread_t tid;
static pthread_t try;
static pthread_mutex_t mutex;
static int tty_fd=-1;
static int debug=0;
static int noupdate_c=0;



void system_shutdown()
{
	fork_exec2("/etc/init.d/umount stop");
}

static void clear_power_saving()
{
	char ps[10]={0};
	struct uci_context *ctx = guci2_init();
	if( UCI_OK == guci2_get(ctx, "mcu.global.power_saveing", ps )){
		if(strcmp(ps,"1")==0){
				guci2_set(ctx,"wireless.radio0.disabled","0");
				guci2_set(ctx,"wireless.radio1.disabled","0");
				guci2_set(ctx,"network.modem_1_1_2.disabled","0");
				guci2_set(ctx, "mcu.global.power_saveing", "0" );
				guci2_commit(ctx,"mcu");
				guci2_commit(ctx,"network");
				guci2_commit(ctx,"wireless");
		}
	}
	guci2_free(ctx);
}

static void power_saving_mode(int on)
{
	struct uci_context *ctx = guci2_init();
	if(on){
		if(debug)
			LOG(LOG_DEBUG,"\ne750-mcu Power saving ON\n");
		guci2_set(ctx,"wireless.radio0.disabled","1");
		guci2_set(ctx,"wireless.radio1.disabled","1");
		guci2_set(ctx,"network.modem_1_1_2.disabled","1");
		guci2_set(ctx, "mcu.global.power_saveing", "1" );
	}
	else{
		if(debug)
			LOG(LOG_DEBUG,"\ne750-mcu Power saving OFF\n");
		guci2_set(ctx,"wireless.radio0.disabled","0");
		guci2_set(ctx,"wireless.radio1.disabled","0");
		guci2_set(ctx,"network.modem_1_1_2.disabled","0");
		guci2_set(ctx, "mcu.global.power_saveing", "0" );
	}
	guci2_commit(ctx,"mcu");
	guci2_commit(ctx,"network");
	guci2_commit(ctx,"wireless");
	execCommand("/etc/init.d/network reload");
	guci2_free(ctx);
}

static void get_system_time(json_object * out)
{
	if(0 == access("/var/state/time_step",F_OK)) {
		time_t now =time(NULL);
		struct tm* ptm = localtime(&now);
		char sztmp[50] = {0};
		strftime(sztmp,50,"%H:%M",ptm);
		gjson_add_string(out, "clock", sztmp);
	}
	else{
		gjson_add_string(out, "clock", "unsync");
	}
}

static int get_disk_status(json_object * out)
{
	FILE *fp;
	char line[1024];

	if ((fp = fopen("/proc/mounts", "r"))) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, "/dev/sd") && strstr(line, "/mnt/")) {
				fclose(fp);
				gjson_add_string(out, "disk", "1");
				return 0;
			}
		}
		fclose(fp);
	}

	return -1;
}


/* get AP inforamtion */
static int get_ap_info(json_object * out)
{
	char hide_psk[5]={0};
	struct uci_context* ctx = guci2_init();
	if(ctx==NULL){
		return -1;
	}
	int iNumAPs = guci2_section_count(ctx, "wireless.@wifi-iface");
	for (int i = 0; i < iNumAPs; i++) {
		
		char ap_mode[10] = {0};
		char hw_mode[10] = {0};
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "mode", ap_mode);

		if ( strcmp(ap_mode, "ap") != 0 ) {
			continue;
		}
		char wifi_ssid[33] = {0};	
		char wifi_device[16] = {0};
		char key[33] = {0};
		char wifi_disabled[10] = {0};
		char network[10] = {0};
		
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "device", wifi_device);
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "ssid", wifi_ssid);
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "key", key);
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "disabled", wifi_disabled);
		guci2_get_idx(ctx, "wireless.@wifi-iface", i, "network", network);
		guci2_get_name(ctx,"wireless",wifi_device,"hwmode",hw_mode);
		
		
		if( !strcmp(hw_mode,"11a") ){
			if(!strcmp(network,"lan")){
				
				if(strcmp(wifi_disabled, "1")){
					gjson_add_string(out, "ssid_5g", wifi_ssid);
					gjson_add_string(out, "up_5g", "1");
					gjson_add_string(out, "key_5g", key);
				}
				/*else
					gjson_add_string(out, "up_5g", "0");*/
			}
			/*else{
				gjson_add_string(out, "guest_ssid_5g", wifi_ssid);
				if(strcmp(wifi_disabled, "1"))
					gjson_add_string(out, "guest_up_5g", "1");
				else
					gjson_add_string(out, "guest_up_5g", "0");
				gjson_add_string(out, "guest_key_5g", key);				
			}*/
		}
		else{
			if(!strcmp(network,"lan")){
				if(strcmp(wifi_disabled, "1")){
					gjson_add_string(out, "ssid", wifi_ssid);
					gjson_add_string(out, "up", "1");
					gjson_add_string(out, "key", key);
				}
				/*else
					gjson_add_string(out, "up", "0");*/
			}
			/*else{
				gjson_add_string(out, "guest_ssid", wifi_ssid);
				if(strcmp(wifi_disabled, "1"))
					gjson_add_string(out, "guest_up", "1");
				else
					gjson_add_string(out, "guest_up", "0");
				gjson_add_string(out, "guest_key", key);
			}*/
		}
		
	}
	guci2_get(ctx, "mcu.global.hide_psk", hide_psk );
	if(strcmp(hide_psk,"1")==0)
		gjson_add_string(out, "hide_psk", hide_psk);
	if(ctx){
		guci2_free(ctx);
	}
	return 0;
}

extern int router_client_statistics(json_object* input, json_object* output);
extern int modem_mcu_interface(char* bus,json_object *output);

static int mcu_get_modem_info(json_object * modem)
{
	char *bus="1-1.2";
	if(0 == modem_mcu_interface(bus,modem)) {
		return 0;
	}
	return -1;		
}

static int get_router_info(json_object * out)
{
	struct uci_context *ctx = guci2_init();
	char bridge[10] = {0}, ps[10]={0};
	char *ip = NULL;
	
	guci2_get(ctx, "glconfig.bridge.mode", bridge);
	if(strlen(bridge)==0){
		strcpy(bridge,"router");
	}
	if( strcmp(bridge,"router")==0 )
		gjson_add_string(out, "work_mode", "Router");	
	else if( strcmp(bridge,"ap")==0 )
		gjson_add_string(out, "work_mode", "AP");
	else if( strcmp(bridge,"wds")==0 )
		gjson_add_string(out, "work_mode", "WDS");
	else if( strcmp(bridge,"relay")==0 )
		gjson_add_string(out, "work_mode", "Extender");

	if( (strcmp(bridge,"ap")==0) || (strcmp(bridge,"relay")==0) ){
		ip = getShellCommandReturnDynamic("ifstatus lan | jsonfilter -e '@[\"ipv4-address\"][0].address'");
	}
	else if(strcmp(bridge,"wds")==0){
		ip = getShellCommandReturnDynamic("ifstatus lan | jsonfilter -e '@[\"ipv4-address\"][0].address'");
	}
	else if(strcmp(bridge,"router")==0){
		char ipaddr[20] = {0};
		guci2_get(ctx, "network.lan.ipaddr", ipaddr);
		if(strlen(ipaddr))
			gjson_add_string(out, "lan_ip",ipaddr);
	}
	if(ip){
		if( ip[strlen(ip)-1] == '\n' ){
			ip[strlen(ip)-1] = '\0';
		}
		gjson_add_string(out, "lan_ip",ip);
		free(ip);
	}
	if( UCI_OK == guci2_get(ctx, "mcu.global.power_saveing", ps )){
		gjson_add_string(out, "ps",ps);
	}
	else{
		gjson_add_string(out, "ps","0");
	}
	guci2_free(ctx);
	return 0;
}

static int check_ovpn_ip()
{
	int skfd;
    struct sockaddr_in *ina;
    static struct ifreq ifr;
    /* Get address of interface... */
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "tun0");
    if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
        close(skfd);
        return -1;
    } else {
        return 0;
    }
}

static int get_vpn_info(json_object * out)
{
	char ov_en[5]={0};
	char wg_en[5]={0};
	
	struct uci_context* ctx = guci2_init();
	if(ctx==NULL){
		return -1;
	}
	guci2_get_name(ctx,"glconfig","openvpn","enable",ov_en);
	guci2_get_idx(ctx, "wireguard.@proxy", 0, "enable", wg_en);
	if(!strcmp(ov_en,"1")){
		char server[64]={0};
		char clientid[10]={0};
		guci2_get_name(ctx,"glconfig","openvpn","clientid",clientid);	
		if(strlen(clientid)){
			guci2_get_name(ctx,"ovpnclients",clientid,"description",server);	
		}
		gjson_add_string(out, "vpn_server",server);
		gjson_add_string(out, "vpn_type","openvpn");
		char *up = getShellCommandReturnDynamic("ifstatus ovpn | jsonfilter -e '@.up'");
		if(up){
			if(strstr(up,"true")&& (check_ovpn_ip()==0))
				gjson_add_string(out, "vpn_status","connected");
			else
				gjson_add_string(out, "vpn_status","connecting");
			free(up);
		}
	}
	else if(!strcmp(wg_en,"1")){
		char server[128]={0};
		guci2_get_idx(ctx, "wireguard.@proxy", 0, "main_server", server);
		gjson_add_string(out, "vpn_type","wireguard");
		gjson_add_string(out, "vpn_server",server);
		char *last = getShellCommandReturnDynamic("wg show wg0 dump|tail -n 1|awk -F ' ' '{print $5}'");
		if(last){
			time_t now = time(NULL);
			time_t t = strtoull(last, NULL, 10);
			time_t diff = now - t;
			/* handshake between 0~3 minute, assume it is connected */
			if (diff >= 0 && diff <= 150)
				gjson_add_string(out, "vpn_status","connected");
			else
				gjson_add_string(out, "vpn_status","connecting");
			free(last);
		}
	}
	else{
		gjson_add_string(out, "vpn_status","off");
	}
	if(ctx){
		guci2_free(ctx);
	}
}

static int get_clients_info(json_object * out)
{
	int lannum=0,wlannum=0;
	char count[10]={0};
	json_object *clients = json_object_new_object();
	if(clients==NULL){
		return -1;
	}
	
	char *arp = NULL;
	arp = getShellCommandReturnDynamic("cat /proc/net/arp |grep -v address");
	if( arp == NULL ){
		gjson_add_string(out, "clients","0");
		return 0;
	}
	else{
		free(arp);
	}
	router_client_statistics(NULL,clients);	
	lannum = gjson_get_int(clients,"lan_cli_count");
	wlannum = gjson_get_int(clients,"wifi_cli_count");
	sprintf(count,"%d",lannum+wlannum);
	if(strlen(count)){
		gjson_add_string(out, "clients",count);
	}
	if(clients){
		json_object_put(clients);//free
	}
	return 0;
}

int get_network_meth(json_object * out)
{
	FILE *fp=NULL;
	char cur_meth[100]={0};
	char cur_status[100]={0};
	char path[200]={0};
	char msg[100]={0};
	char tor[5]="0";
	struct uci_context* ctx = guci2_init();
	guci2_get(ctx, "tor.global.enable", tor);
	guci2_free(ctx);
	if(!strcmp(tor, "1")){
		char *percent=getShellCommandReturnDynamic("cat /var/lib/tor/control.log |grep Bootstrapped|tail -n 1|awk -F \"[ :]\" '{print $9}'");
		if( percent != NULL ){
			if(strstr(percent,"100%")){
				gjson_add_string(out, "tor","1");
				free(percent);
			}
			else if (strstr(percent,"%")){
				sprintf(cur_meth,"Tor booting %s",percent);
				if(cur_meth[strlen(cur_meth)-1]=='\n')
					cur_meth[strlen(cur_meth)-1]='\0';
				gjson_add_string(out, "method_nw",cur_meth);
				free(percent);
				return 0;
			}
		}
	}
	if( access("/tmp/run/mwan3/indicator",F_OK) == 0 ){
		fp = fopen("/tmp/run/mwan3/indicator","r");
		if( fp==NULL )
			return -1;
		fread(cur_meth,99,1,fp);
		fclose(fp);
		if( strlen(cur_meth) == 0 )
			return -1;
		if(cur_meth[strlen(cur_meth)-1]=='\n')
			cur_meth[strlen(cur_meth)-1]='\0';
		sprintf(path,"/tmp/run/mwan3/iface_state/%s",cur_meth);
		//LOG(LOG_DEBUG,"\ne750-mcu :|%s|",path);
		if( access(path,F_OK) )
			return -1;
		
		fp = fopen(path,"r");
		if( fp==NULL )
			return -1;
		fread(cur_status,99,1,fp);
		fclose(fp);
		if( strlen(cur_status) == 0 )
			return -1;
		if(strncmp(cur_status,"online",6))
			return -1;
		
		struct uci_context* ctx = guci2_init();
		
		char tmp[33]={0};
		if(strstr(cur_meth,"modem"))
			gjson_add_string(out, "method_nw","modem");
		else if(strstr(cur_meth,"wwan")){
			getShellCommandReturn("iwinfo wlan-sta info 2>/dev/null|grep ESSID|cut -d '\"' -f 2",tmp);
			if(tmp[strlen(tmp)-1]=='\n')
				tmp[strlen(tmp)-1]='\0';
			if(strlen(tmp)){
				if(strlen(tmp)>16)
					tmp[16]='\0';
				sprintf(msg,"repeater|%s",tmp);
				gjson_add_string(out, "method_nw",msg);
			}
			else
				gjson_add_string(out, "method_nw","repeater");
		}
		else if(strstr(cur_meth,"wan")){
			guci2_get(ctx, "network.wan.proto", tmp );
			if(strlen(tmp)){
				sprintf(msg,"cable|%s",tmp);
				gjson_add_string(out, "method_nw",msg);
			}
			else
				gjson_add_string(out, "method_nw","cable");
		}
		else
			gjson_add_string(out, "method_nw",cur_meth);
		
		guci2_free(ctx);
	}
	return 0;
}

static int format_massage(char *buf)
{
	int len=strlen(buf)-1;//ignore '}' of endpoint
	char new[MSG_MAX]={0};
	int i=0,j=0;
	new[0] = buf[0];
	for(i=1,j=1;i<len;i++){
		if((buf[i]=='{')||(buf[i]=='}'))
			new[j++] = buf[i]-100;
		else if((buf[i]=='"')&&(buf[i-1]=='\\')&&(buf[i+1]!=' '))
			new[j++] = buf[i]-30;
		else if((buf[i]=='\\')&&(buf[i+1]=='\\'))
			new[j++] = buf[++i];
		else if(buf[i]!='\\')
			new[j++] = buf[i];		
	}
	new[j]= '}';
	memcpy(buf,new,strlen(new)+1);
	return strlen(buf)+1;
}

static int  get_mcu_status(char *data)
{
	char mcu_data[100]={0};
	FILE *stream=NULL;
	int percent=0,charging=0,c_count=0;
	float temp=0.0;
	int count=0;
	count = sscanf(data,",%d,%f,%d,%d",&percent,&temp,&charging,&c_count);
	if( count < 3 )
		return -1;
	sprintf(mcu_data,"{\"T\":%.1f,\"P\":%d,\"C\":%d,\"N\":%d}",temp,percent,charging,c_count);
	if(access("/tmp/mcu_data",F_OK)==0){
		truncate("/tmp/mcu_data",0);
	}
    stream = fopen("/tmp/mcu_data", "w+");
	if( stream == NULL )
		return -1;
	fwrite(mcu_data,strlen(mcu_data)+1,1,stream);
	fclose(stream);
	return 0;
	
}

static int  get_mcu_fg(char *data)
{
	FILE *stream=NULL;
	if(access("/tmp/mcu_fg",F_OK)==0){
		truncate("/tmp/mcu_fg",0);
	}
    stream = fopen("/tmp/mcu_fg", "w+");
	if( stream == NULL )
		return -1;
	fwrite(data,strlen(data)+1,1,stream);
	fclose(stream);
	return 0;
	
}

static int result_hander(char try_flag)
{
	char command[1024]={0};
	if(strlen(respon) == 0){
		
		return -1;
	}
	if(debug)
		LOG(LOG_DEBUG,"\ne750-mcu recived:%s\n",respon);
	if(try_flag==0){
		if( strstr(respon,"{\"shut_down\": \"1\"}")){
			if(debug)
				LOG(LOG_DEBUG,"\ne750-mcu system shutdown\n");
			system_shutdown();
		}
		else if( strstr(respon,"Power_Saving_on") ){
			power_saving_mode(1);
		}
		else if( strstr(respon,"Power_Saving_off") ){
			power_saving_mode(0);
		}
		else if( strncmp(respon,"{OK}",4)==0 ){
			if(respon[4]==','){
				get_mcu_status(respon+4);
			}
			return 0;
		}
		else if( strncmp(respon,"{FG},",5)==0 ){
			get_mcu_fg(respon+5);
			return 0;
		}
	}
	else{
		if( strncmp(respon,"{OK}",4)==0 ){
			return 0;
		}
	}
	return -1;
}

static int uart_write_read(char *buf,int len,int timeout)
{
	int ret=0;
	/*back data for try again*/
	if( buf != last_msg ){
		//LOG(LOG_DEBUG,"\ne750-mcu :\n%s\n",buf);
		len = format_massage(buf);
		if(!strcmp(last_msg,buf)){//如果信息没有改变，不发送
			if(debug)
				LOG(LOG_DEBUG,"\ne750-mcu message no change for %d times\n",noupdate_c);
			if( (noupdate_c++) > 6 )//强制更新
				noupdate_c = 0;
			else
				return 0;
		}
		noupdate_c = 0;
		memset(last_msg,0,MSG_MAX);
		memcpy(last_msg,buf,len);
	}
	if(debug)
		LOG(LOG_DEBUG,"\ne750-mcu send message:\n%s\n",buf);
	//pthread_mutex_lock(&mutex);
	if(uartRxPeek(tty_fd) > 0){//如果缓冲区中有数据，先读出并处理
		memset(respon,0,RESPAN_MAX);
		uartRxExpires(tty_fd,RESPAN_MAX,respon,RECV_TIMEOUT);
		result_hander(0);
	}
	flushIoBuffer(tty_fd);
	ret = uartTx(tty_fd,len,buf);
	//pthread_mutex_unlock(&mutex);
	return ret;
}

static void mcu_set_config(int num)
{
	struct uci_context* ctx = guci2_init();
	char content[65]={0}, display_mask[10]={0},custom_en[10]={0},uci_debug[10]={0};
	char screen[5][2]={"1","1","1","1","1"};
	int i=0,mask=0x0;
	
	guci2_get(ctx, "mcu.global.screen1_en", screen[0] );
	guci2_get(ctx, "mcu.global.screen2_en", screen[1] );
	guci2_get(ctx, "mcu.global.screen3_en", screen[2] );
	guci2_get(ctx, "mcu.global.screen4_en", screen[3] );
	guci2_get(ctx, "mcu.global.screen5_en", screen[4] );

	if( UCI_OK != guci2_get(ctx, "mcu.global.content", content )){
		memcpy(content," ",2);
	}	
	
	if( UCI_OK != guci2_get(ctx, "mcu.global.custom_en", custom_en )){
		memcpy(custom_en,"0",2);
	}
	
	if( UCI_OK != guci2_get(ctx, "mcu.global.debug", uci_debug )){
		memcpy(uci_debug,"0",2);
	}
	if(strcmp(uci_debug,"1")==0)
		debug = 1;
	else
		debug = 0;
	
	for(i=0;i<5;i++){
		if(screen[i][0]=='1')
			mask |= (0x1<<i);
	}
	
	sprintf(display_mask,"%02x",mask);
	
	json_object * out = json_object_new_object();
	gjson_add_string(out, "display_mask",display_mask);
	gjson_add_string(out, "custom_en",custom_en);
	gjson_add_string(out, "content",content);

	char *buf=(char *)json_object_to_json_string(out);
	memset(message,0,MSG_MAX);
	strcpy(message,buf);
	if(debug)
		LOG(LOG_DEBUG,"\ne750-mcu notify message:%s\n",message);
	
	uart_write_read(message,strlen(message)+1,RECV_TIMEOUT);
			
	if(out!=NULL)
		json_object_put(out);//free
	
	
	guci2_free(ctx);
}

static void event_sig_handler(int num)
{
	//LOG(LOG_DEBUG,"\ne750-mcu wait...\n");
	json_object *out = json_object_new_object();	
	get_ap_info(out);
	mcu_get_modem_info(out);
	get_router_info(out);
	get_vpn_info(out);
	get_clients_info(out);
	get_system_time(out);
	get_network_meth(out);
	get_disk_status(out);
	gjson_add_string(out, "mcu_status","1");
	char *buf=(char *)json_object_to_json_string(out);
	memset(message,0,MSG_MAX);
	strcpy(message,buf);
	if(out!=NULL){
		json_object_put(out);//free
	}
	uart_write_read(message,strlen(message)+1,RECV_TIMEOUT);
/*	if( result_hander(0) == 0 ){
		LOG(LOG_DEBUG,"\ne750-mcu Response:%s\n",respon);
	}*/
	/*else{
		try_again();
	}*/
}

static void message_notify(int num)
{
	struct json_object *out=NULL;
	if(0 > gjson_parse_file("/tmp/mcu_message",&out)){
			LOG(LOG_DEBUG,"\ne750-mcu message format error\n");
			return ;
	}
	if(out == NULL){
			LOG(LOG_DEBUG,"\ne750-mcu message format error\n");
			return ;
	}
	char *buf=(char *)json_object_to_json_string(out);
	memset(message,0,MSG_MAX);
	strcpy(message,buf);
	if(out!=NULL){
		json_object_put(out);//free
	}
	if(debug)
		LOG(LOG_DEBUG,"\ne750-mcu notify message:%s\n",message);
	uart_write_read(message,strlen(message)+1,RECV_TIMEOUT);
/*	if( result_hander(0) == 0 ){
		LOG(LOG_DEBUG,"\ne750-mcu Response:%s\n",respon);
	}*/
}


static void set_debug()
{
	struct uci_context* ctx = guci2_init();
	char uci_debug[10]={0};

	if( UCI_OK != guci2_get(ctx, "mcu.global.debug", uci_debug )){
		memcpy(uci_debug,"0",2);
	}
	if(strcmp(uci_debug,"1")==0)
		debug = 1;
	else
		debug = 0;
	
	guci2_free(ctx);
}

int hander_e750()
{	
	if(pthread_mutex_init(&mutex, NULL) < 0){
		return -1;
	}
	clear_power_saving();//After restarting, exit the power saveing mode
	set_debug();
    signal(SIGUSR1, event_sig_handler);
	signal(SIGUSR2, message_notify);
	signal(SIGALRM, mcu_set_config);
	tty_fd =  uartOpen("/dev/ttyS0",115200,0,100);
	if(tty_fd < 0){
		pthread_mutex_destroy(&mutex);
		return -1;
	}
	event_sig_handler(SIGUSR1);
    while(1){
        //pause();
		if(uartRxPeek(tty_fd) > 0){
			//pthread_mutex_lock(&mutex);
			memset(respon,0,RESPAN_MAX);
			uartRxExpires(tty_fd,RESPAN_MAX,respon,RECV_TIMEOUT);
			result_hander(0);
			//pthread_mutex_unlock(&mutex);
		}
		sleep(1);
		
    }
	uartClose(tty_fd);
	pthread_mutex_destroy(&mutex);
	return 0;
}

static void printf_help()
{
	printf("\nUsage:\n");
	printf("\te750-mcu:\tRun the MCU daemon\n");
	printf("\te750-mcu -h:\tGet help\n");
	printf("\te750-mcu argv[1]:\tSend temporary message and display on LCD\n");
	printf("\nSignals:\n");
	printf("\tUSR1:\tUpdate router information to LCD\n");
	printf("\tUSR2:\tRead /tmp/mcu_message JSON file and send the content to the MCU\n");
	printf("\tALRM:\tRead MCU configuration file and set the MCU display\n");
}

void send_tmp_msg(const char * msg)
{
	char tmp_msg[80]={0};
	json_object *out = json_object_new_object();
	gjson_add_string(out, "msg",msg);
	char *buf=(char *)json_object_to_json_string(out);
	strcpy(tmp_msg,buf);
	if(out!=NULL){
		json_object_put(out);//free
	}
	tty_fd =  uartOpen("/dev/ttyS0",115200,0,100);
	uart_write_read(tmp_msg,strlen(tmp_msg)+1,RECV_TIMEOUT);
	uartClose(tty_fd);	
}


int main(int argc,char *argv[])
{
	if(argc < 2){
		hander_e750();
		return 0;
	}
	else if ((strcmp(argv[1],"--help")==0)||(strcmp(argv[1],"-h")==0)){
		printf_help();
	}
	else {
		char tmp_mesg[65]={0};
		if(strlen(argv[1])>64){
			printf("The message is too big, trimmed to 64 characters\n");
			memcpy(tmp_mesg,argv[1],64);
		}
		else{
			strcpy(tmp_mesg,argv[1]);
		}
		
		send_tmp_msg(tmp_mesg);
	}
	return 0;
}
