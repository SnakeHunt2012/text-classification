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
    //map<string, int> word_count_map;
    //for (map<string, int>::const_iterator iter = title.begin(); iter != title.end(); ++iter)
    //    word_count_map[iter->first] += iter->second;
    //for (map<string, int>::const_iterator iter = content.begin(); iter != content.end(); ++iter)
    //    word_count_map[iter->first] += iter->second;
    //
    //int term_count = 0;
    //for (map<string, int>::const_iterator iter = word_count_map.begin(); iter != word_count_map.end(); ++iter)
    //    term_count += iter->second;
    //
    //map<string, double> feature_value_map;
    //for (map<string, int>::const_iterator word_count_iter = word_count_map.begin(); word_count_iter != word_count_map.end(); ++word_count_iter) {
    //    const string &word = word_count_iter->first;
    //    const double tf = (double) word_count_iter->second / term_count;
    //    map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
    //    if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
    //        feature_value_map[word] = tf * word_idf_iter->second;
    //}
    //normalize(feature_value_map);
    //
    //map<int, double> index_value_map;
    //for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
    //    map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
    //    if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
    //        index_value_map[word_index_iter->second] = feature_value_iter->second;
    //}
    //try {
    //    string netloc = parse_netloc(url);
    //    map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
    //    if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
    //        index_value_map[((GlobalDict *) global_dict)->get_word_count() + netloc_index_iter->second] = 1;
    //} catch (runtime_error &err) {
    //    // cout << err.what() << endl;
    //}
    
    map<int, double> index_value_map;
    compile_tfidf_feature(global_dict, url, title, content, index_value_map);

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
    //map<string, int> word_count_map;
    //for (map<string, int>::const_iterator iter = title.begin(); iter != title.end(); ++iter)
    //    word_count_map[iter->first] += iter->second;
    //for (map<string, int>::const_iterator iter = content.begin(); iter != content.end(); ++iter)
    //    word_count_map[iter->first] += iter->second;
    //
    //int term_count = 0;
    //for (map<string, int>::const_iterator iter = word_count_map.begin(); iter != word_count_map.end(); ++iter)
    //    term_count += iter->second;
    //
    //map<string, double> feature_value_map;
    //for (map<string, int>::const_iterator word_count_iter = word_count_map.begin(); word_count_iter != word_count_map.end(); ++word_count_iter) {
    //    const string &word = word_count_iter->first;
    //    const double tf = (double) word_count_iter->second / term_count;
    //    map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
    //    if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
    //        feature_value_map[word] = tf * word_idf_iter->second;
    //}
    //normalize(feature_value_map);
    //
    //map<int, double> index_value_map;
    //for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
    //    map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
    //    if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
    //        index_value_map[word_index_iter->second] = feature_value_iter->second;
    //}
    //try {
    //    string netloc = parse_netloc(url);
    //    map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
    //    if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
    //        index_value_map[((GlobalDict *) global_dict)->get_word_count() + netloc_index_iter->second] = 1;
    //} catch (runtime_error &err) {
    //    // cout << err.what() << endl;
    //}

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
