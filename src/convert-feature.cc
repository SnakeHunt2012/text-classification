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

//regex_t compile_regex(const char *);
//std::string regex_search(const regex_t *, int, const std::string &);
//std::string regex_replace(const regex_t *, const std::string &, const std::string &);
//void regex_free(regex_t *);

//void load_segmenter(const char *, qss::segmenter::Segmenter **);
//void segment(qss::segmenter::Segmenter *, const std::string &, std::vector<std::string> &);

//void normalize(std::map<std::string, double> &);
//void reduce_word_count(std::vector<std::string> &, std::map<std::string, int> &, int);
//std::string parse_netloc(const std::string &);

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
    
    regex_t url_regex = compile_regex("^([^\t]*)"); // 1
    regex_t title_regex = compile_regex("^[^\t]*\t([^\t]*)"); // 2
    regex_t content_regex = compile_regex("^[^\t]*\t[^\t]*\t([^\t]*)"); // 3
    regex_t btag_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)"); // 4
    regex_t entity_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)"); // 5
    regex_t keywords_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)"); // 6
    regex_t pdate_regex = compile_regex("^[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t[^\t]*\t([^\t]*)"); // 9
    
    qss::segmenter::Segmenter *segmenter;
    load_segmenter("./qsegconf.ini", &segmenter);
    
    string line;
    while (getline(cin, line)) {
        string url, title, content, btag, entity, keywords, pdate;
        try {
            vector<string> splited_line;
            StringUtil::Split2(splited_line, line, '\t', true);
            if (splited_line.size() < 9)
                continue;
            url = splited_line[0];
            title = splited_line[1];
            content = splited_line[2];
            if (content.length() > 10000)
                content = content.substr(0, 10000);
            btag = splited_line[3];
            entity = splited_line[4];
            keywords = splited_line[5];
            pdate = splited_line[8];
            //url = regex_search(&url_regex, 1, line);
            //title = regex_search(&title_regex, 1, line);
            //content = regex_search(&content_regex, 1, line);
            //btag = regex_search(&btag_regex, 1, line);
            //entity = regex_search(&entity_regex, 1, line);
            //keywords = regex_search(&keywords_regex, 1, line);
            //pdate = regex_search(&pdate_regex, 1, line);
        } catch (runtime_error &err) {
            //cout << err.what() << endl;
            continue;
        }

        vector<string> title_seg_vec, content_seg_vec;
        segment(segmenter, title, title_seg_vec);
        segment(segmenter, content, content_seg_vec);

        map<string, int> title_reduce_map, content_reduce_map;
        reduce_word_count(title_seg_vec, title_reduce_map, 1);
        reduce_word_count(content_seg_vec, content_reduce_map, 1);

        int title_term_count = 0, content_term_count = 0;
        for (map<string, int>::const_iterator iter = title_reduce_map.begin(); iter != title_reduce_map.end(); ++iter)
            title_term_count += iter->second;
        for (map<string, int>::const_iterator iter = content_reduce_map.begin(); iter != content_reduce_map.end(); ++iter)
            content_term_count += iter->second;

        map<string, double> title_feature_value_map, content_feature_value_map;
        for (map<string, int>::const_iterator word_count_iter = title_reduce_map.begin(); word_count_iter != title_reduce_map.end(); ++word_count_iter) {
            const string &word = word_count_iter->first;
            const double tf = (double) word_count_iter->second / title_term_count;
            map<string, double>::const_iterator word_idf_iter = global_dict.word_idf_map.find(word);
            if (word_idf_iter != global_dict.word_idf_map.end())
                title_feature_value_map[word] = tf * word_idf_iter->second;
        }
        for (map<string, int>::const_iterator word_count_iter = content_reduce_map.begin(); word_count_iter != content_reduce_map.end(); ++word_count_iter) {
            const string &word = word_count_iter->first;
            const double tf = (double) word_count_iter->second / content_term_count;
            map<string, double>::const_iterator word_idf_iter = global_dict.word_idf_map.find(word);
            if (word_idf_iter != global_dict.word_idf_map.end())
                content_feature_value_map[word] = tf * word_idf_iter->second;
        }
        normalize(title_feature_value_map);
        normalize(content_feature_value_map);

        map<int, double> title_index_value_map, content_index_value_map;
        for (map<string, double>::const_iterator feature_value_iter = title_feature_value_map.begin(); feature_value_iter != title_feature_value_map.end(); ++feature_value_iter) {
            map<string, int>::const_iterator word_index_iter = global_dict.word_index_map.find(feature_value_iter->first);
            if (word_index_iter != global_dict.word_index_map.end())
                title_index_value_map[word_index_iter->second] = feature_value_iter->second;
        }
        for (map<string, double>::const_iterator feature_value_iter = content_feature_value_map.begin(); feature_value_iter != content_feature_value_map.end(); ++feature_value_iter) {
            map<string, int>::const_iterator word_index_iter = global_dict.word_index_map.find(feature_value_iter->first);
            if (word_index_iter != global_dict.word_index_map.end())
                content_index_value_map[word_index_iter->second] = feature_value_iter->second;
        }

        cout << url << "\t" << title << "\t{ ";
        bool begin_flag = true;
        for (map<int, double>::const_iterator iter = title_index_value_map.begin(); iter != title_index_value_map.end(); ++iter) {
            if (begin_flag) {
                begin_flag = false;
            } else {
                cout << ", ";
            }
            cout << "\"" << iter->first << "\": " << iter->second;
        }
        cout << " }\t" << btag << "\t" << entity << "\t" << keywords << "\t" << content << "\t"<< pdate << "\t{";
        begin_flag = true;
        for (map<int, double>::const_iterator iter = content_index_value_map.begin(); iter != content_index_value_map.end(); ++iter) {
            if (begin_flag) {
                begin_flag = false;
            } else {
                cout << ", ";
            }
            cout << "\"" << iter->first << "\": " << iter->second;
        }
        cout << "}" << endl;
    }

    regex_free(&url_regex);
    regex_free(&title_regex);
    regex_free(&content_regex);
    regex_free(&btag_regex);
    regex_free(&entity_regex);
    regex_free(&keywords_regex);
    regex_free(&pdate_regex);
    
    return 0;
}

