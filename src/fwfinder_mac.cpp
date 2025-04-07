#ifdef __APPLE__

    #include <fwfinder.hpp>

    #include <expected>
    #include <string>

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    return std::unexpected("TODO");
}

#endif // __APPLE__
