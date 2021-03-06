#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include <assert.h>
#include <argp.h>
#include <time.h>

#include "xgboost/c_api.h"

#include "common.h"
#include "global_dict.h"
#include "sparse_matrix.h"
#include "classifier.h"

using namespace std;

const char *argp_program_version = "feature 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE MODEL_FILE TEST_FILE"; /* A description of the arguments we accept. */

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
    char *label_file, *template_file, *model_file, *test_file, *netloc_file, *output_file;
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
            if (state->arg_num == 2) arguments->model_file = arg;
            if (state->arg_num == 3) arguments->test_file = arg;
            if (state->arg_num >= 4) argp_usage(state);
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

void predict(struct arguments &arguments, map<string, pair<string, string> > &url_tag_map, map<string, pair<string, float> > &url_pred_map, map<string, vector<string> > &url_title_map);
void predict(struct arguments &arguments, map<string, pair<string, string> > &url_tag_map, map<string, vector<pair<string, pair<float, float> > > > &url_res_map, map<string, vector<string> > &url_title_map);

int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.model_file = NULL;
    arguments.test_file = NULL;
    arguments.netloc_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    //map<string, pair<string, string> > url_tag_map;
    //map<string, pair<string, float> > url_pred_map;
    //map<string, vector<string> > url_title_map;
    //predict(arguments, url_tag_map, url_pred_map, url_title_map);
    //
    //int true_counter = 0, false_counter = 0;
    //for (map<string, pair<string, string> >::const_iterator iter = url_tag_map.begin(); iter != url_tag_map.end(); ++iter) {
    //    string url = iter->first;
    //    string tag = iter->second.first;
    //    string pred = iter->second.second;
    //    float proba = url_pred_map[url].second;
    //    vector<string> title_seg_vec = url_title_map[url];
    //    if (tag != pred && proba > -100.0) {
    //         ++false_counter;
    //    } else
    //        ++true_counter;
    //    cout << iter->first << "\t";
    //    for (vector<string>::const_iterator title_seg_iter = title_seg_vec.begin(); title_seg_iter != title_seg_vec.end(); ++title_seg_iter)
    //        cout << *title_seg_iter;
    //    cout << "\t" << tag << "\t" << pred << "\t" << proba << endl;;
    //}
    //
    //cout << "acc: " << (double) true_counter / (true_counter + false_counter) << endl;
    
    map<string, vector<string> > url_title_map;
    map<string, pair<string, string> > url_tag_map;
    map<string, vector<pair<string, pair<float, float> > > > url_res_map;
    predict(arguments, url_tag_map, url_res_map, url_title_map);
    
    int true_counter = 0, false_counter = 0;
    for (map<string, vector<pair<string, pair<float, float> > > >::const_iterator iter = url_res_map.begin(); iter != url_res_map.end(); ++iter) {
        string url = iter->first;
        string tag = url_tag_map[url].first;
        string pred = iter->second[0].first;
        float probability = iter->second[0].second.first;
        float confidence = iter->second[0].second.second;
        vector<string> title_seg_vec = url_title_map[url];
        if (tag == pred)
             ++true_counter;
        else
            ++false_counter;
        cout << url << "\t";
        for (vector<string>::const_iterator title_seg_iter = title_seg_vec.begin(); title_seg_iter != title_seg_vec.end(); ++title_seg_iter)
            cout << *title_seg_iter;
        cout << "\t" << tag << "\t" << pred << "\t" << probability << "\t" << confidence;
        cout << "\t### info ###\t";
        for (size_t offset = 0; offset < 10; ++offset)
            cout << iter->second[offset].first << ":" << iter->second[offset].second.first << ":" << iter->second[offset].second.second << " ";
        cout << endl;
    }
    
    cout << "acc: " << (double) true_counter / (true_counter + false_counter) << endl;
    
    return 0;
}

void predict(struct arguments &arguments, map<string, pair<string, string> > &url_tag_map, map<string, pair<string, float> > &url_pred_map, map<string, vector<string> > &url_title_map)
{
    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);
    Classifier classifier(arguments.label_file, arguments.template_file, arguments.netloc_file, arguments.model_file);

    vector<string> url_vec, tag_vec;
    vector<vector<string> > title_vec, content_vec;
    load_data_file(arguments.test_file, global_dict, url_vec, title_vec, content_vec, tag_vec);

    assert(url_vec.size() == tag_vec.size());
    assert(url_vec.size() == title_vec.size());
    assert(url_vec.size() == content_vec.size());
    assert(url_vec.size() == tag_vec.size());

    clock_t start = clock();
    for (size_t i = 0; i < url_vec.size(); ++i) {
        map<string, int> title_reduce_map, content_reduce_map;
        reduce_word_count(title_vec[i], title_reduce_map, 10);
        reduce_word_count(content_vec[i], content_reduce_map, 1);

        string tag;
        float proba;
        classifier.classify(url_vec[i], title_reduce_map, content_reduce_map, tag, &proba);

        url_tag_map[url_vec[i]] = make_pair(tag_vec[i], tag);
        url_pred_map[url_vec[i]] = make_pair(tag, proba);
        url_title_map[url_vec[i]] = title_vec[i];
    }
    cout << (clock() - start) / CLOCKS_PER_SEC << endl;
}

void predict(struct arguments &arguments, map<string, pair<string, string> > &url_tag_map, map<string, vector<pair<string, pair<float, float> > > > &url_res_map, map<string, vector<string> > &url_title_map)
{
    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);
    Classifier classifier(arguments.label_file, arguments.template_file, arguments.netloc_file, arguments.model_file);

    vector<string> url_vec, tag_vec;
    vector<vector<string> > title_vec, content_vec;
    load_data_file(arguments.test_file, global_dict, url_vec, title_vec, content_vec, tag_vec);

    assert(url_vec.size() == tag_vec.size());
    assert(url_vec.size() == title_vec.size());
    assert(url_vec.size() == content_vec.size());
    assert(url_vec.size() == tag_vec.size());

    clock_t start = clock();
    for (size_t i = 0; i < url_vec.size(); ++i) {
        map<string, int> title_reduce_map, content_reduce_map;
        reduce_word_count(title_vec[i], title_reduce_map, 10);
        reduce_word_count(content_vec[i], content_reduce_map, 1);

        vector<pair<string, pair<float, float> > > res_vec;
        classifier.classify(url_vec[i], title_reduce_map, content_reduce_map, res_vec);

        url_tag_map[url_vec[i]] = make_pair(tag_vec[i], res_vec[0].first);
        url_res_map[url_vec[i]] = res_vec;
        url_title_map[url_vec[i]] = title_vec[i];
    }
    cout << (clock() - start) / CLOCKS_PER_SEC << endl;
}
