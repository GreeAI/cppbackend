#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    std::string author_name;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;

    bool operator==(const AuthorInfo&) const = default;
    bool operator!=(const AuthorInfo&) const = default;
};

struct BookInfo {
    std::string id;
    std::string title;
    int publication_year;
    std::string author;

    bool operator==(const BookInfo&) const = default;
    bool operator!=(const BookInfo&) const = default;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks(std::istream& cmd_input) const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<detail::AuthorInfo> SelectAuthor() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    std::optional<detail::AuthorInfo> AuthorExists(const std::string& name) const;
    detail::AddBookParams GetBookTitleAndYear(std::istream& cmd_input) const;
    std::optional<std::string> AskAuthorName() const;
    bool AskAddAuthor(const std::string& name) const;
    std::vector<std::string> GetTags() const;
    std::vector<std::string> AskTags() const;

    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool ShowBook(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;

    std::optional<detail::BookInfo> SelectBook() const;
    std::optional<detail::BookInfo> SelectBook(const std::vector<detail::BookInfo>&) const;
    std::optional<detail::BookInfo> SelectBookAnyWay(std::istream& cmd_input) const;
    void PrintBookDetail(const detail::BookInfo&, const std::vector<std::string>&) const;
    void PrintTags(const std::vector<std::string>&) const;

    std::vector<std::string> SplitStrs(const std::string& str) const;
    std::optional<std::string> GetOneMoreArg(std::istream& cmd_input) const;

    detail::AuthorInfo GetEditedAuthor(detail::AuthorInfo) const;
    detail::BookInfo GetEditedBook(detail::BookInfo) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui