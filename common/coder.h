// #include "string"
#include <vector>
#include "global_common.h"

// using namespace std;

typedef std::pair<std::string, std::string> encoded_record;

std::string encode_vec(const std::vector<std::pair<std::string,std::string> > &data);
std::vector<std::pair<std::string, std::string> > decode_vec(const std::string &e);
