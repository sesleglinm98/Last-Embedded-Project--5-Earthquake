/*
 *  ======== httpget.c ========
 *  HTTP Client GET example application
 */
#include <xdc/std.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* TI-RTOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>
#include <ti/net/http/httpcli.h>

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Idle.h>
#include <ti/sysbios/knl/Intrinsics.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Swi.h>

#include <ti/drivers/I2C.h>
/* Example/Board Header file */
#include "Board.h"

#include <sys/socket.h>

extern Semaphore_Handle semaphore0;
//extern Semaphore_Handle semaphore1;
extern Event_Handle event0;

#define TASKSTACKSIZE   2048

#define HOSTNAME          "turkiyedepremapi.herokuapp.com"
#define REQUEST_URI       "/api?sehir=(ESKISEHIR)"
#define USER_AGENT        "HTTPCli (ARM; TI-RTOS)"
#define SOCKETTEST_IP     "192.168.137.1"
#define HTTPTASKSTACKSIZE 4096

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

char deprem[20];

uint8_t txBuffer[2];
 uint8_t rxBuffer[2];
 uint8_t tx2Buffer[2];
 uint8_t rx2Buffer[2];
 uint8_t rx3Buffer[2];
 uint8_t rx4Buffer[2];
 uint8_t tx3Buffer[2];
 uint8_t tx4Buffer[2];
 uint8_t writeBuffer[2];
 uint8_t readBuffer[2];
 uint8_t t1;
 uint8_t t2;
 uint8_t t3;
 uint8_t t4;
 uint16_t x;
 uint16_t y;

 char data[64];
     int ret;
     int len;
     struct sockaddr_in addr;
     char *ptr3,*ptr4;
     int received = 0;

/*
 *  ======== printError ========
 */
