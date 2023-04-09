#include "tetris_application.hpp"

TetrisApplication::TetrisApplication(CommandLineArguments command_line_arguments)
    : Application{ "TetrisApplication", WindowPosition::Centered, width, height, std::move(command_line_arguments) } {
    try_load_settings();
    static constexpr auto num_tetrions = u8{ 1 };

    if (is_replay_mode()) {
        m_recording_reader = std::make_unique<RecordingReader>(*(this->command_line_arguments().recording_path));
    }

    const auto seeds = create_seeds(num_tetrions);

    if (game_is_recorded()) {
        const auto seeds_span = std::span{ seeds.data(), seeds.size() };
        m_recording_writer = create_recording_writer(create_tetrion_headers(seeds_span));
    }

    for (u8 tetrion_index = 0; tetrion_index < num_tetrions; ++tetrion_index) {
        const auto starting_level = starting_level_for_tetrion(tetrion_index);
        spdlog::info("starting level for tetrion {}: {}", tetrion_index, starting_level);

        m_tetrions.push_back(
                std::make_unique<Tetrion>(seeds.at(tetrion_index), starting_level, recording_writer_optional())
        );

        auto on_event_callback = create_on_event_callback(tetrion_index);

        const auto tetrion_pointer = m_tetrions.back().get();
        if (is_replay_mode()) {
            m_inputs.push_back(
                    create_recording_input(tetrion_index, m_recording_reader.get(), tetrion_pointer, [](InputEvent) {})
            );
        } else {
            m_inputs.push_back(
                    create_input(m_settings.controls.at(tetrion_index), tetrion_pointer, std::move(on_event_callback))
            );
        }
    }
    for (const auto& tetrion : m_tetrions) {
        tetrion->spawn_next_tetromino();
    }
}

void TetrisApplication::update_inputs() {
    for (const auto& input : m_inputs) {
        input->update();
    }
}

void TetrisApplication::update_tetrions() {
    for (const auto& tetrion : m_tetrions) {
        tetrion->update();
    }
}

void TetrisApplication::update() {
    update_inputs();
    update_tetrions();
}

void TetrisApplication::render() const {
    Application::render(); // call parent function to clear the screen
    for (const auto& tetrion : m_tetrions) {
        tetrion->render(*this);
    }
}

[[nodiscard]] std::unique_ptr<Input> TetrisApplication::create_input(
        Controls controls,
        Tetrion* associated_tetrion,
        Input::OnEventCallback on_event_callback
) {
    return std::visit(
            overloaded{
                    [&](KeyboardControls& keyboard_controls) -> std::unique_ptr<Input> {
                        auto keyboard_input = std::make_unique<KeyboardInput>(
                                associated_tetrion, std::move(on_event_callback), keyboard_controls
                        );
                        m_event_dispatcher.register_listener(keyboard_input.get());
                        return keyboard_input;
                    },
            },
            controls
    );
}

[[nodiscard]] std::unique_ptr<Input> TetrisApplication::create_recording_input(
        const u8 tetrion_index,
        RecordingReader* const recording_reader,
        Tetrion* const associated_tetrion,
        Input::OnEventCallback on_event_callback
) {
    return std::make_unique<ReplayInput>(
            associated_tetrion, tetrion_index, std::move(on_event_callback), recording_reader
    );
}

[[nodiscard]] Input::OnEventCallback TetrisApplication::create_on_event_callback(const int tetrion_index) {
    if (m_recording_writer) {
        return [tetrion_index, this](InputEvent event) {
            m_recording_writer->add_event(static_cast<u8>(tetrion_index), Application::simulation_step_index(), event);
        };
    } else {
        return Input::OnEventCallback{}; // empty std::function object
    }
}

void TetrisApplication::try_load_settings() try {
    std::ifstream settings_file{ settings_filename };
    m_settings = nlohmann::json::parse(settings_file);
    spdlog::info("settings loaded");
} catch (...) {
    spdlog::error("unable to load settings from \"{}\"", settings_filename);
    spdlog::warn("applying default settings");
}

[[nodiscard]] bool TetrisApplication::is_replay_mode() const {
    return this->command_line_arguments().recording_path.has_value();
}

[[nodiscard]] bool TetrisApplication::game_is_recorded() const {
    return not is_replay_mode();
}

[[nodiscard]] Random::Seed TetrisApplication::seed_for_tetrion(const u8 tetrion_index, const Random::Seed common_seed)
        const {
    return (is_replay_mode() ? m_recording_reader->tetrion_headers().at(tetrion_index).seed : common_seed);
}

[[nodiscard]] auto TetrisApplication::starting_level_for_tetrion(const u8 tetrion_index) const
        -> decltype(CommandLineArguments::starting_level) {
    return is_replay_mode() ? m_recording_reader->tetrion_headers().at(tetrion_index).starting_level
                            : this->command_line_arguments().starting_level;
}

[[nodiscard]] TetrisApplication::TetrionHeaders TetrisApplication::create_tetrion_headers(
        const std::span<const Random::Seed> seeds
) const {
    const auto num_tetrions = seeds.size();
    auto headers = TetrionHeaders{};
    headers.reserve(num_tetrions);
    for (u8 tetrion_index = 0; tetrion_index < num_tetrions; ++tetrion_index) {
        assert(tetrion_index < seeds.size());
        const auto tetrion_seed = seeds[tetrion_index];
        const auto starting_level = starting_level_for_tetrion(tetrion_index);
        headers.push_back(Recording::TetrionHeader{ .seed{ tetrion_seed }, .starting_level{ starting_level } });
    }
    return headers;
}

[[nodiscard]] std::unique_ptr<RecordingWriter> TetrisApplication::create_recording_writer(TetrionHeaders tetrion_headers
) {
    static constexpr auto recordings_directory = "recordings";
    const auto recording_directory_path = std::filesystem::path{ recordings_directory };
    if (not std::filesystem::exists(recording_directory_path)) {
        std::filesystem::create_directory(recording_directory_path);
    }
    const auto filename = fmt::format("{}.rec", utils::current_date_time_iso8601());
    const auto file_path = recording_directory_path / filename;

    return std::make_unique<RecordingWriter>(file_path, std::move(tetrion_headers));
}

[[nodiscard]] std::vector<Random::Seed> TetrisApplication::create_seeds(const u8 num_tetrions) const {
    auto seeds = std::vector<Random::Seed>{};
    seeds.reserve(num_tetrions);
    const auto common_seed = Random::generate_seed();
    for (u8 tetrion_index = 0; tetrion_index < num_tetrions; ++tetrion_index) {
        const auto seed = seed_for_tetrion(tetrion_index, common_seed);
        spdlog::info("seed for tetrion {}: {}", tetrion_index, seed);
        seeds.push_back(seed);
    }
    return seeds;
}

[[nodiscard]] tl::optional<RecordingWriter*> TetrisApplication::recording_writer_optional() {
    if (m_recording_writer) {
        return m_recording_writer.get();
    }
    return tl::nullopt;
}