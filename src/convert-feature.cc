#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <argp.h>
#include <assert.h>

#include "common.h"
#include "global_dict.h"

using namespace std;

const char *argp_program_version = "convert-feature 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE NETLOC_FILE CORPUS_FILE FEATURE_FILE"; /* A description of the arguments we accept. */

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
            if (state->arg_num == 3) arguments->corpus_file = arg;
            if (state->arg_num == 4) arguments->feature_file = arg;
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

int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.netloc_file = NULL;
    arguments.corpus_file = NULL;
    arguments.feature_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);
    
    regex_t url_regex = compile_regex("^([^\t]*)");
    regex_t title_regex = compile_regex("^[^\t]*\t([^\t]*)");
    regex_t btag_regex = compile_regex("^[^\t]*\t[^\t]*\t([^\t]*)");
    regex_t entity_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)");
    regex_t keywords_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)");
    
    ifstream input(arguments.corpus_file);
    ofstream output(arguments.feature_file);

    if (!input) throw runtime_error("error: unable to open input file: " + string(arguments.corpus_file));
    if (!output) throw runtime_error("error: unable to open output file: " + string(arguments.feature_file));

    qss::segmenter::Segmenter *segmenter;
    load_segmenter("/home/huangjingwen/work/news-content/mod_content/mod_segment/conf/qsegconf.ini", &segmenter);
    
    string line;
    while (getline(input, line)) {
        string url = regex_search(&url_regex, 1, line);
        string title = regex_search(&title_regex, 1, line);
        string btag = regex_search(&btag_regex, 1, line);
        string entity = regex_search(&entity_regex, 1, line);
        string keywords = regex_search(&keywords_regex, 1, line);

        vector<string> title_seg_vec;
        segment(segmenter, title, title_seg_vec);

        map<string, int> title_reduce_map;
        reduce_word_count(title_seg_vec, title_reduce_map, 1);

        int term_count = 0;
        for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
            term_count += iter->second;

        map<string, double> feature_value_map;
        for (map<string, int>::const_iterator word_count_iter = title_reduce_map.begin(); word_count_iter != title_reduce_map.end(); ++word_count_iter) {
            const string &word = word_count_iter->first;
            const double tf = (double) word_count_iter->second / term_count;
            map<string, double>::const_iterator word_idf_iter = global_dict.word_idf_map.find(word);
            if (word_idf_iter != global_dict.word_idf_map.end())
                feature_value_map[word] = tf * word_idf_iter->second;
        }
        normalize(feature_value_map);

        map<int, double> index_value_map;
        for (map<string, double>::const_iterator feature_value_iter = feature_value_map.begin(); feature_value_iter != feature_value_map.end(); ++feature_value_iter) {
            map<string, int>::const_iterator word_index_iter = global_dict.word_index_map.find(feature_value_iter->first);
            if (word_index_iter != global_dict.word_index_map.end())
                index_value_map[word_index_iter->second] = feature_value_iter->second;
        }

        output << url << "\t" << title << "\t{ ";
        bool begin_flag = true;
        for (map<int, double>::const_iterator iter = index_value_map.begin(); iter != index_value_map.end(); ++iter) {
            if (begin_flag) {
                begin_flag = false;
            } else {
                output << ", ";
            }
            output << "\"" << iter->first << "\": " << iter->second;
        }
        output << " }\t" << btag << "\t" << entity << "\t" << keywords << endl;
    }

    regex_free(&url_regex);
    regex_free(&title_regex);
    regex_free(&btag_regex);
    regex_free(&entity_regex);
    regex_free(&keywords_regex);
    
    return 0;
}
