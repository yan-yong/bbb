# -*- coding: utf-8 -*-
import urllib, urllib2, StringIO, gzip, json, sys, time
from threading import Thread 
def gzip_compress(content):
    str_io = StringIO.StringIO()
    zfile = gzip.GzipFile(mode='wb', fileobj=str_io)
    try:
        zfile.write(content)
    finally:
        zfile.close()
    return str_io.getvalue()
def gzip_decompress(content):
    str_io = StringIO.StringIO(content)
    zfile = gzip.GzipFile(mode='rb', fileobj = str_io)
    try:
        data = zfile.read()
    finally:
        zfile.close()
    return data

def http_fetch(thread_id):
    print "thread %d start" % thread_id
    post_data = []
    for i in xrange(10, 20):
       cur_item = {} 
       cur_item["lk"] = "http://www.baidu.com/%d.html" % i
       cur_item["op"] = "g"
       post_data.append(cur_item)
    #Content-Encoding: 表示压缩请求content, 当一次请求内容较大时建议这样做
    #Accept-Encoding:  表示可以接受压缩的结果, 建议带这个头
    http_headers = {'Content-Encoding':'gzip', 'Accept-Encoding':'gzip'}
    post_data = json.dumps(post_data)
    '''请求content建议压缩'''
    post_data = gzip_compress(post_data);
    #print gzip_decompress(post_data)
    
    request  = urllib2.Request("http://172.19.34.110:2345/", data=post_data, headers = http_headers);
    request.add_header('Accept-encoding','gzip')
    beg_time = time.time()
    print "thread %d, processing..." % thread_id
    response = urllib2.urlopen(request);
    end_time = time.time()
    response_content = response.read()
    #当客户端携带了'Accept-Encoding':'gzip'时，返回内容较大时，服务端会进行压缩，客户端需要判断
    if response.info().getheader("Content-Encoding") == "gzip":
        response_content = gzip_decompress(response_content)
    #print 'Sending:  %s' % post_data
    print 'Receive: %s' % response_content 
    print "thread %d end, use time: %f s" % (thread_id, end_time - beg_time)

thread_num = 10
for i in range(thread_num):
    thread = Thread(target=http_fetch, args=(i,)) 
    thread.start()
