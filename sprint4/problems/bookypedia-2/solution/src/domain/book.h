#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"
#include <memory>

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Worker {
public:
    virtual ui::detail::AuthorInfo AddAuthor(const std::string& name) = 0;
    virtual ui::detail::BookInfo AddBook(ui::detail::AddBookParams) = 0;
    virtual void AddTag(const std::string& book_id, const std::string& tag) = 0;
    virtual void DeleteAuthor(const ui::detail::AuthorInfo&, const std::vector<ui::detail::BookInfo>&) = 0;
    virtual void DeleteBook(const ui::detail::BookInfo&) = 0;
    virtual void DeleteBookTags(const ui::detail::BookInfo&) = 0;

    virtual void UpdateAuthor(const ui::detail::AuthorInfo&) = 0;
    virtual void UpdateBook(const ui::detail::BookInfo&) = 0;



    virtual void Commit() = 0;
protected:
    virtual ~Worker() = default;
};


class Book {
public:
    Book(BookId id, AuthorId auth_id, std::string title, int year)
        : id_(std::move(id))
        , auth_id_(auth_id)
        , title_(std::move(title))
        , year_(year){
    }

    const BookId& GetBookId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return auth_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const int GetYear() const noexcept {
        return year_;
    }

private:
    BookId id_;
    AuthorId auth_id_;
    std::string title_;
    int year_;
};

class BookRepository {
public:
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string&) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooksByTitle(const std::string&) = 0;

    virtual std::vector<std::string> GetBookTags(const ui::detail::BookInfo& book) = 0;

    virtual std::shared_ptr<Worker> GetWorker() = 0;

protected:
    virtual std::vector<ui::detail::BookInfo> GetBooksByQuery(const std::string&) = 0;
    ~BookRepository() = default;
};

}  // namespace domain