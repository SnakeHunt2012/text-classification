C = gcc
CXX = g++

MYHOME=/home/huangjingwen/work/text-classification

INC = -I . -I$(MYHOME)/qmodule/qcommon-1.0.0/include -I $(MYHOME)/qmodule/transcode-1.0.0/include -I $(MYHOME)/qmodule/segment-2.2.1/include -I ./jsoncpp

OS = $(shell uname)
CFLAGS = -Wall 
#CFLAGS = -std=c++11 -Wall 

CXXFLAGS_NOMDBM = $(CFLAGS)
CFLAGS += $(INC)
CXXFLAGS = $(CFLAGS)

RELEASE = 1

ifeq ($(RELEASE), 1)
	CXXFLAGS += -O3
else
	CXXFLAGS += -g
endif


OBJS = common.o global_dict.o sparse_matrix.o classifier.o /home/huangjingwen/work/text-classification/src/jsoncpp/jsoncpp.o

BINDIR = ./

LIBS = -l xgboost /home/huangjingwen/software/transcode/lib/libtranscode.a /home/huangjingwen/software/segment/lib/libsegment.a $(MYHOME)/qmodule/segment-2.2.1/lib/libsegment.a $(MYHOME)/qmodule/transcode-1.0.0/lib/libtranscode.a

.PHONY: all

all: develop predict classifier.a

develop: develop.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

predict: predict.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

classifier.a: classifier.o $(OBJS)
	ar -r classifier.a $(OBJS)

clean:
	rm -f *.o  *~ /cores/core.* core *.core *.a
