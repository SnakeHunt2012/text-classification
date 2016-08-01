#!/usr/bin/env python
# -*- coding: utf-8 -*-

from re import compile
from json import loads, dumps
from jieba import cut, enable_parallel
from codecs import open
from numpy import log
from argparse import ArgumentParser

def main():

    parser = ArgumentParser()
    parser.add_argument("corpus_file", help = "row corpus file (input)")
    parser.add_argument("threshold", help = "idf threshold (input)", type = int)
    parser.add_argument("template_path", help = "template file path to dump in json format (output)")
    args = parser.parse_args()

    corpus_file = args.corpus_file
    threshold = args.threshold
    template_path = args.template_path

    enable_parallel(24)
    
    rptid_search = compile("(?<=<rptid:)[^>]*(?=>)")
    content_search = compile("(?<=<content:)[^>]*(?=>)")
    image_sub = compile("\[img\][^\[\]]+\[/img\]")
    br_sub = compile("\[br\]")

    rptid_set = set()
    df_dict = dict()
    with open(corpus_file, 'r') as fd:
        for line in fd:
            if not line.startswith("<flag:0>"):
                continue
            line = line.strip()

            result = rptid_search.search(line)
            if not result:
                continue
            rptid = result.group(0)
            if rptid in rptid_set:
                continue
            rptid_set.add(rptid)

            result = content_search.search(line)
            if not result:
                continue
            content = result.group(0)
            content = image_sub.sub("", content)
            content = br_sub.sub("\n", content)
            
            seg_set = set([seg.encode("utf-8") for seg in cut(content)])
            for word in seg_set:
                if word not in df_dict:
                    df_dict[word] = 0
                df_dict[word] += 1

    idf_dict = {}
    for word in df_dict:
        if df_dict[word] > threshold:
            idf_dict[word] = log(float(len(rptid_set)) / df_dict[word])

    word_list = list(idf_dict)
    word_index_dict = dict((word_list[index], index) for index in xrange(len(word_list))) # word -> index
    index_word_dict = dict((index, word_list[index]) for index in xrange(len(word_list))) # index -> word

    with open(template_path, 'w') as fd:
        fd.write(dumps({"word_idf_dict": idf_dict,
                        "word_index_dict": word_index_dict,
                        "index_word_dict": index_word_dict}, indent=4, ensure_ascii=False))

if __name__ == "__main__":

    main()
