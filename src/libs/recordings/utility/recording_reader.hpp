#pragma once

#include "./helper.hpp"

#include "./recording.hpp"
#include "./tetrion_snapshot.hpp"

#include <filesystem>

namespace recorder {

    struct RecordingReader : public Recording {
    private:
        std::vector<Record> m_records;
        std::vector<TetrionSnapshot> m_snapshots;

        explicit RecordingReader(
                std::vector<TetrionHeader>&& tetrion_headers,
                AdditionalInformation&& information,
                std::vector<Record>&& records,
                std::vector<TetrionSnapshot>&& snapshots
        );

    public:
        RecordingReader(RecordingReader&& old) noexcept;

        static helper::expected<RecordingReader, std::string> from_path(const std::filesystem::path& path);

        [[nodiscard]] const Record& at(usize index) const;

        [[nodiscard]] usize num_records() const;
        [[nodiscard]] auto begin() const;
        [[nodiscard]] auto end() const;

        [[nodiscard]] const std::vector<Record>& records() const;

        [[nodiscard]] const std::vector<TetrionSnapshot>& snapshots() const;

        [[nodiscard]] static helper::
                expected<std::pair<recorder::AdditionalInformation, std::vector<recorder::TetrionHeader>>, std::string>
                is_header_valid(const std::filesystem::path& path);

    private:
        [[nodiscard]] static helper::expected<
                std::tuple<std::ifstream, std::vector<TetrionHeader>, recorder::AdditionalInformation>,
                std::string>
        get_header_from_path(const std::filesystem::path& path);


        [[nodiscard]] static helper::reader::ReadResult<TetrionHeader> read_tetrion_header_from_file(std::ifstream& file
        );

        [[nodiscard]] static helper::reader::ReadResult<Record> read_record_from_file(std::ifstream& file);
    };

} // namespace recorder
