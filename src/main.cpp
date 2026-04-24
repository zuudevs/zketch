/// demo_window.cpp — zketch demo
///
/// Menampilkan sebuah window 800×600 dengan:
///   - Panel latar belakang penuh
///   - Label judul di bagian atas
///   - Dua Button: "Klik Saya" dan "Nonaktifkan"
///
/// Jalankan: ./bin/zketch

#include "zk/core/application.hpp"
#include "zk/log/logger.hpp"
#include "zk/ui/button.hpp"
#include "zk/ui/label.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/window.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <memory>

int main()
{
    // -----------------------------------------------------------------------
    // 1. Buat Application (blocking / event-driven mode)
    // -----------------------------------------------------------------------
    auto app_result = zk::core::Application::create("ZketchDemo", zk::core::PumpMode::Blocking);
    if (!app_result) {
        ZK_LOG(zk::log::Level::Fatal, zk::log::Domain::Core,
               "Gagal membuat Application");
        return 1;
    }
    auto& app = *app_result;

    app.set_error_handler([](const zk::error::Error& err) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Core, err.message);
    });

    // -----------------------------------------------------------------------
    // 2. Buat Window 800×600
    // -----------------------------------------------------------------------
    auto win_result = zk::ui::Window::create(
        "zketch — Demo Window",
        zk::metric::Pos<int>{ 100, 100 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );
    if (!win_result) {
        ZK_LOG(zk::log::Level::Fatal, zk::log::Domain::Core,
               "Gagal membuat Window");
        return 1;
    }
    auto& win = *win_result;

    // -----------------------------------------------------------------------
    // 3. Panel latar belakang (memenuhi seluruh area klien)
    // -----------------------------------------------------------------------
    auto bg_panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );

    // -----------------------------------------------------------------------
    // 4. Label judul
    // -----------------------------------------------------------------------
    auto title_label = std::make_unique<zk::ui::Label>(
        "Selamat Datang di zketch!",
        zk::metric::Pos<int>{ 20, 20 },
        zk::metric::Size<uint16_t>{ 760, 50 }
    );
    title_label->set_font(zk::ui::FontConfig{ "Segoe UI", 24.0f, true, false });
    title_label->set_color(zk::ui::colors::DarkGray);

    // -----------------------------------------------------------------------
    // 5. Label status — diperbarui saat tombol diklik
    // -----------------------------------------------------------------------
    auto status_label = std::make_unique<zk::ui::Label>(
        "Belum ada aksi.",
        zk::metric::Pos<int>{ 20, 90 },
        zk::metric::Size<uint16_t>{ 760, 30 }
    );
    status_label->set_font(zk::ui::FontConfig{ "Segoe UI", 14.0f, false, true });
    status_label->set_color(zk::ui::colors::Gray);

    // Simpan pointer mentah sebelum ownership dipindahkan ke panel
    zk::ui::Label* status_ptr = status_label.get();

    // -----------------------------------------------------------------------
    // 6. Button "Klik Saya"
    // -----------------------------------------------------------------------
    auto btn_click = std::make_unique<zk::ui::Button>(
        "Klik Saya",
        zk::metric::Pos<int>{ 20, 150 },
        zk::metric::Size<uint16_t>{ 160, 44 }
    );

    // -----------------------------------------------------------------------
    // 7. Button "Nonaktifkan" — menonaktifkan btn_click saat diklik
    // -----------------------------------------------------------------------
    auto btn_disable = std::make_unique<zk::ui::Button>(
        "Nonaktifkan",
        zk::metric::Pos<int>{ 200, 150 },
        zk::metric::Size<uint16_t>{ 160, 44 }
    );

    // Simpan pointer mentah sebelum ownership dipindahkan
    zk::ui::Button* btn_click_ptr   = btn_click.get();
    zk::ui::Button* btn_disable_ptr = btn_disable.get();

    // Handler: btn_click memperbarui label status
    btn_click_ptr->set_on_click([status_ptr]() {
        status_ptr->set_text("Tombol 'Klik Saya' ditekan!");
        ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
               "Tombol 'Klik Saya' ditekan");
    });

    // Handler: btn_disable menonaktifkan / mengaktifkan btn_click secara toggle
    btn_disable_ptr->set_on_click([btn_click_ptr, btn_disable_ptr, status_ptr]() {
        const bool currently_disabled =
            (btn_click_ptr->state() == zk::ui::ButtonState::Disabled);

        btn_click_ptr->set_disabled(!currently_disabled);
        btn_disable_ptr->set_on_click(nullptr); // reset sementara agar tidak loop

        if (!currently_disabled) {
            status_ptr->set_text("Tombol 'Klik Saya' dinonaktifkan.");
            ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
                   "btn_click dinonaktifkan");
        } else {
            status_ptr->set_text("Tombol 'Klik Saya' diaktifkan kembali.");
            ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
                   "btn_click diaktifkan kembali");
        }
    });

    // -----------------------------------------------------------------------
    // 8. Susun widget ke dalam panel, lalu panel ke window
    // -----------------------------------------------------------------------
    bg_panel->add_child(std::move(title_label));
    bg_panel->add_child(std::move(status_label));
    bg_panel->add_child(std::move(btn_click));
    bg_panel->add_child(std::move(btn_disable));

    win.add_panel(std::move(bg_panel));

    // -----------------------------------------------------------------------
    // 9. Daftarkan event window
    // -----------------------------------------------------------------------
    win.on_resize([](zk::metric::Size<uint16_t> new_size) {
        ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
               "Window di-resize");
        (void)new_size;
    });

    win.on_close([]() {
        ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
               "Window ditutup");
    });

    win.on_key_down([](uint32_t vkey) {
        if (vkey == VK_ESCAPE) {
            ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
                   "ESC ditekan — menutup aplikasi");
            ::PostQuitMessage(0);
        }
    });

    // -----------------------------------------------------------------------
    // 10. Tampilkan window dan jalankan message loop
    // -----------------------------------------------------------------------
    win.show();

    auto run_result = app.run();
    return run_result.value_or(1);
}
