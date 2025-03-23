#include "postgres.h"

#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

/* ============================ AuthorRepositoryImpl ===========================================*/

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<AuthorInfo> AuthorRepositoryImpl::GetData(std::string_view query){
    std::vector<AuthorInfo> result;

    pqxx::read_transaction read{connection_};
    for(auto [id, name] : read.query<std::string, std::string>(operator""_zv(query.data(), query.size()))){
        result.emplace_back(id, name);
    }
    return result;
}

/* ============================ BookRepositoryImpl ===========================================*/

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET title=$2;
)"_zv,
        book.GetBookId().ToString(), book.GetAuthorId(),book.GetTitle(), book.GetPublicationYear());
    work.commit();
}

std::vector<BookInfo> BookRepositoryImpl::GetData(std::string_view query){
    std::vector<BookInfo> result;

    pqxx::read_transaction read{connection_};
    for(auto [title, year] : read.query<std::string, int>(operator""_zv(query.data(), query.size()))){
        result.emplace_back(title, year);
    }
    return result;
}

/* ============================ Database ===========================================*/

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    // work.exec(R"(DROP TABLE authors;)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    // work.exec(R"(DROP TABLE books;)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID,
    title varchar(100) UNIQUE,
    publication_year integer
);
)"_zv);
    // ... создать другие таблицы

    // коммитим изменения
    work.commit();
}

}  // namespace postgres