Void taskFxn(UArg arg1, UArg arg2)
{

    //uint16_t toplam;
   // uint16_t toplam2;
   // float son;



    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Transaction i2cTransaction;
    int selo= 16384;
    float selo2 = 9.81;

    const float gMultiplier = (float)(selo2/selo);    //MPU9150_ACCEL_CONFIG_2G_RANGE;


                   I2C_Params_init(&i2cParams);
                   i2cParams.bitRate = I2C_400kHz;
                   i2c = I2C_open(Board_I2C_TMP, &i2cParams);
                   if(i2c == NULL){
                   System_printf("error initializing ý2c\n");
                             }
                   else{
                 System_printf("ý2c initialized!\n");
                            }

                   writeBuffer[0] = 0x6B;    //MPU9150_PWR_MGMT_1;
                   writeBuffer[1] = 0b10000000;       //MPU9150_PWR_MGMT_1_DEVICE_RESET;
                   i2cTransaction.slaveAddress = 0x68;
                   i2cTransaction.writeBuf = writeBuffer;
                   i2cTransaction.writeCount = 2;
                   i2cTransaction.readBuf = readBuffer;
                   i2cTransaction.readCount = 0;
                      if (!I2C_transfer(i2c, &i2cTransaction)) {
                       System_printf("putting sensor into reset mode failed");
                    }

                   /* loop to verify device has finished resetting and is back into sleep mode */
                       writeBuffer[0] = 0x6B;     //MPU9150_PWR_MGMT_1;
                       i2cTransaction.slaveAddress = 0x68;
                       i2cTransaction.writeBuf = writeBuffer;
                       i2cTransaction.writeCount = 1;
                       i2cTransaction.readBuf = readBuffer;
                       i2cTransaction.readCount = 1;
                       bool transferOK = true;
                       do {
                           transferOK = I2C_transfer(i2c, &i2cTransaction);
                       } while ((readBuffer[0] !=0b01000000) || (!transferOK));  // MPU9150_PWR_MGMT_1_SLEEP
                       if (!transferOK) {
                           System_printf("enabling clock synchronized with gyro X failed");
                       }

                       /* remove reset and enable the clock locked on gyro X */
                          writeBuffer[0] = 0x6B;    //MPU9150_PWR_MGMT_1;
                           writeBuffer[1] = 0b00000001;  //MPU9150_PWR_MGMT_1_CLKSEL_XG;
                           i2cTransaction.slaveAddress = 0x68;
                           i2cTransaction.writeBuf = writeBuffer;
                           i2cTransaction.writeCount = 2;
                           i2cTransaction.readBuf = readBuffer;
                           i2cTransaction.readCount = 0;
                           if (!I2C_transfer(i2c, &i2cTransaction)) {
                               System_printf("enabling clock synchronized with gyro X failed");
                           }


                               writeBuffer[0] = 0x6A;    //MPU9150_REG_USER_CTRL;
                               writeBuffer[1] = 0x00; //MPU9150_USER_CTRL_MASTER_DISABLE;
                               i2cTransaction.slaveAddress = 0x68;
                               i2cTransaction.writeBuf = writeBuffer;
                               i2cTransaction.writeCount = 2;
                               i2cTransaction.readBuf = readBuffer;
                               i2cTransaction.readCount = 0;
                               if (!I2C_transfer(i2c, &i2cTransaction)) {
                                   System_printf("disabling i2c master mode failed");
                               }

                               writeBuffer[0] = 0x6A;    //MPU9150_REG_USER_CTRL;
                               writeBuffer[1] = 0x20; //MPU9150_USER_CTRL_MASTER_EN;
                               i2cTransaction.slaveAddress = 0x68;
                               i2cTransaction.writeBuf = writeBuffer;
                               i2cTransaction.writeCount = 2;
                               i2cTransaction.readBuf = readBuffer;
                               i2cTransaction.readCount = 0;
                               if (!I2C_transfer(i2c, &i2cTransaction)) {
                               System_printf("disabling i2c master mode failed");
                                                              }

                               /* enable low pass filter */
                                  writeBuffer[0] = 0x1A; // MPU9150_REG_CONFIG;
                                   writeBuffer[1] = 0b00000100; //MPU9150_CONFIG_DLPF_21HZ;
                                   i2cTransaction.slaveAddress = 0x68;
                                   i2cTransaction.writeBuf = writeBuffer;
                                   i2cTransaction.writeCount = 2;
                                   i2cTransaction.readBuf = readBuffer;
                                   i2cTransaction.readCount = 0;
                                   if (!I2C_transfer(i2c, &i2cTransaction)) {
                                       System_printf("setting DLPF failed");
                                   }

                                   /* for 50 HZ: 1000/20 -> 1000 / 1 + 19 -> SMPLRT_DIV = 19 */
                                       writeBuffer[0] = 0x19; // MPU9150_SMPLRT_DIV;
                                       writeBuffer[1] = 19;
                                       i2cTransaction.slaveAddress = 0x68;
                                       i2cTransaction.writeBuf = writeBuffer;
                                       i2cTransaction.writeCount = 2;
                                       i2cTransaction.readBuf = readBuffer;
                                       i2cTransaction.readCount = 0;
                                       if (!I2C_transfer(i2c, &i2cTransaction)) {
                                           System_printf("enabling sample rate failed");
                                       }

                                       /* set accel sensitivity to 2g */
                                           writeBuffer[0] = 0x1C; //MPU9150_REG_ACCEL_CONFIG;
                                           writeBuffer[1] = 0b00000000; //MPU9150_ACCEL_CONFIG_AFS_SEL_2G;
                                           i2cTransaction.slaveAddress = 0x68;
                                           i2cTransaction.writeBuf = writeBuffer;
                                           i2cTransaction.writeCount = 2;
                                           i2cTransaction.readBuf = readBuffer;
                                           i2cTransaction.readCount = 0;
                                           if (!I2C_transfer(i2c, &i2cTransaction)) {
                                               System_printf("setting accel sensor range failed");
                                           }
                                           System_flush();

        while (1) {
            txBuffer[0] = 0x3c;
            i2cTransaction.slaveAddress = 0x68; // sensor id
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 1;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 1;


            if(I2C_transfer(i2c, &i2cTransaction)){
                        t1 = rxBuffer[0];
                     //   System_printf(" low x :%d ",t1);
                      //  System_flush();
                        }
                        else {
                              System_printf("I2C Bus fault\n");
                                }
                             System_flush();

                        tx2Buffer[0] = 0x3b;
                        i2cTransaction.slaveAddress = 0x68; // sensor id
                        i2cTransaction.writeBuf = tx2Buffer;
                        i2cTransaction.writeCount = 1;
                        i2cTransaction.readBuf = rx2Buffer;
                        i2cTransaction.readCount = 1;

            if(I2C_transfer(i2c, &i2cTransaction)){
            t2 = rx2Buffer[0];
           // System_printf("  high x :%d ", t2);
           // System_flush();
            }
            else {
                  System_printf("I2C Bus fault\n");
                    }

                 System_flush();

              tx3Buffer[0] = 0x3d;
              i2cTransaction.slaveAddress = 0x68; // sensor id
              i2cTransaction.writeBuf = tx3Buffer;
              i2cTransaction.writeCount = 1;
              i2cTransaction.readBuf = rx3Buffer;
              i2cTransaction.readCount = 1;

             if(I2C_transfer(i2c, &i2cTransaction)){
            t3 = rx3Buffer[0];
            //System_printf(" high y :%d ", t3);
           // System_flush();
            }
            else {
                  System_printf("I2C Bus fault\n");
            }

                tx4Buffer[0] = 0x3e;
                i2cTransaction.slaveAddress = 0x68; // sensor id
                i2cTransaction.writeBuf = tx4Buffer;
                i2cTransaction.writeCount = 1;
                i2cTransaction.readBuf = rx4Buffer;
                i2cTransaction.readCount = 1;

            if(I2C_transfer(i2c, &i2cTransaction)){
            t4 = rx4Buffer[0];
           // System_printf(" low y :%d ", t4);
          //  System_flush();
            }
            else {
                  System_printf("I2C Bus fault\n");
                    }

                 x = rx2Buffer[0] << 8 | rxBuffer[0];
                 x = x * gMultiplier;
                 y = rx3Buffer[0] << 8 | rx4Buffer[0];
                 y = y * gMultiplier;
                 System_printf(" sonuc x: %d ve y: %d \n",x , y);
                 System_flush();
                  if(x >40 ||  x < 35 || (y > 0 && y <39)){

                    Semaphore_post(semaphore0);
                    Task_sleep(2000);
                        }
                 Task_sleep(500);
        }
             I2C_close(i2c);
    }

