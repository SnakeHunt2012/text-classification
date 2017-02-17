#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <assert.h>
#include <math.h>
#include <argp.h>
#include <regex.h>

#include "segmenter.h"
#include "config.h"

#include "common.h"
#include "global_dict.h"
#include "stringUtil.h"

using namespace std;

const char *argp_program_version = "convert-feature 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE NETLOC_FILE"; /* A description of the arguments we accept. */

/* Keys for options without short-options. */
#define OPT_DEBUG       1
#define OPT_PROFILE     2

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",     'v',             0,             0, "produce verbose output"},
    {"quite",       'q',             0,             0, "don't produce any output"},
    {"silent",      's',             0,             OPTION_ALIAS},

    {0,0,0,0, "The following options are about algorithm:" },
    {0,0,0,0, "The following options are about output format:" },
    
    {"output",      'o',             "FILE",        0, "output information to FILE instead of standard output"},
    {"debug",       OPT_DEBUG,       0,             0, "output debug information"},
    {"profile",     OPT_PROFILE,     0,             0, "output profile information"},
    
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
    char *label_file, *template_file, *netloc_file, *corpus_file, *feature_file, *output_file;
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
        case OPT_DEBUG:
            arguments->debug = true;
            break;
        case OPT_PROFILE:
            arguments->profile = true;
            break;
            
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) arguments->label_file = arg;
            if (state->arg_num == 1) arguments->template_file = arg;
            if (state->arg_num == 2) arguments->netloc_file = arg;
            if (state->arg_num >= 3) argp_usage(state);
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

int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.netloc_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);
    
    regex_t tag_regex = compile_regex("(<LBL>)(.*)(</LBL>)");
    regex_t title_regex = compile_regex("(<TITLE>)(.*)(</TITLE>)");
    regex_t url_regex = compile_regex("(<URL>)(.*)(</URL>)");
    regex_t content_regex = compile_regex("(<CONTENT>)(.*)(</CONTENT>)");
    regex_t image_regex = compile_regex("\\[img\\][^\\[]*\\[/img\\]");
    regex_t br_regex = compile_regex("\\[br\\]");
    
    qss::segmenter::Segmenter *segmenter;
    load_segmenter("./qsegconf.ini", &segmenter);
    
    string line;
    while (getline(cin, line)) {
        string tag = regex_search(&tag_regex, 2, line);
        string url = regex_search(&url_regex, 2, line);
        string title = regex_search(&title_regex, 2, line);
        string content = regex_search(&content_regex, 2, line);
        content = regex_replace(&image_regex, " ", content);
        content = regex_replace(&br_regex, " ", content);

        vector<string> title_seg_vec, content_seg_vec;
        segment(segmenter, title, title_seg_vec);
        segment(segmenter, content, content_seg_vec);

        map<string, int> title_reduce_map, content_reduce_map;
        reduce_word_count(title_seg_vec, title_reduce_map);
        reduce_word_count(content_seg_vec, content_reduce_map);

        //map<int, double> title_index_value_map, content_index_value_map;
        //compile_tfidf_feature(&global_dict, title_reduce_map, title_index_value_map);
        //compile_tfidf_feature(&global_dict, content_reduce_map, content_index_value_map);

        //map<string, int> word_reduce_map;
        //for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
        //    word_reduce_map[iter->first] += iter->second;
        //for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
        //    word_reduce_map[iter->first] += iter->second;

        map<int, double> index_value_map;
        compile_bm25_feature_merge_without_url(&global_dict, url, title_reduce_map, content_reduce_map, index_value_map);

        cout << global_dict.tag_label_map[tag] + 1;
        for (map<int, double>::const_iterator iter = index_value_map.begin(); iter != index_value_map.end(); ++iter)
            cout << " " << iter->first + 1 << ":" << iter->second;
        cout << endl;
    }

    regex_free(&tag_regex);
    regex_free(&url_regex);
    regex_free(&title_regex);
    regex_free(&content_regex);
    
    return 0;
}

