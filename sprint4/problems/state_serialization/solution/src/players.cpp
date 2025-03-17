#include "players.h"
#include <cstddef>
#include <exception>
#include <stdexcept>

namespace players {

size_t Players::DogMapKeyHasher::operator()(const DogMapId &value) const {
  size_t x = static_cast<size_t>(value.first);
  size_t y = util::TaggedHasher<Map::Id>()(value.second);

  // Для уменьшения коллизий значения x и y умножаются на 2
  return x * 2 + y * 2 * 2;
}

std::pair<Token, std::shared_ptr<Player>>
Players::AddPlayer(int id, const std::string &name, Dog *dog,
                   const GameSession *session) {
  auto player = std::make_shared<Player>(id, name, dog, session);

  Token token = GenerateToken();
  token_players_[token] = player;

  DogMapId dog_map_id =
      std::make_pair(dog->GetId(), session->GetMap()->GetId());
  dog_map_players_.emplace(dog_map_id, player);

  session_players_[player->GetGameSession()].push_back(player);

  return std::make_pair(token, player);
}

std::shared_ptr<Player> Players::FindByDogIdAndMapId(int dog_id,
                                                     std::string map_id) const {
  try {
    return dog_map_players_.at(DogMapId(std::make_pair(dog_id, map_id)));
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return nullptr;
  }
}

const std::shared_ptr<Player> Players::FindByToken(Token token) {
  try {
    return token_players_.at(token);
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return nullptr;
  }
}

const Token Players::FindByPlayer(std::shared_ptr<Player> player) const {
  for (auto [token, player_for] : token_players_) {
    if (player_for == player) {
      return token;
    }
  }
  throw std::logic_error("FindByPlayer");
}

const std::vector<std::shared_ptr<Player>>
Players::FindPlayersBySession(const GameSession *game_session) {
  if (session_players_.contains(game_session)) {
    return session_players_.at(game_session);
  }

  throw std::logic_error("Session is not exists");
}

void Players::LoadPlayerToken(const std::shared_ptr<Player>  player, const Token& token) {
  auto [it, add] = token_players_.emplace(std::move(token), player);

  if(!add) {
    throw std::logic_error("Player was added");
  }

}

void Players::LoadPlayerInSession(const std::shared_ptr<Player> player, const GameSession* session) {
  session_players_[session].emplace_back(player);
}

} // namespace players