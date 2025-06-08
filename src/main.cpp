#include "html/tokenizer.hpp"

#include <print>
#include <fstream>
#include <sstream>
#include <mwl/mwl.hpp>
#include <kori/core.hpp>

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
        ss << stream.rdbuf();
    }

    auto tokenizer = hanami::html::Tokenizer{};
    auto tokens = tokenizer.tokenize(ss.str());

    for (const auto& token : tokens)
    {
        std::visit(kori::VariantOverloadSet {
            [](const hanami::html::DOCTYPEToken& token)
            {
                std::println("DOCTYPE(name = {}, force_quirks = {})", token.name, token.force_quirks);
            },
            [](const hanami::html::StartTagToken& token)
            {
                std::println("StartTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const hanami::html::EndTagToken& token)
            {
                std::println("EndTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const hanami::html::CommentToken& token)
            {
                std::println("CommentToken(data = {})", token.data);
            },
            [](const hanami::html::CharacterToken& token)
            {
                // Don't print newline or space tokens
                if (token.data == '\n' || token.data == ' ')
                {
                    return;
                }

                std::println("CharacterToken(data = {})", token.data);
            },
            [](const hanami::html::EOFToken&)
            {
                std::println("EOFToken");
            }
        }, token);
    }

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
