#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <vector>
#include <sstream>

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

using namespace std::literals;
using namespace collision_detector;

class TestItemGathererProvider : public ItemGathererProvider{
public:
    using Items = std::vector<Item>;
    using Gatherers = std::vector<Gatherer>;

    TestItemGathererProvider(Items items, Gatherers gatherers)
    : items_(std::move(items)), gatherers_(std::move(gatherers)){}

    size_t ItemsCount() const override{
        return items_.size();
    }

    Item GetItem(size_t idx) const override{
        return items_[idx];
    }

    size_t GatherersCount() const override{
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override{
        return gatherers_[idx];
    }
private:
    Items items_;
    Gatherers gatherers_;
};

using Items = TestItemGathererProvider::Items;
using Gatherers = TestItemGathererProvider::Gatherers;
using Events = std::vector<GatheringEvent>;

namespace Catch {
    
template<>
struct StringMaker<GatheringEvent> {
  static std::string convert(GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.item_id << "," << value.gatherer_id << "," << value.sq_distance << "," << value.time << ")";

      return tmp.str();
  }
};

}  // namespace Catch 

namespace collision_detector{

inline bool operator==(const GatheringEvent& lhs, const GatheringEvent& rhs){
    // return std::tie(lhs.gatherer_id, lhs.item_id, lhs.sq_distance, lhs.time) == std::tie(rhs.gatherer_id, rhs.item_id, rhs.sq_distance, rhs.time);
    bool gatherer_id_is_equals = (lhs.gatherer_id == rhs.gatherer_id);
    bool item_id_is_equals = (lhs.item_id == rhs.item_id);
    bool sq_distance_is_equals = (std::abs(lhs.sq_distance - rhs.sq_distance) <= 10e-10);
    bool time_is_equals = (std::abs(lhs.time - rhs.time) <= 10e-10);
    return (gatherer_id_is_equals && item_id_is_equals && sq_distance_is_equals && time_is_equals);
}

} // namespace collision_detector

SCENARIO("Collision detection") {

    SECTION("Case: Gatherer collect item"){
        GIVEN("1 gatherer and 1 item"){
            Items items {
                {{3.0, 1.0}, 1.0}
            };

            Gatherers gatherers {
                {{0.0, 0.0}, {5.0, 0.0}, 0.6}
            };

            TestItemGathererProvider provider(items, gatherers);

            Events found_events = FindGatherEvents(provider);
            Events expected_events {
                {0, 0, 1, 0.6}
            };


            CHECK(found_events == expected_events);
        }
    }

    SECTION("Case: Gatherers not collect item"){
        GIVEN("3 gatherers and 1 item"){
            Items items {
                {{3.0, 1.0}, 1.0}
            };
            
            Gatherers gatherers {
                {{0.0, 0.0}, {0.0, 0.0}, 0.6},
                {{0.0, 0.0}, {2.0, 0.0}, 0.6},
                {{0.0, 4.0}, {3.0, 4.0}, 0.6}
            };

            TestItemGathererProvider provider(items, gatherers);

            Events found_events = FindGatherEvents(provider);
            Events expected_events {};
            CHECK(found_events == expected_events);
        }
    }

    SECTION("Case: Chronological order of events"){
        GIVEN("3 gatherers and 3 items"){
            Items items {
                {{3.0, 0.0}, 1.0},
                {{2.0, 4.0}, 1.0},
                {{1.0, 10.0}, 1.0}

            };
            
            Gatherers gatherers {
                {{0.0, 0.0}, {5.0, 0.0}, 0.6},
                {{0.0, 4.0}, {5.0, 4.0}, 0.6},
                {{0.0, 10.0}, {5.0, 10.0}, 0.6}
            };

            TestItemGathererProvider provider(items, gatherers);

            Events found_events = FindGatherEvents(provider);
            Events expected_events {
                {2, 2, 0, 0.2},
                {1, 1, 0, 0.4},
                {0, 0, 0, 0.6}
            };
            CHECK(found_events == expected_events); 
        }

        GIVEN("2 gatherers and 1 items"){
            Items items {
                {{3.0, 0.5}, 1.0}
            };
            
            Gatherers gatherers {
                {{1.0, 0.0}, {6.0, 0.0}, 0.6},
                {{0.0, 0.0}, {5.0, 0.0}, 0.6}
            };

            TestItemGathererProvider provider(items, gatherers);

            Events found_events = FindGatherEvents(provider);
            Events expected_events {
                {0, 0, 0.25, 0.4},
                {0, 1, 0.25, 0.6}
            };
            CHECK(found_events == expected_events); 
        }
    }

    SECTION("Case: Correct data in events"){
        GIVEN("3 gatherers and 2 items"){
            Items items {
                {{4.5, 1}, 1.0},
                {{3, 2.5}, 1.0}
            };
            
            Gatherers gatherers {
                {{1.0, 0.0}, {5.0, 5.0}, 0.6},
                {{1.0, 3.0}, {5.0, 1.0}, 0.6},
                {{3.0, 0.0}, {3, 5.0}, 0.6}
            };

            TestItemGathererProvider provider(items, gatherers);

            Events found_events = FindGatherEvents(provider);
            Events expected_events {
                {0, 2, 2.25, 0.2},
                {1, 1, 0.2, 0.45},
                {1, 0, 0, 0.5},
                {1, 2, 0.0, 0.5},
                {0, 1, 0.05, 0.9}
            };
            CHECK(found_events == expected_events);             
        }
    }
}