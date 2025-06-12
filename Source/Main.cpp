#include "DOM/Text.hpp"
#include "HTML/Parser.hpp"
#include "HTML/Tokenizer.hpp"

#include <print>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <mwl/mwl.hpp>
#include <Kori/Core.hpp>

#include <cairo/cairo.h>

using namespace Hanami;

int main()
{
    auto mwl_state = mwl::State::create({ .client_api = mwl::ClientAPI::Wayland });
    KoriDefer { mwl_state.destroy(); };

    bool running = true;

    auto win = mwl::Window::create(mwl_state, "Hanami", 1920, 1080);
    KoriDefer { win.destroy(); };
    win.set_close_callback([&] { running = false; });

    double x_scroll = 0.0;
    double y_scroll = 0.0;

    win.set_mouse_scroll_callback([&](const mwl::MouseScrollEvent& event)
    {
        if (event.axis() == mwl::ScrollAxis::Horizontal)
        {
            x_scroll -= event.value();
        }
        else
        {
            y_scroll -= event.value();
        }
    });

    // HTML Tokenize
    std::stringstream ss;
    {
        std::ifstream stream("Tests/Parsing/Large.html");

        if (!stream)
        {
            std::println("Failed reading html file. Does the file exist?");
            return -1;
        }

        ss << stream.rdbuf();
    }

    auto parser = HTML::Parser{};
    parser.parse(ss.str());

    auto* document = parser.document();

    std::vector<DOM::Text*> text_elements;

    [&text_elements](this auto&& self, HTML::Node* node) -> void
    {
        if (auto* text = dynamic_cast<HTML::Text*>(node))
        {
            text_elements.emplace_back(text);
        }

        for (auto* child : node->children())
        {
            self(child);
        }
    }(document->body());

    auto compute_text_for_rendering = [](const HTML::Text* text) -> std::string
    {
        std::string result;

        std::ranges::unique_copy(text->whole_text(), std::back_inserter(result), [](char a, char b)
        {
            return std::isspace(a) && std::isspace(b);
        });

        const auto removed = std::ranges::remove(result, '\n');
        result.erase(removed.begin(), removed.end());

        return result;
    };


    while (running)
    {
        mwl_state.dispatch_events();

        auto buffer = win.fetch_screen_buffer();
        auto* surface = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(&buffer[0]),
            CAIRO_FORMAT_ARGB32,
            win.width(), win.height(),
            cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, win.width()));

        auto* cairo_ctx = cairo_create(surface);

        // Clear to white
        cairo_rectangle(cairo_ctx, 0, 0, win.width(), win.height());
        cairo_set_source_rgb(cairo_ctx, 1, 1, 1);
        cairo_fill(cairo_ctx);

        // Draw texts
        cairo_select_font_face(cairo_ctx, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

        double x = x_scroll;
        double y = y_scroll;

        constexpr double font_size = 24.0;

        cairo_set_font_size(cairo_ctx, font_size);

        for (const auto* text : text_elements)
        {
            auto str = compute_text_for_rendering(text);

            if (str.empty())
            {
                continue;
            }

            cairo_move_to(cairo_ctx, x, y);
            cairo_set_source_rgb(cairo_ctx, 0, 0, 0);
            cairo_show_text(cairo_ctx, str.data());
            y += font_size;
        }

        cairo_surface_finish(surface);
        cairo_destroy(cairo_ctx);

        win.present_screen_buffer(buffer);
    }

    return 0;
}