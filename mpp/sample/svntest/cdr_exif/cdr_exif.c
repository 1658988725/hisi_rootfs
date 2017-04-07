/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>

#include "cdr_gps_data_analyze.h"

#include <sys/vfs.h>


#define FILE_NAME "write-exif.jpg"

#define FILE_BYTE_ORDER EXIF_BYTE_ORDER_INTEL

//  0xff, 0xd8, 0xff, 0xe1,
static const unsigned char exif_header[] = {
	0xff, 0xe1,
};

/* length of data in exif_header */
static const unsigned int exif_header_len = sizeof(exif_header);


/* length of data in image_jpg */
static const unsigned int image_jpg_len ;//= sizeof(image_jpg);

/* start of JPEG image data section */
static const unsigned int image_data_offset = 20;
#define image_data_len (image_jpg_len - image_data_offset)

const char *pucMaker = "深圳市伊爱高新技术开发有限公司";
const char *pucModel = "咔咔行车记录仪";

/* Create a brand-new tag with a data field of the given length, in the
 * given IFD. This is needed when exif_entry_initialize() isn't able to create
 * this type of tag itself, or the default data length it creates isn't the
 * correct length.
 */
static ExifEntry *create_tag(ExifData *exif, ExifIfd ifd, ExifTag tag, size_t len)
{
	void *buf;
	ExifEntry *entry;
	
	/* Create a memory allocator to manage this ExifEntry */
	ExifMem *mem = exif_mem_new_default();
	assert(mem != NULL); /* catch an out of memory condition */

	/* Create a new ExifEntry using our allocator */
	entry = exif_entry_new_mem (mem);
	assert(entry != NULL);

	/* Allocate memory to use for holding the tag data */
	buf = exif_mem_alloc(mem, len);
	assert(buf != NULL);

	/* Fill in the entry */
	entry->data = buf;
	entry->size = len;
	entry->tag = tag;
	entry->components = len;
	entry->format = EXIF_FORMAT_UNDEFINED;

	/* Attach the ExifEntry to an IFD */
	exif_content_add_entry (exif->ifd[ifd], entry);

	/* The ExifMem and ExifEntry are now owned elsewhere */
	exif_mem_unref(mem);
	exif_entry_unref(entry);

	return entry;
}

//需要参考PowerExif.exe软件打开图片
//添加对应的只需要对应的代码ID,exifname、数据类型、和数据长度;
int cdr_write_exif_to_jpg(char *pSrcFileName, GPS_INFO sGpsInfo)
{	
	int rc = 1;
	char GPSversionid[5]={0x02,0x02,0x00,0x00,0x00};
	FILE *f = NULL;
    
	unsigned char *exif_data;
	unsigned int exif_data_len;

    FILE *f2 = fopen(pSrcFileName, "r");
	if (!f2) {
		fprintf(stderr, "Error creating file \n");
		return -1;
	}
    fseek (f2, 0, SEEK_END);   ///将文件指针移动文件结尾
    int iFileSize = ftell (f2); ///求出当前文件指针距离文件开始的字节数
    fseek (f2, 0, SEEK_SET); 
    
    char *buf = calloc(iFileSize,sizeof(char));
    if(buf == NULL){
       printf("calloc fails!\n");
       fclose(f2);
       return -1;
    }

    int i = 0;
    int pos = 0;
    char temp = 0;

    for(i=0; i<iFileSize-1; i++)  
    {  
        temp = fgetc(f2);  
        if(EOF == temp) break;  
        buf[pos++] = temp;  
    }  
    buf[pos] = 0; 
    fclose(f2);
    
	ExifEntry *entry = NULL;
    
	ExifData *exif = exif_data_new();
	if (!exif) {
		fprintf(stderr, "Out of memory\n");
         free(buf);
         buf=NULL;
		return 2;
	}
    
	/* Set the image options */
	//exif_data_set_option(exif, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
	//exif_data_set_data_type(exif, EXIF_DATA_TYPE_COMPRESSED);
	//exif_data_set_byte_order(exif, FILE_BYTE_ORDER);

	/* Create the mandatory EXIF fields with default data */
	exif_data_fix(exif);

	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_VERSION_ID,4);    
	/* Write the special header needed for a comment tag */    
	entry->format = EXIF_FORMAT_BYTE;
	entry->components = 4;
	exif_set_sshort(entry->data, FILE_BYTE_ORDER, GPSversionid[0]);
	exif_set_sshort(entry->data+1, FILE_BYTE_ORDER, GPSversionid[1]);
	exif_set_sshort(entry->data+2, FILE_BYTE_ORDER, GPSversionid[2]);
	exif_set_sshort(entry->data+3, FILE_BYTE_ORDER, GPSversionid[3]);

	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE_REF,2);
	entry->format = EXIF_FORMAT_ASCII;
	//entry->components = 2;
	memcpy(entry->data, "N", 2);
	ExifRational a;
	ExifRational b;
	ExifRational c;
	//ExifSRational d;
    
	//a.numerator = sGpsInfo.latitude;//纬度
	a.numerator = sGpsInfo.latitude_Degree;//纬度
	a.denominator = 1;
    b.numerator = sGpsInfo.latitude_Cent;
	b.denominator = 1;
    c.numerator = sGpsInfo.latitude_Second;
	c.denominator = 1;//100;
	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE,24);
	entry->format = EXIF_FORMAT_RATIONAL;
	entry->components = 3;
	exif_set_rational(entry->data,FILE_BYTE_ORDER,a);
	exif_set_rational(entry->data+8,FILE_BYTE_ORDER,b);
	exif_set_rational(entry->data+16,FILE_BYTE_ORDER,c);

	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE_REF,2);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = 2;
	memcpy(entry->data, "E", 2);

    a.numerator = sGpsInfo.longitude_Degree;//sGpsInfo.longitude;//经度
	a.denominator = 1;
	b.numerator = sGpsInfo.longitude_Cent;//55;
	b.denominator = 1;
	c.numerator = sGpsInfo.longitude_Second;//5512;
	c.denominator = 1;//100;  
	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE,24);
	entry->format = EXIF_FORMAT_RATIONAL;
	entry->components = 3;
	exif_set_rational(entry->data,FILE_BYTE_ORDER,a);
	exif_set_rational(entry->data+8,FILE_BYTE_ORDER,b);
	exif_set_rational(entry->data+16,FILE_BYTE_ORDER,c);
    	
