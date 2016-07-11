#!/usr/bin/env python
# -*- coding: utf-8 -*-

from re import compile
from sys import stdin, stdout
from jieba import cut, enable_parallel

def main():

    enable_parallel(24)

    rptid_search = compile("(?<=<rptid:)[^>]*(?=>)")
    content_search = compile("(?<=<content:)[^>]*(?=>)")
    image_sub = compile("\[img\][^\[\]]+\[/img\]")
    br_sub = compile("\[br\]")

    for line in stdin:
        if not line.startswith("<flag:0>"):
            continue
        line = line.strip()
    
        result = rptid_search.search(line)
        if not result:
            continue
        rptid = result.group(0)
    
        result = content_search.search(line)
        if not result:
            continue
        content = result.group(0)
        content = image_sub.sub("", content)
        content = br_sub.sub(" ", content)
        
        seg_set = set([seg.encode("utf-8") for seg in cut(content)])

        for word in seg_set:
            stdout.write("%s\t%s\n" % (word, rptid))

if __name__ == "__main__":

    main()
