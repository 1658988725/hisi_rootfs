include ../Makefile.param
####SENSOR_TYPE ?= SONY_IMX122_DC_1080P_30FPS

TARGET      = cdr_app
OBJPATH     = .
SOURCES     = $(wildcard \
*.c ./cdr_sd/*.c  ./cdr_mpp/*c ./osd_region/*c \
./cdr_rec/*c ./cdr_net/*c ./cdr_device/*c ./cdr_mp4/*c ./cdr_log/*c ./cdr_http/*c \
./librtsp/*c ./librtsp/comm/*.c ./librtsp/net/*c ./librtsp/rtcp/*c \
./librtsp/rtp/*c ./librtsp/rtsp/*c ./librtsp/utils/*c \
./cdr_comm/*c ./cdr_config/*c ./cdr_exif/*c ./cdr_push/*c \
./cdr_bubiao/*c ./cdr_queue/*c) 

CDRBASE_DIR = $(shell pwd)
OBJS        = $(patsubst %.c,%.o,$(SOURCES))
FULLOBJS    = $(patsubst %.c,$(OBJPATH)/%.o,$(SOURCES))

CDRINCPATH := -I$(CDRBASE_DIR)/cdr_sd
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_mpp
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_net
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_rec
CDRINCPATH += -I$(CDRBASE_DIR)/include
CDRINCPATH += -I$(CDRBASE_DIR)/mmap
CDRINCPATH += -I$(CDRBASE_DIR)/osd_region
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_device
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_mp4
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_log
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/comm
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/net
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/rtcp
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/rtp
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/rtsp
CDRINCPATH += -I$(CDRBASE_DIR)/librtsp/utils
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_comm
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_config
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_exif
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_http
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_push
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_bubiao
CDRINCPATH += -I$(CDRBASE_DIR)/cdr_queue

OTHER_LIB:= -lstdc++ -lpthread ./lib/libmp4v2.a ./lib/libmxml.a \
-lm ./lib/libaacdec.a ./lib/libaacenc.a ./lib/libexif.a ./lib/libcurl.a

##COMPFLAGS   = -c -O2 -fPIC  -lm -D_GNU_SOURCE -DRTSARM

###添加-c 选项把源代码都编译成为中间文件.
COMPFLAGS   = -c -O2 -fPIC -D_GNU_SOURCE -D__USE_XOPEN

all:$(TARGET)
$(TARGET):$(OBJS)	
	@$(CC) $(CFLAGS) $(FULLOBJS) $(COMM_OBJ) \
	-o $(TARGET) $(MPI_LIBS) $(AUDIO_LIBA) $(SENSOR_LIBS) $(OTHER_LIB) $(CDRINCPATH)
	arm-hisiv300-linux-strip $(TARGET)
	ls -lh $(TARGET)
	@cp $(TARGET) /nfs/sample/hisi3516v200/ -f
	
$(OBJS):$(SOURCES) $(COMM_OBJ)	
	@echo $*.c
	@$(CC) $*.c -o $(OBJPATH)/$@ $(CDRINCPATH) $(CFLAGS) $(COMPFLAGS)
clean:			
	@rm -rf $(FULLOBJS)	
	@rm -f $(OBJPATH)/.*.swp
	@rm -f $(TARGET)	
	@rm -f $(COMM_OBJ)



