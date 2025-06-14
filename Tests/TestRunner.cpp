#include <print>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <filesystem>

#define STRINGIY_IMPL(x) #x
#define STRINGIFY(x) STRINGIY_IMPL(x)

#define COLOR_RED   31
#define COLOR_GREEN 32
#define COLOR_WHITE 37

template<typename... Args>
void println_colored(uint8_t color, std::format_string<Args...> fmt, Args&&... args)
{
    std::println("\u001B[{}m{}\u001B[0m", color, std::format(fmt, std::forward<Args>(args)...));
}

int main()
{
    std::vector<std::filesystem::path> tests;

    for (auto path : std::filesystem::directory_iterator(STRINGIFY(TESTS_BUILD_DIR)))
    {
        if (path.is_directory())
        {
            continue;
        }

        auto relpath = std::filesystem::relative(path);

        if (relpath.filename().compare("TestRunner") == 0)
        {
            continue;
        }

        if (!access(relpath.c_str(), X_OK))
        {
            std::println("Discovered test {}", relpath.c_str());
            tests.emplace_back(relpath);
        }
    }

    std::println("Discovered {} tests\n", tests.size());

    uint32_t passed = 0;
    uint32_t failed = 0;

    std::println("========== Running {} Tests ==========", tests.size());
    for (const auto& path : tests)
    {
        std::print("- {}: ", path.filename().c_str());
        const int result = std::system(std::format("./{}", path.c_str()).c_str());
        passed += result == 0;
        failed += result != 0;

        if (result == 0)
        {
            println_colored(COLOR_GREEN, "PASSED");
        }
        else
        {
            println_colored(COLOR_RED, "FAILED");
        }
    }

    std::println();

    auto status_color = failed > 0 ? COLOR_RED : COLOR_GREEN;
    println_colored(status_color, "===================");
    println_colored(status_color, "TestRunner Finished");
    println_colored(status_color, "===================");
    println_colored(COLOR_WHITE, "# TOTAL: {}", tests.size());
    println_colored(COLOR_GREEN, "# PASSED: {}", passed);
    println_colored(failed > 0 ? COLOR_RED : COLOR_WHITE, "# FAILED: {}", failed);
    println_colored(status_color, "===================");

    return 0;
}
