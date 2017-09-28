#include <stdio.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef HAVE_ATTR_PACKED
#define ATTR_PACKED __attribute__((packed))
#else
#define ATTR_PACKED
#endif

#define BUFFER_LENGTH (4+512)

// Maximum allowed number of SMART Attributes
#define NUMBER_ATA_SMART_ATTRIBUTES     30

/* ata_smart_attribute is the vendor specific in SFF-8035 spec */ 
#pragma pack(1)
struct ata_smart_attribute {
  unsigned char id;
  // meaning of flag bits: see MACROS just below
  // WARNING: MISALIGNED!
  unsigned short flags; 
  unsigned char current;
  unsigned char worst;
  unsigned char raw[6];
  unsigned char reserv;
} ATTR_PACKED;
#pragma pack()

// Format of data returned by SMART READ DATA
// Table 62 of T13/1699-D (ATA8-ACS) Revision 6a, September 2008
#pragma pack(1)
struct ata_smart_values {
  //unsigned short int revnumber;
  unsigned short revnumber1;
  unsigned short revnumber2;
  unsigned short revnumber3;
  struct ata_smart_attribute vendor_attributes [NUMBER_ATA_SMART_ATTRIBUTES];
  unsigned char offline_data_collection_status;
  unsigned char self_test_exec_status;  //IBM # segments for offline collection
  unsigned short int total_time_to_complete_off_line; // IBM different
  unsigned char vendor_specific_366; // Maxtor & IBM curent segment pointer
  unsigned char offline_data_collection_capability;
  unsigned short int smart_capability;
  unsigned char errorlog_capability;
  unsigned char vendor_specific_371;  // Maxtor, IBM: self-test failure checkpoint see below!
  unsigned char short_test_completion_time;
  unsigned char extend_test_completion_time_b; // If 0xff, use 16-bit value below
  unsigned char conveyance_test_completion_time;
  unsigned short extend_test_completion_time_w; // e04130r2, added to T13/1699-D Revision 1c, April 2005
  unsigned char reserved_377_385[9];
  unsigned char vendor_specific_386_510[125]; // Maxtor bytes 508-509 Attribute/Threshold Revision #
  unsigned char chksum;
} ATTR_PACKED;
#pragma pack()

struct HddInfo {
    char SerialNum[21];
    char FWVersion[9];
    char ModelName[41];
    unsigned int NumMaxLba; /** Total Number of user addressable sectors **/
};


// swap two bytes.  Point to low address
void swap2(char *location){
  char tmp=*location;
  *location=*(location+1);
  *(location+1)=tmp;
  return;
}

// swap four bytes.  Point to low address
void swap4(char *location){
  char tmp=*location;
  *location=*(location+3);
  *(location+3)=tmp;
  swap2(location+1);
  return;
}

// swap eight bytes.  Points to low address
void swap8(char *location){
  char tmp=*location;
  *location=*(location+7);
  *(location+7)=tmp;
  tmp=*(location+1);
  *(location+1)=*(location+6);
  *(location+6)=tmp;
  swap4(location+2);
  return;
}

#define swap16(x) (((x) & 0xFF00) >> 8 | \
		((x) & 0x00FF) << 8)

// Typesafe variants using overloading
inline void swapx(unsigned short * p)
  { swap2((char*)p); }
inline void swapx(unsigned int * p)
  { swap4((char*)p); }
inline void swapx(uint64_t * p)
  { swap8((char*)p); }

#define SMART_CMD (0x031F)

#define ATA_SMART_CMD                   0xb0
#define ATA_SMART_READ_VALUES           0xd0
#define ATA_SMART_ENABLE           		0xd8
#define ATA_SMART_DISABLE          		0xd9
#define ATA_SMART_READ_THRESHOLDS       0xd1

static struct HddInfo sHddInfo;

int ReadIdentify(unsigned char *buf, unsigned int buf_size);
unsigned short PrintDeviceIdentifer(unsigned char *buf);

int isSmartFeatureSetEanble(unsigned short *isEnabled)
{
	int fd;
	int ret = 0;
    unsigned char buf[BUFFER_LENGTH];
	struct ata_smart_values *data;

	// Read Identifier including SMART feature set enable/disable
	fd=open("/dev/sda", O_RDONLY|O_NONBLOCK);
    if(fd < 0) {
        perror("open");
    }

	ret =  ReadIdentify(buf, BUFFER_LENGTH);
	if ( ret != 0 ) {
    	printf("ERROR ataReadIdentify. ret = %d\n", ret);
		close(fd);
		return ret;
    }

	close(fd);

	*isEnabled = PrintDeviceIdentifer(buf);

	return ret;
}

