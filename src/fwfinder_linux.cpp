#ifdef __linux__

    #include <fwfinder.hpp>

auto Fw::find_all() noexcept
    -> std::expected<Fw::FreeWiliDevices, std::string> {
    return std::unexpected("TODO");
}

#endif // __linux__
