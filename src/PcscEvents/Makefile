
FULLNAME = pcscTracker

DISABLE_PNP ?= "no"

ifeq ($(DISABLE_PNP), yes)
DISABLE_PNP_DEF = -DDISABLE_PCSC_PNP
else
DISABLE_PNP_DEF =
endif

CXXFLAGS = -g -O2  -I. -I /usr/include/PCSC -Wno-write-strings $(DISABLE_PNP_DEF) -pthread `wx-config --cflags`
LDFLAGS = -pthread -ldl `wx-config --libs`

SRCS = PcscTracker.cpp PcscEventsGui.cpp main.cpp
OBJECTS = $(SRCS:.cpp=.o)

#######################
# don't modify
#######################
bindir = $(prefix)/bin
incdir = $(prefix)/include

all: $(FULLNAME)

$(FULLNAME): $(OBJECTS)
	$(CXX) -o $(FULLNAME) $(OBJECTS) $(LDFLAGS) 

-include $(OBJECTS:.o=.d)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

install: all
	cp -p $(FULLNAME) $(bindir)/.

uninstall:
	rm -fr $(bindir)/$(FULLNAME)

.PHONY: clean uninstall install
clean:
	rm -fr *.o *.d
	rm -fr $(FULLNAME)

