#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    std::vector<domain::Author> saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    std::vector<ui::detail::AuthorInfo> Get() override {
        std::vector<ui::detail::AuthorInfo> result;

        result.reserve(saved_authors.size());

        for (const domain::Author& auth : saved_authors) {
            ui::detail::AuthorInfo inf {util::detail::UUIDToString(*auth.GetId()), auth.GetName()};
            result.emplace_back(inf);
        }

        return result;
    }

    std::optional<ui::detail::AuthorInfo> GetAuthorIdIfExists(const std::string& name) override {
        return std::nullopt;
    }
};

struct MockBookRepository : domain::BookRepository {
    std::vector<ui::detail::BookInfo> saved_books;

    std::vector<ui::detail::BookInfo> GetBooks() override {
        return saved_books;
    }

    std::vector<ui::detail::BookInfo> GetBooksByTitle(const std::string&) override {
        return {};
    }

    std::vector<std::string> GetBookTags(const ui::detail::BookInfo& book) override {
        return {};
    }

    std::vector<ui::detail::BookInfo> GetBooksByQuery(const std::string&) override {
        return {};
    }

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) override {
        std::vector<ui::detail::BookInfo> result;

        for (const ui::detail::BookInfo info : saved_books) {
            //TODO
        }

        return result;
    }

    std::shared_ptr<domain::Worker> GetWorker() override {
        throw std::invalid_argument("not implemented");
    }

};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;

};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}