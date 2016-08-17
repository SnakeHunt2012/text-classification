#ifndef GLOBAL_DICT_H_
#define GLOBAL_DICT_H_

#include <string>
#include <vector>
#include <map>

class GlobalDict {
public:
    
    GlobalDict(const char *, const char *, const char *);
    ~GlobalDict() {}

    int get_label_count(void) const { return label_count; }
    int get_word_count(void) const { return word_count; }
    int get_netloc_count(void) const { return netloc_count; }
    
    std::map<std::string, std::string> sub_parent_map;
    std::map<std::string, int> tag_label_map;
    std::map<int, std::string> label_tag_map;
    
    std::map<std::string, double> word_idf_map;
    std::map<std::string, int> word_index_map; 
    std::map<int, std::string> index_word_map;

    std::map<std::string, int> netloc_index_map;
    std::map<int, std::string> index_netloc_map;

private:

    void load_label_file(const char *);
    void load_template_file(const char *);
    void load_netloc_file(const char *);

    int label_count;
    int word_count;
    int netloc_count;
};

#endif  // GLOBAL_DICT_H_
