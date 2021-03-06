#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <math.h>
#include <assert.h>

#include "config.h"

#include "common.h"

using namespace std;

DMatrixHandle load_X(SparseMatrix &sparse_matrix)
{
    unsigned long *indptr;
    unsigned int *indices;
    float *data;
    sparse_matrix.get_data(&indptr, &indices, &data);
    
    unsigned int nindptr = sparse_matrix.get_nindptr();
    unsigned int nelem = sparse_matrix.get_nelem();
        
    DMatrixHandle matrix_handel;
    int error_code = XGDMatrixCreateFromCSR(indptr, indices, data, nindptr + 1, nelem, &matrix_handel);
    if (error_code) throw runtime_error("error: XGDMatrixCreateFromCSR failed");
    return matrix_handel;
}

float *load_y(vector<int> &label_vec)
{
    float *label_array = (float *) malloc(sizeof(float) * label_vec.size());
    for (size_t i = 0; i < label_vec.size(); ++i) {
        label_array[i] = (float) label_vec[i];
    }
    return label_array;
}

void compile_tfidf_feature(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    map<string, int> word_count_map;
    for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;
    for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;

    int term_count = 0;
    for (map<string, int>::const_iterator iter = word_count_map.begin(); iter != word_count_map.end(); ++iter)
        term_count += iter->second;

    map<string, double> feature_value_map;
    for (map<string, int>::const_iterator word_count_iter = word_count_map.begin(); word_count_iter != word_count_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = (double) word_count_iter->second / term_count;
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(feature_value_map);

    for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[word_index_iter->second] = feature_value_iter->second;
    }

    if (url == "")
        return;
    
    try {
        string netloc = parse_netloc(url);
        map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
        if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
            index_value_map[((GlobalDict *) global_dict)->get_word_count() + netloc_index_iter->second] = 1;
    } catch (runtime_error &err) {
        // cout << err.what() << endl;
    }
}

void compile_tfidf_feature(void *global_dict,  const map<string, int> &term_reduce_map, map<int, double> &index_value_map) {
    const string url = "";
    const map<string, int> dummy_reduce_map;
    compile_tfidf_feature(global_dict, url, dummy_reduce_map, term_reduce_map, index_value_map);
}

void compile_bm25_feature(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    compile_bm25_feature_merge(global_dict, url, title_reduce_map, content_reduce_map, index_value_map);
}

void compile_bm25_feature_merge(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    const double k = 1, b = 0.5;
    const int average_document_length = 518;
    
    map<string, int> word_count_map;
    for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;
    for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;

    int term_count = 0;
    for (map<string, int>::const_iterator iter = word_count_map.begin(); iter != word_count_map.end(); ++iter)
        term_count += iter->second;

    map<string, double> feature_value_map;
    for (map<string, int>::const_iterator word_count_iter = word_count_map.begin(); word_count_iter != word_count_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = ((double) word_count_iter->second * (k + 1)) / ((double) word_count_iter->second + k * (1 - b + b * (double) average_document_length / (double) term_count));
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(feature_value_map);

    for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[word_index_iter->second] = feature_value_iter->second;
    }

    if (url == "")
        return;
    
    try {
        string netloc = parse_netloc(url);
        map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
        if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
            index_value_map[((GlobalDict *) global_dict)->get_word_count() + netloc_index_iter->second] = 1;
    } catch (runtime_error &err) {
        // cout << err.what() << endl;
    }
}

void compile_bm25_feature_merge_without_url(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    const double k = 1, b = 0.5;
    const int average_document_length = 518;
    
    map<string, int> word_count_map;
    for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;
    for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
        word_count_map[iter->first] += iter->second;

    int term_count = 0;
    for (map<string, int>::const_iterator iter = word_count_map.begin(); iter != word_count_map.end(); ++iter)
        term_count += iter->second;

    map<string, double> feature_value_map;
    for (map<string, int>::const_iterator word_count_iter = word_count_map.begin(); word_count_iter != word_count_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = ((double) word_count_iter->second * (k + 1)) / ((double) word_count_iter->second + k * (1 - b + b * (double) average_document_length / (double) term_count));
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(feature_value_map);

    for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[word_index_iter->second] = feature_value_iter->second;
    }
}

