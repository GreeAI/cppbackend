#pragma once
#include <string>
#include <vector>
#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
    
struct BookTag {};

struct BookInfo {
    std::string title;
    int publication_year;
};

inline std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    using namespace std::literals;
    
    out << book.title << ", "s << std::to_string(book.publication_year);
    return out;
}

}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId book_id, std::string author_id, std::string title, int publication_year)
        : book_id_(std::move(book_id))
        , author_id(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(publication_year) {
    }

    const BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const std::string& GetAuthorId() const noexcept {
        return author_id;
    }
    
    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetPublicationYear() const noexcept {
        return publication_year_;
    }

private:
    BookId book_id_;
    std::string author_id;
    std::string title_; 
    int publication_year_;
};

class BookRepository {
public:
    virtual void Save(const Book& author) = 0;

    virtual std::vector<detail::BookInfo> GetData(std::string_view query) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain