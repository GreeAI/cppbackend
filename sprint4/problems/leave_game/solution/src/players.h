#pragma once
#include "model.h"

#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

namespace players {

using namespace model;
namespace detail {
struct TokenTag {};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class Players;

class Player {
public:
  Player(int id, std::string name, Dog *dog, const GameSession *session)
      : id_(id), name_(name), dog_(dog), session_(session) {}
  int GetId() const { return id_; }

  std::string GetName() const { return name_; }

  Dog *GetDog() { return dog_; }

  const Dog *GetDog() const { return static_cast<const Dog *>(dog_); }

  const GameSession *GetGameSession() const { return session_; }

  void SetPlayerTimeClock(const Dog::SpeedSignal::slot_type& slot) const {
        dog_->SetSlotSpeed(slot);
    }

private:
  friend Players;

  int id_;
  std::string name_;
  Dog *dog_;
  const GameSession *session_;
};

using SharedPlayer = std::shared_ptr<Player>;

class Players {
public:

  Players() = default;

  using TokenPlayer = std::unordered_map<Token, SharedPlayer,
                                         util::TaggedHasher<Token>>;
  using SessionPlayers = std::unordered_map<const GameSession*, std::vector<SharedPlayer>>;

  using DogMapId = std::pair<int, Map::Id>;
  struct DogMapKeyHasher {
    size_t operator()(const DogMapId &value) const;
  };
  using DogMapPlayer =
      std::unordered_map<DogMapId, SharedPlayer, DogMapKeyHasher>;

  std::pair<Token, SharedPlayer>
  AddPlayer(int id, const std::string &name, Dog *dog, const GameSession *session);

  SharedPlayer FindByDogIdAndMapId(int dog_id, std::string map_id) const;

  const SharedPlayer FindByToken(Token token);
  
  const Token FindByPlayer(SharedPlayer player) const;

  const TokenPlayer GetPlayers() { return token_players_; }

  const std::vector<SharedPlayer> FindPlayersBySession(const GameSession* game_session);

  void LoadPlayerToken(const SharedPlayer player, const Token& token);

  void LoadPlayerInSession(const SharedPlayer player, const GameSession* session);

  void DeletePlayer(const SharedPlayer erasing_player);

private:
  TokenPlayer token_players_;
  DogMapPlayer dog_map_players_;
  SessionPlayers session_players_;

  Token GenerateToken() {
    std::ostringstream out;
    out << std::setw(32) << std::setfill('0') << std::hex << generator1_()
        << generator2_();
    std::string token = out.str();
    size_t size = token.size();
    if (token.size() > 32) {
      token.resize(32);
    }
    return Token(std::move(token));
  }
  std::random_device random_device_;

  std::mt19937_64 generator1_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};

  std::mt19937_64 generator2_{[this] {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    return dist(random_device_);
  }()};
};
} // namespace players
