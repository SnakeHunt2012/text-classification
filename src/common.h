#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include <vector>
#include <utility>
#include <map>

#include <regex.h>

#include "segmenter.h"
#include "config.h"
#include "transcode.h"
#include "xgboost/c_api.h"

#include "global_dict.h"
#include "sparse_matrix.h"

regex_t compile_regex(const char *);
std::string regex_search(const regex_t *, int, const std::string &);
std::string regex_replace(const regex_t *, const std::string &, const std::string &);
void regex_free(regex_t *);

void load_segmenter(const char *, qss::segmenter::Segmenter **);
void segment(qss::segmenter::Segmenter *, const std::string &, std::vector<std::string> &);

void normalize(std::map<std::string, double> &);
void reduce_word_count(std::vector<std::string> &, std::map<std::string, int> &, int = 1);
std::string parse_netloc(const std::string &);
void parse_pred(const float *, int, int *, float *);
void parse_pred(const float *, int, std::vector<std::pair<int, std::pair<float, float> > > &);
bool compare(const std::pair<int, std::pair<float, float> > &, const std::pair<int, std::pair<float, float> > &);
void compile_tfidf_feature(void *, const std::string &, const std::map<std::string, int> &, const std::map<std::string, int> &, std::map<int, double> &);
void compile_tfidf_feature(void *, const std::map<std::string, int> &, std::map<int, double> &);
void compile_bm25_feature(void *, const std::string &, const std::map<std::string, int> &, const std::map<std::string, int> &, std::map<int, double> &);

void load_data_file(const char *, GlobalDict &, std::vector<std::string> &, SparseMatrix &, std::vector<int> &);
void load_data_file(const char *, GlobalDict &, std::vector<std::string> &, std::vector<std::vector<std::string> > &, std::vector<std::vector<std::string> > &, std::vector<std::string> &);
DMatrixHandle load_X(SparseMatrix &);
float *load_y(std::vector<int> &);

#endif  // COMMON_H_
