//
//  set_voltage_new.c
//  
//
//  Created by jstefanop on 1/25/18.
//

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define PIC_COMMAND_1                       0x55
#define PIC_COMMAND_2                       0xaa
#define GET_VOLTAGE                         0x18
#define SET_VOLTAGE                         0x10
#define JUMP_FROM_LOADER_TO_APP             0x06
#define RESET_PIC                           0x07
#define READ_PIC_SOFTWARE_VERSION           0x17
static unsigned char Pic_command_1[1] = {PIC_COMMAND_1};
static unsigned char Pic_command_2[1] = {PIC_COMMAND_2};
static unsigned char Pic_set_voltage[1] = {SET_VOLTAGE};
static unsigned char Pic_get_voltage[1] = {GET_VOLTAGE};
static unsigned char Pic_read_pic_software_version[1] = {READ_PIC_SOFTWARE_VERSION};
static unsigned char Pic_jump_from_loader_to_app[1] = {JUMP_FROM_LOADER_TO_APP};
static unsigned char Pic_reset[1] = {RESET_PIC};
pthread_mutex_t iic_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;



void pic_send_command(int fd)
{
    //printf("--- %s\n", __FUNCTION__);
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_command_1, 1);
    write(fd, Pic_command_2, 1);
    pthread_mutex_unlock(&i2c_mutex);
}

void pic_read_pic_software_version(unsigned char *version, int fd)
{
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_read_pic_software_version, 1);
    read(fd, version, 1);
    pthread_mutex_unlock(&i2c_mutex);
}

void pic_read_voltage(unsigned char *voltage, int fd)
{
    
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_get_voltage, 1);
    read(fd, voltage, 1);
    pthread_mutex_unlock(&i2c_mutex);
    
    usleep(500000);
}

void pic_set_voltage(unsigned char *voltage, int fd)
{
    
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_set_voltage, 1);
    write(fd, voltage, 1);
    pthread_mutex_unlock(&i2c_mutex);
    
    usleep(500000);
}



void pic_jump_from_loader_to_app(int fd)
{
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_jump_from_loader_to_app, 1);
    pthread_mutex_unlock(&i2c_mutex);
    usleep(500000);
}

void pic_reset(int fd)
{
    pic_send_command(fd);
    
    printf("\n--- %s\n", __FUNCTION__);
    pthread_mutex_lock(&i2c_mutex);
    write(fd, Pic_reset, 1);
    pthread_mutex_unlock(&i2c_mutex);
    usleep(600*1000);
}


void main (int argc, char *argv[]){
    
    if (argc != 3) {
        printf("Incorrect arguments\n");
        printf("Usage:\n");
        printf("./set_voltage [chain# 1-4] [voltage in hex]\n");
        exit(1);
    }
    
    int chain = atoi(argv[1]);
    unsigned char set_voltage = strtol(argv[2], NULL, 16);
    
    if(chain > 4 || chain == 0){
        printf("Invalid chain #, valid range 1-4\n");
        exit(1);
    }
    if(strtol(argv[2], NULL, 16) > 0xfe){
        printf("Invalid hex voltage, valid range 0x00-0xfe\n");
        exit(1);
    }
    
    int fd;
    char filename[40];
    unsigned char version = 0;
    unsigned char voltage = 0;
    int const i2c_slave_addr[4] = {0xa0,0xa2,0xa4,0xa6};
    
    chain--;
    
    sprintf(filename,"/dev/i2c-0");
    
    if ((fd = open(filename,O_RDWR)) < 0) {
        printf("Failed to open the bus\n");
        exit(1);
    }
    
    pthread_mutex_lock(&iic_mutex);
    if (ioctl(fd,I2C_SLAVE,i2c_slave_addr[chain] >> 1 )) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(1);
    }
   // pic_reset(fd);
   // pic_jump_from_loader_to_app(fd);
    pic_read_pic_software_version(&version, fd);
    printf("\n version = 0x%02x\n", version);
    
    if(version != 0x03){
        printf("Wrong PIC version\n");
        exit(1);
    }
    
    printf("reading voltage\n");
    pic_read_voltage(&voltage, fd);
    
    printf("\n voltage = 0x%02x\n", voltage);
    
    printf("setting voltage\n");
    pic_set_voltage(&set_voltage, fd);
    
    printf("reading voltage\n");
    pic_read_voltage(&voltage, fd);
    printf("\n voltage = 0x%02x\n", voltage);
    
   // pic_reset(fd);
    pthread_mutex_unlock(&iic_mutex);
    
    if(voltage != set_voltage)
        printf("ERROR: Voltage was not successfully set\n");
    else
        printf("Success: Voltage updated!\n");
    
}



