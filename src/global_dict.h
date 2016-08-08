#ifndef GLOBAL_DICT_H_
#define GLOBAL_DICT_H_

#include <string>
#include <vector>
#include <map>

struct GlobalDict {
    
    GlobalDict(const char *, const char *, const char *);
    ~GlobalDict() {}

    void load_label_file(const char *);
    void load_template_file(const char *);
    void load_netloc_file(const char *);
    
    std::map<std::string, std::string> sub_parent_map;
    std::map<std::string, int> tag_label_map;
    std::map<int, std::string> label_tag_map;
    
    std::map<std::string, double> word_idf_map;
    std::map<std::string, int> word_index_map; 
    std::map<int, std::string> index_word_map;

    std::map<std::string, int> netloc_index_map;
    std::map<int, std::string> index_netloc_map;
};

#endif  // GLOBAL_DICT_H_
