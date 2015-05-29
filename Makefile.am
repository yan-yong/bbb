include $(top_srcdir)/common.mk

SUBDIRS=$(common_path)

AM_CPPFLAGS=-I$(boost_path)/include -I$(top_srcdir) -I$(common_path)
AM_LDFLAGS=-rdynamic -lpthread -L$(common_path)/_lib -lz -lrt

LDADD=$(boost_path)/lib/libboost_system.a $(boost_path)/lib/libboost_thread.a $(common_path)/_lib/libjson.a $(common_path)/_lib/libhttpserver.a $(common_path)/_lib/libbitmap.a $(common_path)/_lib/libutility.a $(common_path)/_lib/libcthread.a $(common_path)/_lib/liblog.a

sbin_PROGRAMS=bloomfilter
bloomfilter_SOURCES=bloomfilter.cpp  handle_task.cpp  main.cpp
