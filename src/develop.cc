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

int main(int argc, char *argv[])
{
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
    XGBoosterSetParam(classifier, "nthread", "5");
    
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
    for (int iter = 0; iter < 500; ++iter) {
        cout << "iter: " << iter << endl;
        XGBoosterUpdateOneIter(classifier, iter, X_train);
    }

    // validate
    unsigned long y_pred_length_train, y_pred_length_validate;
    const float *y_pred_train, *y_pred_validate;
    unsigned long counter;
    
    if (XGBoosterPredict(classifier, X_train, 0, 0, &y_pred_length_train, &y_pred_train))
        throw runtime_error("error: XGBoosterPredict failed");
    counter = 0;
    for (size_t i = 0; i < y_pred_length_train; ++i)
        if (label_train[i] == y_pred_train[i])
            counter += 1;
        else
            cout << "url: " << url_train[i] << "\t" << "label: " << global_dict.label_tag_map[(int) label_train[i]] << "\t" << "y_pred_train: " << global_dict.label_tag_map[(int) y_pred_train[i]] << endl;
    cout << "acc on training set: " << (double) counter / y_pred_length_train << endl;

    counter = 0;
    if (XGBoosterPredict(classifier, X_validate, 0, 0, &y_pred_length_validate, &y_pred_validate))
        throw runtime_error("error: XGBoosterPredict failed");
    for (size_t i = 0; i < y_pred_length_validate; ++i)
        if (label_validate[i] == y_pred_validate[i])
            counter += 1;
        else
            cout << "url: " << url_validate[i] << "\t" << "label: " << global_dict.label_tag_map[(int) label_validate[i]] << "\t" << "y_pred_validate: " << global_dict.label_tag_map[(int) y_pred_validate[i]] << endl;
    cout << "acc on validation set: " << (double) counter / y_pred_length_validate << endl;

    // dump model
    if (XGBoosterSaveModel(classifier, arguments.model_file))
        throw runtime_error("error: dumping model failed");

    return 0;
}
