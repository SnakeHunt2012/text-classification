#!/usr/bin/env python
# -*- coding: utf-8 -*-

from json import loads, dumps
from pickle import load, dump
from argparse import ArgumentParser
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.metrics import roc_auc_score, accuracy_score

def main():

    parser = ArgumentParser()
    parser.add_argument("train_file", help = "data file in pickle format {'url_list': [], 'label_list': [], 'feature_matrix': coo_matrix} (input)")
    parser.add_argument("validate_file", help = "data file in pickle format {'url_list': [], 'label_list': [], 'feature_matrix': coo_matrix} (input)")
    parser.add_argument("score_path", help = "path to dump score in json format (output)")
    parser.add_argument("model_path", help = "path to dump model in pickle format (output)")
    args = parser.parse_args()

    train_file = args.train_file
    validate_file = args.validate_file
    score_path = args.score_path
    model_path = args.model_path
    
    with open(train_file, "rb") as fd:
        data_dict_train = load(fd)
    url_train = data_dict_train["url_list"]
    y_train = data_dict_train["label_list"]
    X_train = data_dict_train["feature_matrix"]
    assert len(url_train) == len(y_train) == X_train.shape[0]

    with open(validate_file, "rb") as fd:
        data_dict_validate = load(fd)
    url_validate = data_dict_validate["url_list"]
    y_validate = data_dict_validate["label_list"]
    X_validate = data_dict_validate["feature_matrix"]
    assert len(url_validate) == len(y_validate) == X_validate.shape[0]

    #classifier = RandomForestClassifier(
    #    n_estimators=5000,
    #    criterion='gini',
    #    max_depth=None,
    #    min_samples_split=2,
    #    min_samples_leaf=5,
    #    min_weight_fraction_leaf=0.0,
    #    max_features='auto',
    #    max_leaf_nodes=None,
    #    bootstrap=True,
    #    oob_score=False,
    #    n_jobs=50,
    #    random_state=None,
    #    verbose=0,
    #    warm_start=False,
    #    class_weight=None
    #)
    classifier = GradientBoostingClassifier(
        loss='deviance',
        learning_rate=0.1,
        n_estimators=50,
        subsample=1.0,
        min_samples_split=2,
        min_samples_leaf=10,
        min_weight_fraction_leaf=0.0,
        max_depth=3,
        init=None,
        random_state=None,
        max_features=None,
        verbose=0,
        max_leaf_nodes=None,
        warm_start=False,
        presort='auto'
    )
    
    print "training ..."
    classifier.fit(X_train, y_train)
    print "training done"

    pred_train = classifier.predict(X_train.toarray())
    pred_validate = classifier.predict(X_validate.toarray())
    proba_train = classifier.predict_proba(X_train.toarray())
    proba_validate = classifier.predict_proba(X_validate.toarray())

    acc_train = accuracy_score(y_train, pred_train)
    acc_validate = accuracy_score(y_validate, pred_validate)

    score_dict = {
        "url_train": url_train,
        "y_train": y_train,
        "pred_train": pred_train.tolist(),
        "proba_train": proba_train.tolist(),
        "url_validate": url_validate,
        "y_validate": y_validate,
        "pred_validate": pred_validate.tolist(),
        "proba_validate": proba_validate.tolist(),
        "acc_train": acc_train,
        "acc_validate": acc_validate,
        "feature_importance": classifier.feature_importances_.tolist()
    }
    print "dumping socre ..."
    with open(score_path, 'w') as fd:
        fd.write(dumps(score_dict, indent = 4))
    print "dumping socre done"

    print "dumping model ..."
    with open(model_path, "wb") as fd:
        dump(classifier, fd)
    print "dumping model done"


if __name__ == "__main__":

    main()
