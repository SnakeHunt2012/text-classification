#include <stdexcept>
#include <iostream>

#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "global_dict.h"
#include "sparse_matrix.h"

#include "classifier.h"

using namespace std;

Classifier::Classifier(const char *label_file, const char *template_file, const char *netloc_file, const char *model_file)
{
    global_dict = (void *) new GlobalDict(label_file, template_file, netloc_file);
    assert(!XGBoosterCreate(NULL, 0, &model));
    assert(!XGBoosterLoadModel(model, model_file));
}

Classifier::~Classifier()
{
    assert(!XGBoosterFree(model));
    delete (GlobalDict *) global_dict;
}

void Classifier::classify(const std::string &url, const map<string, int> &title, const map<string, int> &content, string &tag, float *proba)
{
    map<int, double> index_value_map;
    //compile_tfidf_feature(global_dict, url, title, content, index_value_map);
    compile_bm25_feature(global_dict, url, title, content, index_value_map);

    SparseMatrix sparse_matrix;
    sparse_matrix.push_back(index_value_map);

    DMatrixHandle matrix_handle = load_X(sparse_matrix);

    unsigned long proba_array_length;
    float *proba_array;
    if (XGBoosterPredict(model, matrix_handle, 1, 0, &proba_array_length, (const float **) &proba_array))
         throw runtime_error("error: XGBoosterPredict failed");

    XGDMatrixFree(matrix_handle);

    int pred;
    parse_pred(proba_array, proba_array_length, &pred, proba);
    //free(proba_array);

    map<int, string>::const_iterator label_tag_iter = ((GlobalDict *) global_dict)->label_tag_map.find(pred);
    if (label_tag_iter == ((GlobalDict *) global_dict)->label_tag_map.end())
        throw runtime_error("error: pred not in label_tag_map");
    tag = label_tag_iter->second;
}

void Classifier::classify(const string &url, const map<string, int> &title, const map<string, int> &content, vector<pair<string, pair<float, float> > > &res_vec) {
    map<int, double> index_value_map;
    //compile_tfidf_feature(global_dict, url, title, content, index_value_map);
    compile_bm25_feature(global_dict, url, title, content, index_value_map);

    SparseMatrix sparse_matrix;
    sparse_matrix.push_back(index_value_map);

    DMatrixHandle matrix_handle = load_X(sparse_matrix);

    unsigned long proba_array_length;
    float *proba_array;
    if (XGBoosterPredict(model, matrix_handle, 1, 0, &proba_array_length, (const float **) &proba_array))
         throw runtime_error("error: XGBoosterPredict failed");

    XGDMatrixFree(matrix_handle);

    vector<pair<int, pair<float, float> > > sorted_label_vec;
    parse_pred(proba_array, proba_array_length, sorted_label_vec);
    assert(sorted_label_vec.size() == proba_array_length);
    //free(proba_array);

    assert(res_vec.size() == 0);
    for (size_t offset = 0; offset < sorted_label_vec.size(); ++offset) {
        map<int, string>::const_iterator label_tag_iter = ((GlobalDict *) global_dict)->label_tag_map.find(sorted_label_vec[offset].first);
        if (label_tag_iter == ((GlobalDict *) global_dict)->label_tag_map.end())
            throw runtime_error("error: pred not in label_tag_map");
        res_vec.push_back(make_pair(label_tag_iter->second, sorted_label_vec[offset].second));
    }
    assert(res_vec.size() == proba_array_length);
}    
