#!/usr/bin/env python
# -*- coding: utf-8 -*-

from re import compile
from codecs import open
from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("content_file", help = "content file (input)")
    parser.add_argument("tag", help = "label (input)")
    parser.add_argument("corpus_file", help = "corpus file (output)")
    args = parser.parse_args()

    content_file = args.content_file
    tag = args.tag
    corpus_file = args.corpus_file

    rptid_re = compile("(?<=<putid:)[^>]*(?=>)")
    url_re = compile("(?<=<url:)[^>]*(?=>)")
    title_re = compile("(?<=<title:)[^>]*(?=>)")
    content_re = compile("(?<=<content:)[^>]*(?=>)")
    image_sub = compile("\[img\][^\[\]]+\[/img\]")
    br_sub = compile("\[br\]")

    rptid_set = set()
    with open(content_file, 'r') as in_fd:
        with open(corpus_file, 'w') as out_fd:
            for line in in_fd:
                # extract pvtid
                result = rptid_re.search(line)
                if not result:
                    continue
                if result.group(0) in rptid_set:
                    continue
                rptid_set.add(result.group(0))
                
                # extract url
                result = url_re.search(line)
                if not result:
                    continue
                url = result.group(0)

                # extract title
                result = title_re.search(line)
                if not result:
                    continue
                title = result.group(0)
                
                # extract content
                result = content_re.search(line)
                if not result:
                    continue
                content = result.group(0)
                content = image_sub.sub("", content)
                #content = br_sub.sub("", content)

                out_fd.write("<LBL>%s</LBL><TITLE>%s</TITLE><URL>%s</URL><CONTENT>%s</CONTENT>\n" % (tag, title, url, content))

if __name__ == "__main__":

    main()
