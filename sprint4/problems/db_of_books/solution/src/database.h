#pragma once

#include <iostream>
#include <optional>
#include <boost/json.hpp>
#include <pqxx/pqxx>

namespace db{

using namespace std::literals;
using pqxx::operator"" _zv;
namespace json = boost::json;

enum class ResultCode{
    ADD_BOOK,
    ALL_BOOKS,
    EXIT
};  

class DatabaseManager{
public:
    constexpr static auto INSERT_BOOK = "insert_book_query"_zv;

    DatabaseManager(std::string_view database_uri)
    : conn_{database_uri.data()}{

        pqxx::work w(conn_);
        // w.exec("DROP TABLE books;"_zv);
        w.exec("CREATE TABLE IF NOT EXISTS books ("
                "id SERIAL PRIMARY KEY NOT NULL," 
                "title varchar(100) NOT NULL," 
                "author varchar(100) NOT NULL," 
                "year integer NOT NULL,"
                "ISBN varchar(13) UNIQUE"
                ");"_zv);
        w.commit();

        conn_.prepare(INSERT_BOOK, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);
    }

    ResultCode AddBook(const json::value& payload){
        pqxx::work w(conn_);

        try{
            BookArgs args = MakeBookArgs(payload);
            w.exec_prepared(INSERT_BOOK, 
                            args.title, 
                            args.author, 
                            args.year,   
                            args.ISBN
                            );
            w.commit();

            std::cout << MakeResult(true) << std::endl;
        } catch(...){
            std::cout << MakeResult(false) << std::endl;
        }
        return ResultCode::ADD_BOOK;
    }

    ResultCode AllBooks(){
        pqxx::read_transaction read(conn_);
        auto query_text = "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv;
        json::array books;
        for(auto [id, title, author, year, ISBN] : read.query<int, std::string, std::string, 
                                                                int, std::optional<std::string>>(query_text)){
            json::object book;
            book["id"] = id;
            book["title"] = title;
            book["author"] = author;
            book["year"] = year;
            if(ISBN.has_value()){
                book["ISBN"] = ISBN.value();
            } else {
                json::value null;
                null.emplace_null();
                book["ISBN"] = null;
            }
            

            books.push_back(book);
        }
        std::cout << json::serialize(books) << std::endl;
        return ResultCode::ALL_BOOKS;
    }

private:
    struct BookArgs{
        std::string title;
        std::string author;
        unsigned year;
        std::optional<std::string> ISBN;
    };
    
    BookArgs MakeBookArgs(const json::value& payload){
        BookArgs result;

        result.title = payload.at("title").as_string().c_str();
        result.author = payload.at("author").as_string().c_str();
        result.year = payload.at("year").as_int64();
        if(!payload.at("ISBN").is_null()){
            result.ISBN = payload.at("ISBN").as_string().c_str();
        }

        return result;
    }

    std::string MakeResult(bool result){
        json::object json_result;
        json_result["result"] = result;
        return json::serialize(json_result);
    }

    pqxx::connection conn_;
};

} // namespace db;