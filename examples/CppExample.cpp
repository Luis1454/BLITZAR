#include <blitzar/blitzar.hpp>

int main()
{
    const blitzar::Context context{};
    return context.valid() ? 0 : 1;
}
