
#include "nfd.hpp"
#include "helper/utils.hpp"
#if defined(_HAVE_FILE_DIALOGS)

#include "helper/types.hpp"
#include "nfd_include.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <functional>
#include <memory>

namespace {

    using FilterItemType = std::unique_ptr<nfdfilteritem_t, std::function<void(const nfdfilteritem_t* const)>>;
    template<typename T>
    using UniquePtr = std::unique_ptr<T, std::function<void(const T* const)>>;


    [[nodiscard]] FilterItemType get_filter_items(const std::vector<helper::AllowedFile>& allowed_files) {
        const auto size = allowed_files.size();
        FilterItemType filterItem{ allowed_files.empty() ? nullptr : new nfdfilteritem_t[size],
                                   [size](const nfdfilteritem_t* const value) {
                                       if (value == nullptr) {
                                           return;
                                       }

                                       for (usize i = 0; i < size; ++i) {
                                           const auto& item =
                                                   value[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

                                           delete item.name; // NOLINT(cppcoreguidelines-owning-memory)
                                           delete item.spec; // NOLINT(cppcoreguidelines-owning-memory)
                                       }

                                       delete[] value; // NOLINT(cppcoreguidelines-owning-memory)
                                   } };


        if (not allowed_files.empty()) {

            for (usize i = 0; i < allowed_files.size(); ++i) {
                const auto& allowed_file = allowed_files.at(i);

                const auto& filter_name = allowed_file.name;
                const auto filter_name_size = filter_name.size() + 1;

                auto* name = new nfdnchar_t[filter_name_size];
                std::memcpy(name, filter_name.c_str(), filter_name_size * sizeof(nfdnchar_t));


                const NFD::string& extension_list =
                        fmt::format(NFD_CHAR("{}"), fmt::join(allowed_file.extension_list, NFD_CHAR(",")));
                const auto extension_list_size = extension_list.size() + 1;

                auto* extensions = new nfdnchar_t[extension_list_size];
                std::memcpy(extensions, extension_list.c_str(), extension_list_size * sizeof(nfdnchar_t));

                filterItem.get()[i] = { name, extensions };
            }
        }

        return filterItem;

    } // namespace


} // namespace


helper::expected<std::filesystem::path, NFD::string> helper::openFileDialog(
        const std::vector<AllowedFile>& allowed_files,
        helper::optional<std::filesystem::path> default_path
) {

    NFD::UniquePath outPath{};
    auto filterItem = get_filter_items(allowed_files);

    const auto path_deallocator = [](const nfdnchar_t* const char_value) {
        if (char_value == nullptr) {
            return;
        }

        delete[] char_value; // NOLINT(cppcoreguidelines-owning-memory)
    };

    UniquePtr<nfdnchar_t> default_path_value{ nullptr, path_deallocator };

    if (default_path.has_value()) {
        const auto& str = default_path.value().string();
        const auto str_size = str.size() + 1;
        default_path_value = UniquePtr<nfdnchar_t>{ new nfdnchar_t[str_size], path_deallocator };
        std::memcpy(default_path_value.get(), str.c_str(), str_size * sizeof(nfdnchar_t));
    }

    const nfdresult_t result =
            NFD::OpenDialog(outPath, filterItem.get(), allowed_files.size(), default_path_value.get());
    if (result == NFD_OKAY) {
        return std::filesystem::path{ outPath.get() };
    }

    if (result == NFD_CANCEL) {
        return helper::unexpected<NFD::string>{ "The user pressed cancel." };
    }

    return helper::unexpected<NFD::string>{ "Error: " + NFD::string{ NFD::GetError() } };
}


[[nodiscard]] helper::expected<std::vector<std::filesystem::path>, NFD::string> helper::openMultipleFilesDialog(
        const std::vector<AllowedFile>& allowed_files,
        helper::optional<std::filesystem::path> default_path
) {

    NFD::UniquePathSet outPaths{};
    auto filterItem = get_filter_items(allowed_files);

    const auto path_deallocator = [](const nfdnchar_t* const char_value) {
        if (char_value == nullptr) {
            return;
        }

        delete[] char_value; // NOLINT(cppcoreguidelines-owning-memory)
    };

    UniquePtr<nfdnchar_t> default_path_value{ nullptr, path_deallocator };

    if (default_path.has_value()) {
        const auto& str = default_path.value().string();
        const auto str_size = str.size() + 1;
        default_path_value = UniquePtr<nfdnchar_t>{ new nfdnchar_t[str_size], path_deallocator };
        std::memcpy(default_path_value.get(), str.c_str(), str_size * sizeof(nfdnchar_t));
    }

    const nfdresult_t result =
            NFD::OpenDialogMultiple(outPaths, filterItem.get(), allowed_files.size(), default_path_value.get());
    if (result == NFD_OKAY) {
        std::vector<std::filesystem::path> result_vector{};

        nfdpathsetsize_t count_paths{};
        const auto temp_result = NFD::PathSet::Count(outPaths, count_paths);
        ASSERT(temp_result == NFD_OKAY && "PathSet get count is successful");

        for (nfdpathsetsize_t i = 0; i < count_paths; ++i) {
            NFD::UniquePathSetPath outPath{};
            const auto temp_result2 = NFD::PathSet::GetPath(outPaths, i, outPath);
            ASSERT(temp_result2 == NFD_OKAY && "PathSet get path is successful");
            result_vector.emplace_back(outPath.get());
        }

        return result_vector;
    }

    if (result == NFD_CANCEL) {
        return helper::unexpected<NFD::string>{ "The user pressed cancel." };
    }

    return helper::unexpected<NFD::string>{ "Error: " + NFD::string{ NFD::GetError() } };
}

[[nodiscard]] helper::expected<std::filesystem::path, NFD::string> helper::openFolderDialog(
        helper::optional<std::filesystem::path> default_path
) {

    NFD::UniquePath outPath{};

    const auto path_deallocator = [](const nfdnchar_t* const char_value) {
        if (char_value == nullptr) {
            return;
        }

        delete[] char_value; // NOLINT(cppcoreguidelines-owning-memory)
    };


    UniquePtr<nfdnchar_t> default_path_value{ nullptr, path_deallocator };

    if (default_path.has_value()) {
        const auto& str = default_path.value().string();
        const auto str_size = str.size() + 1;
        default_path_value = UniquePtr<nfdnchar_t>{ new nfdnchar_t[str_size], path_deallocator };
        std::memcpy(default_path_value.get(), str.c_str(), str_size * sizeof(nfdnchar_t));
    }

    const nfdresult_t result = NFD::PickFolder(outPath, default_path_value.get());
    if (result == NFD_OKAY) {
        return std::filesystem::path{ outPath.get() };
    }

    if (result == NFD_CANCEL) {
        return helper::unexpected<NFD::string>{ "The user pressed cancel." };
    }

    return helper::unexpected<NFD::string>{ "Error: " + NFD::string{ NFD::GetError() } };
}


#endif
