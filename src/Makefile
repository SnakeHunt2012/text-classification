C = gcc
CXX = g++

MYHOME=/home/huangjingwen/work/text-classification

INC = -I . -I$(MYHOME)/qmodule/qcommon-1.0.0/include -I $(MYHOME)/qmodule/transcode-1.0.0/include -I $(MYHOME)/qmodule/segment-2.2.1/include -I ./jsoncpp -I $(MYHOME)/xgboost/include

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

LIBS = /home/huangjingwen/software/xgboost/lib/libxgboost.a /home/huangjingwen/software/xgboost/lib/librabit.a /home/huangjingwen/software/xgboost/lib/libdmlc.a -lpthread -fopenmp $(MYHOME)/qmodule/segment-2.2.1/lib/libsegment.a $(MYHOME)/qmodule/transcode-1.0.0/lib/libtranscode.a

.PHONY: all

all: test develop predict classifier.a convert-corpus convert-feature

develop: develop.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

predict: predict.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

convert-corpus: convert-corpus.o $(OBJS)
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

convert-feature: convert-feature.o ${OBJS}
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

convert-feature-dev: convert-feature-dev.o ${OBJS}
	$(CXX) $(CXXFLAGS) $< ${OBJS} -o $(BINDIR)/$@ ${LIBS}

classifier.a: classifier.o $(OBJS)
	ar -r classifier.a $(OBJS)

test: test.o $(OBJS) classifier.o
	$(CXX) $(CXXFLAGS) $< $(OBJS) -o $(BINDIR)/$@ ${LIBS}

clean:
	rm -f *.o  *~ /cores/core.* core *.core *.a
