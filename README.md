C++ implementation for Minecraft 1.18+ Overworld terrain
#
### Example Usage
```c++
#include <iostream>
#include <cstdint>
#include "chunk_sampler.hpp"

int main()
{
    int64_t worldSeed = 12345ULL;
    int x = 11, z = -273;

    mc::ChunkSampler cs(worldSeed);
    //setSeed(worldSeed);

    std::cout << "height=" << cs.getHeight(x, z) << "\n";
    std::cout << "height=" << cs.getHeight(x, z, mc::Heightmap::Type::OCEAN_FLOOR_WG) << "\n";
    std::cout << "height=" << cs.getHeight(x, z, true) << "\n";// skip caves
    std::cout << "lava=" << (cs.isLava(x, -21, z) ? "true" : "false") << "\n";
    std::cout << "water=" << (cs.isWater(x, 19, z) ? "true" : "false") << "\n";

    return 0;
}
```
#
### Compile
```bash
make
g++ example.cpp libchunksampler.a -O3 -std=c++17 -o example
```