lib_LIBRARIES = libDttSP.a
# for gprof and gcov 
#AM_CXXFLAGS= -pg -fprofile-arcs -ftest-coverage
#AM_CXXFLAGS= -Wa,-al # for listings

#optimizations
#CXXFLAGS=-g -Wall -O0
CXXFLAGS= -O3 -g -ffast-math -Wall
#CXXFLAGS = -O2 -g -Wall -Wunused -Wextra
#CXXFLAGS+= -DVALGRIND
AM_CXXFLAGS= -Wfloat-equal -Wshadow -Wno-multichar -Wredundant-decls 
AM_CXXFLAGS+=-Weffc++ 

################################
######### libDttSP.a ###########
################################
libDttSP_a_SOURCES = am_demod.c banal.c bufvec.c correctIQ.c cwtones.c cxops.c \
				  dcblock.c dttspagc.c fastrig.c filterbank.c filter.c fm_demod.c graphiceq.c \
				  hilbert.c isoband.c keyd.c keyer.c keyerio.c lmadf.c malloc16.c meter.c metermon.c \
				  noiseblanker.c oscillator.c ovsv.c resample.c resampleF.c ringb.c sdr.c sdrexport.c \
				  spectrum.c speechproc.c splitfields.c spottone.c update.c window.c winmain.c wscompand.c 

#libDttSP_a_LIBADD =  -lz -lpthread @LIBS@

INCLUDES = -I$(top_srcdir)/include
