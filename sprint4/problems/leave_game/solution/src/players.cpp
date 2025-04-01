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

std::pair<Token, SharedPlayer>
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

SharedPlayer Players::FindByDogIdAndMapId(int dog_id,
                                                     std::string map_id) const {
  try {
    return dog_map_players_.at(DogMapId(std::make_pair(dog_id, map_id)));
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return nullptr;
  }
}

const SharedPlayer Players::FindByToken(Token token) {
  try {
    return token_players_.at(token);
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return nullptr;
  }
}

const Token Players::FindByPlayer(SharedPlayer player) const {
  for (auto [token, player_for] : token_players_) {
    if (player_for == player) {
      return token;
    }
  }
  throw std::logic_error("FindByPlayer");
}

const std::vector<SharedPlayer>
Players::FindPlayersBySession(const GameSession *game_session) {
  if (session_players_.contains(game_session)) {
    return session_players_.at(game_session);
  }

  throw std::logic_error("Session is not exists");
}

void Players::LoadPlayerToken(const SharedPlayer  player, const Token& token) {
  auto [it, add] = token_players_.emplace(std::move(token), player);

  if(!add) {
    throw std::logic_error("Player was added");
  }

}

void Players::LoadPlayerInSession(const SharedPlayer player, const GameSession* session) {
  session_players_[session].emplace_back(player);
}

void Players::DeletePlayer(const SharedPlayer erasing_player) {
  auto token_it = std::find_if(token_players_.begin(), 
                                token_players_.end(), 
                                [erasing_player](const auto& token_and_player){
                                    return token_and_player.second == erasing_player;});

    token_players_.erase(token_it);

    std::vector<SharedPlayer>& players_in_session = session_players_.at(erasing_player->GetGameSession());
    auto session_it = std::find_if(players_in_session.begin(), 
                                    players_in_session.end(), 
                                    [erasing_player](const SharedPlayer player){
                                        return player == erasing_player;});

    players_in_session.erase(session_it);
}

} // namespace players