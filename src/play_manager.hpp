

#pragma once

#include "game_manager.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>
#include "random.hpp"

struct StartState {
    std::size_t num_players;
    // std::vector<void> state;
    Random::Seed seed;
};


/* abstract */ struct PlayManager {
    private:
        Settings m_settings;
public:
    explicit PlayManager();
    virtual ~PlayManager() = default;
    virtual tl::expected<StartState, std::string> init(Settings settings, Random::Seed seed);
    virtual std::pair<std::size_t, std::unique_ptr<Input>>
    get_input(std::size_t index, GameManager* associated_game_manager, EventDispatcher* event_dispatcher) = 0;
    Settings settings();
};


struct SinglePlayer : public PlayManager {
public:
    explicit SinglePlayer();
    tl::expected<StartState, std::string> init(Settings settings, Random::Seed seed) override;
    std::pair<std::size_t, std::unique_ptr<Input>>
    get_input(std::size_t index, GameManager* associated_game_manager, EventDispatcher* event_dispatcher) override;
};


namespace util {
    KeyboardControls assert_is_keyboard_controls(Controls& controls);
}