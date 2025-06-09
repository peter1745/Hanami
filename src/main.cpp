#include "html/tokenizer.hpp"
#include "html/tree_builder.hpp"

#include <print>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <mwl/mwl.hpp>
#include <kori/core.hpp>

#include "html/parser.hpp"

using namespace hanami;

int main()
{
    auto mwl_state = mwl::State::create({ .client_api = mwl::ClientAPI::Wayland });
    kori_defer { mwl_state.destroy(); };

    bool running = true;

    auto win = mwl::Window::create(mwl_state, "Hanami", 1920, 1080);
    kori_defer { win.destroy(); };
    win.set_close_callback([&] { running = false; });

    // HTML Tokenize
    std::stringstream ss;
    {
        std::ifstream stream("tests/parsing/basic.html");

        if (!stream)
        {
            std::println("Failed reading html file. Does the file exist?");
            return -1;
        }

        ss << stream.rdbuf();
    }

    auto parser = html::Parser{};
    parser.parse(ss.str());

    while (running)
    {
        mwl_state.dispatch_events();

        // TEMP
        auto buffer = win.fetch_screen_buffer();
        buffer.fill(0xFF222222);
        win.present_screen_buffer(buffer);
    }

    return 0;
}
