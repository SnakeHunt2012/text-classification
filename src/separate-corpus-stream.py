#!/usr/bin/env python
# -*- coding: utf-8 -*-

from re import compile
from sys import stdin, stdout
from time import sleep
from json import load, loads
from urllib import quote, urlopen

def fetch_dict(url, title, content):

    host = "http://merger212.se.zzzc.qihoo.net:19528"

    result_dict = None
    retry_counter = 3
    while retry_counter > 0:
        try:
            result_fd = urlopen("%s/mod_content/MyProcess?url=%s&title=%s&content=%s" % (host, quote(url), quote(title), quote(content)))
            result_str = result_fd.read()
            result_dict = loads(result_str)
            break
        except IOError, exception:
            #print "### IOError ###: ", url, title, content
            sleep(1)
            break;  # simply igonore it
        retry_counter -= 1
    return result_dict

def main():

    tag_search = compile("(?<=<LBL>).*(?=</LBL>)")
    title_search = compile("(?<=<TITLE>).*(?=</TITLE>)")
    url_search = compile("(?<=<URL>).*(?=</URL>)")
    content_search = compile("(?<=<CONTENT>).*(?=</CONTENT>)")
    image_search = compile("(?<=\[img\])[^\[\]]+(?=\[/img\])")
    image_sub = compile("\[img\][^\[\]]+\[/img\]")
    br_sub = compile("\[br\]")

    for line in stdin:
        result = tag_search.search(line)
        if not result:
            continue
        tag = result.group(0)
        
        result = title_search.search(line)
        if not result:
            continue
        title = result.group(0)

        result = url_search.search(line)
        if not result:
            continue
        url = result.group(0)
        
        result = content_search.search(line)
        if not result:
            continue
        content = result.group(0)
        content = image_sub.sub("", content)
        content = br_sub.sub("", content)

        result_dict = fetch_dict(url, title, content)

        if "tag" not in result_dict:
            #print "### KeyError ### tag %s not in dict: %r" % (tag, result_dict)
            continue
        
        tag = result_dict["tag"].split(",")[0].encode("utf-8")
        if "|" in tag:
            super_tag, sub_tag = tag.split("|")
            tag = super_tag
            if super_tag == "社会" and sub_tag in set(["情感", "婚恋"]):
                tag = "情感"
                #print "%s|%s -> %s" % (super_tag, sub_tag, tag)
        if tag in set(["国内", "社会"]) and ("体彩" in title or "福彩" in title or "彩票" in title or "大乐透" in title or "体育彩票" in title or "福利彩票" in title):
            #print "%s -> %s\t%s" % (tag, "彩票", title)
            tag = "彩票"

        stdout.write("<LBL>%s</LBL><URL>%s</URL><TITLE>%s</TITLE><CONTENT>%s</CONTENT>\n" % (tag, url, title, content))
        stdout.flush()
        
if __name__ == "__main__":

    main()
