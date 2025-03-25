#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books);

    void AddAuthor(const std::string& name) override;

    std::vector<ui::detail::AuthorInfo> GetAuthors() override;

    void AddBook(const ui::detail::AddBookParams& params) override;

    std::vector<ui::detail::BookInfo> GetBooks() override;

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) override;

    void AddBookAuthorAndTags(const std::string& author_name,
                              ui::detail::AddBookParams params,
                              const std::vector<std::string>& tags) override;

    void AddBookAndTags(const ui::detail::AddBookParams& params, const std::vector<std::string>& tags) override;

    std::optional<ui::detail::AuthorInfo> GetAuthorIdIfExists(const std::string& name) override;

    std::vector<ui::detail::BookInfo> GetBooksByTitle(const std::string& title) override;

    std::vector<std::string> GetBookTags(const ui::detail::BookInfo& title) override;

    void DeleteBook(const ui::detail::BookInfo& book) override;

    void DeleteAuthor(const ui::detail::AuthorInfo& author,
                      const std::vector<ui::detail::BookInfo>& books) override;

    void UpdateAuthor(const ui::detail::AuthorInfo&) override;
    void UpdateBook(const ui::detail::BookInfo&, const std::vector<std::string>&) override;
private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app