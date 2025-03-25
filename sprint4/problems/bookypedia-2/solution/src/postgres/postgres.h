#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

#include "../ui/view.h"
#include <memory>

namespace postgres {

class WorkerImpl : public domain::Worker {
public:
    WorkerImpl(pqxx::connection& connection);

    ui::detail::AuthorInfo AddAuthor(const std::string& name) override;
    ui::detail::BookInfo   AddBook(ui::detail::AddBookParams) override;
    void AddTag(const std::string& book_id, const std::string& tag) override;

    void DeleteAuthor(const ui::detail::AuthorInfo&, const std::vector<ui::detail::BookInfo>&) override;
    void DeleteBook(const ui::detail::BookInfo& book) override;
    void DeleteBookTags(const ui::detail::BookInfo& book) override;


    void UpdateAuthor(const ui::detail::AuthorInfo&) override;
    void UpdateBook(const ui::detail::BookInfo&) override;

    void Commit() override;

    ~WorkerImpl() override;

private:
    pqxx::connection& connection_;
    pqxx::work work_;
};

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;

    std::vector<ui::detail::AuthorInfo> Get() override;

    std::optional<ui::detail::AuthorInfo> GetAuthorIdIfExists(const std::string& name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {

    }

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) override;

    std::shared_ptr<domain::Worker> GetWorker() override {
        auto res = std::make_shared<WorkerImpl>(connection_);
        return res;
    }

    std::vector<ui::detail::BookInfo> GetBooks() override;
    std::vector<ui::detail::BookInfo> GetBooksByTitle(const std::string&) override;

    std::vector<std::string> GetBookTags(const ui::detail::BookInfo& book) override;

private:
    std::vector<ui::detail::BookInfo> GetBooksByQuery(const std::string&) override;
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl     books_{connection_};
};

}  // namespace postgres