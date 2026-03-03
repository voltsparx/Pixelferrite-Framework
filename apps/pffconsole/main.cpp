#include <pixelferrite/console.hpp>
#include <pixelferrite/colors.hpp>

int main() {
    pixelferrite::colors::initialize();
    pixelferrite::ConsoleEngine console;
    console.show_banner();
    console.run_repl();
    return 0;
}
