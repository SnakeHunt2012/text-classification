#!/usr/bin/env python
# -*- coding: utf-8 -*-

from codecs import open
from argparse import ArgumentParser

def load_patch_dict(patch_file):

    patch_dict = {}
    new_category_set = set()
    with open(patch_file, 'r') as fd:
        for line in fd:
            splited_line = line.strip("\n").strip("\r").split("\t")
            assert len(splited_line) == 4
            old_parent_tag, old_sub_tag, new_parent_tag, new_sub_tag = splited_line
            if len(old_parent_tag) == 0:
                assert len(old_sub_tag) == 0
                new_category_set.add("%s|%s" % (new_parent_tag, new_sub_tag))
                continue
            old_category = "%s|%s" % (old_parent_tag, old_sub_tag)
            new_category = "%s|%s" % (new_parent_tag, new_sub_tag)
            patch_dict[old_category] = new_category
    return patch_dict, new_category_set

def main():

    parser = ArgumentParser()
    parser.add_argument("new_patch_file", help = "new_patch_file")
    parser.add_argument("new_new_patch_file", help = "new_new_patch_file")
    args = parser.parse_args()

    new_patch_file = args.new_patch_file
    new_new_patch_file = args.new_new_patch_file

    new_patch_dict, new_category_set = load_patch_dict(new_patch_file)
    new_new_patch_dict, new_new_category_set = load_patch_dict(new_new_patch_file)

    aggregate_patch_dict = {}

    for key, value in new_patch_dict.iteritems():
        if new_patch_dict[key] not in new_new_patch_dict:
            print new_patch_dict[key]
        aggregate_patch_dict[key] = new_new_patch_dict[new_patch_dict[key]]
    for category in new_category_set:
        aggregate_patch_dict[category] = new_new_patch_dict[category]

    for old_category, new_category in aggregate_patch_dict.iteritems():
        print "%s\t%s" % (old_category, new_category)
    for category in new_category_set:
        print "\t%s" % category

if __name__ == "__main__":

    main()
