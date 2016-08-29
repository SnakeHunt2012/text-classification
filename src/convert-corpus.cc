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

const char *argp_program_version = "convert-corpus 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE NETLOC_FILE FROM_CORPUS_FILE TO_CORPUS_FILE"; /* A description of the arguments we accept. */

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
    char *label_file, *template_file, *netloc_file, *from_corpus_file, *to_corpus_file, *output_file;
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
            if (state->arg_num == 3) arguments->from_corpus_file = arg;
            if (state->arg_num == 4) arguments->to_corpus_file = arg;
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
    arguments.from_corpus_file = NULL;
    arguments.to_corpus_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);
    
    vector<string> url_vec, tag_vec;
    vector<vector<string> > title_vec, content_vec;
    load_data_file(arguments.from_corpus_file, global_dict, url_vec, title_vec, content_vec, tag_vec);

    assert(url_vec.size() == tag_vec.size());
    assert(url_vec.size() == title_vec.size());
    assert(url_vec.size() == content_vec.size());
    assert(url_vec.size() == tag_vec.size());

    ofstream file;
    file.open(arguments.to_corpus_file);
    for (size_t i = 0; i < url_vec.size(); ++i) {
        for (vector<string>::const_iterator iter = title_vec[i].begin(); iter != title_vec[i].end(); ++iter)
            file << *iter << " ";
        for (vector<string>::const_iterator iter = content_vec[i].begin(); iter != content_vec[i].end(); ++iter)
            file << *iter << " ";
        file << "__label__" << tag_vec[i] << endl;
    }
    file.close();

    return 0;
}