void compile_bm25_feature_separate(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    const double k = 1, b = 0.5;
    const int average_title_length = 10, average_content_length = 518;
    
    int title_term_count = 0, content_term_count = 0;
    for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        title_term_count += iter->second;
    for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
        content_term_count += iter->second;

    map<string, double> title_feature_value_map, content_feature_value_map;
    for (map<string, int>::const_iterator word_count_iter = title_reduce_map.begin(); word_count_iter != title_reduce_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = ((double) word_count_iter->second * (k + 1)) / ((double) word_count_iter->second + k * (1 - b + b * (double) average_title_length / (double) title_term_count));
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            title_feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(title_feature_value_map);
    for (map<string, int>::const_iterator word_count_iter = content_reduce_map.begin(); word_count_iter != content_reduce_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = ((double) word_count_iter->second * (k + 1)) / ((double) word_count_iter->second + k * (1 - b + b * (double) average_content_length / (double) content_term_count));
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            content_feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(content_feature_value_map);

    for (map<string, double>::const_iterator feature_value_iter = title_feature_value_map.begin(); feature_value_iter != title_feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[word_index_iter->second] = feature_value_iter->second;
    }
    for (map<string, double>::const_iterator feature_value_iter = content_feature_value_map.begin(); feature_value_iter != content_feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[((GlobalDict *) global_dict)->get_word_count() + word_index_iter->second] = feature_value_iter->second;
    }

    if (url == "")
        return;
    
    try {
        string netloc = parse_netloc(url);
        map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
        if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
            index_value_map[((GlobalDict *) global_dict)->get_word_count() * 2 + netloc_index_iter->second] = 1;
    } catch (runtime_error &err) {
        // cout << err.what() << endl;
    }
}

void compile_bm25_feature_title(void *global_dict, const string &url, const map<string, int> &title_reduce_map, const map<string, int> &content_reduce_map, map<int, double> &index_value_map)
{
    const double k = 1, b = 0.5;
    const int average_title_length = 10;
    
    int title_term_count = 0;
    for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        title_term_count += iter->second;

    map<string, double> title_feature_value_map;
    for (map<string, int>::const_iterator word_count_iter = title_reduce_map.begin(); word_count_iter != title_reduce_map.end(); ++word_count_iter) {
        const string &word = word_count_iter->first;
        const double tf = ((double) word_count_iter->second * (k + 1)) / ((double) word_count_iter->second + k * (1 - b + b * (double) average_title_length / (double) title_term_count));
        map<string, double>::const_iterator word_idf_iter = ((GlobalDict *) global_dict)->word_idf_map.find(word);
        if (word_idf_iter != ((GlobalDict *) global_dict)->word_idf_map.end())
            title_feature_value_map[word] = tf * word_idf_iter->second;
    }
    normalize(title_feature_value_map);

    for (map<string, double>::const_iterator feature_value_iter = title_feature_value_map.begin(); feature_value_iter != title_feature_value_map.end(); ++feature_value_iter) {
        map<string, int>::const_iterator word_index_iter = ((GlobalDict *) global_dict)->word_index_map.find(feature_value_iter->first);
        if (word_index_iter != ((GlobalDict *) global_dict)->word_index_map.end())
            index_value_map[word_index_iter->second] = feature_value_iter->second;
    }

    if (url == "")
        return;
    
    try {
        string netloc = parse_netloc(url);
        map<string, int>::const_iterator netloc_index_iter = ((GlobalDict *) global_dict)->netloc_index_map.find(netloc);
        if (netloc_index_iter != ((GlobalDict *) global_dict)->netloc_index_map.end())
            index_value_map[((GlobalDict *) global_dict)->get_word_count() + netloc_index_iter->second] = 1;
    } catch (runtime_error &err) {
        // cout << err.what() << endl;
    }
}

void load_data_file(const char *data_file, GlobalDict &global_dict, vector<string> &url_vec, SparseMatrix &sparse_matrix, vector<int> &label_vec)
{
    ifstream input(data_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(data_file));

    regex_t tag_regex = compile_regex("(<LBL>)(.*)(</LBL>)");
    regex_t title_regex = compile_regex("(<TITLE>)(.*)(</TITLE>)");
    regex_t url_regex = compile_regex("(<URL>)(.*)(</URL>)");
    regex_t content_regex = compile_regex("(<CONTENT>)(.*)(</CONTENT>)");
    regex_t image_regex = compile_regex("\\[img\\][^\\[]*\\[/img\\]");
    regex_t br_regex = compile_regex("\\[br\\]");

    qss::segmenter::Segmenter *segmenter;
    load_segmenter("/home/huangjingwen/work/news-content/mod_content/mod_segment/conf/qsegconf.ini", &segmenter);
    //load_segmenter("./qsegconf.ini", &segmenter);

    string line;
    while (getline(input, line)) {
        string tag = regex_search(&tag_regex, 2, line);
        string title = regex_search(&title_regex, 2, line);
        string url = regex_search(&url_regex, 2, line);
        string content = regex_search(&content_regex, 2, line);
        content = regex_replace(&image_regex, " ", content);
        content = regex_replace(&br_regex, " ", content);

        if (content.size() > 6000)
            content = content.substr(0, 6000);

        // parse label from tag
        string::size_type spliter_index = tag.rfind("|");
        if (spliter_index != string::npos) {
            tag = string(tag, spliter_index + 1, tag.size());
        }
        if (global_dict.sub_parent_map.find(tag) == global_dict.sub_parent_map.end())
            throw runtime_error(string("error: tag " + tag + " not found in sub_parent_map"));
        int label = global_dict.tag_label_map[global_dict.sub_parent_map[tag]];

        vector<string> content_seg_vec, title_seg_vec;
        segment(segmenter, content, content_seg_vec);
        segment(segmenter, title, title_seg_vec);

        map<string, int> title_reduce_map, content_reduce_map;
        reduce_word_count(title_seg_vec, title_reduce_map, 10);
        reduce_word_count(content_seg_vec, content_reduce_map, 1);

        map<int, double> index_value_map;
        //compile_tfidf_feature(&global_dict, url, title_reduce_map, content_reduce_map, index_value_map);
        compile_bm25_feature(&global_dict, url, title_reduce_map, content_reduce_map, index_value_map);

        url_vec.push_back(url);
        sparse_matrix.push_back(index_value_map);
        label_vec.push_back(label);
    }
    assert(url_vec.size() == sparse_matrix.get_nindptr());
    assert(label_vec.size() == sparse_matrix.get_nindptr());

    regex_free(&tag_regex);
    regex_free(&title_regex);
    regex_free(&url_regex);
    regex_free(&content_regex);
    regex_free(&image_regex);
    regex_free(&br_regex);
}

void load_data_file(const char *data_file, GlobalDict &global_dict, vector<string> &url_vec, vector<vector<string> > &title_vec, vector<vector<string> > &content_vec, vector<string> &tag_vec)
{
    ifstream input(data_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(data_file));

    regex_t tag_regex = compile_regex("(<LBL>)(.*)(</LBL>)");
    regex_t title_regex = compile_regex("(<TITLE>)(.*)(</TITLE>)");
    regex_t url_regex = compile_regex("(<URL>)(.*)(</URL>)");
    regex_t content_regex = compile_regex("(<CONTENT>)(.*)(</CONTENT>)");
    regex_t image_regex = compile_regex("\\[img\\][^\\[]*\\[/img\\]");
    regex_t br_regex = compile_regex("\\[br\\]");

    qss::segmenter::Segmenter *segmenter;
    load_segmenter("/home/huangjingwen/work/news-content/mod_content/mod_segment/conf/qsegconf.ini", &segmenter);
    //load_segmenter("./qsegconf.ini", &segmenter);

    string line;
    while (getline(input, line)) {
        string tag = regex_search(&tag_regex, 2, line);
        string title = regex_search(&title_regex, 2, line);
        string url = regex_search(&url_regex, 2, line);
        string content = regex_search(&content_regex, 2, line);
        content = regex_replace(&image_regex, " ", content);
        content = regex_replace(&br_regex, " ", content);

        if (content.size() > 6000)
            content = content.substr(0, 6000);

        // parse tag
        string::size_type spliter_index = tag.rfind("|");
        if (spliter_index != string::npos) {
            tag = string(tag, spliter_index + 1, tag.size());
        }
        tag = global_dict.sub_parent_map[tag];

        vector<string> title_seg_vec;
        segment(segmenter, title, title_seg_vec);

        vector<string> content_seg_vec;
        segment(segmenter, content, content_seg_vec);

        url_vec.push_back(url);
        title_vec.push_back(title_seg_vec);
        content_vec.push_back(content_seg_vec);
        tag_vec.push_back(tag);
    }
    
    regex_free(&tag_regex);
    regex_free(&title_regex);
    regex_free(&url_regex);
    regex_free(&content_regex);
    regex_free(&image_regex);
    regex_free(&br_regex);
}

void load_segmenter(const char *conf_file, qss::segmenter::Segmenter **segmenter)
{
    qss::segmenter::Config::get_instance()->init(conf_file);
    *segmenter = qss::segmenter::CreateSegmenter();
    if (!segmenter)
        throw runtime_error("error: loading segmenter failed");
}

void segment(qss::segmenter::Segmenter *segmenter, const string &line, vector<string> &seg_res)
{
    int buffer_size = line.size() * 2;
    char *buffer = (char *) malloc(sizeof(char) * buffer_size);
    int res_size = segmenter->segmentUtf8(line.c_str(), line.size(), buffer, buffer_size);
    
    stringstream ss(string(buffer, res_size));
    for (string token; ss >> token; seg_res.push_back(token)) ;
    
    free(buffer);
}

int get_string_length(const string &src_string) {
    const char *str = src_string.c_str();
    size_t str_len = src_string.size();
    if(str == NULL || str_len <=0) {   
        return -1; 
    }   
    
    int count = 0;
    size_t index = 0;
    while (index <= str_len && str[index])
        {   
            count++;
            if ((str[index] & 0xF0) == 0xF0)
                index += 4;
            else if ((str[index] & 0xE0) == 0xE0)
                index += 3;
            else if ((str[index] & 0xC0) == 0xC0)
                index += 2;
            else if ((str[index] & 0x80) == 0)
                index++;
            else
                return -1;//malformed UTF-8
        }   
    return count;
}

void normalize(map<string, double> &feature_value_map)
{
    double feature_norm = 0;
    for (map<string, double>::const_iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
        feature_norm += iter->second * iter->second;
    feature_norm = sqrt(feature_norm);
    for (map<string, double>::iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
        iter->second /= feature_norm;
}

void reduce_word_count(vector<string> &key_vec, map<string, int> &key_count_map, int weight)
{
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        if (get_string_length(*iter) > 1) // remove single word
            key_count_map[*iter] += weight;
}

string parse_netloc(const string &url)
{
    regex_t netloc_regex = compile_regex("(//)([^/]*)(/)?");
    string netloc = regex_search(&netloc_regex, 2, url);
    regex_free(&netloc_regex);
    return netloc;
}

void parse_pred(const float *confidence_array, int label_count, int *pred, float *confidence)
{
    assert(label_count > 1);

    *pred = 0;
    for (size_t offset = 0; offset < label_count; ++offset)
        if (*(confidence_array + offset) > *(confidence_array + *pred))
            *pred = offset;
    *confidence = *(confidence_array + *pred);
}

void parse_pred(const float *confidence_array, int label_count, vector<pair<int, pair<float, float> > > &predict_vector) {
    assert(label_count > 1);

    float base = 0;
    for (size_t offset = 0; offset < label_count; ++offset)
        base += exp(*(confidence_array + offset));
    assert(base > 0);

    assert(predict_vector.size() == 0);
    for (size_t offset = 0; offset < label_count; ++offset)
        predict_vector.push_back(make_pair(offset, make_pair(*(confidence_array + offset), exp(*(confidence_array + offset)) / base)));

    sort(predict_vector.begin(), predict_vector.end(), compare);
}

bool compare(const pair<int, pair<float, float> > &left, const pair<int, pair<float, float> > &right) {
    return left.second.first > right.second.first;
}

regex_t compile_regex(const char *pattern)
{
    regex_t regex;
    int error_code = regcomp(&regex, pattern, REG_EXTENDED);
    if (error_code != 0) {
        size_t length = regerror(error_code, &regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, &regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to compile regex '") + pattern + "', message: " + error_message);
    }
    return regex;
}

string regex_search(const regex_t *regex, int field, const string &line)
{
    regmatch_t match_res[field + 1];
    int error_code = regexec(regex, line.c_str(), field + 1, match_res, 0);
    if (error_code != 0) {
        size_t length = regerror(error_code, regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
    }
    return string(line, match_res[field].rm_so, match_res[field].rm_eo - match_res[field].rm_so);
}

string regex_replace(const regex_t *regex, const string &sub_string, const string &ori_string)
{
    regmatch_t match_res;
    int error_code;
    string res_string = ori_string;
    while ((error_code = regexec(regex, res_string.c_str(), 1, &match_res, 0)) != REG_NOMATCH) {
        if (error_code != 0) {
            size_t length = regerror(error_code, regex, NULL, 0);
            char *buffer = (char *) malloc(sizeof(char) * length);
            (void) regerror(error_code, regex, buffer, length);
            string error_message = string(buffer);
            free(buffer);
            throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
        }
        res_string = string(res_string, 0, match_res.rm_so) + sub_string + string(res_string, match_res.rm_eo, res_string.size() - match_res.rm_eo);
    }
    return res_string;
}

void regex_free(regex_t *regex)
{
    regfree(regex);
}