void printError(char *errString, int code)
{
    System_printf("Error! code = %d, desc = %s\n", code, errString);
    BIOS_exit(code);
}

/*
 *  ======== httpTask ========
 *  Makes a HTTP GET request
 */
Void httpTask(UArg arg0, UArg arg1)
{
    bool moreFlag = false;

    HTTPCli_Struct cli;
    HTTPCli_Field fields[3] = {
        { HTTPStd_FIELD_NAME_HOST, HOSTNAME },
        { HTTPStd_FIELD_NAME_USER_AGENT, USER_AGENT },
        { NULL, NULL }
    };

    while(1){
        received = 0;
        Semaphore_pend(semaphore0, BIOS_WAIT_FOREVER);

    System_printf("Sending a HTTP GET request to '%s'\n", HOSTNAME);
    System_flush();

    HTTPCli_construct(&cli);

    HTTPCli_setRequestFields(&cli, fields);

    ret = HTTPCli_initSockAddr((struct sockaddr *)&addr, HOSTNAME, 0);
    if (ret < 0) {
        printError("httpTask: address resolution failed", ret);
    }

    ret = HTTPCli_connect(&cli, (struct sockaddr *)&addr, 0, NULL);
    if (ret < 0) {
        printError("httpTask: connect failed", ret);
    }

    ret = HTTPCli_sendRequest(&cli, HTTPStd_GET, REQUEST_URI, false);
    if (ret < 0) {
        printError("httpTask: send failed", ret);
    }

    ret = HTTPCli_getResponseStatus(&cli);
    if (ret != HTTPStd_OK) {
        printError("httpTask: cannot get status", ret);
    }

    System_printf("HTTP Response Status Code: %d\n", ret);

    ret = HTTPCli_getResponseField(&cli, data, sizeof(data), &moreFlag);
    if (ret != HTTPCli_FIELD_ID_END) {
        printError("httpTask: response field processing failed", ret);
    }

    len = 0;

        do {
                ret = HTTPCli_readResponseBody(&cli, data, sizeof(data), &moreFlag);

                if (ret < 0) {
                    printError("httpTask: response body processing failed", ret);
                }

                ptr3 = strstr(data,"tarih");
                   if(ptr3){
                     if(received) continue;

                     ptr4 = strstr(ptr3,",");
                     *ptr4 = 0;
                     System_printf("tarih: %s\n", ptr3+7);
                     System_flush();
                     strcpy(deprem, ptr3+7);
                     received++;
                        }

                   len += ret;
            } while (moreFlag);

    System_printf("Recieved %d bytes of payload\n", len);
    System_flush();

    Event_post(event0, Event_Id_00);

    HTTPCli_disconnect(&cli);
    HTTPCli_destruct(&cli);
}
}


Void socketTask(UArg arg0, UArg arg1)
{
    while(1) {

        Event_pend(event0, Event_Id_00, Event_Id_NONE, BIOS_WAIT_FOREVER);

        sendData2Server(SOCKETTEST_IP, 5011, deprem, strlen(deprem));
    }
}

void sendData2Server(char *serverIP, int serverPort, char *data, int size)
{
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        System_printf("Socket not created");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));  /* clear serverAddr structure */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);     /* convert port # to network order */
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));

    int connStat = connect(sockfd, (struct sockaddr *)&serverAddr, /* connecting….*/
                  sizeof(serverAddr));
    if(connStat < 0) {
        System_printf("Error while connecting to server\n");
        if (sockfd > 0)
            close(sockfd);
    }

    int numSend = send(sockfd, data, size, 0);       /* send data to the server*/
    if(numSend < 0) {
        System_printf("Error while sending data to server\n");
        if (sockfd > 0) close(sockfd);
    }

    if (sockfd > 0) close(sockfd);
}


/*
 *  ======== netIPAddrHook ========
 *  This function is called when IP Addr is added/deleted
 */
void netIPAddrHook(unsigned int IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
       static Task_Handle taskHandle1, taskHandle2;
       Task_Params taskParams;
       Error_Block eb;

       // Create a HTTP task when the IP address is added
       if (fAdd && !taskHandle1 && !taskHandle2) {
          Error_init(&eb);

       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle1 = Task_create((Task_FuncPtr)httpTask, &taskParams, &eb);

       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle2 = Task_create((Task_FuncPtr)socketTask, &taskParams, &eb);

       if (taskHandle1 == NULL || taskHandle2 == NULL) {
           printError("netIPAddrHook: Failed to create HTTP and Socket Tasks\n", -1);
       }
   }
}

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initEMAC();
    Board_initI2C();

    /* Turn on user LED */
    GPIO_write(Board_LED0, Board_LED_ON);

    System_printf("Starting the HTTP GET example\nSystem provider is set to "
            "SysMin. Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
