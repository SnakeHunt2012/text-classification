#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include <stdlib.h>
#include <math.h>
#include <argp.h>
#include <regex.h>

#include "config.h"
#include "segmenter.h"
#include "xgboost/c_api.h"

#include "global_dict.h"
#include "sparse_matrix.h"

using namespace std;

const char *argp_program_version = "feature 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE TRAIN_FILE VALIDATE_FILE MODEL_FILE"; /* A description of the arguments we accept. */

/* Keys for options without short-options. */
#define OPT_DEBUG       1
#define OPT_PROFILE     2
#define OPT_NETLOC_FILE 3

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",     'v',             0,             0, "produce verbose output"},
    {"quite",       'q',             0,             0, "don't produce any output"},
    {"silent",      's',             0,             OPTION_ALIAS},

    {0,0,0,0, "The following options are about algorithm:" },
    
    {"netloc-file", OPT_NETLOC_FILE, "NETLOC_FILE", 0, "netloc dict file in json format"},

    {0,0,0,0, "The following options are about output format:" },
    
    {"output",      'o',             "FILE",        0, "output information to FILE instead of standard output"},
    {"debug",       OPT_DEBUG,       0,             0, "output debug information"},
    {"profile",     OPT_PROFILE,     0,             0, "output profile information"},
    
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
    char *label_file, *template_file, *train_file, *validate_file, *model_file, *netloc_file, *output_file;
    bool verbose, silent, debug, profile;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments *) state->input;
    switch (key)
        {
        case 'v':
            arguments->verbose = true;
            break;
        case 'q': case 's':
            arguments->silent = true;
            break;
        case 'o':
            arguments->output_file = arg;
            break;
        case OPT_NETLOC_FILE:
            arguments->netloc_file = arg;
            break;
        case OPT_DEBUG:
            arguments->debug = true;
            break;
        case OPT_PROFILE:
            arguments->profile = true;
            break;
            
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) arguments->label_file = arg;
            if (state->arg_num == 1) arguments->template_file = arg;
            if (state->arg_num == 2) arguments->train_file = arg;
            if (state->arg_num == 3) arguments->validate_file = arg;
            if (state->arg_num == 4) arguments->model_file = arg;
            if (state->arg_num >= 5) argp_usage(state);
            break;
            
        case ARGP_KEY_END:
            if (state->arg_num < 2) argp_usage(state);
            break;
            
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;
            
        default:
            return ARGP_ERR_UNKNOWN;
        }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, prog_doc };

regex_t compile_regex(const char *);
string regex_search(const regex_t *, int, const string &);
string regex_replace(const regex_t *, const string &, const string &);

void load_segmenter(const char *, qss::segmenter::Segmenter **);
void segment(qss::segmenter::Segmenter *, const string &, vector<string> &);
void normalize(map<string, double> &);

void load_data_file(const char *, GlobalDict &, vector<string> &, SparseMatrix &, vector<int> &);
DMatrixHandle load_X(SparseMatrix &);
float *load_y(vector<int> &);

