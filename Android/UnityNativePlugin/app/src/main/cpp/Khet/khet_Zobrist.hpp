#ifndef __KHET_ZOBRIST_H__
#define __KHET_ZOBRIST_H__

#include "khet_Globals.hpp"
#include "khet_RandomGenerator.hpp"
#include "khet_Types.hpp"
#include <limits>

namespace Khet
{
    // Zobrist keys for each piece type in each position and orientation.
    class Zobrist
    {
    public:
        static Zobrist* Instance()
        {
            if (_instance == nullptr)
            {
                printf("sizeof Zobrist: %lu\n", sizeof(Zobrist));
                _instance = new Zobrist();
            }
            
            return _instance;
        }
        
        inline uint64_t Silver() const { return _silver; }
        inline uint64_t Key(Square sq, int32_t loc) const { return _keys[sq][loc]; }
        
    private:
        static Zobrist* _instance;
        
        // This key indicates is present if it's silver's turn.
        uint64_t _silver;
        
        // Need a hash key for each possible square value in each position.
        static_assert(std::numeric_limits<Square>::max() == 0xFF, "Square is the wrong size.");
        uint64_t _keys[0xFF][BoardArea];
        
        Zobrist()
        {
            printf("new Zobrist\n");
            // Note: This is the seed that Stockfish uses.
            RandomGenerator gen(1070372);
            _silver = gen.Next();
            for (int32_t sq = 0; sq < 0xFF; sq++)
            {
                for (int32_t loc = 0; loc < BoardArea; loc++)
                {
                    _keys[sq][loc] = gen.Next();
                }
            }
        }
    };
    
    Zobrist* Zobrist::_instance = nullptr;
}

#endif // __ZOBRIST_H__
