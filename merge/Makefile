TARGET:= merge
all: $(TARGET) 

.PHONY: all clean install

CFLAGS= -DNDEBUG -D_POSIX_C_SOURCE=200112L -DMONGO_HAVE_STDINT -g -Wall -march=x86-64
#CFLAGS=-DNDEBUG -DINT32_MAX=0x7fffffff -DMONGO_STATIC_BUILD -DMONGO_HAVE_UNISTD -g -Wall -mcmodel=medium  -march=x86-64 -std=c99
#LIBS=libluajit.a -lpthread -L /usr/local/lib -L /usr/lib64 -levent -ldl  -lroleffi -lmmap_lowmem_mt
LIBS=-lluajit-5.1 -levent -lpthread -L /usr/lib64 -ldl -lrt -lstdc++
SOURCES:=$(wildcard *.cpp ../common/*.cpp)
CSOURCES:= $(wildcard ../mongo_c_driver/*.c)  
INCLUDE:=-I. -I../common -I../logic -I../luajit -I../mongo_c_driver -I../libevent/include
DEPENDS:=$(SOURCES:.cpp=.d)
CDEPENDS:=$(CSOURCES:.c=.d) 
OBJECTS:=$(SOURCES:.cpp=.o)
COBJECTS:=$(CSOURCES:.c=.o)
sinclude $(DEPENDS) $(CDEPENDS)

$(TARGET):$(OBJECTS) $(COBJECTS)
	$(CXX) -o $@ $^ $(LIBS) 

$(COBJECTS):%.o:%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDE) $< -o $@

$(OBJECTS):%.o:%.cpp 
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDE) $< -o $@

$(DEPENDS):%.d:%.cpp
	$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(CDEPENDS):%.d:$.c
	$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(FFITARGET):role_data.c
	$(CC) -o $(FFITARGET)  -O2 -fPIC -shared  role_data.c 
clean:
	rm -f $(TARGET) $(OBJECTS) $(COBJECTS) $(DEPENDS) $(CDEPENDS)

install:
	cp $(TARGET) ../../bin
