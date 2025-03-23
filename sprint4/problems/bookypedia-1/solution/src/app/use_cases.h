#pragma once
#include <string>
#include <vector>
#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

using domain::detail::AuthorInfo;
using domain::detail::BookInfo;

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;

    virtual void AddBook(int year, const std::string& title, const std::string& author_id) = 0;

    virtual std::vector<AuthorInfo> ShowAuthors() = 0;

    virtual std::vector<BookInfo> ShowBooks() = 0;

    virtual std::vector<BookInfo> ShowAuthorBooks(std::string_view author_id) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app