#if(0)
    /*gps time*/
	entry = create_tag(exif, EXIF_IFD_GPS, 0x0007,24);
	entry->format = EXIF_FORMAT_RATIONAL;
	entry->components = 3;
	ExifRational x;
	ExifRational y;
	ExifRational z;
	x.denominator = 1;
    x.numerator = 12;
	y.denominator = 2;
	y.numerator = 3;
	exif_set_rational(entry->data,FILE_BYTE_ORDER,x);
	exif_set_rational(entry->data+8,FILE_BYTE_ORDER,y);
	exif_set_rational(entry->data+16,FILE_BYTE_ORDER,z);
#endif

#if(1)

	//GPS速度单位K KM/H
	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_SPEED_REF,2);
	entry->format = EXIF_FORMAT_ASCII;
	//entry->components = 1;
	memcpy(entry->data, "K", 2);	

	//GPS速度值.
	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_SPEED,8);
	entry->format = EXIF_FORMAT_RATIONAL;
	entry->components = 1;
	a.numerator = sGpsInfo.speed;
	a.denominator = 1;
	exif_set_rational(entry->data,FILE_BYTE_ORDER,a);	

#endif


	/*date time*/
	entry = create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL,20);
	entry->format = EXIF_FORMAT_ASCII;
	//entry->components = 20;
    char pDataTime[21] = {0};
    sprintf(pDataTime,"%04d-%02d-%02d %02d:%02d:%02d",sGpsInfo.D.year,sGpsInfo.D.month,sGpsInfo.D.day,
        sGpsInfo.D.hour,sGpsInfo.D.minute,sGpsInfo.D.second);
    memcpy(entry->data,pDataTime,20);	//memcpy(entry->data,"2012-12-11 10:00:00",19);

    /*EXIF_TAG_MAKER_NOTE*/
    //entry = create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_MAKER_NOTE,strlen(pucMakerNote)+1);

	//厂商信息和设备信息
    entry = create_tag(exif, EXIF_IFD_0, EXIF_TAG_MAKE,strlen(pucMaker)+1);
	entry->format = EXIF_FORMAT_ASCII;
    memcpy(entry->data,pucMaker,strlen(pucMaker));

    entry = create_tag(exif, EXIF_IFD_0, EXIF_TAG_MODEL,strlen(pucModel)+1);
	entry->format = EXIF_FORMAT_ASCII;
    memcpy(entry->data,pucModel,strlen(pucModel));
	

	//printf("exif_data_len=%d\n",exif_data_len);
	exif_data_save_data(exif, &exif_data, &exif_data_len);
	//printf("exif_data_len=%d\n",exif_data_len);
    
	assert(exif_data != NULL);    
    
	//f = fopen(FILE_NAME, "wb");//write exif to this file pSrcFileName
	f = fopen(pSrcFileName, "wb");//write exif to this file pSrcFileName
	//f = fopen("19700101095721.jpg", "wb");//write exif to this file pSrcFileName
	if (!f) {
		fprintf(stderr, "Error creating file %s\n", FILE_NAME);
		exif_data_unref(exif);
         free(buf);
         buf=NULL;
		return rc;
	}
	if (fwrite(buf, image_data_offset, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
		
	/* Write EXIF header */
	if (fwrite(exif_header, exif_header_len, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	/* Write EXIF block length in big-endian order */
	if (fputc((exif_data_len+2) >> 8, f) < 0) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	if (fputc((exif_data_len+2) & 0xff, f) < 0) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	/* Write EXIF data block */
	if (fwrite(exif_data, exif_data_len, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}

    //printf("exif_data:%s \nimage_data_offset %d exif_header_len %d\n",exif_data,image_data_offset,exif_header_len);
       
    int image_data_len2 =  iFileSize - image_data_offset;
	/* Write JPEG image data, skipping the non-EXIF header */
	if (fwrite(buf+image_data_offset, image_data_len2, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	printf("Wrote exif to jpg file\n");
	rc = 0;

    free(buf);
    buf = NULL;

	fflush(f);
errout:
	if (fclose(f)) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		rc = 1;
	}
	/* The allocator we're using for ExifData is the standard one, so use
	 * it directly to free this pointer.
	 */
	free(exif_data);
	exif_data_unref(exif);

	return rc;
}


#include <stdio.h>  
#include <string.h>  
#include <libexif/exif-data.h>  
#include "cdr_count_file.h"  
#include "cdr_bubiao_analyze.h"

static int iIndex = 0;
static unsigned char ucJiWeiFlag = 0;

char LatitudeBuff[41] = {0};
char LongitudeBuff[41] = {0};

float s_dlatitude = 0.0;    //经度
float s_dlongitude = 0.0;   //纬度

void read_exif_entry(ExifEntry *ee, void* ifd)  
{  
    char v[1024] = {0};  

    float lati_tmp = 0.0;
    float lati_cent_tmp = 0.0;
    float lati_second_tmp = 0.0;

    float long_tmp = 0.0;
    float long_cent_tmp = 0.0;
    float long_second_tmp = 0.0;
    
#if(0)    
    printf("%s %s\n"  
           ,exif_tag_get_name_in_ifd(ee->tag, *((ExifIfd*)ifd))  
            ,exif_tag_get_title_in_ifd(ee->tag, *((ExifIfd*)ifd))  
            //, exif_tag_get_description_in_ifd(ee->tag, *((ExifIfd*)ifd))  
            //, exif_entry_get_value(ee, v, sizeof(v))
            );  
#endif

   //Latitude  纬度
   //Longitude 经度
   if(strcmp("Latitude",exif_tag_get_title_in_ifd(ee->tag, *((ExifIfd*)ifd))) == 0x00){
      memset(LatitudeBuff,0,sizeof(LatitudeBuff));
      snprintf(LatitudeBuff,27+13,"%s",exif_entry_get_value(ee, v, sizeof(v)));
      sscanf(LatitudeBuff,"%f,%f,%f",&lati_tmp,&lati_cent_tmp,&lati_second_tmp);
      s_dlatitude = lati_tmp + lati_cent_tmp/60 + (lati_second_tmp/60)/60;     
   }
      
   if(strcmp("Longitude",exif_tag_get_title_in_ifd(ee->tag, *((ExifIfd*)ifd))) == 0x00){
      memset(LongitudeBuff,0,sizeof(LongitudeBuff));
      snprintf(LongitudeBuff,27+13,"%s",exif_entry_get_value(ee, v, sizeof(v)));
      sscanf(LongitudeBuff,"%f,%f,%f",&long_tmp,&long_cent_tmp,&long_second_tmp);      
      s_dlongitude = long_tmp + long_cent_tmp/60 + (long_second_tmp/60)/60;
      ucJiWeiFlag = 1;
   }

   if(ucJiWeiFlag == 1){
       g_sSearchTotailPictBuf[iIndex].dlatitude = s_dlatitude;
       g_sSearchTotailPictBuf[iIndex].dlongitude = s_dlongitude;
       //printf("s_dlatitude:%f s_dlongitude:%f\n",s_dlatitude,s_dlongitude);
       ucJiWeiFlag = 0;
   }
   
}  
  
void read_exif_content(ExifContent *ec, void *user_data)  
{  
    ExifIfd ifd = exif_content_get_ifd(ec);  
    if (ifd == EXIF_IFD_COUNT){  
        fprintf(stderr, "exif_content_get_ifd error\n");  
    }
    //printf("======IFD: %d %s======\n", ifd, exif_ifd_get_name(ifd));  
    if(ifd == 0x03){    
      exif_content_foreach_entry(ec, read_exif_entry, &ifd);  
    }
}


/*
此处理要完成g_sDataSearchAckBag[].位置信息的填写
获取指定文件的 多媒体检索项数据
并将其填写到 g_sDataSearchAckBag[iIndexOfItems] 中
*/
int test_read_exif(char *pFile,int iIndexOfItems)  
{  
    iIndex = iIndexOfItems;
    
    ExifData* ed = exif_data_new_from_file(pFile);  
    if (!ed) {  
        fprintf(stderr, "An error occur\n");  
        return 1;  
    }  
    exif_data_foreach_content(ed, read_exif_content, NULL);  
  
    exif_data_unref(ed);
    
    return 0;  
}  



