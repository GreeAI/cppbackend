#pragma once
#include <string>
#include <vector>
#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {

struct AuthorTag {};

struct AuthorInfo {
    std::string id;
    std::string name;
};

inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

}  // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;

class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

class AuthorRepository {
public:
    virtual void Save(const Author& author) = 0;

    virtual std::vector<detail::AuthorInfo> GetData(std::string_view query) = 0;
protected:
    ~AuthorRepository() = default;
};


}  // namespace domain