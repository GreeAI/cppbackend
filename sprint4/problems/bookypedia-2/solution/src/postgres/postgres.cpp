#include "postgres.h"

#include <pqxx/zview.hxx>
#include <boost/uuid/uuid.hpp>
#include <pqxx/pqxx>
#include "../ui/view.h"
#include "../domain/book.h"

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

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
        author.GetId().ToString(), work.esc(author.GetName()));
    work.commit();
}

std::vector<ui::detail::AuthorInfo> AuthorRepositoryImpl::Get() {
    pqxx::read_transaction tr(connection_);

    std::string query = "SELECT * FROM authors ORDER BY name;";

    auto resp = tr.query<std::string, std::string>(query);

    std::vector<ui::detail::AuthorInfo> result;

    for (auto& [id, name] : resp) {
        ui::detail::AuthorInfo auth{id, name};
        result.push_back(auth);
    }

    return result;
}

std::optional<ui::detail::AuthorInfo> AuthorRepositoryImpl::GetAuthorIdIfExists(const std::string& name) {
    pqxx::read_transaction tr(connection_);

    std::string query = "SELECT id, name FROM authors WHERE name='" + name + "';";

    auto resp = tr.query01<std::string, std::string>(query);

    if (resp.has_value()) {
        auto [id, name] = *resp;
        ui::detail::AuthorInfo res;
        res.id = id;
        res.name = name;
        return res;
    }

    return std::nullopt;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT books_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year integer NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID,
    tag varchar(30) NOT NULL
);
)"_zv);

    // коммитим изменения
    work.commit();
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetBooks() {
    std::string query
    = "SELECT id, publication_year, title, author_id FROM books ORDER BY title;";
    return GetBooksByQuery(query);
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetBooksByTitle(const std::string& title) {
    std::string query
    = "SELECT id, publication_year, title, author_id FROM books WHERE title='" + title + "' ORDER BY title;";
    return GetBooksByQuery(query);
}

std::vector<std::string> BookRepositoryImpl::GetBookTags(const ui::detail::BookInfo& book) {
    pqxx::read_transaction tr(connection_);

    std::string query
    = "SELECT tag FROM book_tags WHERE book_id='" + book.id + "' ORDER BY tag;";

    auto resp = tr.query<std::string>(query);

    std::vector<std::string> result;

    for (auto& [tag] : resp) {
        result.push_back(tag);
    }

    return result;
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetBooksByQuery(const std::string& query) {
    pqxx::read_transaction tr(connection_);

    auto resp = tr.query<std::string, int, std::string, std::string>(query);

    std::vector<ui::detail::BookInfo> result;

    for (auto& [id, year, title, author_id] : resp) {
        ui::detail::BookInfo book;
        book.title = title;
        book.publication_year = year;
        book.id = id;
        book.author = tr.query_value<std::string>("SELECT name FROM authors WHERE id = '" + author_id + "';");
        result.push_back(book);
    }

    return result;
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetAuthorBooks(const std::string& author_id) {
    std::string query = "SELECT id, publication_year, title, author_id\
                         FROM books \
                         WHERE author_id='" + author_id + "' \
                         ORDER BY publication_year;";
    return GetBooksByQuery(query);
}

WorkerImpl::WorkerImpl(pqxx::connection& connection) : connection_(connection),
    work_{connection_} {}

ui::detail::AuthorInfo WorkerImpl::AddAuthor(const std::string& name) {
    ui::detail::AuthorInfo info;
    info.name = name;
    info.id = domain::AuthorId::New().ToString();

    work_.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        info.id, info.name);
    return info;
}

ui::detail::BookInfo WorkerImpl::AddBook(ui::detail::AddBookParams params) {
    ui::detail::BookInfo info;

    info.author = params.author_name;
    info.id = domain::BookId::New().ToString();
    info.publication_year = params.publication_year;
    info.title = params.title;

    work_.exec_params(
        R"(
    INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
    )"_zv,
        info.id, params.author_id, params.title, params.publication_year);
    return info;
}

void WorkerImpl::AddTag(const std::string& book_id, const std::string& tag) {
    work_.exec_params(
        R"(
            INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
        )"_zv,
        book_id, tag
    );
}

void WorkerImpl::DeleteAuthor(const ui::detail::AuthorInfo& author, const std::vector<ui::detail::BookInfo>& books) {
    for (const auto& book : books) {
        DeleteBook(book);
    }

    work_.exec(
        "DELETE FROM authors WHERE id='" + author.id + "';"
    );
}

void WorkerImpl::DeleteBook(const ui::detail::BookInfo& book) {
    DeleteBookTags(book);
    work_.exec(
        "DELETE FROM books WHERE id='" + book.id + "';"
                );
}

void WorkerImpl::DeleteBookTags(const ui::detail::BookInfo& book) {
    work_.exec(
        "DELETE FROM book_tags WHERE book_id='" + book.id + "';"
    );
}

void WorkerImpl::UpdateAuthor(const ui::detail::AuthorInfo& author) {
    work_.exec(
        "UPDATE authors SET name='" + author.name + "' WHERE id='" + author.id + "';"
    );
}

void WorkerImpl::UpdateBook(const ui::detail::BookInfo& book) {
    work_.exec(
        "UPDATE books SET title='" + book.title
        + "', publication_year =" + std::to_string(book.publication_year) + " WHERE id='" + book.id + "';"
    );
}

void WorkerImpl::Commit() {
    work_.commit();
}

WorkerImpl::~WorkerImpl() {
    work_.commit();
}

    }  // namespace postgres