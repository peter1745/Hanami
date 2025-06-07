#include <mwl/mwl.hpp>
#include <kori/core.hpp>
#include <cstring>

int main()
{
    auto mwl_state = mwl::State::create({ .client_api = mwl::ClientAPI::Wayland });
    kori_defer { mwl_state.destroy(); };

    auto win = mwl::Window::create(mwl_state, "Hanami", 1920, 1080);
    kori_defer { win.destroy(); };

    bool running = true;

    win.set_close_callback([&] { running = false; });

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
