#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
    : authors_{authors}, books_{books} {
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors() {
    return authors_.Get();
}

void UseCasesImpl::AddBook(const ui::detail::AddBookParams& params) {
    auto worker = books_.GetWorker();
    worker->AddBook(params);
    worker->Commit();
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(author_id);
}

void UseCasesImpl::AddBookAuthorAndTags(const std::string& author_name,
                                        ui::detail::AddBookParams params,
                                        const std::vector<std::string>& tags) {
    auto worker = books_.GetWorker();

    ui::detail::AuthorInfo author = worker->AddAuthor(author_name);

    params.author_id = author.id;

    ui::detail::BookInfo book = worker->AddBook(params);

    for (auto& tag : tags) {
        worker->AddTag(book.id, tag);
    }
    worker->Commit();
}

void UseCasesImpl::AddBookAndTags(const ui::detail::AddBookParams& params, const std::vector<std::string>& tags) {
    auto worker = books_.GetWorker();
    ui::detail::BookInfo book = worker->AddBook(params);

    for (auto& tag : tags) {
        worker->AddTag(book.id, tag);
    }

    worker->Commit();
}

std::optional<ui::detail::AuthorInfo> UseCasesImpl::GetAuthorIdIfExists(const std::string& name) {
    return authors_.GetAuthorIdIfExists(name);
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    return books_.GetBooksByTitle(title);
}

std::vector<std::string> UseCasesImpl::GetBookTags(const ui::detail::BookInfo& book) {
    return books_.GetBookTags(book);
}

void UseCasesImpl::DeleteBook(const ui::detail::BookInfo& book) {
    auto worker = books_.GetWorker();
    worker->DeleteBook(book);
    worker->Commit();
}

void UseCasesImpl::DeleteAuthor(const ui::detail::AuthorInfo& author,
                                const std::vector<ui::detail::BookInfo>& books) {
    auto worker = books_.GetWorker();
    worker->DeleteAuthor(author, books);
    worker->Commit();
}

void UseCasesImpl::UpdateAuthor(const ui::detail::AuthorInfo& new_author) {
    auto worker = books_.GetWorker();
    worker->UpdateAuthor(new_author);
    worker->Commit();
}

void UseCasesImpl::UpdateBook(const ui::detail::BookInfo& new_book, const std::vector<std::string>& tags) {
    auto worker = books_.GetWorker();

    worker->UpdateBook(new_book);
    worker->DeleteBookTags(new_book);

    for (const auto& tag : tags) {
        worker->AddTag(new_book.id, tag);
    }

    worker->Commit();
}

}  // namespace app