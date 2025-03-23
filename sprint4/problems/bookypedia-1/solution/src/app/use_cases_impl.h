#pragma once
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    void AddAuthor(const std::string& name) override;

    void AddBook(int year, const std::string& title, const std::string& author_id) override;

    std::vector<AuthorInfo> ShowAuthors() override;

    std::vector<BookInfo> ShowBooks() override;

    std::vector<BookInfo> ShowAuthorBooks(std::string_view author_id) override;
private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app