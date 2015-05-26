CXXFLAGS := -Wno-deprecated#-g -D_DEBUG
LDFLAGS := -Wl,--dynamic-list=expsyms -lpthread -llog4cpp
CXXFLAGSDUM := -m32 

bin/%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

outdir := $(wildcard bin)
cppfiles := $(wildcard *.cpp)
prereqfile := $(wildcard prereq)
expsyms := $(wildcard expsyms)

ifeq ($(strip $(prereqfile)),)
    $(shell $(CXX) -MM $(cppfiles) >> _prereq; sed 's/.*\.o/bin\/&/g' < _prereq > prereq; rm -f _prereq)
endif

ifeq ($(strip $(expsyms)),)
    $(shell echo -e "{\n    devMountPt;\n};" > expsyms)
endif

ifeq ($(strip $(outdir)),)
    $(shell mkdir bin)
    outdir := bin
endif

objects := $(patsubst %.cpp,$(outdir)/%.o,$(cppfiles))

all : $(outdir)/virapix $(outdir)/dummy32

$(outdir)/virapix : $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $(LDFLAGS) $^

$(outdir)/dummy32 : 32bit/Dummy32.cpp
	$(CXX) $(CXXFLAGSDUM) -o $@ $^

include prereq

.PHONY : install
install:
	mkdir -p $(DESTDIR)/virapix/bin 2> /dev/null
	mkdir -p $(DESTDIR)/virapix/cfg 2> /dev/null
	mkdir -p $(DESTDIR)/virapix/log 2> /dev/null

	cp -f $(outdir)/virapix $(DESTDIR)/virapix/bin
	cp -f $(outdir)/dummy32 $(DESTDIR)/virapix/bin
	cp -f scripts/bin/* $(DESTDIR)/virapix/bin
	cp -f cfg/.virapixrc $(DESTDIR)/virapix/cfg
	cp -f cfg/fluxbox-menu $(DESTDIR)/virapix/cfg

.PHONY : clean
clean:
	rm -f bin/*
	rm -rf /virapix/*
	rm -f prereq
	rm -f expsyms
