include Makefile.local

TARGET =  acm_hier

SRCS = main.cpp classification.cpp layout.cpp

OBJS = $(SRCS:.cpp=.o)

EXE = $(SRCS:.cpp=)

INCLUDES = 

CPPFLAGS = -O2 -c -g $(IFLAGS) -std=c++11 $(CPPFLAGSLOCAL)

LDFLAGS =  $(LDFLAGSLOCAL)
LIBS =  -lcurl

.SUFFIXES: .cpp

.cpp.o:  
	$(CXX) $(CPPFLAGS)  $< -o $@

all : $(TARGET)

run: acm_hier
	./acm_hier $(ASSIGNMENTID) $(BRIDGESUSER) $(BRIDGESAPI)

acm_hier: $(OBJS)
	$(LD) -g -o acm_hier $(OBJS) $(LDFLAGS) $(LIBS)

depend: $(SRCS)
	gcc -MD  $(IFLAGS) $(SRCS)

backup:


clean:
	-rm $(OBJS) acm_hier
