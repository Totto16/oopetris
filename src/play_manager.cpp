

#include "play_manager.hpp"
#include "game_manager.hpp"
#include "random.hpp"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <tl/expected.hpp>
#include <utility>

PlayManager::PlayManager(){};

tl::expected<StartState, std::string> PlayManager::init(Settings settings, Random::Seed seed) {
    m_settings = settings;
    return StartState{ 0, seed };
};

Settings PlayManager::settings() {
    return m_settings;
}

SinglePlayer::SinglePlayer() : PlayManager{} {};


tl::expected<StartState, std::string> SinglePlayer::init(Settings settings, Random::Seed seed) {
    PlayManager::init(settings, seed);
    return StartState{ 1, seed };
}

std::pair<std::size_t, std::unique_ptr<Input>>
SinglePlayer::get_input(std::size_t index, GameManager* associated_game_manager, EventDispatcher* event_dispatcher) {

    if (index != 0) {
        throw std::range_error{ "SinglePlayer mode: error in index of get_input" };
    }

    auto keyboard_input = std::make_unique<KeyboardInput>(
            associated_game_manager, util::assert_is_keyboard_controls(settings().controls.at(index))
    );
    event_dispatcher->register_listener(keyboard_input.get());
    return std::pair<std::size_t, std::unique_ptr<Input>>{ 0, std::move(keyboard_input) };
}


namespace util {
    KeyboardControls assert_is_keyboard_controls(Controls& controls) {
        return std::visit(
                overloaded{
                        [&](KeyboardControls& keyboard_controls) -> KeyboardControls { return keyboard_controls; },
                        [&]([[maybe_unused]] ReplayControls& replay_controls) -> KeyboardControls {
                            throw std::runtime_error{ "in assert_is_keyboard_controls: input is not KeyboardControls" };
                        } },
                controls
        );
    }
} // namespace util