int enableSmartFeatureSet(unsigned char *buf)
{
	int fd;

	// make SmartFeatureSet enable
	buf[0] = ATA_SMART_CMD;
	buf[1] = buf[3] = 1;
	buf[2] = ATA_SMART_ENABLE; 
	buf[4] = 0x4f;
	buf[5] = 0xc2;

	// Set SMART feature enable/disable
	fd=open("/dev/sda", O_RDONLY|O_NONBLOCK);
	if(fd < 0) {
		perror("open");
	}

	int ret = ioctl(fd,SMART_CMD,buf);
	if( ret != 0 )
	{
		printf("[%d]%s ioctl error!!!. ret = %d\n",__LINE__,__func__, ret);
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}

void hex_print(const uint8_t *block, unsigned int len, unsigned int number_space_separators)
{
    int i;
#if 0
    int current_idx;
    const uint8_t *buf;
    unsigned short var = 0;

    int count=0;
    for( current_idx=0; current_idx<len; current_idx+=12 )
    {
        buf = block+current_idx;

        var = buf[0];
        printf("%0d\t%0d\t", count, (var & 0x00FF));

        var = ntohs((buf[1] << 8) | buf[2]);
        printf("%x\t", var);

        var = buf[3];
        printf("%d\t", var & 0x00FF);

        var = buf[4];
        printf("%d\t", var & 0x00FF);

        var = ntohs((buf[5] << 8) | buf[6]);
        printf("%d\n", var);

        count ++;
    }
#else
    // print by hex
    for (i=0; i<len; i++)
    {
        if(i%12 == 0 && i != 0 )
           printf("\n");

        printf("%02X", block[i]);

        if (number_space_separators) printf("%*c", number_space_separators, ' ');
    }
    printf("\n");
#endif
}

unsigned short PrintDeviceIdentifer(unsigned char *buf)
{
    int i, size;
    bool bWWNSupport=true;
    unsigned oui;

    unsigned short *pbuf = (unsigned short *)buf; 

    // World wide name

    for(i=0;i < ((512)/2);i++)
       pbuf[i] = swap16(pbuf[i+2]); 

    sHddInfo.SerialNum[21] = 0;
    sHddInfo.FWVersion[9] = 0;
    sHddInfo.ModelName[41] = 0;
	unsigned short SmartFeatureSet;
	SmartFeatureSet = (buf[171] & 0x0001);
	
    memcpy(sHddInfo.SerialNum,&buf[20],20);
    memcpy(sHddInfo.FWVersion,&buf[46],8);
    memcpy(sHddInfo.ModelName,&buf[54],40);
    sHddInfo.NumMaxLba = (swap16(pbuf[101]) << 16) | swap16(pbuf[100]);

    if ((swap16(pbuf[87]) & 0xc100) != 0x4100)
        bWWNSupport = false; // word not valid or WWN support bit 8 not set

    printf("============================================================\n");
    printf(" Hdd info Model Name : %s\n",sHddInfo.ModelName);
    printf(" Hdd info Serianl Num : %s\n",sHddInfo.SerialNum);
    printf(" Hdd info F/W version: %s\n",sHddInfo.FWVersion);
    if( bWWNSupport )
    {
        oui = ((swap16(pbuf[108]) & 0x0fff) << 12) | (swap16(pbuf[109]) >> 4);
        printf(" Hdd info OUI : 0x%03X\n", oui);
    }
    else
    {
        printf("87 = %02X\n", pbuf[87]);
        printf("WWN is not supproted\n");
    }
    //printf(" Hdd info Max LBA : %d\n",((sHddInfo.NumMaxLba/(1000*1000)*512)));
    //size = (sHddInfo.NumMaxLba/(1000*1000))*512;
    //printf(" Hdd info size : %d\n",size/1000);
    printf("============================================================\n\n");

/*
    It was verified with smartctl
    "smartctl -i /dev/sda"
    Model Family:     Seagate Momentus SpinPoint M8 (AF)
    Device Model:     ST1000LM024 HN-M101MBB
    Serial Number:    S360J9GH501989
    LU WWN Device Id: 5 0004cf 21219f3e1
    Firmware Version: 2BA30004
*/
	return SmartFeatureSet;
}

// Read IDENTIFY DEVICE
int ReadIdentify(unsigned char *buf, unsigned int buf_size)
{
    int fd;

    memset(buf, 0x00, 516);
    buf[0] = 0xec;
    buf[3] = 1;

    fd=open("/dev/sda", O_RDONLY|O_NONBLOCK);
    if(fd < 0) {
        perror("open");
        return -1;
    }

    if(ioctl(fd,SMART_CMD,buf)!= 0)
    {
        perror("identify device");
        return -1;
    }

    close(fd);

    return 0;
}

// Reads SMART attributes into *data
int ataReadSmartValues(struct ata_smart_values *data)
{  
    int fd; 
    int i;

    unsigned char buf[BUFFER_LENGTH];
    buf[0] = ATA_SMART_CMD;
    buf[1] = buf[3] = 1;
    buf[2] = ATA_SMART_READ_VALUES;

    fd=open("/dev/sda", O_RDONLY|O_NONBLOCK);
    if(fd < 0) {
        perror("open");
    }
	
    if(ioctl(fd,SMART_CMD,&buf)!= 0)
    {
	printf("[%d]%s ioctl error!!! \n",__LINE__,__func__);
	close(fd);
        return -1;
    }

    memcpy(data, buf, BUFFER_LENGTH);
    #if 1
    // swap endian order if needed
    swap2((char *)&(data->revnumber1));
    swap2((char *)&(data->revnumber2));
    swap2((char *)&(data->revnumber3));
    swap2((char *)&(data->total_time_to_complete_off_line));
    swap2((char *)&(data->smart_capability));
    swapx(&data->extend_test_completion_time_w);
    #endif

    int _buf[12];
    struct ata_smart_attribute *x;
    for ( i=0; i<NUMBER_ATA_SMART_ATTRIBUTES ; i++)
    {
        x=data->vendor_attributes+i;
        memcpy(_buf, (data->vendor_attributes+i), 12);
        for( int j; j<12; j++ )
        {
            printf("%0x ", _buf[j]); 
        }   
        printf("\n");
    }
    

    for (i=0; i<NUMBER_ATA_SMART_ATTRIBUTES; i++){
      x=data->vendor_attributes+i;
      //swap2((char *)&(x->flags));
    }

    close(fd);
    return 0;
}

int main(void)
{  
    int fd; 
    int i;
	int ret;
	
    unsigned char buf[BUFFER_LENGTH];
	unsigned short isEnabled;
    ata_smart_values smartval; 

	// Check smart feature set
	if( isSmartFeatureSetEanble(&isEnabled) != 0 )
	{
		printf("[%d]%s error !!! \n", __LINE__,__func__);
		return 1;
	}

	if( !isEnabled )
	{
		printf("smart is not enabled\n");

		// Enable Smart Feature Set
		if( enableSmartFeatureSet(buf) != 0 )
		{
			printf("[%d]%s error !!! \n", __LINE__,__func__);
			return 1;
		}
		
		printf("Successfully enabled\n");
	}

	return 0;

	// Read Smart data
    if (ataReadSmartValues(&smartval)) {
       printf("ERROR ataReadSmartValues\n");
       return 1;
    }

    // print device identifier
    PrintDeviceIdentifer(buf);   

    // print smart attributes
    struct ata_smart_attribute *x;

    printf("ID#\tFLAG\tVALUE\tWORST\tRAW\n");
    for (i=0; i<NUMBER_ATA_SMART_ATTRIBUTES; i++){
       x=smartval.vendor_attributes+i;
       printf("%d\t0x%04X\t%d\t%d\t%d\n", x->id,x->flags, x->current,x->worst, ntohs((x->raw[0] << 8) | x->raw[1]));
    }

    // print smart attributes by hex
    printf("\n============================================================\n");
    printf("smart parameter raw data(12byte*30)\n");
    hex_print((uint8_t *)(&smartval)+6,NUMBER_ATA_SMART_ATTRIBUTES*12,1);
    printf("============================================================\n");
    printf("raw data(516 byte)\n");
    hex_print((uint8_t *)(&smartval),516,1);
    printf("============================================================\n");
	
    return 0;
}