//void load_segmenter(const char *conf_file, qss::segmenter::Segmenter **segmenter)
//{
//    qss::segmenter::Config::get_instance()->init(conf_file);
//    *segmenter = qss::segmenter::CreateSegmenter();
//    if (!segmenter)
//        throw runtime_error("error: loading segmenter failed");
//}
//
//void segment(qss::segmenter::Segmenter *segmenter, const string &line, vector<string> &seg_res)
//{
//    int buffer_size = line.size() * 2;
//    char *buffer = (char *) malloc(sizeof(char) * buffer_size);
//    int res_size = segmenter->segmentUtf8(line.c_str(), line.size(), buffer, buffer_size);
//    
//    stringstream ss(string(buffer, res_size));
//    for (string token; ss >> token; seg_res.push_back(token)) ;
//    
//    free(buffer);
//}
//
//void normalize(map<string, double> &feature_value_map)
//{
//    double feature_norm = 0;
//    for (map<string, double>::const_iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
//        feature_norm += iter->second * iter->second;
//    feature_norm = sqrt(feature_norm);
//    for (map<string, double>::iterator iter = feature_value_map.begin(); iter != feature_value_map.end(); ++iter)
//        iter->second /= feature_norm;
//}
//
//void reduce_word_count(vector<string> &key_vec, map<string, int> &key_count_map, int weight)
//{
//    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
//        key_count_map[*iter] += weight;
//}
//
//regex_t compile_regex(const char *pattern)
//{
//    regex_t regex;
//    int error_code = regcomp(&regex, pattern, REG_EXTENDED);
//    if (error_code != 0) {
//        size_t length = regerror(error_code, &regex, NULL, 0);
//        char *buffer = (char *) malloc(sizeof(char) * length);
//        (void) regerror(error_code, &regex, buffer, length);
//        string error_message = string(buffer);
//        free(buffer);
//        throw runtime_error(string("error: unable to compile regex '") + pattern + "', message: " + error_message);
//    }
//    return regex;
//}
//
//string regex_search(const regex_t *regex, int field, const string &line)
//{
//    regmatch_t match_res[field + 1];
//    int error_code = regexec(regex, line.c_str(), field + 1, match_res, 0);
//    if (error_code != 0) {
//        size_t length = regerror(error_code, regex, NULL, 0);
//        char *buffer = (char *) malloc(sizeof(char) * length);
//        (void) regerror(error_code, regex, buffer, length);
//        string error_message = string(buffer);
//        free(buffer);
//        throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
//    }
//    return string(line, match_res[field].rm_so, match_res[field].rm_eo - match_res[field].rm_so);
//}
//
//string regex_replace(const regex_t *regex, const string &sub_string, const string &ori_string)
//{
//    regmatch_t match_res;
//    int error_code;
//    string res_string = ori_string;
//    while ((error_code = regexec(regex, res_string.c_str(), 1, &match_res, 0)) != REG_NOMATCH) {
//        if (error_code != 0) {
//            size_t length = regerror(error_code, regex, NULL, 0);
//            char *buffer = (char *) malloc(sizeof(char) * length);
//            (void) regerror(error_code, regex, buffer, length);
//            string error_message = string(buffer);
//            free(buffer);
//            throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
//        }
//        res_string = string(res_string, 0, match_res.rm_so) + sub_string + string(res_string, match_res.rm_eo, res_string.size() - match_res.rm_eo);
//    }
//    return res_string;
//}
//
//void regex_free(regex_t *regex)
//{
//    regfree(regex);
//}
