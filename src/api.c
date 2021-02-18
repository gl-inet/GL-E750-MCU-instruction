#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gl/debug.h>
#include <json-c/json.h>
#include <gl/gjson.h>
#include <gl/guci2.h>
#include <gl/shell.h>

int mcu_get_data(json_object * input, json_object * output)
{
    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    double temp=0;
    int percent=0;
    int charging=0;
    int c_count=0;
    json_object *obj=NULL;	

    stream = fopen("/tmp/mcu_data", "r");
    if (stream == NULL)
        return 0;
    fseek(stream,0,SEEK_SET);
    if(getline(&line, &len, stream) == -1)
		return 0;

	obj = json_tokener_parse(line);
	if(obj != NULL){
		temp = gjson_get_double(obj,"T");
		c_count = gjson_get_int(obj,"N");
		percent = gjson_get_int(obj,"P");
		charging = gjson_get_int(obj,"C");
		//LOG(LOG_DEBUG,"temp %lf\n",temp);
		if(temp < 0)
			temp = 0;
		if(percent < 0)
			percent = 0;
		if(charging < 0)
			charging = 0;		
	}	
	
    free(line);
    fclose(stream);

	if(percent > 100 )
		percent = 100;
	if(percent < 0 )
		percent = 0;		
	
	gjson_add_double(output,"Temperature",temp);
	gjson_add_int(output,"Charge_cnt",c_count);
	gjson_add_int(output,"Percent",percent);
	gjson_add_int(output,"Charging",charging);
	return 0;
}


int mcu_get_config(json_object * input, json_object * output)
{
	char content[65]={0},custom_en[10]={0},hide_psk[3]={0};
	char screen[5][3]={0};

	struct uci_context* ctx = guci2_init();	
	guci2_get(ctx, "mcu.global.screen1_en", screen[0] );
	guci2_get(ctx, "mcu.global.screen2_en", screen[1] );
	guci2_get(ctx, "mcu.global.screen3_en", screen[2] );
	guci2_get(ctx, "mcu.global.screen4_en", screen[3] );
	guci2_get(ctx, "mcu.global.screen5_en", screen[4] );
	guci2_get(ctx, "mcu.global.content", content );
	guci2_get(ctx, "mcu.global.custom_en", custom_en );
	guci2_get(ctx, "mcu.global.hide_psk", hide_psk );

	if(strcmp(hide_psk,"1")==0){
		gjson_add_boolean(output, "hide_psk",1);
	}
	else
	{
		gjson_add_boolean(output, "hide_psk",0);
	}
	
	if(strcmp(screen[0],"0") == 0 ){
		gjson_add_boolean(output,"screen1_en", 0);
	}
	else
	{
		gjson_add_boolean(output,"screen1_en", 1);
	}
	if(strcmp(screen[1],"0") == 0 ){
		gjson_add_boolean(output,"screen2_en", 0);
	}
	else
	{
		gjson_add_boolean(output,"screen2_en", 1);
	}	
	if(strcmp(screen[2],"0") == 0 ){
		gjson_add_boolean(output,"screen3_en", 0);
	}
	else
	{
		gjson_add_boolean(output,"screen3_en", 1);
	}
	if(strcmp(screen[3],"0") == 0 ){
		gjson_add_boolean(output,"screen4_en", 0);
	}
	else
	{
		gjson_add_boolean(output,"screen4_en", 1);
	}
	if(strcmp(screen[4],"0") == 0 ){
		gjson_add_boolean(output,"screen5_en", 0);
	}
	else
	{
		gjson_add_boolean(output,"screen5_en", 1);
	}
	if(strcmp(custom_en,"1") == 0 ){
		gjson_add_boolean(output,"custom_en", 1);
	}
	else
	{
		gjson_add_boolean(output,"custom_en", 0);
	}

	gjson_add_string(output, "content",content);

	guci2_free(ctx);
	return 0;
}


int mcu_set_config(json_object * input, json_object * output)
{
	bool hide_psk = gjson_get_boolean(input, "hide_psk");
	bool screen1_en = gjson_get_boolean(input, "screen1_en");
	bool screen2_en = gjson_get_boolean(input, "screen2_en");
	bool screen3_en = gjson_get_boolean(input, "screen3_en");
	bool screen4_en = gjson_get_boolean(input, "screen4_en");
	bool screen5_en = gjson_get_boolean(input, "screen5_en");
	bool custom_en = gjson_get_boolean(input,"custom_en");
	const char *content = gjson_get_string(input,"content");

	struct uci_context* ctx = guci2_init();
	if(hide_psk){
		guci2_set(ctx, "mcu.global.hide_psk", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.hide_psk", "0");
	}
	if(screen1_en){
		guci2_set(ctx, "mcu.global.screen1_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.screen1_en", "0");
	}
	if(screen2_en){
		guci2_set(ctx, "mcu.global.screen2_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.screen2_en", "0");
	}
	if(screen3_en){
		guci2_set(ctx, "mcu.global.screen3_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.screen3_en", "0");
	}
	if(screen4_en){
		guci2_set(ctx, "mcu.global.screen4_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.screen4_en", "0");
	}
	if(screen5_en){
		guci2_set(ctx, "mcu.global.screen5_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.screen5_en", "0");
	}
	if(custom_en){
		guci2_set(ctx, "mcu.global.custom_en", "1");
	}
	else
	{
		guci2_set(ctx, "mcu.global.custom_en", "0");
	}
	if(content){
		guci2_set(ctx, "mcu.global.content", content);
	}
	else
	{
		guci2_set(ctx, "mcu.global.custom_en", "");
	}
	guci2_commit(ctx, "mcu");
	guci2_free(ctx);
	execCommand("/etc/init.d/e750_mcu reload");
	return 0;
}

/** The implementation of the GetAPIFunctions function **/
#include <gl/glapibase.h>

static api_info_t gl_lstCgiApiFuctionInfo[] = {
	//you can simply add or remove entities from here
	map("/mcu/get", "get", mcu_get_data),
	map("/mcu/get_config", "get", mcu_get_config),
	map("/mcu/set_config", "post", mcu_set_config),
};

api_info_t *get_api_entity(int *pLen)
{
	(*pLen) = sizeof(gl_lstCgiApiFuctionInfo) / sizeof(gl_lstCgiApiFuctionInfo[0]);
	return gl_lstCgiApiFuctionInfo;
}
