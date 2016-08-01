#!/usr/bin/env
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt

from json import loads
from numpy import array, sum, mean, std, arange
#from codecs import open
from argparse import ArgumentParser

from matplotlib import pyplot
from matplotlib.font_manager import FontProperties
from sklearn.metrics import confusion_matrix

font = FontProperties(fname=r"c:\windows\fonts\simsun.ttc", size=14)

def load_category_dict(label_file):

    with open(label_file, 'r') as fd:
        label_dict = loads(fd.read())
    tag_label_dict = dict((key, int(value)) for key, value in label_dict["tag_label_dict"].iteritems())
    label_tag_dict = dict((int(key), value) for key, value in label_dict["label_tag_dict"].iteritems())
    #for index in xrange(len(label_tag_dict)):
    #    print "%s" % repr(label_tag_dict[index])
    #print
    return tag_label_dict, label_tag_dict

def plot_confusion_matrix(cm, tag_label_dict, title="Confusion Matrix"):

    tag_list = [tag_label_dict[index] for index in xrange(len(tag_label_dict))]
    pyplot.imshow(cm, interpolation='nearest')
    pyplot.title(title)
    pyplot.colorbar()
    tick_marks = arange(len(tag_label_dict))
    pyplot.xticks(tick_marks, tag_list, rotation=90, fontproperties=font)
    pyplot.yticks(tick_marks, tag_list, fontproperties=font)
    pyplot.tight_layout()
    pyplot.xlabel("True Label")
    pyplot.ylabel("Predicted Label")

def main():

    parser = ArgumentParser()
    parser.add_argument("label_file", help = "label dict in json format")
    parser.add_argument("score_file", help = "score file in json format")
    args = parser.parse_args()

    label_file = args.label_file
    score_file = args.score_file

    tag_label_dict, label_tag_dict = load_category_dict(label_file)
    
    with open(score_file, 'r') as fd:
        score_dict = loads(fd.read())

    url_train = score_dict["url_train"]
    url_validate = score_dict["url_validate"]
    y_train = score_dict["y_train"]
    y_validate = score_dict["y_validate"]
    pred_train = score_dict["pred_train"]
    pred_validate = score_dict["pred_validate"]
    proba_train = score_dict["proba_train"]
    proba_validate = score_dict["proba_validate"]
    acc_train = score_dict["acc_validate"]
    acc_validate = score_dict["acc_validate"]
    feature_importance = score_dict["feature_importance"]

    cm_train = confusion_matrix(y_train, pred_train)
    cm_train_normalized = cm_train.astype("float") / cm_train.sum(axis=1)
    cm_validate = confusion_matrix(y_validate, pred_validate)
    cm_validate_normalized = cm_validate.astype("float") / cm_validate.sum(axis=1)

    pyplot.figure()
    plot_confusion_matrix(cm_train, label_tag_dict, title = "Confusion Matrix on training set")
    pyplot.show()
    
    pyplot.figure()
    plot_confusion_matrix(cm_train_normalized, label_tag_dict, title = "Confusion Matrix on training set (normalized)")
    pyplot.show()
    
    pyplot.figure()
    plot_confusion_matrix(cm_validate, label_tag_dict, title = "Confusion Matrix on validation set")
    pyplot.show()
    
    pyplot.figure()
    plot_confusion_matrix(cm_validate_normalized, label_tag_dict, title = "Confusion Matrix on validation set (normalized)")
    pyplot.show()
    
if __name__ == "__main__":

    main()
