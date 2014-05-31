
FULLNAME = PcscTracker
OSXVERSION ?= 10.6
DISABLE_PNP ?= "no"
WXPREFIX ?= /usr/local
MACOSX_DEPLOYMENT_TARGET = $(OSXVERSION)
CXX="g++"


OSXSDKPATH := /Developer/SDKs/MacOSX$(OSXVERSION).sdk
ifeq ($(wildcard $(OSXSDKPATH)/SDKSettings.plist),)
OSXSDKPATH := /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$(OSXVERSION).sdk
endif

ifeq ($(DISABLE_PNP), yes)
DISABLE_PNP_DEF = -DDISABLE_PCSC_PNP
else
DISABLE_PNP_DEF =
endif

WX_CFLAGS = `$(WXPREFIX)/bin/wx-config --cflags`
WX_LIBS = `$(WXPREFIX)/bin/wx-config --libs`

CXXFLAGSBASE = -g -O2 -I. -I../. -DNDEBUG -D_REENTRANT -Wno-deprecated-declarations -Wno-write-strings $(DISABLE_PNP_DEF) $(WX_CFLAGS) -isysroot $(OSXSDKPATH) -mmacosx-version-min=$(OSXVERSION)

CXXFLAGS = -arch i386 -arch x86_64 $(CXXFLAGSBASE)

LDFLAGS = -Wl,-syslibroot,$(OSXSDKPATH) -arch i386 -arch x86_64 $(WX_LIBS) -framework PCSC

SRCS = ../PcscTracker.cpp ../PcscEventsGui.cpp ../main.cpp
OBJECTS = ../PcscTracker.o ../PcscEventsGui.o ../main.o

#######################
# don't modify
#######################

all: $(FULLNAME)

$(FULLNAME): $(OBJECTS)
	mkdir -p $(FULLNAME).app/Contents/MacOS
	MACOSX_DEPLOYMENT_TARGET=$(OSXVERSION) $(CXX) -o $(FULLNAME).app/Contents/MacOS/$(FULLNAME) $(OBJECTS) $(LDFLAGS)
	strip $(FULLNAME).app/Contents/MacOS/$(FULLNAME)
	upx --lzma --best $(FULLNAME).app/Contents/MacOS/$(FULLNAME)
	codesign -s "Developer ID Application: Mounir IDRASSI" -fv $(FULLNAME).app
	rm -f $(FULLNAME)_1.0.dmg
	hdiutil create -srcfolder $(FULLNAME).app -volname "PcscTracker 1.0  -  Mounir IDRASSI (mounir@idrix.fr)" $(FULLNAME)_1.0.dmg

-include ../PcscTracker.d ../PcscEventsGui.d ../main.d

.cpp.o:
	MACOSX_DEPLOYMENT_TARGET=$(OSXVERSION) $(CXX) $(CXXFLAGS) -c $< -o $@
	MACOSX_DEPLOYMENT_TARGET=$(OSXVERSION) $(CXX) -MM $(CXXFLAGSBASE) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp


.PHONY: clean uninstall install
clean:
	rm -fr ../*.o ../*.d
	rm -fr $(FULLNAME).app/Contents/MacOS/$(FULLNAME)
	rm -fr $(FULLNAME).app/Contents/_CodeSignature

