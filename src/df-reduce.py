#!/usr/bin/env python
# -*- coding: utf-8 -*-

from sys import stdin, stdout

def main():

    current_word = None
    current_rptid_set = set([])
    for line in stdin:
        splited_line = line.strip().split()
        if len(splited_line) != 2:
            continue
        word, rptid = splited_line
        if current_word is None:
            current_word = word
        if current_word != word:
            stdout.write("%s\t%d\n" % (current_word, len(current_rptid_set)))
            current_word = word
            current_rptid_set.clear()
        current_rptid_set.add(rptid)
    stdout.write("%s\t%d\n" % (current_word, len(current_rptid_set)))

if __name__ == "__main__":

    main()
