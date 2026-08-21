#include <blitzar/blitzar.hpp>

#include <cassert>
#include <utility>

int main()
{
    blitzar::Context first;
    assert(first.valid());
    assert(first.status() == blitzar::Status::Ok);

    blitzar::Context second(std::move(first));
    assert(!first.valid());
    assert(second.valid());

    blitzar::Context third;
    third = std::move(second);
    assert(!second.valid());
    assert(third.valid());
    return 0;
}
