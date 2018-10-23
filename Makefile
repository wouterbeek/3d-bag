# -*- Makefile -*-

cc=g++
cflags=-c -fPIC -g -std=c++17 -Wall -Werror -Wextra `gdal-config --cflags`
libs=`gdal-config --libs`
objs=$(srcs:.cpp=.o)
rm=rm -f
srcs=$(wildcard src/*.cpp)

all: drivers gml2wkt

drivers: src/drivers.o
	$(cc) -o $@ $^ $(libs)

drivers.o: src/drivers.cpp
	$(cc) $(cflags) -o $@ $<

gml2wkt: src/gml2wkt.o
	$(cc) -o $@ $^ $(libs)

gml2wkt.o: src/gml2wkt.cpp
	$(cc) $(cflags) -o $@ $<

clean:
	$(rm) $(objs) drivers gml2wkt
