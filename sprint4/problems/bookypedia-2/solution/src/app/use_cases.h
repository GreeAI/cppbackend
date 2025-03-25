#pragma once

#include <string>
#include <vector>
#include <optional>

namespace ui {
namespace detail {
struct AuthorInfo;
struct BookInfo;
struct AddBookParams;
}
}

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual void AddBook(const ui::detail::AddBookParams& params) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) = 0;

    virtual void AddBookAuthorAndTags(const std::string& author_name,
                                      ui::detail::AddBookParams params,
                                      const std::vector<std::string>& tags) = 0;

    virtual void AddBookAndTags(const ui::detail::AddBookParams& params, const std::vector<std::string>& tags) = 0;

    virtual std::optional<ui::detail::AuthorInfo> GetAuthorIdIfExists(const std::string& name) = 0;

    virtual std::vector<ui::detail::BookInfo> GetBooksByTitle(const std::string& title) = 0;

    virtual std::vector<std::string> GetBookTags(const ui::detail::BookInfo& book) = 0;

    virtual void DeleteBook(const ui::detail::BookInfo& book) = 0;

    virtual void DeleteAuthor(const ui::detail::AuthorInfo& author,
                              const std::vector<ui::detail::BookInfo>& books) = 0;

    virtual void UpdateAuthor(const ui::detail::AuthorInfo&) = 0;
    virtual void UpdateBook(const ui::detail::BookInfo&, const std::vector<std::string>&) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app