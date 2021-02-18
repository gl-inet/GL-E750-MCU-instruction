LDFLAGS += libglmodem-util.a libglsdk.a -L/usr/lib/ -ljson-c -luci -lubox -lubus -ldl -lpthread  -L/usr/lib/lua/ -liwinfo -luuid  -lblobmsg_json libglmodem-util.a -lcurl -I include

all : e750-mcu mcu_update libraries

e750-mcu :
	$(CC) $(CFLAGS)  app.c uart_posix.c -o $@ $(LDFLAGS) 

mcu_update :
	$(CC) $(CFLAGS)  mcu_update.c uart_posix.c -o $@ $(LDFLAGS)
	
libraries:
	$(CC) $(CFLAGS) -Wall api.c -shared -fPIC -o libglmcu.so $(LDFLAGS)

clean:
	rm -fr $(PROC) e750-mcu mcu_update
