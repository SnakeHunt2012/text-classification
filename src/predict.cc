#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include <argp.h>

#include "xgboost/c_api.h"

#include "common.h"
#include "global_dict.h"
#include "sparse_matrix.h"

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

    GlobalDict global_dict(arguments.label_file, arguments.template_file, arguments.netloc_file);

    vector<string> url_test;
    SparseMatrix matrix_test;
    vector<int> label_test;
    
    load_data_file(arguments.test_file, global_dict, url_test, matrix_test, label_test);
    DMatrixHandle X_test = load_X(matrix_test);

    BoosterHandle classifier;
    if (XGBoosterLoadModel(classifier, arguments.model_file))
        throw runtime_error("error: XGBoosterLoadModel failed");

    // validate
    unsigned long y_pred_length;
    const float *y_pred, *y_pred_validate;
    unsigned long counter = 0;
    
    if (XGBoosterPredict(classifier, X_test, 0, 0, &y_pred_length, &y_pred))
        throw runtime_error("error: XGBoosterPredict failed");
    for (size_t i = 0; i < y_pred_length; ++i)
        if (label_test[i] == y_pred[i])
            counter += 1;
        else
            cout << "url: " << url_test[i] << "\t" << "label: " << global_dict.label_tag_map[(int) label_test[i]] << "\t" << "y_pred: " << global_dict.label_tag_map[(int) y_pred[i]] << endl;
    cout << "acc on testing set: " << (double) counter / y_pred_length << endl;

    return 0;
}