int main(int argc, char *argv[])
{
    char *label_file, *template_file, *train_file, *validate_file, *data_file, *netloc_file, *output_file;
    bool verbose, silent, debug, profile;
    
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.train_file = NULL;
    arguments.validate_file = NULL;
    arguments.model_file = NULL;
    arguments.netloc_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);

    vector<string> url_train, url_validate;
    SparseMatrix matrix_train, matrix_validate;
    vector<int> label_train, label_validate;
    
    load_data_file(arguments.train_file, global_dict, url_train, matrix_train, label_train);
    load_data_file(arguments.validate_file, global_dict, url_validate, matrix_validate, label_validate);

    DMatrixHandle X_train = load_X(matrix_train), X_validate = load_X(matrix_validate);
    float *y_train = load_y(label_train), *y_validate = load_y(label_validate);

    if (XGDMatrixSetFloatInfo(X_train, "label", y_train, matrix_train.get_nindptr()))
        throw runtime_error("error: XGDMatrixSetUIntInfo failed");

    BoosterHandle classifier;
    XGBoosterCreate(&X_train, 1, &classifier);
    
    // general parameters
    XGBoosterSetParam(classifier, "booster", "gbtree");
    XGBoosterSetParam(classifier, "silent", "0");
    XGBoosterSetParam(classifier, "nthread", "12");
    
    // booster parameters
    XGBoosterSetParam(classifier, "eta", "0.3");
    XGBoosterSetParam(classifier, "min_child_weight", "1");
    //XGBoosterSetParam(classifier, "max_depth", "6");         // ignored if define max_leaf_nodes
    XGBoosterSetParam(classifier, "max_leaf_nodes", "10");   // ignore max_depth
    XGBoosterSetParam(classifier, "gamma", "0");
    XGBoosterSetParam(classifier, "max_delta_step", "0");    // usually not needed
    XGBoosterSetParam(classifier, "sub_sample", "1");        // the fraction of observations to be randomly samples for each tree
    XGBoosterSetParam(classifier, "colsample_bytree", "1");  // the fraction of columns to be randomly samples for each tree
    XGBoosterSetParam(classifier, "colsample_bylevel", "1"); // the subsample ratio of columns for each split, in each level
    XGBoosterSetParam(classifier, "lambda", "1");  // L1 regularization weight, many data scientists don't use it often
    XGBoosterSetParam(classifier, "alpha", "0");   // L2 regularization weight, used in case of very high dimensionality so that the algorithm runs faster
    XGBoosterSetParam(classifier, "scale_pos_weight", "1");  // a value greater than 0 should be used in case of high class imbalance as it helps in faster convergence

    // learning parameters
    XGBoosterSetParam(classifier, "objective", "multi:softmax");
    XGBoosterSetParam(classifier, "num_class", "25");
    //XGBoosterSetParam(classifier, "eval_metric", "merror"); // default according to objective
    XGBoosterSetParam(classifier, "seed", "0");

    // train
    for (int iter = 0; iter < 100; ++iter) {
        cout << "iter: " << iter << endl;
        XGBoosterUpdateOneIter(classifier, iter, X_train);
    }

    // validate
    unsigned long y_pred_length;
    const float *y_pred;
    if (XGBoosterPredict(classifier, X_validate, 0, 0, &y_pred_length, &y_pred))
        throw runtime_error("error: XGBoosterPredict failed");
    cout << "y_validate_length: " << matrix_validate.get_nindptr() << "\t" << "y_pred_length: " << y_pred_length << endl;
    unsigned long counter = 0;
    for (size_t i = 0; i < y_pred_length; ++i) {
        cout << "url: " << url_validate[i] << "\t" << "label: " << label_validate[i] << "\t" << "y_pred: " << y_pred[i] << endl;
        if (label_validate[i] == y_pred[i])
            counter += 1;
    }
    cout << "y_pred_length: "<< y_pred_length << "\t" << "counter: " << "\t" << counter << endl;

    return 0;
}

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
    load_segmenter("./seg/conf/qsegconf.ini", &segmenter);

    string line;
    while (getline(input, line)) {
        string tag = regex_search(&tag_regex, 2, line);
        string title = regex_search(&title_regex, 2, line);
        string url = regex_search(&url_regex, 2, line);
        string content = regex_search(&content_regex, 2, line);
        content = regex_replace(&image_regex, " ", content);
        content = regex_replace(&br_regex, " ", content);

        // parse label from tag
        string::size_type spliter_index = tag.rfind("|");
        if (spliter_index != string::npos) {
            tag = string(tag, spliter_index + 1, tag.size());
        }
        if (global_dict.sub_parent_map.find(tag) == global_dict.sub_parent_map.end())
            throw runtime_error(string("error: tag " + tag + " not found in sub_parent_map"));
        int label = global_dict.tag_label_map[global_dict.sub_parent_map[tag]];

        vector<string> content_seg_vec;
        vector<string> title_seg_vec;
        segment(segmenter, content, content_seg_vec);

        // assemble tf
        int term_count = 0;
        map<string, int> word_tf_map; // term_count_map
        for (vector<string>::const_iterator iter = content_seg_vec.begin(); iter != content_seg_vec.end(); ++iter) {
            map<string, int>::iterator word_tf_iter;
            if ((word_tf_iter = word_tf_map.find(*iter)) == word_tf_map.end())
                word_tf_iter = word_tf_map.insert(word_tf_iter, map<string, int>::value_type(*iter, 0));
            word_tf_iter->second += 1;
            //word_tf_map[*iter] += 1;
            term_count += 1;
        }
        for (vector<string>::const_iterator iter = title_seg_vec.begin(); iter != title_seg_vec.end(); ++iter) {
            map<string, int>::iterator word_tf_iter;
            if ((word_tf_iter = word_tf_map.find(*iter)) == word_tf_map.end())
                word_tf_iter = word_tf_map.insert(word_tf_iter, map<string, int>::value_type(*iter, 0));
            word_tf_iter->second += 10;
            //word_tf_map[*iter] += 10;
            term_count += 10;
        }

        // assemble feature (feature -> value)
        map<string, double> feature_value_map;
        for (map<string, int>::const_iterator iter = word_tf_map.begin(); iter != word_tf_map.end(); ++iter) {
            const string &word = iter->first;
            const double &tf = (double)iter->second / term_count; // the result is equla to not divided by term_count for final feature values
            map<string, double>::const_iterator word_idf_iter = global_dict.word_idf_map.find(word);
            if (word_idf_iter != global_dict.word_idf_map.end())
                feature_value_map[word] = tf * word_idf_iter->second;
        }
        normalize(feature_value_map);

        // assemble feature (index -> value)
        map<int, double> index_value_map;
        for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
            map<string, int>::const_iterator word_index_iter = global_dict.word_index_map.find(feature_value_iter->first);
            if (word_index_iter != global_dict.word_index_map.end())
                index_value_map[word_index_iter->second] = feature_value_iter->second;
        }

        url_vec.push_back(url);
        sparse_matrix.push_back(index_value_map);
        label_vec.push_back(label);
    }
    assert(url_vec.size() == sparse_matrix.get_nindptr());
    assert(label_vec.size() == sparse_matrix.get_nindptr());
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

void normalize(map<string, double> &feature_value_map)
{
    double feature_norm = 0;
    for (map<string, double>::const_iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
        feature_norm += iter->second * iter->second;
    feature_norm = sqrt(feature_norm);
    for (map<string, double>::iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
        iter->second /= feature_norm;
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
