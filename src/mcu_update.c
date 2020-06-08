/*************************************************************************
	> File Name: mcu_update.c
	> Author:lincoln 
	> Mail: 1720013893@qq.com
	> Created Time: 2019年07月22日 星期一 10时38分19秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdlib.h>
#include <gl/shell.h>
#include "uart.h"
#include <signal.h>
#define DEV_NAME  "/dev/ttyS0"
unsigned char tx_buf[512] = {0};
unsigned char tx_buf_tmp[512] = {0};
unsigned char rx_buf[40]={0};

static void printf_help()
{
	printf("Usage:\n");
	printf("\tmcu_update file.bin\t--update mcu firmware\n");
	printf("\tmcu_update file.bin -r\t--Try again after update failed\n");
}


int  main(int argc, char*  argv[])
{
    int i = 0x10000;
    char up_cont = 0;
    char completion = 0;
    char completion_temp = 0;
	
    int AP_count = 0;
    int read_count = 0;
    char txflag = 1;
    FILE *fp = NULL;	
    FILE *fd_pk = NULL;	
	int32_t fd = -1;
/*    
    sigset_t t;
    sigemptyset(&t);//定义信号集对象，并清空初始化
    sigaddset(&t,SIGINT);//SIGINT即为ctrl+c，是2号进程
    sigprocmask(SIG_BLOCK,&t,NULL);//设置阻塞信号集，阻塞SIGINT信号
*/
    if((argc==2)&&!(strcmp("-h",argv[1]))){
		printf_help();
        return 0;
    }

    fp=fopen(argv[1],"rb"); 
    if(NULL==fp){ 
        printf("open file err,please check file and try again\n");
        return -1; 
    }
    fclose(fp);

	execCommand("/etc/init.d/e750_mcu stop");
	if(check_process_running("e750_mcu")){
		printf("Please kill e750-mcu and try again\n");
		return -1;
	}
	printf("Start Upgrading\n");
    fd_pk = fopen("/proc/sys/kernel/printk", "w");

	if (fd_pk == NULL) 
	{
		printf("fd_pk is NULL and open file fail\n");
		return 0 ;
	}
	fwrite("1	4	1	7", 7, 1, fd_pk);
	fclose(fd_pk);
	
	if(argc==3){
			if(!strcmp(argv[2],"-r")){
				fd = uartOpen(DEV_NAME,115200,0,100);
				goto xuchuan;
			}      
    }

	while(i--){
			;
	}
	
    fd = uartOpen(DEV_NAME,115200,0,100);

    memset(rx_buf,'\0',sizeof(rx_buf));
    sprintf(rx_buf,"{ \"system\": %s","\"APUpdate\" }");

    uartTx(fd,strlen(rx_buf),rx_buf);

    memset(rx_buf,'\0',sizeof(rx_buf));
    read_count =uartRx(fd,4,rx_buf);

    if(strcmp("{OK}",rx_buf)){
        printf("Out AProm err1, please reset\n");
	    execCommand("/etc/init.d/e750_mcu start");
        return (-1);
    }
    else{
        printf("Out AProm succeeded\n");
    }

    memset(rx_buf,'\0',sizeof(rx_buf));
    read_count =uartRx(fd,4,rx_buf);

    if(strcmp("{OK}",rx_buf)){
        printf("Enter LDrom err\n");
        return 0;
    }
    else{
        printf("Enter LDrom succeeded\n");
    }

    xuchuan:
    up_cont = 0;
    completion = 0;
    completion_temp = 0;
    fp=fopen(argv[1],"rb"); 
    if(NULL==fp){ 
        printf("file open err,please check file & try again\n");
        return -1; 
    }

    fseek(fp,0,SEEK_END); 

    AP_count = ftell(fp);

    printf("file size : %d\n",AP_count);
    AP_count =((AP_count/512)+1)*512;
    memset(rx_buf,'\0',sizeof(rx_buf));
    sprintf(rx_buf,"{ \"AP_COUNT\": \"%d\" }",AP_count);

    uartTx(fd,strlen(rx_buf),rx_buf);
    memset(rx_buf,'\0',sizeof(rx_buf));
    read_count =uartRx(fd,4,rx_buf);

    if(strcmp("{OK}",rx_buf)){
        printf("size error: %s ,Please Try Again \n",rx_buf);
        fclose(fp);
        return (-1);
     }
    else{
        printf("size :%s\n",rx_buf);
    }

    fseek(fp,0L,SEEK_SET); 

    while(1){
        if(txflag){
            txflag = 0;
            memset(tx_buf,'\0',sizeof(tx_buf));
            read_count = fread(tx_buf,1, sizeof(tx_buf),fp);

            if(512==read_count){
                uartTx(fd,512,tx_buf);
                up_cont++;
                if(up_cont==(((AP_count/512+1))/10+1)){
                    up_cont = 0;

                    completion++;
                    printf("completion : %d%\%\n",completion*10);
                }
                memset(tx_buf_tmp,'\0',sizeof(tx_buf_tmp));

                read_count =uartRx(fd,512,tx_buf_tmp);

                for(i=0;i<512;i++){
                    if(tx_buf[i]!=tx_buf_tmp[i]){

                        printf("data err0: %d ,Begin Retransmission \n",i);
                        memset(rx_buf,'\0',sizeof(rx_buf));
                        sprintf(rx_buf,"{RETX}");
                        i = strlen(rx_buf);
                        for(i;i<512;i++);{
                            rx_buf[i] = 255;
                        }
                        uartTx(fd,512,rx_buf);
                        fclose(fp);
                        return (-1);
                    }
                }
                memset(rx_buf,'\0',sizeof(rx_buf));
                read_count =uartRx(fd,4,rx_buf);

                if(!strcmp("{OK}",rx_buf)){
                    txflag=1;
                }
                else{
                    printf("rx err0: %s ,Begin Retransmission \n",rx_buf);
                    fclose(fp);
                    goto xuchuan;
                    return (-1);
                }
            } 

            else{
                for(i=read_count;i<512;i++){
                    tx_buf[i] = 255;
                }

                uartTx(fd,512,tx_buf);
                completion ++;

                printf("completion : %d%\%\n",completion*10);
                memset(tx_buf_tmp,'\0',sizeof(tx_buf_tmp));

                read_count =uartRx(fd,512,tx_buf_tmp);

                for(i=0;i<512;i++){
                    if(tx_buf[i]!=tx_buf_tmp[i]){

                        printf("data err1: %d ,Begin Retransmission \n",i);
                        memset(rx_buf,'\0',sizeof(rx_buf));
                        sprintf(rx_buf,"{ \"RETX\": %s","\"1\" }");
                        uartTx(fd,strlen(rx_buf),rx_buf);
                        fclose(fp);
                        return (-1);
                    }
                }
                memset(rx_buf,'\0',sizeof(rx_buf));
                read_count =uartRx(fd,4,rx_buf);
                if(!strcmp("{OK}",rx_buf)){    
                    break;
                }
                else{
                    printf("rx err1: %s ,Begin Retransmission \n",rx_buf);
                    fclose(fp);
                    return (-1);
                }

            }

        }
    }
    uartClose(fd);
    fclose(fp); 	
    fd_pk = fopen("/proc/sys/kernel/printk", "w");
	if (fd_pk == NULL) 
	{
		printf("fd_pk is NULL and open file fail\n");
		return 0 ;
	}
	fwrite("7	4	1	7", 7, 1, fd_pk);
	fclose(fd_pk);

    printf("MCU update completedn\n");
	return 0;
}
