#include <blitzar/blitzar.hpp>

#include <iostream>

int main()
{
    const blitzar::Context context{};
    if (!context.valid()) {
        const auto status = static_cast<blitzar_status>(context.status());
        std::cerr << "BLITZAR context error: "
                  << blitzar_status_message(status) << '\n';
        return 1;
    }
    std::cout << "BLITZAR context ready\n";
    return 0;
}
