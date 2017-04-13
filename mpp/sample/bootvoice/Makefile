include ../Makefile.param
####SENSOR_TYPE ?= SONY_IMX122_DC_1080P_30FPS

TARGET      = bootvoice
OBJPATH     = .
SOURCES     = $(wildcard *.c ./audio/*.c ./comm/*.c) 

CDRBASE_DIR = $(shell pwd)
OBJS        = $(patsubst %.c,%.o,$(SOURCES))
FULLOBJS    = $(patsubst %.c,$(OBJPATH)/%.o,$(SOURCES))
CDRINCPATH := -I$(CDRBASE_DIR)/audio
CDRINCPATH += -I$(CDRBASE_DIR)/include
CDRINCPATH += -I$(CDRBASE_DIR)/comm

OTHER_LIB:= -lstdc++ -lpthread  ./lib/libmxml.a -lm ./lib/libaacdec.a ./lib/libaacenc.a 

###添加-c 选项把源代码都编译成为中间文件.
COMPFLAGS   = -c -O2 -fPIC -g -rdynamic

all:$(TARGET)
$(TARGET):$(OBJS)	
	$(CC) $(CFLAGS) $(FULLOBJS)  \
	-o $(TARGET) $(MPI_LIBS) $(AUDIO_LIBA) $(OTHER_LIB) $(CDRINCPATH)
	arm-hisiv300-linux-strip $(TARGET)
	ls -lh $(TARGET)
	@cp $(TARGET) /nfs/sample/hisi3516v200/ -f
	
$(OBJS):$(SOURCES) 
###$(COMM_OBJ)
	$(CC) $*.c -o $(OBJPATH)/$@ $(CDRINCPATH) $(CFLAGS) \
	$(MPI_LIBS) $(AUDIO_LIBA)  $(OTHER_LIB) $(COMPFLAGS)
###	$(CC) $(CFLAGS) $(CDRINCPATH) $*.c -o $(OBJPATH)/$@  $(MPI_LIBS) $(AUDIO_LIBA) $(SENSOR_LIBS) $(OTHER_LIB) 
clean:		
	###@echo $(FULLOBJS)
	@rm -rf $(FULLOBJS)	
	@rm -f $(OBJPATH)/.*.swp
	@rm -f $(TARGET)	
	@rm -f $(COMM_OBJ)



