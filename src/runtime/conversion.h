#include <vector>
#include <string>
#include <exception>


OBJ import_string(std::string &str);
OBJ import_date(int64 days_since_epoch);
OBJ import_time(int64 nanosecs_since_epoch);
OBJ import_obj(std::string &str);
OBJ import_symbol(std::string &str);
OBJ import_date(std::string &str);
OBJ import_time(std::string &str);

OBJ import_byte_array(std::vector<unsigned char> &vector);
OBJ import_int_array(std::vector<int> &vector);

////////////////////////////////////////////////////////////////////////////////

std::string export_as_text(OBJ);

std::string export_symbol(OBJ);
std::string export_date(OBJ);
std::string export_time(OBJ);
std::string export_string(OBJ);

std::vector<bool>           export_bool_vector(OBJ obj);
std::vector<unsigned char>  export_byte_vector(OBJ obj);
std::vector<int>            export_int32_vector(OBJ obj);
std::vector<long long>      export_int64_vector(OBJ obj);
std::vector<double>         export_double_vector(OBJ obj);
std::vector<std::string>    export_string_vector(OBJ obj);
std::vector<std::string>    export_text_vector(OBJ obj);
