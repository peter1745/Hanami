#include "html/tokenizer.hpp"
#include "html/tree_builder.hpp"

#include <print>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <mwl/mwl.hpp>
#include <kori/core.hpp>

using namespace hanami;

void print_tokens(std::span<const html::Token> tokens)
{
    for (const auto& token : tokens)
    {
        std::visit(kori::VariantOverloadSet {
            [](const html::DOCTYPEToken& token)
            {
                std::println("DOCTYPE(name = {}, force_quirks = {})", token.name, token.force_quirks);
            },
            [](const html::StartTagToken& token)
            {
                std::println("StartTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const html::EndTagToken& token)
            {
                std::println("EndTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const html::CommentToken& token)
            {
                std::println("CommentToken(data = {})", token.data);
            },
            [](const html::CharacterToken& token)
            {
                // Don't print newline or space tokens
                std::println("CharacterToken(data = {})", token.data);
            },
            [](const html::EOFToken&)
            {
                std::println("EOFToken");
            }
        }, token);
    }
}

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

    auto tree_builder = html::TreeBuilder{};
    auto tokenizer = html::Tokenizer{};
    auto tokens = tokenizer.tokenize(ss.str());

    print_tokens(tokens);

    tree_builder.process_all_tokens(tokens);

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
