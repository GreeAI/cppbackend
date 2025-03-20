#pragma once
#include "database.h"

namespace request_handler{

using namespace db;

class RequestManager{
public:
    RequestManager(const char* db_uri)
    : db_(db_uri){}

    ResultCode ManageQuery(std::string_view query){
        json::value json_query = ParseQuery(query);

        std::string action = json_query.at("action").as_string().c_str();
        ResultCode return_code;
        if(action == "add_book"){
            return_code = db_.AddBook(json_query.at("payload"));
        } else if(action == "all_books"){
            return_code = db_.AllBooks();
        } else if(action == "exit"){
            return_code = ResultCode::EXIT;
        } 
        
        return return_code;
    }
private:
    json::value ParseQuery(std::string_view query){
        try{
            return json::parse(json::string(
                                query.data(), 
                                query.size())
                                );
        } catch(...){
        }
        return {};
    }  
    
    DatabaseManager db_;
};

} // namespace request_handler