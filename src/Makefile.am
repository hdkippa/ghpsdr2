bin_PROGRAMS = ghpsdr2
# for gprof and gcov 
#AM_CXXFLAGS= -pg -fprofile-arcs -ftest-coverage
#AM_CXXFLAGS= -Wa,-al # for listings

# for kcachegrind, run with calltree ghpsdr2, then look at it with kcachegrind should also have -O0
#AM_CPPFLAGS = -DLINUX -D__USE_MYSQL__  -DVALGRIND
# normal compile
AM_CPPFLAGS = 

#optimization on or off here
#CXXFLAGS=-g -Wall -O0
CXXFLAGS= -O3 -pg -ffast-math -Wall
#CXXFLAGS= -O2 -g -Wall -Wunused -Wextra
AM_CXXFLAGS= -Wfloat-equal -Wshadow -Wno-multichar -Wredundant-decls 
AM_CXXFLAGS+=-Weffc++ 

################################
########### ghpsdr #############
################################
ghpsdr2_SOURCES =  agc.c alex_rx_test.c alex_setup.c alex_tx_test.c audio.c audiostream.c band.c bandscope.c \
				   bandscope_control.c bandscope_update.c buffer.c command.c cw.c display.c display_setup.c expression.c \
				   filter.c frequency.c hpsdr_setup.c libusbio.c main.c meter.c meter_update.c metis.c mode.c ozy.c preamp.c \
				   property.c receiver.c setup.c sinewave.c soundcard.c spectrum_buffers.c spectrum.c spectrum_update.c subrx.c \
				   test.c transmit.c udp.c udt.c vfo.c volume.c xvtr_setup.c

#ghpsdr2_LDFLAGS = @PACKAGE_LDFLAGS@ -L$(top_srcdir)/DttSP 
#ghpsdr2_LDFLAGS = -g -static @PACKAGE_LDFLAGS@
ghpsdr2_LDFLAGS = -g -pg @PACKAGE_LDFLAGS@ -L$(top_srcdir)/DttSP 

ghpsdr2_LDADD =  -lDttSP -lz -lpthread @LIBS@ @PACKAGE_LIBS@\
				 -lusb-1.0 \
				 -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 \
				 -lm -lpangocairo-1.0 -lgio-2.0 -lcairo -lpango-1.0 -lfreetype \
				 -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lfftw3f


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/Dttsp @PACKAGE_CFLAGS@
