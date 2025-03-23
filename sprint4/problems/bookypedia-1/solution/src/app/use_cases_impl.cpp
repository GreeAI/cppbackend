#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(int year, 
                        const std::string& title, 
                        const std::string& author_id) {
    books_.Save({BookId::New(), author_id, title, year});
}

std::vector<AuthorInfo> UseCasesImpl::ShowAuthors() {
    return authors_.GetData("SELECT id, name FROM authors ORDER BY name ASC;");
}

std::vector<BookInfo> UseCasesImpl::ShowBooks() {
    return books_.GetData("SELECT title, publication_year FROM books ORDER BY title ASC;");
}

std::vector<BookInfo> UseCasesImpl::ShowAuthorBooks(std::string_view author_id) {
    using namespace std::literals;
    std::string query = "SELECT title, publication_year FROM books WHERE author_id = \'"s + author_id.data() + "\' ORDER BY publication_year ASC;"s;
    return books_.GetData(query);
}

}  // namespace app